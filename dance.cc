
#include "dance.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USE_HEURISTIC 1

void *DanceMatrix::Malloc(size_t n)
{
    n = (n + 8) & ~8;
    arena_used_ += n;
    if (arena_used_ <= sizeof memory_arena_) return &memory_arena_[arena_used_ - n];
    printf("Out of memory in Malloc(%zu)!\n", n);
    exit(EXIT_FAILURE);
}

void DanceMatrix::init(int cols)
{
    arena_used_ = 0;
    ncolumns_ = cols;
    columns_ = (struct column_object *)Malloc(ncolumns_ * sizeof *columns_);
    head_.right = &columns_[0];
    head_.left = &columns_[ncolumns_-1];

    for (int i=0; i < ncolumns_; ++i) {
        columns_[i].name = i;
        columns_[i].size = 0;
        columns_[i].up = &columns_[i];
        columns_[i].down = &columns_[i];
        if (i > 0) {
            columns_[i].left = &columns_[i-1];
        } else {
            columns_[i].left = &head_;
        }
        if (i < cols-1) {
            columns_[i].right = &columns_[i+1];
        } else {
            columns_[i].right = &head_;
        }
    }
}

void DanceMatrix::addrow(int nentries, int *entries)
{
    struct data_object *h = nullptr;

    struct data_object *news = (struct data_object *)Malloc(nentries * sizeof *news);
    for (int i=0; i < nentries; ++i) {
        struct data_object *o = &news[i];
        o->column = &columns_[entries[i]];
        o->column->size += 1;
        o->down = &columns_[entries[i]];
        o->up = o->down->up;
        o->down->up = o;
        o->up->down = o;
        if (h != nullptr) {
            o->left = h->left;
            o->right = h;
            o->left->right = o;
            o->right->left = o;
        } else {
            o->left = o;
            o->right = o;
            h = o;
        }
    }
}

static void dancing_cover(struct column_object *c)
{
    auto cright = c->right;
    auto cleft = c->left;
    cright->left = cleft;
    cleft->right = cright;
    for (auto i = c->down; i != c; i = i->down) {
        for (auto j = i->right; j != i; j = j->right) {
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
        for (auto j = i->left; j != i; j = j->left) {
            j->column->size += 1;
            j->down->up = j;
            j->up->down = j;
        }
    }
    c->left->right = c;
    c->right->left = c;
}

dance_result DanceMatrix::dancing_search(
    int k,
    dance_result (*f)(int, struct data_object **),
    struct data_object **solution)
{
    dance_result result = {0, false};

    if (head_.right == &head_) {
        return f(k, solution);
    }

    /* Choose a column object |c|. */
#if USE_HEURISTIC
    struct column_object *c = nullptr;
    if (true) {
        int minsize = INT_MAX;
        for (auto j = head_.right; j != &head_; j = j->right) {
            auto jj = &j->as<column_object>();
            if (jj->size < minsize) {
                c = jj;
                minsize = jj->size;
                if (minsize <= 1) break;
            }
        }
    }
#else
    struct column_object *c = (struct column_object *)head_.right;
#endif

    /* Cover column |c|. */
    dancing_cover(c);

    for (auto r = c->down; r != c; r = r->down) {
        solution[k] = r;
        for (auto j = r->right; j != r; j = j->right) {
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
        for (auto j = r->left; j != r; j = j->left) {
            dancing_uncover(j->column);
        }
    }

    /* Uncover column |c| and return. */
    dancing_uncover(c);
    return result;
}

int DanceMatrix::solve(dance_result (*f)(int, struct data_object **))
{
    struct data_object **solution = (struct data_object **)Malloc(ncolumns_ * sizeof *solution);
    dance_result result = this->dancing_search(0, f, solution);
    return result.count;
}
