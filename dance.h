#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if 0
template<class T>
class DancePtr {
    T *ptr_;
public:
    DancePtr() = default;
    DancePtr(T *p) : ptr_(p) {}
    operator T*() const { return ptr_; }
    T* operator->() const { return ptr_; }
    T& operator*() const { return *ptr_; }
};
#else
template<class T>
using DancePtr = T*;
#endif

struct dance_result {
    int count;
    int short_circuit;
};

struct column_object;

struct data_object {
    DancePtr<data_object> up, down, left, right;
    DancePtr<column_object> column;
    template<class T> T& as() { return *(T*)this; }
};

struct column_object : public data_object {
    int size;
    int name;
};

struct dance_matrix {
    int nrows, ncolumns;
    DancePtr<column_object> columns;
    column_object head;
};

void dance_init(struct dance_matrix *m, int cols);
void dance_addrow(struct dance_matrix *m, int nentries, int *entries);
void dance_free(struct dance_matrix *m);
int dance_solve(struct dance_matrix *m, dance_result (*f)(int, struct data_object **));
