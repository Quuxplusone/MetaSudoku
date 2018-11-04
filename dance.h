#pragma once

#include <type_traits>

extern char dance_memory_arena[];

#if 0
template<class T>
class DancePtr {
    T *ptr_;
public:
    DancePtr() = default;
    DancePtr(T *p) : ptr_(p) {}
    T *get() const { return ptr_; }
    operator T*() const { return get(); }
    T* operator->() const { return get(); }
    T& operator*() const { return *get(); }
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

struct dance_matrix *dance_init(int cols);
void dance_addrow(struct dance_matrix *m, int nentries, int *entries);
void dance_free();
int dance_solve(struct dance_matrix *m, dance_result (*f)(int, struct data_object **));
