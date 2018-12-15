#pragma once

#include <type_traits>
#include <limits.h>

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
    DancePtr<data_object> up, down;
    DancePtr<column_object> column;
    char padding[8];
    data_object *leftmost_node_in_row() {
        data_object *result = this;
        result = (data_object*)(size_t(result) & ~127);
        return result;
    }
    data_object *rightmost_node_in_row() {
        data_object *result = leftmost_node_in_row();
        return result + 3;
    }
};

struct column_object : public data_object {
    int size;
    int name;
    DancePtr<column_object> left, right;
};

class DanceMatrix {
public:
    explicit DanceMatrix() = default;
    void init(int ncols);

    void addrow(const int *entries);

    template<class F>
    int solve(const F& f)
    {
        data_object **solution = (struct data_object **)Malloc(ncolumns_ * sizeof *solution);
        dance_result result = this->dancing_search(0, f, solution);
        return result.count;
    }

    template<int RowsInSolution, class F>
    int solve(const F& f)
    {
        data_object *solution[RowsInSolution];
        dance_result result = this->dancing_search(0, f, solution);
        return result.count;
    }

private:
    void *Malloc(size_t n);

    template<class F>
    dance_result dancing_search(int k, const F& f, struct data_object **solution)
    {
        dance_result result = {0, false};

        if (head_.right == &head_) {
            return f(k, solution);
        }

        /* Choose a column object |c|. */
        struct column_object *c = nullptr;
        if (true) {
            int minsize = INT_MAX;
            for (auto j = head_.right; j != &head_; j = j->right) {
                if (j->size < minsize) {
                    c = j;
                    minsize = j->size;
                    if (minsize <= 1) break;
                }
            }
        }

        /* Cover column |c|. */
        dancing_cover(c);

        for (auto r = c->down; r != c; r = r->down) {
            solution[k] = r;
            auto *first = r->leftmost_node_in_row();
            auto *last = first + 4;
            for (auto *j = first; j != last; ++j) {
                if (j == r) continue;
                dancing_cover(j->column);
            }
            dance_result subresult = this->dancing_search(k+1, f, solution);
            result.count += subresult.count;
            if (subresult.short_circuit) {
                result.short_circuit = true;
                return result;
            }
            r = solution[k];
            c = r->column;
            first = r->leftmost_node_in_row() + 3;
            last = first - 4;
            for (auto *j = first; j != last; --j) {
                if (j == r) continue;
                dancing_uncover(j->column);
            }
        }

        /* Uncover column |c| and return. */
        dancing_uncover(c);
        return result;
    }

    static void dancing_cover(struct column_object *c)
    {
        auto cright = c->right;
        auto cleft = c->left;
        cright->left = cleft;
        cleft->right = cright;
        for (auto i = c->down; i != c; i = i->down) {
            auto *first = i->leftmost_node_in_row();
            auto *last = first + 4;
            for (auto *j = first; j != last; ++j) {
                if (j == i) continue;
                auto jup = j->up;
                auto jdown = j->down;
                jdown->up = jup;
                jup->down = jdown;
                j->column->size -= 1;
            }
        }
    }

    static void dancing_uncover(struct column_object *c)
    {
        for (auto i = c->up; i != c; i = i->up) {
            auto *first = i->leftmost_node_in_row() + 3;
            auto *last = first - 4;
            for (auto *j = first; j != last; --j) {
                if (j == i) continue;
                j->column->size += 1;
                j->down->up = j;
                j->up->down = j;
            }
        }
        c->left->right = c;
        c->right->left = c;
    }

public:
    int nrows_;
private:
    int ncolumns_;
    DancePtr<column_object> columns_;
    column_object head_;
    size_t arena_used_ = 0;
    alignas(8) char memory_arena_[120000];
};

inline void DanceMatrix::addrow(const int *entries)
{
    data_object *news = (data_object *)Malloc(4 * sizeof *news);
    for (int i=0; i < 4; ++i) {
        struct data_object *o = &news[i];
        o->column = &columns_[entries[i]];
        o->column->size += 1;
        o->down = &columns_[entries[i]];
        o->up = o->down->up;
        o->down->up = o;
        o->up->down = o;
    }
}

