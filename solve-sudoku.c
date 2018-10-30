
/*
   This program is copyright Arthur O'Dwyer, December 2005.
   It is free for all non-commercial use. Please keep it free.

   This program implements Donald Knuth's "dancing links"
   algorithm for finding an exact row covering of a matrix
   of 0s and 1s. Knuth's small example is demonstrated, and
   then an application to Sudoku solving is shown.

   For Knuth's original paper, see
   http://xxx.lanl.gov/PS_cache/cs/pdf/0011/0011047.pdf

   For information on Dancing Links in general, see
   http://en.wikipedia.org/wiki/Dancing_Links
*/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 
   The |USE_HEURISTIC| macro tells the |dance_solve| routine to
   check out the columns with the fewest "1" entries first. The
   same answers will be produced, but possibly in a different
   order, and possibly faster. (It certainly speeds up the solving
   of newspaper Sudoku puzzles!)
*/
#define USE_HEURISTIC 1


struct data_object {
    struct data_object *up, *down, *left, *right;
    struct column_object *column;
};

struct column_object {
    struct data_object data; /* must be the first field */
    size_t size;
    char *name;
};

struct dance_matrix {
    size_t nrows, ncolumns;
    struct column_object *columns;
    struct column_object head;
};

int dance_init(struct dance_matrix *m,
        size_t rows, size_t cols, const int *data);
int dance_init_named(struct dance_matrix *m,
        size_t rows, size_t cols, const int *data, char **names);
int dance_addrow(struct dance_matrix *m,
        size_t nentries, size_t *entries);
int dance_addrow_named(struct dance_matrix *m,
        size_t nentries, char **names);
int dance_free(struct dance_matrix *m);
int dance_print(struct dance_matrix *m);
int dance_solve(struct dance_matrix *m,
                int (*f)(size_t, struct data_object **));
 int dancing_search(size_t k, struct dance_matrix *m,
                    int (*f)(size_t, struct data_object **),
                    struct data_object **solution);
 void dancing_cover(struct column_object *c);
 void dancing_uncover(struct column_object *c);


void *Malloc(size_t n)
{
    void *p = malloc(n);
    if (p != NULL) return p;
    printf("Out of memory in Malloc(%lu)!\n", (long unsigned)n);
    exit(EXIT_FAILURE);
}

int dance_init(struct dance_matrix *m,
        size_t rows, size_t cols, const int *data)
{
    char **names = Malloc(cols * sizeof *names);
    size_t i;
    int rc;

    if (cols < 26) {
        const char *alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        for (i=0; i < cols; ++i) {
            names[i] = Malloc(2);
            sprintf(names[i], "%c", alpha[i]);
        }
    }
    else {
        for (i=0; i < cols; ++i) {
            names[i] = Malloc(30);
            sprintf(names[i], "%lu", (long unsigned)i);
        }
    }

    rc = dance_init_named(m, rows, cols, data, names);
    for (i=0; i < cols; ++i)
      free(names[i]);
    free(names);
    return rc;
}

int dance_init_named(struct dance_matrix *m,
        size_t rows, size_t cols, const int *data, char **names)
{
    size_t i, j;

    m->nrows = rows;
    m->ncolumns = cols;
    m->columns = Malloc(m->ncolumns * sizeof *m->columns);
    m->head.data.right = &m->columns[0].data;
    m->head.data.left = &m->columns[m->ncolumns-1].data;

    for (i=0; i < m->ncolumns; ++i) {
        m->columns[i].name = Malloc(strlen(names[i])+1);
        strcpy(m->columns[i].name, names[i]);
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


int dance_addrow_named(struct dance_matrix *m, size_t nentries, char **names)
{
    size_t *entries = Malloc(nentries * sizeof *entries);
    size_t i, j;
    int rc;

    for (i=0; i < nentries; ++i) {
        for (j=0; j < m->ncolumns; ++j) {
            if (strcmp(m->columns[j].name, names[i]) == 0)
              break;
        }
        if (j == m->ncolumns) return -1;
        entries[i] = j;
    }
    rc = dance_addrow(m, nentries, entries);
    free(entries);
    return rc;
}


int dance_free(struct dance_matrix *m)
{
    size_t i;
    for (i=0; i < m->ncolumns; ++i) {
        struct data_object *p = m->columns[i].data.down;
        while (p != &m->columns[i].data) {
            struct data_object *q = p->down;
            free(p);
            p = q;
        }
        free(m->columns[i].name);
    }
    free(m->columns);
    return 0;
}


int dance_print(struct dance_matrix *m)
{
    size_t i;
    struct data_object *p;

    for (i=0; i < m->ncolumns; ++i) {
        printf("Column %s has %lu elements:\n", m->columns[i].name, 
            (long unsigned)m->columns[i].size);
        p = m->columns[i].data.down;
        while (p != &m->columns[i].data) {
            printf("  X");
            p = p->down;
        }
        printf("\n");
    }

    return 0;
}


int dance_solve(struct dance_matrix *m,
                int (*f)(size_t, struct data_object **))
{
    struct data_object **solution = Malloc(m->ncolumns * sizeof *solution);
    int ns = dancing_search(0, m, f, solution);
    free(solution);
    return ns;
}


int dancing_search(size_t k, struct dance_matrix *m,
    int (*f)(size_t, struct data_object **),
    struct data_object **solution)
{
    struct column_object *c = NULL;
    struct data_object *r, *j;
    int count = 0;

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
        count += dancing_search(k+1, m, f, solution);
        r = solution[k];
        c = r->column;
        for (j = r->left; j != r; j = j->left) {
            dancing_uncover(j->column);
        }
    }

    /* Uncover column |c| and return. */
    dancing_uncover(c);
    return count;
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


void sudoku_solve(int grid[9][9]);
int print_sudoku_result(size_t n, struct data_object **sol);

void sudoku_solve(int grid[9][9])
{
    struct dance_matrix mat;
    int ns;
    size_t constraint[4];
    int rows = 0;
    int cols = 9*(9+9+9)+81;
    /*
       1 in the first row; 2 in the first row;... 9 in the first row;
       1 in the second row;... 9 in the ninth row;
       1 in the first column; 2 in the first column;...
       1 in the first box; 2 in the first box;... 9 in the ninth box;
       Something in (1,1); Something in (1,2);... Something in (9,9)
    */
    int i, j, k;

    dance_init(&mat, 0, cols, NULL);
    /*
       Input the grid, square by square. Each possibility for
       a single number in a single square gives us a row of the
       matrix with exactly four entries in it.
    */
    for (j=0; j < 9; ++j) {
        int seen_this_row[9] = {0};
        for (i=0; i < 9; ++i) {
            int box = (j/3)*3 + (i/3);
            if (grid[j][i] != 0) {
                constraint[0] = 9*j + grid[j][i]-1;
                constraint[1] = 81 + 9*i + grid[j][i]-1;
                constraint[2] = 162 + 9*box + grid[j][i]-1;
                constraint[3] = 243 + (9*j+i);
                dance_addrow(&mat, 4, constraint); ++rows;
                seen_this_row[grid[j][i]-1] = 1;
            }
            else {
                for (k=0; k < 9; ++k) {
                    if (seen_this_row[k]) continue;
                    constraint[0] = 9*j + k;
                    constraint[1] = 81 + 9*i + k;
                    constraint[2] = 162 + 9*box + k;
                    constraint[3] = 243 + (9*j+i);
                    dance_addrow(&mat, 4, constraint); ++rows;
                }
            }
        }
    }

    printf("The completed matrix has %d columns and %d rows.\n", 
        (int)mat.ncolumns, (int)mat.nrows);
    printf("Solving...\n");

    ns = dance_solve(&mat, print_sudoku_result);

    printf("There w%s %d solution%s found.\n", (ns==1)? "as": "ere", 
        ns, (ns==1)? "": "s");
    dance_free(&mat);
}

int print_sudoku_result(size_t n, struct data_object **sol)
{
    int grid[9][9];
    size_t i, j;

    for (i=0; i < n; ++i) {
        int constraint[4];
        int row, col, val;
        row = col = val = 0;  /* shut up "unused" warning from compiler */
        constraint[0] = atoi(sol[i]->left->column->name);
        constraint[1] = atoi(sol[i]->column->name);
        constraint[2] = atoi(sol[i]->right->column->name);
        constraint[3] = atoi(sol[i]->right->right->column->name);
        for (j=0; j < 4; ++j) {
            if (constraint[j] < 81) {
                row = constraint[j] / 9;
                val = constraint[j] % 9 + 1;
            }
            else if (constraint[j] < 162) {
                col = (constraint[j]-81) / 9;
            }
        }
        grid[row][col] = val;
    }

    printf("Solution:\n");
    for (j=0; j < 9; ++j) {
        printf("  ");
        for (i=0; i < 9; ++i)
          printf(" %d", grid[j][i]);
        printf("\n");
    }

    return 1;
}

int sudoku_example_one[9][9] = {
    {4,8,0,9,2,0,3,0,0},
    {9,5,0,0,8,0,0,0,4},
    {0,0,2,5,0,0,0,0,0},
    {0,0,0,0,0,4,0,0,7},
    {5,4,0,0,3,0,0,9,2},
    {8,0,0,7,0,0,0,0,0},
    {0,0,0,0,0,5,2,0,0},
    {3,0,0,0,7,0,0,6,1},
    {0,0,5,0,1,9,0,4,3},
};

int sudoku_example_17[9][9] = {
    {0, 0, 0, 1, 0, 2, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 1, 2},
    {1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 2, 0, 0},
    {0, 0, 0, 0, 0, 0, 1, 0, 0},
    {0, 1, 0, 0, 2, 0, 0, 0, 0},
    {3, 0, 0, 0, 0, 0, 0, 4, 1},
    {0, 0, 5, 6, 0, 0, 0, 0, 0},
    {0, 0, 0, 7, 0, 0, 8, 0, 0},
};

int main()
{
    sudoku_solve(sudoku_example_one);
    sudoku_solve(sudoku_example_17);
}
