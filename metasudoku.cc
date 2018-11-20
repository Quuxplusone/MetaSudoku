#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dance.h"
#include "sudoku.h"
#include "odo-sudoku.h"
#include "work-queue.h"

static size_t count_of_viable_grids = 0;

Odometer odometer_from_grid(const int grid[9][9])
{
    // Filling the grid in non-reading order actually
    // helps us find solvable Sudokus more quickly.
    // The particular order chosen here is arbitrary.
    static const int transform_idx[81] = {
        30, 71, 34, 51, 36,  9, 20, 53, 38,
        33,  0, 31, 70, 57, 52, 37,  8, 21,
        72, 29, 50, 35, 10, 19, 54, 39,  6,
        49, 32,  1, 56, 69, 58,  7, 22, 61,
        28, 73, 48, 11, 18, 55, 60,  5, 40,
        47, 12, 27,  2, 59, 68, 41, 62, 23,
        74, 15, 76, 79, 26, 17,  4, 65, 42,
        77, 46, 13, 16,  3, 44, 67, 24, 63,
        14, 75, 78, 45, 80, 25, 64, 43, 66,
    };

    Odometer odometer;
    for (int pre_idx = 0; pre_idx < 81; ++pre_idx) {
        int idx = transform_idx[pre_idx];
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
    // However, since we fill the squares in non-reading order, we
    // should actually remap the numbers so that they *do* read in
    // reading order (starting with '1' in the upper-left-most position).
    int mapping[10] {};
    int next_unseen_value = 1;
    for (int i=0; i < 81; ++i) {
        int value = grid[i/9][i%9];
        if (value == 0) continue;
        if (mapping[value] == 0) {
            mapping[value] = next_unseen_value++;
        }
        grid[i/9][i%9] = mapping[value];
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

struct Taskmaster : public RoundRobinPool<Workspace, Odometer, NUM_THREADS, Taskmaster>
{
    std::mutex mtx_;
    std::atomic<int> solutions_{0};

    size_t count_processed() {
        size_t count = 0;
        this->for_each_state([&](const Workspace& workspace) {
            count += workspace.processed;
        });
        return count;
    }

    void process(Workspace& workspace, const Odometer& odometer) {
        workspace.complete_odometer_sudoku(odometer);
        int solution_count = workspace.count_solutions_to_odometer_sudoku();
        if (solution_count == 1) {
            std::lock_guard<std::mutex> lk(mtx_);
            printf("This sudoku grid was a meta solution!\n");
            int grid[9][9];
            odometer_to_grid(odometer, grid);
            print_sudoku_grid(grid);
            printf("The unique solution to the sudoku grid above is:\n");
            print_unique_sudoku_solution(grid);
            int found = ++solutions_;
            if (found >= 2) {
                throw ConsumerShutDownException();
            }
        }
        workspace.processed += 1;
    }
};

template<int SHORT_CUT_FACTOR>
constexpr size_t pow9() {
    static_assert(SHORT_CUT_FACTOR <= 16, "9**16 is 2**50");
    size_t result = 1;
    for (int i = 0; i < SHORT_CUT_FACTOR; ++i) result *= 9;
    return result;
}

template<int SHORT_CUT_FACTOR>
int count_solutions_with_odometer(Taskmaster& taskmaster, Odometer& odometer, int wheel_idx, int next_unseen_value)
{
    if (wheel_idx == odometer.num_wheels - SHORT_CUT_FACTOR) {
        if (SHORT_CUT_FACTOR != 0 || next_unseen_value >= 9) {
#if JUST_COUNT_VIABLE_GRIDS
            count_of_viable_grids += pow9<SHORT_CUT_FACTOR>();
            if ((count_of_viable_grids & 0xFFFF) == 0) {
                printf("\rmeta %zu", count_of_viable_grids);
            }
#else
            static size_t counter = 0;
            ++counter;
            if ((counter & 0xFFFF) == 0) {
                size_t processed = taskmaster.count_processed();
                printf("\rmeta %zu (+%zu)", counter, counter - processed);
                if ((counter - processed) > 1000000) {
                    // Sleep and let the worker threads catch up.
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    printf("\rmeta %zu (+%zu)", counter, counter - taskmaster.count_processed());
                }
                fflush(stdout);
            }
            taskmaster.push(odometer);
#endif
        }
        return 0;
    }

    OdometerWheel *wheel = &odometer.wheels[wheel_idx];
    int result = 0;
    for (int value = 1; value < next_unseen_value; ++value) {
        if (has_prior_conflict(odometer, *wheel, value)) continue;
        wheel->value = value;
        result += count_solutions_with_odometer<SHORT_CUT_FACTOR>(taskmaster, odometer, wheel_idx+1, next_unseen_value);
        if (result >= 2) {
            printf("short-circuiting with result %d!\n", result);
            return result;  // short-circuit
        }
    }
    if (next_unseen_value <= 9) {
        wheel->value = next_unseen_value;
        result += count_solutions_with_odometer<SHORT_CUT_FACTOR>(taskmaster, odometer, wheel_idx+1, next_unseen_value+1);
    }
    return result;
}

bool metasudoku_has_exactly_one_solution(const int grid[9][9])
{
    Taskmaster taskmaster;
    taskmaster.for_each_state([&](Workspace& workspace) {
        workspace.begin_odometer_sudoku(grid);
    });
    taskmaster.start_threads();

    Odometer odometer = odometer_from_grid(grid);
    try {
        count_solutions_with_odometer<0>(taskmaster, odometer, 0, 1);
    } catch (const ProducerShutDownException&) {
        puts("caught the short-circuit");
        taskmaster.shutdown_from_producer_side();
    }

    taskmaster.shutdown_when_empty();
    taskmaster.wait();
    int num_solutions = taskmaster.solutions_;
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

const int sudoku_example_gordon_royle_unique[9][9] = {
    {0,0,0,0,0,0,0,1,4},
    {0,0,0,0,0,0,2,0,3},
    {8,0,0,0,5,0,0,0,0},
    {0,0,0,2,0,7,0,0,0},
    {0,3,1,0,0,0,0,0,0},
    {0,0,0,0,0,0,6,5,0},
    {6,0,0,0,0,0,7,0,0},
    {0,0,0,1,4,0,0,0,0},
    {0,0,0,3,0,0,0,0,0},
};

int main()
{
    if (count_sudoku_solutions(sudoku_example_newspaper) != 1) {
        puts("FAILED SELF TEST"); exit(1);
    } else if (count_sudoku_solutions(sudoku_example_17) != 1) {
        puts("FAILED SELF TEST"); exit(1);
    } else if (count_sudoku_solutions(sudoku_example_moose) != 1) {
        puts("FAILED SELF TEST"); exit(1);
    } else if (count_sudoku_solutions(sudoku_example_gordon_royle_unique) != 1) {
        puts("FAILED SELF TEST"); exit(1);
    }

    const auto& grid = sudoku_example_gordon_royle_unique;

#if JUST_COUNT_VIABLE_GRIDS
    Taskmaster dummy;
    Odometer odometer = odometer_from_grid(grid);
    count_of_viable_grids = 0;
    count_solutions_with_odometer<9>(dummy, odometer, 0, 1);
    printf("\nWith SHORT_CUT_FACTOR=9, the number of viable grids is <= %zu\n", count_of_viable_grids);
    count_of_viable_grids = 0;
    count_solutions_with_odometer<0>(dummy, odometer, 0, 1);
    printf("\nThe number of viable grids is exactly %zu\n", count_of_viable_grids);
#else
    bool r = metasudoku_has_exactly_one_solution(grid);
    printf("metasudoku %s have exactly one solution\n", r ? "does" : "does not");
#endif
    return 0;
}
