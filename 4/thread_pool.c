#include "thread_pool.h"
#include <pthread.h>
#include <stdlib.h>

struct thread_task {
    thread_task_f function;
    void *arg;
    void *result;
    enum {
        CREATED = 1, RUNNING, FINISHED
    } state;
    pthread_mutex_t mutex;
    pthread_cond_t is_finished;
    int is_detached;
    struct thread_pool *pool;
};

struct task_list_node {
    struct thread_task *task;
    struct task_list_node *next;
    struct task_list_node *prev;
};

struct thread_pool {
    pthread_t *threads;
    size_t max_size;
    size_t count;
    size_t task_cnt;
    pthread_mutex_t mutex;
    pthread_cond_t wait_for_task;
    int to_delete;
// task queue
    struct task_list_node *tasks_front;
    struct task_list_node *tasks_back;
};

int
thread_pool_new(int max_thread_count, struct thread_pool **pool) {
    if (max_thread_count <= 0 || max_thread_count > TPOOL_MAX_THREADS) {
        return TPOOL_ERR_INVALID_ARGUMENT;
    }
    *pool = (struct thread_pool *) malloc(sizeof(struct thread_pool));
    (*pool)->count = 0;
    (*pool)->max_size = max_thread_count;
    (*pool)->threads = (pthread_t *) malloc(sizeof(pthread_t) * max_thread_count);
    (*pool)->tasks_front = (*pool)->tasks_back = NULL;
    (*pool)->task_cnt = 0;
    (*pool)->to_delete = 0;
    pthread_mutex_init(&(*pool)->mutex, NULL);
    pthread_cond_init(&(*pool)->wait_for_task, NULL);
    return 0;
}

int
thread_pool_thread_count(const struct thread_pool *pool) {
    return pool->count;
}

int
thread_pool_delete(struct thread_pool *pool) {
    if (__atomic_load_n(&pool->task_cnt, __ATOMIC_ACQUIRE) != 0) {
        return TPOOL_ERR_HAS_TASKS;
    }
    pool->to_delete = 1;
    pthread_cond_broadcast(&pool->wait_for_task);
    for (size_t i = 0; i < pool->count; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->wait_for_task);
    free(pool->threads);
    free(pool);
    return 0;
}

void *
thread_task_worker(void *pool) {
    struct thread_pool *p = pool;
    while (1) {
        pthread_mutex_lock(&p->mutex);
        while (p->tasks_front == NULL) {
            pthread_cond_wait(&p->wait_for_task, &p->mutex);
            if (p->to_delete) {
                pthread_mutex_unlock(&p->mutex);
                return NULL;
            }
        }
        struct thread_task *task = p->tasks_front->task;
        struct task_list_node *new_fr = p->tasks_front->next;
        free(p->tasks_front);
        p->tasks_front = new_fr;
        if (p->tasks_front == NULL) {
            p->tasks_back = NULL;
        } else {
            p->tasks_front->prev = NULL;
        }
        pthread_mutex_unlock(&p->mutex);
        __atomic_store_n(&task->state, RUNNING, __ATOMIC_RELAXED);
        task->result = task->function(task->arg);
        pthread_mutex_lock(&task->mutex);
        __atomic_sub_fetch(&p->task_cnt, 1, __ATOMIC_ACQ_REL);
        __atomic_store_n(&task->state, FINISHED, __ATOMIC_RELAXED);
        if (task->is_detached) {
            pthread_mutex_unlock(&task->mutex);
            task->pool = NULL;
            thread_task_delete(task);
            continue;
        }
        pthread_cond_signal(&task->is_finished);
        pthread_mutex_unlock(&task->mutex);
    }
    return NULL;
}

int
thread_pool_push_task(struct thread_pool *pool, struct thread_task *task) {
    if (__atomic_load_n(&pool->task_cnt, __ATOMIC_RELAXED) == TPOOL_MAX_TASKS) {
        return TPOOL_ERR_TOO_MANY_TASKS;
    }
    struct task_list_node *node = (struct task_list_node *) malloc(sizeof(struct task_list_node));
    task->pool = pool;
    task->state = CREATED;
    node->task = task;
    node->next = NULL;
    pthread_mutex_lock(&pool->mutex);
    if (pool->tasks_back != NULL) {
        pool->tasks_back->next = node;
        node->prev = pool->tasks_back;
        pool->tasks_back = node;
    } else {
        node->prev = NULL;
        pool->tasks_front = pool->tasks_back = node;
    }
    pthread_mutex_unlock(&pool->mutex);
    __atomic_add_fetch(&pool->task_cnt, 1, __ATOMIC_ACQ_REL);

    if (pool->count < pool->max_size && pool->task_cnt > pool->count) {
        pthread_t *t = &pool->threads[pool->count];
        if (!pthread_create(t, NULL, thread_task_worker, pool)) {
            pool->count++;
        }
    } else {
        pthread_cond_signal(&pool->wait_for_task);
    }
    return 0;
}

int
thread_task_new(struct thread_task **task, thread_task_f function, void *arg) {
    *task = (struct thread_task *) malloc(sizeof(struct thread_task));
    (*task)->arg = arg;
    (*task)->function = function;
    (*task)->state = CREATED;
    pthread_mutex_init(&(*task)->mutex, NULL);
    pthread_cond_init(&(*task)->is_finished, NULL);
    (*task)->result = NULL;
    (*task)->pool = NULL;
    (*task)->is_detached = 0;
    return 0;
}

bool
thread_task_is_finished(const struct thread_task *task) {
    return __atomic_load_n(&task->state, __ATOMIC_RELAXED) == FINISHED;
}

bool
thread_task_is_running(const struct thread_task *task) {
    return __atomic_load_n(&task->state, __ATOMIC_RELAXED) == RUNNING;
}

int
thread_task_join(struct thread_task *task, void **result) {
    if (task->pool == NULL) {
        return TPOOL_ERR_TASK_NOT_PUSHED;
    }
    pthread_mutex_lock(&task->mutex);
    while (!thread_task_is_finished(task)) {
        pthread_cond_wait(&task->is_finished, &task->mutex);
    }
    pthread_mutex_unlock(&task->mutex);
    *result = task->result;
    task->pool = NULL;
    return 0;
}

#ifdef NEED_TIMED_JOIN

int
thread_task_timed_join(struct thread_task *task, double timeout, void **result)
{
    /* IMPLEMENT THIS FUNCTION */
    (void)task;
    (void)timeout;
    (void)result;
    return TPOOL_ERR_NOT_IMPLEMENTED;
}

#endif

int
thread_task_delete(struct thread_task *task) {
    if (thread_task_is_running(task))
        return TPOOL_ERR_TASK_NOT_PUSHED;
    if (task->pool != NULL) {
        return TPOOL_ERR_TASK_IN_POOL;
    }
    pthread_mutex_destroy(&task->mutex);
    pthread_cond_destroy(&task->is_finished);
    free(task);
    return 0;
}

#ifdef NEED_DETACH

int
thread_task_detach(struct thread_task *task)
{
    if (task->pool == NULL) {
        return TPOOL_ERR_TASK_NOT_PUSHED;
    }
    pthread_mutex_lock(&task->mutex);
    if (thread_task_is_finished(task)) {
        pthread_mutex_unlock(&task->mutex);
        thread_task_delete(task);
        return 0;
    }
    task->is_detached = 1;
    pthread_mutex_unlock(&task->mutex);

    return 0;
}

#endif
