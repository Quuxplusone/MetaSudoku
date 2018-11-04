
#include "dance.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USE_HEURISTIC 1

static char s_buffer[1000000];
static size_t s_index = 0;

void *Malloc(size_t n)
{
    n = (n + 15) & ~15;
    s_index += n;
    if (s_index <= sizeof s_buffer) return &s_buffer[s_index - n];
    printf("Out of memory in Malloc(%lu)!\n", (long unsigned)n);
    exit(EXIT_FAILURE);
}

void dance_init(struct dance_matrix *m, int rows, int cols, const int *data)
{
    m->nrows = rows;
    m->ncolumns = cols;
    m->columns = (struct column_object *)Malloc(m->ncolumns * sizeof *m->columns);
    m->head.data.right = &m->columns[0].data;
    m->head.data.left = &m->columns[m->ncolumns-1].data;

    for (int i=0; i < m->ncolumns; ++i) {
        m->columns[i].name = i;
        m->columns[i].size = 0;
        m->columns[i].data.up = &m->columns[i].data;
        m->columns[i].data.down = &m->columns[i].data;
        if (i > 0)
          m->columns[i].data.left = &m->columns[i-1].data;
        else m->columns[i].data.left = &m->head.data;
        if (i < cols-1)
          m->columns[i].data.right = &m->columns[i+1].data;
        else m->columns[i].data.right = &m->head.data;

        for (int j=0; j < rows; ++j) {
            if (data[j*cols+i] != 0) {
                /*
                   Found one! To insert it in the mesh at the
                   proper place, look to our left, and then
                   look up that column.
                */
                int ccol;
                struct data_object *o = (struct data_object *)Malloc(sizeof *o);
                o->down = &m->columns[i].data;
                o->up = o->down->up;
                o->down->up = o;
                o->up->down = o;
                o->left = o;
                o->right = o;
                o->column = &m->columns[i];

                for (ccol=i-1; ccol >= 0; --ccol) {
                    if (data[j*cols+ccol] != 0) {
                        break;
                    }
                }
                if (ccol >= 0) {
                    int cnt = 0;
                    struct data_object *left;
                    for (auto crow = j; crow >= 0; --crow) {
                        if (data[crow*cols+ccol] != 0) ++cnt;
                    }
                    left = &m->columns[ccol].data;
                    for ( ; cnt > 0; --cnt) {
                        left = left->down;
                    }
                    o->left = left;
                    o->right = left->right;
                    o->left->right = o;
                    o->right->left = o;
                }

                /* Done inserting this "1" into the mesh. */
                m->columns[i].size += 1;
            }
        }
    }
}

void dance_addrow(struct dance_matrix *m, int nentries, int *entries)
{
    struct data_object *h = nullptr;

    struct data_object *news = (struct data_object *)Malloc(nentries * sizeof *news);
    for (int i=0; i < nentries; ++i) {
        struct data_object *o = &news[i];
        o->column = &m->columns[entries[i]];
        o->down = &m->columns[entries[i]].data;
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

void dance_free(struct dance_matrix *m)
{
    s_index = 0;
}

static void dancing_cover(struct column_object *c)
{
    c->data.right->left = c->data.left;
    c->data.left->right = c->data.right;
    for (auto i = c->data.down; i != &c->data; i = i->down) {
        for (auto j = i->right; j != i; j = j->right) {
            j->down->up = j->up;
            j->up->down = j->down;
            j->column->size -= 1;
        }
    }
}

static void dancing_uncover(struct column_object *c)
{
    for (auto i = c->data.up; i != &c->data; i = i->up) {
        for (auto j = i->left; j != i; j = j->left) {
            j->column->size += 1;
            j->down->up = j;
            j->up->down = j;
        }
    }
    c->data.left->right = &c->data;
    c->data.right->left = &c->data;
}

static dance_result dancing_search(
    int k,
    struct dance_matrix *m,
    dance_result (*f)(int, struct data_object **),
    struct data_object **solution)
{
    dance_result result = {0, false};

    if (m->head.data.right == &m->head.data) {
        return f(k, solution);
    }

    /* Choose a column object |c|. */
#if USE_HEURISTIC
    struct column_object *c = nullptr;
    if (true) {
        int minsize = m->nrows + 1;
        for (auto j = m->head.data.right; j != &m->head.data; j = j->right) {
            struct column_object *jj = (struct column_object *)j;
            if (jj->size < minsize) {
                c = jj;
                minsize = jj->size;
                if (minsize <= 1) break;
            }
        }
    }
#else
    struct column_object *c = (struct column_object *)m->head.data.right;
#endif

    /* Cover column |c|. */
    dancing_cover(c);

    for (auto r = c->data.down; r != &c->data; r = r->down) {
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
