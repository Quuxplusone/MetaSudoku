
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dance.h"
#include "sudoku.h"

struct OdometerWheel {
    int i;  // refers to grid[i/9][i%9]
    int num_conflicts;
    const int *conflicts[24];
};

bool has_prior_conflict(const struct OdometerWheel *odo, int value)
{
    for (int i=0; i < odo->num_conflicts; ++i) {
        if (*odo->conflicts[i] == value) {
            return true;
        }
    }
    return false;
}

int count_solutions_with_odometer(const struct OdometerWheel *odometer, int num_wheels,
                                  int flatgrid[81],
                                  int wheel, int next_unseen_value)
{
    if (wheel == num_wheels) {
        if (next_unseen_value >= 9) {
            static size_t counter = 0;
            ++counter;
            if ((counter & 0xFFF) == 0) {
                printf("\rmeta %zu", counter);
                fflush(stdout);
            }
#if 1
            int solution_count = count_sudoku_solutions((const int(*)[9])flatgrid);
            if (solution_count == 1) {
                printf("This sudoku grid was a meta solution!\n");
                print_sudoku_grid((const int(*)[9])flatgrid);
                printf("The unique solution to the sudoku grid above is:\n");
                print_unique_sudoku_solution((const int(*)[9])flatgrid);
                return 1;
            }
#endif
        }
        return 0;
    }

    const struct OdometerWheel *odo = &odometer[wheel];
    int result = 0;
    for (int value = 1; value < next_unseen_value; ++value) {
        if (has_prior_conflict(odo, value)) continue;
        flatgrid[odo->i] = value;
        result += count_solutions_with_odometer(odometer, num_wheels, flatgrid, wheel+1, next_unseen_value);
        if (result >= 2) {
            printf("short-circuiting with result %d!\n", result);
            return result;  // short-circuit
        }
    }
    if (next_unseen_value <= 9) {
        flatgrid[odo->i] = next_unseen_value;
        result += count_solutions_with_odometer(odometer, num_wheels, flatgrid, wheel+1, next_unseen_value+1);
    }
    return result;
}

bool metasudoku_has_exactly_one_solution(const int grid[9][9])
{
    int flatgrid[81];
    memcpy(flatgrid, grid, sizeof flatgrid);

    struct OdometerWheel odometer[81];
    int num_wheels = 0;

    for (int i = 0; i < 81; ++i) {
        if (flatgrid[i] == 0) continue;
        struct OdometerWheel *odo = &odometer[num_wheels++];
        odo->i = i;
        odo->num_conflicts = 0;
        for (int pc = 0; pc < i; ++pc) {
            if (flatgrid[pc] == 0) continue;
            bool same_row = (pc / 9 == i / 9);
            bool same_col = (pc % 9 == i % 9);
            bool same_box = ((pc / 9) / 3 == (i / 9) / 3) && ((pc % 9) / 3 == (i % 9) / 3);
            if (same_row || same_col || same_box) {
                odo->conflicts[odo->num_conflicts++] = &flatgrid[pc];
            }
        }
    }

    // Now we've built our odometer.
    int num_solutions = count_solutions_with_odometer(odometer, num_wheels, flatgrid, 0, 1);
    printf("num_solutions is %d\n", num_solutions);
    return num_solutions == 1;
}


const int sudoku_example_one[9][9] = {
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

const int sudoku_example_17[9][9] = {
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
    if (count_sudoku_solutions(sudoku_example_one) != 1) {
        puts("FAILED SELF TEST"); exit(1);
    } else if (count_sudoku_solutions(sudoku_example_17) != 1) {
        puts("FAILED SELF TEST"); exit(1);
    }

    bool r = metasudoku_has_exactly_one_solution(sudoku_example_17);
    printf("result is %s\n", r ? "true" : "false");
    return 0;
}
