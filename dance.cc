
#include "dance.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USE_HEURISTIC 1

alignas(void*) char dance_memory_arena[1000000];
static size_t s_index = 0;

void *Malloc(size_t n)
{
    n = (n + 8) & ~8;
    s_index += n;
    if (s_index <= sizeof dance_memory_arena) return &dance_memory_arena[s_index - n];
    printf("Out of memory in Malloc(%lu)!\n", (long unsigned)n);
    exit(EXIT_FAILURE);
}

struct dance_matrix *dance_init(int cols)
{
    struct dance_matrix *m = (struct dance_matrix *)Malloc(sizeof *m);
    m->nrows = 0;
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

    return m;
}

void dance_addrow(struct dance_matrix *m, int nentries, int *entries)
{
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

    m->nrows += 1;
}

void dance_free()
{
    s_index = 0;
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

static dance_result dancing_search(
    int k,
    struct dance_matrix *m,
    dance_result (*f)(int, struct data_object **),
    struct data_object **solution)
{
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
        dance_result subresult = dancing_search(k+1, m, f, solution);
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

int dance_solve(struct dance_matrix *m,
                dance_result (*f)(int, struct data_object **))
{
    struct data_object **solution = (struct data_object **)Malloc(m->ncolumns * sizeof *solution);
    dance_result result = dancing_search(0, m, f, solution);
    return result.count;
}
