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
    pthread_mutex_t mutex;

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
    return 0;
}

int
thread_pool_thread_count(const struct thread_pool *pool) {
    return pool->count;
}

int
thread_pool_delete(struct thread_pool *pool) {
    for (size_t i = 0; i < pool->count; i++)
        free(pool->threads[i]);
    free(pool->threads);
    return 0;
}

void *
thread_task_worker(void *pool) {
    struct thread_pool *p = pool;
    pthread_mutex_lock(&p->mutex);
    while (p->tasks_front != NULL) {
        struct thread_task *task = p->tasks_front->task;
        struct task_list_node *new_front = p->tasks_front->next;

        if (p->tasks_front->prev != NULL)
            p->tasks_front->prev->next = p->tasks_front->next;
        if (p->tasks_front->next != NULL)
            p->tasks_front->next->prev = p->tasks_front->prev;
        if (new_front == NULL)
            p->tasks_back = NULL;
        p->tasks_front = new_front;
        pthread_mutex_unlock(&p->mutex);
        __atomic_store_n(&task->state, RUNNING, __ATOMIC_RELAXED);
        task->result = task->function(task->arg);
        __atomic_store_n(&task->state, FINISHED, __ATOMIC_RELAXED);
        pthread_cond_signal(&task->is_finished);
        pthread_mutex_lock(&p->mutex);
    }
    pthread_mutex_unlock(&p->mutex);

    return NULL;
}

int
thread_pool_push_task(struct thread_pool *pool, struct thread_task *task) {
    pthread_mutex_lock(&pool->mutex);
    struct task_list_node *node = (struct task_list_node *) malloc(sizeof(struct task_list_node));
    node->task = task;
    node->next = NULL;
    if (pool->tasks_back != NULL) {
        pool->tasks_back->next = node;
        node->prev = pool->tasks_back;
        pool->tasks_back = node;
    } else {
        node->prev = NULL;
        pool->tasks_front = pool->tasks_back = node;
    }
    pthread_mutex_unlock(&pool->mutex);

    if (pool->count < pool->max_size) {
        pthread_t *t = &pool->threads[pool->count];
        if (pthread_create(t, NULL, thread_task_worker, pool)) {
            pool->count++;
        }
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
    pthread_mutex_lock(&task->mutex);
    if (task->state == CREATED) {
        pthread_mutex_unlock(&task->mutex);
        return TPOOL_ERR_TASK_NOT_PUSHED;
    }
    pthread_cond_wait(&task->is_finished, &task->mutex);
    *result = task->result;
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
    if (task->state == RUNNING)
        return TPOOL_ERR_TASK_NOT_PUSHED;
    if (task->result != NULL)
        free(task->result);
    pthread_mutex_destroy(&task->mutex);
    pthread_cond_destroy(&task->is_finished);
    return 0;
}

#ifdef NEED_DETACH

int
thread_task_detach(struct thread_task *task)
{
    /* IMPLEMENT THIS FUNCTION */
    (void)task;
    return TPOOL_ERR_NOT_IMPLEMENTED;
}

#endif
