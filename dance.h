#pragma once

#include <stdio.h>
#include <sstream>
#include <string>

#include <type_traits>
#include <limits.h>
#include <assert.h>

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
    void make_spacer_node() { column = nullptr; }
    bool is_in_row() const { return column != nullptr; }
    data_object *leftmost_node_in_row() {
        assert(this->is_in_row());
        data_object *result = this;
        while (result[-1].is_in_row()) {
            --result;
        }
        return result;
    }
    data_object *rightmost_node_in_row() {
        assert(this->is_in_row());
        data_object *result = this;
        while (result[1].is_in_row()) {
            ++result;
        }
        return result;
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
    void addrow(int nentries, const int *entries);

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

auto row_to_string = [](data_object *r) {
    std::ostringstream oss;
    oss << "entries=";
    for (auto *j = r->leftmost_node_in_row(); j->is_in_row(); ++j) {
        oss << j->column->name << " ";
    }
    auto result = oss.str();
    result.pop_back();
    return result;
};

        /* Cover column |c|. */
        dancing_cover(c);

        for (auto r = c->down; r != c; r = r->down) {
            solution[k] = r;
            for (auto *j = r->leftmost_node_in_row(); j->is_in_row(); ++j) {
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
            for (auto *j = r->rightmost_node_in_row(); j->is_in_row(); --j) {
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
            for (auto *j = i->leftmost_node_in_row(); j->is_in_row(); ++j) {
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
            for (auto *j = i->rightmost_node_in_row(); j->is_in_row(); --j) {
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
