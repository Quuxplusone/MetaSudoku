
#include "sudoku.h"

#include <stdio.h>
#include "dance.h"

static size_t g_sudoku_counter = 0;

static dance_result count_sudoku_result(int n, struct data_object **cols)
{
    (void)n;
    (void)cols;
    dance_result result;
    result.count = 1;
    result.short_circuit = (++g_sudoku_counter >= 2);
    return result;
}

static int solve_sudoku_with_callback(
    const int grid[9][9],
    dance_result (*f)(int, struct data_object **))
{
    g_sudoku_counter = 0;

    int constraint[4];
    int cols = 9*(9+9+9)+81;
    /*
       1 in the first row; 2 in the first row;... 9 in the first row;
       1 in the second row;... 9 in the ninth row;
       1 in the first column; 2 in the first column;...
       1 in the first box; 2 in the first box;... 9 in the ninth box;
       Something in (1,1); Something in (1,2);... Something in (9,9)
    */
    int i, j, k;

    dance_matrix *mat = dance_init(cols);
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
                dance_addrow(mat, 4, constraint);
                seen_this_row[grid[j][i]-1] = 1;
            }
            else {
                for (k=0; k < 9; ++k) {
                    if (seen_this_row[k]) continue;
                    constraint[0] = 9*j + k;
                    constraint[1] = 81 + 9*i + k;
                    constraint[2] = 162 + 9*box + k;
                    constraint[3] = 243 + (9*j+i);
                    dance_addrow(mat, 4, constraint);
                }
            }
        }
    }

    int ns = dance_solve(mat, f);
    dance_free();
    return ns;
}

int count_sudoku_solutions(const int grid[9][9])
{
    g_sudoku_counter = 0;
    return solve_sudoku_with_callback(grid, count_sudoku_result);
}

void print_sudoku_grid(const int grid[9][9])
{
    for (int j=0; j < 9; ++j) {
        printf("   ");
        for (int i=0; i < 9; ++i)
          printf(" %d", grid[j][i]);
        printf("\n");
    }
}

static dance_result print_unique_sudoku_result(int n, struct data_object **sol)
{
    int grid[9][9];

    for (int i=0; i < n; ++i) {
        int constraint[4];
        int row, col, val;
        row = col = val = 0;  /* shut up "unused" warning from compiler */
        constraint[0] = sol[i]->left->column->name;
        constraint[1] = sol[i]->column->name;
        constraint[2] = sol[i]->right->column->name;
        constraint[3] = sol[i]->right->right->column->name;
        for (int j=0; j < 4; ++j) {
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

    printf("-----\n");
    print_sudoku_grid(grid);

    dance_result result;
    result.count = 1;
    result.short_circuit = false;
    return result;
}

void print_unique_sudoku_solution(const int grid[9][9])
{
    solve_sudoku_with_callback(grid, print_unique_sudoku_result);
}
