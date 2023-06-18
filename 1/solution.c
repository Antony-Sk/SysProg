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
inline static void
merge(int *dst, const int *first, const int size_first, const int *second, const int size_second, float target_lat,
      int64_t *work_time, struct timespec *last_mt) {
    int *result = (int *) malloc(sizeof(int) * (size_first + size_second));
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
        clock_gettime(CLOCK_MONOTONIC, &mt);
        int64_t time = (1000000000LL + mt.tv_nsec - last_mt->tv_nsec) % 1000000000LL +
                       1000000000LL * (mt.tv_sec - last_mt->tv_sec);
        if ((float) time / 1000.f > target_lat) { // +5 баллов: каждая из N корутин получает T / N микросекунд
            *work_time += time;
            coro_yield(); // 15 баллов: yield после каждой итерации циклов сортировки индивидуальных файлов.
            clock_gettime(CLOCK_MONOTONIC, last_mt);
        }
    }
    memcpy(dst, result, (size_first + size_second) * sizeof(int));
    free(result);
}

inline static void
insert_sort(int *array, int size) {
    for (int i = 0; i < size; i++) {
        for (int j = size - 1; j >= i + 1; j--) {
            if (array[j] < array[j - 1]) {
                int tmp = array[j];
                array[j - 1] = array[j];
                array[j] = tmp;
            }
        }
    }
}


static void
sort(int *array, int size, float target_lat, int64_t *work_time, struct timespec *last_mt) {
    if (size <= 1)
        return;
    if (size <= 10) {
        insert_sort(array, size);
        return;
    }
    int *first = array;
    int *second = array + size / 2;
    sort(first, size / 2, target_lat, work_time, last_mt);
    sort(second, size - size / 2, target_lat, work_time, last_mt);
    merge(array, first, size / 2, second, size - size / 2, target_lat, work_time, last_mt);
}

/**
 * Coroutine body. This code is executed by all the coroutines. Here you
 * implement your solution, sort each individual file.
 */
static int
coroutine_func_f(void *context) {
    struct VectorInt *vector = (struct VectorInt *) context;
    clock_gettime(CLOCK_MONOTONIC, &vector->last_mt);
    sort(vector->storage_, vector->size_, vector->tar_lat, &vector->work_time, &vector->last_mt);
    struct timespec mt;
    clock_gettime(CLOCK_MONOTONIC, &mt);
    vector->work_time += (1000000000LL + mt.tv_nsec - vector->last_mt.tv_nsec) % 1000000000LL +
                         1000000000LL * (mt.tv_sec - vector->last_mt.tv_sec);
    return 0;
}

int
main(int argc, char **argv) {
    struct timespec startTime;
    clock_gettime(CLOCK_MONOTONIC, &startTime);

    int number_of_files = argc - 2; // -3 TODO bonus
    int target_latency = strtol(argv[1], NULL, 10);
    int number_of_coroutines = number_of_files; //atoi(argv[2]);


    /* Initialize our coroutine global cooperative scheduler. */
    coro_sched_init();
    /* Start several coroutines. */
    struct VectorInt *arrays = (struct VectorInt *) malloc(sizeof(struct VectorInt) * number_of_files);
    int overallSize = 0;
    for (int i = 0; i < number_of_files; ++i) {
        /*
         * The coroutines can take any 'void *' interpretation of which
         * depends on what you want. Here as an example I give them
         * some names.
         */
        char *name = argv[i + 2]; // TODO: bonus will shift this index
        FILE *file = fopen(name, "r");
        while (1) {
            int elem;
            if (fscanf(file, "%d", &elem) == -1) {
                break;
            }
            push_back(&(arrays[i]), elem);
        }
        fclose(file);
        overallSize += arrays[i].size_;
        /*
         * I have to copy the name. Otherwise all the coroutines would
         * have the same name when they finally start.
         */
        arrays[i].tar_lat = (float) target_latency / (float) number_of_coroutines;
        coro_new(coroutine_func_f, &(arrays[i]));
    }
    /* Wait for all the coroutines to end. */
    struct coro *c;
    int it = 0;
    while ((c = coro_sched_wait()) != NULL) {
        printf("Coro #%d: time: %lf sec, switch count: %lld, ", it, ((double) arrays[it].work_time) / 1000000000,
               coro_switch_count(c));
        printf("finished with status %d\n", coro_status(c));
        coro_delete(c);
        it++;
    }
    /* All coroutines have finished. */
    /* IMPLEMENT MERGING OF THE SORTED ARRAYS HERE. */
    int *result = (int *) malloc(overallSize * sizeof(int));
    struct VectorInt buffer;
    buffer.size_ = buffer.capacity_ = 0;
    buffer.storage_ = NULL;
    int agg = 0;
    for (int i = 0; i < number_of_files; i++) {
        for (int i1 = 0, i2 = 0; i1 < agg || i2 < arrays[i].size_;) {
            if (i1 == agg) {
                push_back(&buffer, arrays[i].storage_[i2]);
                i2++;
            } else if (i2 == arrays[i].size_ || result[i1] < arrays[i].storage_[i2]) {
                push_back(&buffer, result[i1]);
                i1++;
            } else {
                push_back(&buffer, arrays[i].storage_[i2]);
                i2++;
            }
        }
        agg += arrays[i].size_;
        memcpy(result, buffer.storage_, (buffer.size_) * sizeof(int));
        buffer.size_ = 0;
    }
    int check = 1;
    for (int i = 1; i < overallSize; i++) {
        check &= result[i - 1] <= result[i];
    }
    if (check) {
        printf("All files sorted and merged. ");
    } else {
        printf("Something went wrong: arrays are not sorted. ");
    }
    for (int i = 0; i < number_of_files; i++) {
        free(arrays[i].storage_);
    }
    free(arrays);
    free(buffer.storage_);
    free(result);
    struct timespec endTime;
    clock_gettime(CLOCK_MONOTONIC, &endTime);
    printf("Overall time: %f\n", ((float) (1000000000 + endTime.tv_nsec - startTime.tv_nsec)) / 1000000000.f +
                               (float) (endTime.tv_sec - startTime.tv_sec));
    return 0;
}
