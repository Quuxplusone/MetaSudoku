
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dance.h"
#include "sudoku.h"
#include "odo-sudoku.h"

static Workspace workspace;

constexpr Odometer odometer_from_grid(const int grid[9][9])
{
    Odometer odometer;
    for (int idx = 0; idx < 81; ++idx) {
        if (grid[idx/9][idx%9] == 0) continue;
        OdometerWheel new_wheel(idx);
        for (int i=0; i < odometer.num_wheels; ++i) {
            int pc = odometer.wheels[i].idx;
            bool same_row = (pc / 9 == idx / 9);
            bool same_col = (pc % 9 == idx % 9);
            bool same_box = ((pc / 9) / 3 == (idx / 9) / 3) && ((pc % 9) / 3 == (idx % 9) / 3);
            if (same_row || same_col || same_box) {
                new_wheel.add_conflict(i);
            }
        }
        odometer.add_wheel(new_wheel);
    }
    return odometer;
}

void odometer_to_grid(const Odometer& odometer, int grid[9][9])
{
    memset(grid, '\0', 81 * sizeof(int));
    for (int i=0; i < odometer.num_wheels; ++i) {
        const OdometerWheel& wheel = odometer.wheels[i];
        int row = wheel.idx / 9;
        int col = wheel.idx % 9;
        grid[row][col] = wheel.value;
    }
}

bool has_prior_conflict(const Odometer& odometer, const OdometerWheel& wheel, int value)
{
    for (int i=0; i < wheel.num_conflicts; ++i) {
        if (odometer.wheels[wheel.conflicts[i]].value == value) {
            return true;
        }
    }
    return false;
}

int count_solutions_with_odometer(Odometer& odometer, int wheel_idx, int next_unseen_value)
{
    if (wheel_idx == odometer.num_wheels) {
        if (next_unseen_value >= 9) {
            static size_t counter = 0;
            ++counter;
            if ((counter & 0xFFFF) == 0) {
                printf("\rmeta %zu", counter);
                fflush(stdout);
            }
#if 1
            workspace.complete_odometer_sudoku(odometer);
#if 1
            int solution_count = workspace.count_solutions_to_odometer_sudoku();
            if (solution_count == 1) {
                printf("This sudoku grid was a meta solution!\n");
                int grid[9][9];
                odometer_to_grid(odometer, grid);
                print_sudoku_grid(grid);
                printf("The unique solution to the sudoku grid above is:\n");
                print_unique_sudoku_solution(grid);
                return 1;
            }
#endif
#endif
        }
        return 0;
    }

    OdometerWheel *wheel = &odometer.wheels[wheel_idx];
    int result = 0;
    for (int value = 1; value < next_unseen_value; ++value) {
        if (has_prior_conflict(odometer, *wheel, value)) continue;
        wheel->value = value;
        result += count_solutions_with_odometer(odometer, wheel_idx+1, next_unseen_value);
        if (result >= 2) {
            printf("short-circuiting with result %d!\n", result);
            return result;  // short-circuit
        }
    }
    if (next_unseen_value <= 9) {
        wheel->value = next_unseen_value;
        result += count_solutions_with_odometer(odometer, wheel_idx+1, next_unseen_value+1);
    }
    return result;
}

bool metasudoku_has_exactly_one_solution(const int grid[9][9])
{
    workspace.begin_odometer_sudoku(grid);

    Odometer odometer = odometer_from_grid(grid);
    int num_solutions = count_solutions_with_odometer(odometer, 0, 1);
    printf("num_solutions is %d\n", num_solutions);
    return num_solutions == 1;
}


const int sudoku_example_newspaper[9][9] = {
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

const int sudoku_example_moose[9][9] = {
    {0,0,9,0,0,0,8,0,0},
    {0,6,0,0,0,0,0,4,0},
    {3,0,0,1,4,2,0,0,9},
    {0,1,4,0,0,0,9,6,0},
    {0,0,5,6,0,9,7,0,0},
    {0,0,8,0,0,0,2,0,0},
    {0,0,6,0,0,0,3,0,0},
    {0,0,0,9,0,5,0,0,0},
    {0,0,0,0,1,0,0,0,0},
};

int main()
{
    if (count_sudoku_solutions(sudoku_example_newspaper) != 1) {
        puts("FAILED SELF TEST"); exit(1);
    } else if (count_sudoku_solutions(sudoku_example_17) != 1) {
        puts("FAILED SELF TEST"); exit(1);
    } else if (count_sudoku_solutions(sudoku_example_moose) != 1) {
        puts("FAILED SELF TEST"); exit(1);
    }

    bool r = metasudoku_has_exactly_one_solution(sudoku_example_17);
    printf("result is %s\n", r ? "true" : "false");
    return 0;
}
