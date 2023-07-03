//
// Created by Anton Skorkin on 26.06.2023.
//

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifndef SYSPROG_STRING_H
#define SYSPROG_STRING_H

struct String {
    char *storage_;
    size_t size;
    size_t capacity_;
};

void push(struct String * str, const char c) {
    if (str->capacity_ == str->size) {
        str->capacity_ = str->capacity_ * 2 + 1;
        char* new = (char*) malloc(sizeof(char) * str->capacity_);
        memcpy(new, str->storage_, str->size * sizeof(char));
        free(str->storage_);
        str->storage_ = new;
    }
    str->storage_[str->size++] = c;
}

#endif //SYSPROG_STRING_H


