#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "libcoro.h"
#include "vector.h"
/**
 * You can compile and run this code using the commands:
 *
 * $> gcc solution.c libcoro.c
 * $> ./a.out
 */

/**
 * A function, called from inside of coroutines recursively. Just to demonstrate
 * the example. You can split your code into multiple functions, that usually
 * helps to keep the individual code blocks simple.
 */
static void
merge(int *dst, int *first, int size_first, int *second, int size_second) {
    int *result = (int*) malloc(sizeof(int) * (size_first + size_second));
//    int result[size_second + size_first];
    for (int i1 = 0, i2 = 0; i1 < size_first || i2 < size_second;) {
        if (i1 == size_first) {
            result[i1 + i2] = second[i2];
            i2++;
        } else if (i2 == size_second || first[i1] < second[i2]) {
            result[i1 + i2] = first[i1];
            i1++;
        } else {
            result[i1 + i2] = second[i2];
            i2++;
        }
        struct timespec mt;
        struct coro *this = coro_this();
        clock_gettime(CLOCK_MONOTONIC, &mt);
        int time = mt.tv_nsec - coro_get_last_mt(this)->tv_nsec;
        if (time > 0) { // TODO bonus
            *coro_get_work_time(this) += time;
            coro_yield(); // 15 баллов: yield после каждой итерации циклов сортировки индивидуальных файлов.
            clock_gettime(CLOCK_MONOTONIC, coro_get_last_mt(this));
        }
    }
    for (int i = 0; i < size_first + size_second; i++) {
        dst[i] = result[i];
    }
    free(result);
}

static void
sort(int *array, int size) {
    if (size <= 1)
        return;
    int *first = array;
    int *second = array + size / 2;
    sort(first, size / 2);
    sort(second, size - size / 2);
    merge(array, first, size / 2, second, size - size / 2);
}

/**
 * Coroutine body. This code is executed by all the coroutines. Here you
 * implement your solution, sort each individual file.
 */
static int
coroutine_func_f(void *context) {
    struct coro *this = coro_this();
    clock_gettime(CLOCK_MONOTONIC, coro_get_last_mt(this));
    struct VectorInt *vector = (struct VectorInt *) context;
    sort(vector->storage_, vector->size_);
    struct timespec mt;
    clock_gettime(CLOCK_MONOTONIC, &mt);
    *coro_get_work_time(this) += mt.tv_nsec - coro_get_last_mt(this)->tv_nsec;
    return 0;
}

int
main(int argc, char **argv) {
    int number_of_files = argc - 1; // -3 TODO bonus
//    float target_latency = atof(argv[1]);
//    int number_of_coroutines = number_of_files; //atoi(argv[2]);


    /* Initialize our coroutine global cooperative scheduler. */
    coro_sched_init();
    /* Start several coroutines. */
    struct VectorInt *arrays = (struct VectorInt *) malloc(sizeof(struct VectorInt) * number_of_files);
//    printf("%d\n", number_of_files);
    for (int i = 0; i < number_of_files; ++i) {
        /*
         * The coroutines can take any 'void *' interpretation of which
         * depends on what you want. Here as an example I give them
         * some names.
         */
        char *name = argv[i + 1]; // TODO: bonus will shift this index
//        printf("%s\n", name);
        arrays[i].size_ = arrays[i].capacity_ = 0;
        FILE *file = fopen(name, "r");
        while (1) {
            int elem;
            if (fscanf(file, "%d", &elem) == -1) {
                break;
            }
            push_back(&(arrays[i]), elem);
        }
        fclose(file);

        /*
         * I have to copy the name. Otherwise all the coroutines would
         * have the same name when they finally start.
         */
        coro_new(coroutine_func_f, &(arrays[i]));
    }
    /* Wait for all the coroutines to end. */
    struct coro *c;
    while ((c = coro_sched_wait()) != NULL) {
//        printf("here\n");

        /*
         * Each 'wait' returns a finished coroutine with which you can
         * do anything you want. Like check its exit status, for
         * example. Don't forget to free the coroutine afterwards.
         */
        printf("time (ns): %d, switch count: %lld\n", *coro_get_work_time(c), coro_switch_count(c));
        printf("Finished %d\n", coro_status(c));
        coro_delete(c);
    }
    for (int i = 0; i < 10; i++) {
        printf("%d ", arrays[0].storage_[i]);
    }
    printf("\n");
//    struct VectorInt merged;

    /* All coroutines have finished. */

    /* IMPLEMENT MERGING OF THE SORTED ARRAYS HERE. */
    for (int i = 0; i < number_of_files; i++) {
        free(arrays[i].storage_);
    }
    free(arrays);
    return 0;
}
