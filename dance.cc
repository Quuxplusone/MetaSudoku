
#include "dance.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USE_HEURISTIC 1

void *DanceMatrix::Malloc(size_t n)
{
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

    static_assert(sizeof(data_object) == 32);
    while (size_t(&memory_arena_[arena_used_]) % 128 != 0) ++arena_used_;
}
