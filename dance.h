#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
