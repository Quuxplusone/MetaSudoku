
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "dance.h"
#include "sudoku.h"

bool is_plausible_assignment(int grid[9][9]) {
    // It must introduce the digits in order.
    int looking_for = 1;
    for (int j=0; j < 9; ++j) {
        for (int i=0; i < 9; ++i) {
            int cell = grid[j][i];
            if (cell == 0) continue;
            if (cell > looking_for) return false;
            if (cell == looking_for) {
                ++looking_for;
                if (looking_for == 9) return true;
            }
        }
    }
    // It must contain at least 8 digits.
    return false;
}

int count_metasudoku_result(size_t n, struct data_object **sol)
{
    int grid[9][9] = {};
    size_t i, j;

    for (i=0; i < n; ++i) {
        int constraint[5];
        int row, col, val;
        row = col = val = 0;  /* shut up "unused" warning from compiler */
        constraint[0] = atoi(sol[i]->column->name);
        constraint[1] = atoi(sol[i]->right->column->name);
        constraint[2] = atoi(sol[i]->right->right->column->name);
        constraint[3] = atoi(sol[i]->right->right->right->column->name);
        constraint[4] = atoi(sol[i]->right->right->right->right->column->name);
        if (constraint[0] == constraint[2] || constraint[0] == constraint[3]) {
            continue;
        }
        for (j=0; j < 5; ++j) {
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

    static int cc = 0;
    printf("%d\r", ++cc);
    if (!is_plausible_assignment(grid)) {
        return 0;
    }

    printf("Metasolution:\n");
    for (j=0; j < 9; ++j) {
        printf("  ");
        for (i=0; i < 9; ++i)
          printf(" %d", grid[j][i]);
        printf("\n");
    }

    return 1;
}

bool metasudoku_has_exactly_one_solution(int grid[9][9])
{
    struct dance_matrix mat;
    int ns;
    size_t constraint[5];
    int rows = 0;
    int cols = 324;
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
    int maximum_value = 0;
    for (j=0; j < 9; ++j) {
        for (i=0; i < 9; ++i) {
            int box = (j/3)*3 + (i/3);
            if (grid[j][i] != 0) {
                if (maximum_value < 9) maximum_value += 1;
                for (k = 0; k < maximum_value; ++k) {
                    constraint[0] = 9*j + k;
                    constraint[1] = 81 + 9*i + k;
                    constraint[2] = 162 + 9*box + k;
                    constraint[3] = 243 + (9*j+i);
                    dance_addrow(&mat, 4, constraint); ++rows;
                }
            }
        }
    }

    /* Add the a-la-carte rows. Nine per row, per column, and per box. */
    for (i=0; i < 9; ++i) {
        for (k=0; k < 9; ++k) {
            constraint[0] = 9*i + k;  // row
            dance_addrow(&mat, 1, constraint); ++rows;
            constraint[0] = 81 + 9*i + k;  // column
            dance_addrow(&mat, 1, constraint); ++rows;
            constraint[0] = 162 + 9*i + k;  // box
            dance_addrow(&mat, 1, constraint); ++rows;
        }
    }
    /* Add the a-la-carte rows. One per grid cell. */
    for (j=0; j < 9; ++j) {
        for (i=0; i < 9; ++i) {
            if (grid[j][i] == 0) {
                constraint[0] = 243 + (9*j+i);
                dance_addrow(&mat, 1, constraint); ++rows;
            }
        }
    }

    printf("solving with %d rows and %d cols...\n", rows, cols);
    ns = dance_solve(&mat, count_metasudoku_result);
    dance_free(&mat);
    return (ns == 1);
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
    {0,0,0,8,0,1,0,0,0},
    {0,0,0,0,0,0,0,4,3},
    {5,0,0,0,0,0,0,0,0},
    {0,0,0,0,7,0,8,0,0},
    {0,0,0,0,0,0,1,0,0},
    {0,2,0,0,3,0,0,0,0},
    {6,0,0,0,0,0,0,7,5},
    {0,0,3,4,0,0,0,0,0},
    {0,0,0,2,0,0,6,0,0},
};

int main()
{
    bool r = sudoku_has_exactly_one_solution(sudoku_example_one);
    printf("result is %s\n", r ? "true" : "false");
    return 0;
}
