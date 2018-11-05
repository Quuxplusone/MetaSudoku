#pragma once

#include <type_traits>

extern char dance_memory_arena[];

#if 0
template<class T>
class DancePtr {
    T *ptr_;
public:
    constexpr DancePtr() noexcept = default;
    constexpr DancePtr(T *p) noexcept : ptr_(p) {}
    constexpr T *get() const noexcept { return ptr_; }
    constexpr operator T*() const noexcept { return get(); }
    constexpr T* operator->() const noexcept { return get(); }
    constexpr T& operator*() const noexcept { return *get(); }
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

class DanceMatrix {
public:
    explicit DanceMatrix() = default;
    void init(int ncols);
    void addrow(int nentries, int *entries);
    void set_nrows(int n) { nrows = n; }
    int solve(dance_result (*f)(int, struct data_object **));

private:
    void *Malloc(size_t n);
    dance_result dancing_search(
        int k,
        dance_result (*f)(int, struct data_object **),
        struct data_object **solution
    );

    int nrows;
    int ncolumns;
    DancePtr<column_object> columns;
    column_object head;
    size_t arena_used_ = 0;
    alignas(8) char memory_arena_[1000000];
};
