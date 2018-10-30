
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

void Free(void *p)
{
    // Do nothing.
}

int dance_init(struct dance_matrix *m,
        size_t rows, size_t cols, const int *data)
{
    size_t i, j;

    m->nrows = rows;
    m->ncolumns = cols;
    m->columns = Malloc(m->ncolumns * sizeof *m->columns);
    m->head.data.right = &m->columns[0].data;
    m->head.data.left = &m->columns[m->ncolumns-1].data;

    for (i=0; i < m->ncolumns; ++i) {
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

        for (j=0; j < rows; ++j) {
            if (data[j*cols+i] != 0) {
                /*
                   Found one! To insert it in the mesh at the
                   proper place, look to our left, and then
                   look up that column.
                */
                int ccol;
                struct data_object *new = Malloc(sizeof *new);
                new->down = &m->columns[i].data;
                new->up = new->down->up;
                new->down->up = new;
                new->up->down = new;
                new->left = new;
                new->right = new;
                new->column = &m->columns[i];

                for (ccol=i-1; ccol >= 0; --ccol) {
                    if (data[j*cols+ccol] != 0) break;
                }
                if (ccol >= 0) {
                    int crow, cnt=0;
                    struct data_object *left;
                    for (crow=j; crow >= 0; --crow)
                      if (data[crow*cols+ccol] != 0) ++cnt;
                    left = &m->columns[ccol].data;
                    for ( ; cnt > 0; --cnt)
                      left = left->down;
                    new->left = left;
                    new->right = left->right;
                    new->left->right = new;
                    new->right->left = new;
                }

                /* Done inserting this "1" into the mesh. */
                m->columns[i].size += 1;
            }
        }
    }

    return 0;
}

int dance_addrow(struct dance_matrix *m, size_t nentries, size_t *entries)
{
    struct data_object *h = NULL;
    size_t i;

    for (i=0; i < nentries; ++i) {
        struct data_object *new = Malloc(sizeof *new);
        new->column = &m->columns[entries[i]];
        new->down = &m->columns[entries[i]].data;
        new->up = new->down->up;
        new->down->up = new;
        new->up->down = new;
        if (h != NULL) {
            new->left = h->left;
            new->right = h;
            new->left->right = new;
            new->right->left = new;
        }
        else {
            new->left = new->right = new;
            h = new;
        }
        new->column->size += 1;
    }

    m->nrows += 1;
    return nentries;
}

int dance_free(struct dance_matrix *m)
{
    s_index = 0;
    return 0;
}

int dance_solve(struct dance_matrix *m,
                dance_result (*f)(size_t, struct data_object **))
{
    struct data_object **solution = Malloc(m->ncolumns * sizeof *solution);
    dance_result result = dancing_search(0, m, f, solution);
    Free(solution);
    return result.count;
}

dance_result dancing_search(
    size_t k,
    struct dance_matrix *m,
    dance_result (*f)(size_t, struct data_object **),
    struct data_object **solution)
{
    struct column_object *c = NULL;
    struct data_object *r, *j;
    dance_result result = {0, false};

    if (m->head.data.right == &m->head.data) {
        return f(k, solution);
    }

    /* Choose a column object |c|. */
#if USE_HEURISTIC
    { size_t minsize = m->nrows+1;
    for (j = m->head.data.right; j != &m->head.data; j = j->right) {
        struct column_object *jj = (struct column_object *)j;
        if (jj->size < minsize) {
            c = jj;
            minsize = jj->size;
            if (minsize <= 1) break;
        }
    }
    }
#else
    c = (struct column_object *)m->head.data.right;
#endif

    /* Cover column |c|. */
    dancing_cover(c);

    for (r = c->data.down; r != &c->data; r = r->down) {
        solution[k] = r;
        for (j = r->right; j != r; j = j->right) {
            dancing_cover(j->column);
        }
        dance_result subresult = dancing_search(k+1, m, f, solution);
        result.count += subresult.count;
        result.short_circuit = subresult.short_circuit;
        r = solution[k];
        c = r->column;
        for (j = r->left; j != r; j = j->left) {
            dancing_uncover(j->column);
        }
        if (result.short_circuit) {
            break;
        }
    }

    /* Uncover column |c| and return. */
    dancing_uncover(c);
    return result;
}

void dancing_cover(struct column_object *c)
{
    struct data_object *i, *j;

    c->data.right->left = c->data.left;
    c->data.left->right = c->data.right;
    for (i = c->data.down; i != &c->data; i = i->down) {
        for (j = i->right; j != i; j = j->right) {
            j->down->up = j->up;
            j->up->down = j->down;
            j->column->size -= 1;
        }
    }
}

void dancing_uncover(struct column_object *c)
{
    struct data_object *i, *j;

    for (i = c->data.up; i != &c->data; i = i->up) {
        for (j = i->left; j != i; j = j->left) {
            j->column->size += 1;
            j->down->up = j;
            j->up->down = j;
        }
    }
    c->data.left->right = &c->data;
    c->data.right->left = &c->data;
}
