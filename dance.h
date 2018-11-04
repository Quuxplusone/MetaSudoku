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
    int size;
    int name;
};

struct dance_matrix {
    int nrows, ncolumns;
    struct column_object *columns;
    struct column_object head;
};

void dance_init(struct dance_matrix *m, int rows, int cols, const int *data);
void dance_addrow(struct dance_matrix *m, int nentries, int *entries);
void dance_free(struct dance_matrix *m);
int dance_solve(struct dance_matrix *m, dance_result (*f)(int, struct data_object **));
