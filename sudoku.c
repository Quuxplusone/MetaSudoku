
#include "sudoku.h"

#include <stdbool.h>
#include <stdio.h>
#include "dance.h"

int count_sudoku_result(size_t n, struct data_object **cols)
{
    (void)n;
    (void)cols;
    printf("\n  found sudoku solution");
    return 1;
}

bool sudoku_has_exactly_one_solution(int grid[9][9])
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

    ns = dance_solve(&mat, count_sudoku_result);
    dance_free(&mat);
    return (ns == 1);
}
