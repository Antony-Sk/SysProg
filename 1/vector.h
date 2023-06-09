//
// Created by Anton Skorkin on 11.06.2023.
//

#ifndef SYSPROG_VECTOR_H
#define SYSPROG_VECTOR_H

#include <string.h>
#include <stdlib.h>

struct VectorInt {
    int *storage_;
    int size_;
    int capacity_;
};

inline static void push_back(struct VectorInt *vector, int elem) {
    if (vector->capacity_ == vector->size_) {
        vector->capacity_ = vector->capacity_ * 2 + 1;
        int *new_st = (int *) malloc(sizeof(int) * vector->capacity_);
        memcpy(new_st, vector->storage_, vector->size_ * sizeof(int));
        int* old = vector->storage_;
        vector->storage_ = new_st;
        free(old);
    }
    vector->storage_[vector->size_++] = elem;
}

#endif //SYSPROG_VECTOR_H
