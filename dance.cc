
#include "dance.h"

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
    DanceMatrix *m = this;
    m->arena_used_ = 0;
    m->ncolumns = cols;
    m->columns = (struct column_object *)Malloc(m->ncolumns * sizeof *m->columns);
    m->head.right = &m->columns[0];
    m->head.left = &m->columns[m->ncolumns-1];

    for (int i=0; i < m->ncolumns; ++i) {
        m->columns[i].name = i;
        m->columns[i].size = 0;
        m->columns[i].up = &m->columns[i];
        m->columns[i].down = &m->columns[i];
        if (i > 0)
          m->columns[i].left = &m->columns[i-1];
        else m->columns[i].left = &m->head;
        if (i < cols-1)
          m->columns[i].right = &m->columns[i+1];
        else m->columns[i].right = &m->head;
    }
}

void DanceMatrix::addrow(int nentries, int *entries)
{
    DanceMatrix *m = this;
    struct data_object *h = nullptr;

    struct data_object *news = (struct data_object *)Malloc(nentries * sizeof *news);
    for (int i=0; i < nentries; ++i) {
        struct data_object *o = &news[i];
        o->column = &m->columns[entries[i]];
        o->down = &m->columns[entries[i]];
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
        o->column->size += 1;
    }
}

static void dancing_cover(struct column_object *c)
{
    c->right->left = c->left;
    c->left->right = c->right;
    for (auto i = c->down; i != c; i = i->down) {
        for (auto j = i->right; j != i; j = j->right) {
            j->down->up = j->up;
            j->up->down = j->down;
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
    DanceMatrix *m = this;

    dance_result result = {0, false};

    if (m->head.right == &m->head) {
        return f(k, solution);
    }

    /* Choose a column object |c|. */
#if USE_HEURISTIC
    struct column_object *c = nullptr;
    if (true) {
        int minsize = m->nrows + 1;
        for (auto j = m->head.right; j != &m->head; j = j->right) {
            auto jj = &j->as<column_object>();
            if (jj->size < minsize) {
                c = jj;
                minsize = jj->size;
                if (minsize <= 1) break;
            }
        }
    }
#else
    struct column_object *c = (struct column_object *)m->head.right;
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
    struct data_object **solution = (struct data_object **)Malloc(ncolumns * sizeof *solution);
    dance_result result = this->dancing_search(0, f, solution);
    return result.count;
}
