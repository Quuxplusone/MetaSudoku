#pragma once

struct OdometerWheel {
    int i;  // refers to grid[i/9][i%9]
    mutable int value;
    int num_conflicts;
    const int *conflicts[24];
};

void begin_odometer_sudoku(const int flatgrid[81]);

void complete_odometer_sudoku(
    const OdometerWheel *odometer, int num_wheels);

int count_solutions_to_odometer_sudoku();

