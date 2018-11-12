
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dance.h"
#include "sudoku.h"
#include "odo-sudoku.h"

void odometer_to_grid(const OdometerWheel *odometer, int num_wheels,
                      int grid[9][9])
{
    memset(grid, '\0', 81 * sizeof(int));
    for (int i=0; i < num_wheels; ++i) {
        const OdometerWheel *odo = &odometer[i];
        int row = odo->i / 9;
        int col = odo->i % 9;
        grid[row][col] = odo->value;
    }
}

bool has_prior_conflict(const OdometerWheel *odo, int value)
{
    for (int i=0; i < odo->num_conflicts; ++i) {
        if (*odo->conflicts[i] == value) {
            return true;
        }
    }
    return false;
}

int count_solutions_with_odometer(const OdometerWheel *odometer, int num_wheels,
                                  int wheel, int next_unseen_value)
{
    if (wheel == num_wheels) {
        if (next_unseen_value >= 9) {
            static size_t counter = 0;
            ++counter;
            if ((counter & 0xFFFF) == 0) {
                printf("\rmeta %zu", counter);
                fflush(stdout);
            }
#if 1
            complete_odometer_sudoku(odometer, num_wheels);
            int solution_count = count_solutions_to_odometer_sudoku();
            if (solution_count == 1) {
                printf("This sudoku grid was a meta solution!\n");
                int grid[9][9];
                odometer_to_grid(odometer, num_wheels, grid);
                print_sudoku_grid(grid);
                printf("The unique solution to the sudoku grid above is:\n");
                print_unique_sudoku_solution(grid);
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
        odo->value = value;
        result += count_solutions_with_odometer(odometer, num_wheels, wheel+1, next_unseen_value);
        if (result >= 2) {
            printf("short-circuiting with result %d!\n", result);
            return result;  // short-circuit
        }
    }
    if (next_unseen_value <= 9) {
        odo->value = next_unseen_value;
        result += count_solutions_with_odometer(odometer, num_wheels, wheel+1, next_unseen_value+1);
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
                for (int w=0; w+1 < num_wheels; ++w) {
                    if (odometer[w].i == pc) {
                        odo->conflicts[odo->num_conflicts++] = &odometer[w].value;
                        break;
                    }
                }
            }
        }
    }

    begin_odometer_sudoku(flatgrid);

    // Now we've built our odometer.
    int num_solutions = count_solutions_with_odometer(odometer, num_wheels, 0, 1);
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
