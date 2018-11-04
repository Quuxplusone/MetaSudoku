#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct dance_result {
    int count;
    int short_circuit;
} dance_result;

struct data_object {
    struct data_object *up, *down, *left, *right;
    struct column_object *column;
};

struct column_object {
    struct data_object data; /* must be the first field */
    size_t size;
    int name;
};

struct dance_matrix {
    size_t nrows, ncolumns;
    struct column_object *columns;
    struct column_object head;
};

void dance_init(struct dance_matrix *m, size_t rows, size_t cols, const int *data);
void dance_addrow(struct dance_matrix *m, size_t nentries, size_t *entries);
void dance_free(struct dance_matrix *m);
int dance_solve(struct dance_matrix *m, dance_result (*f)(size_t, struct data_object **));
