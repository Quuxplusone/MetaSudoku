#pragma once

#include <array>
#include <assert.h>

struct OdometerWheel {
    int idx = 0;  // refers to grid[idx/9][idx%9]
    int value = 0;
    int num_conflicts = 0;
    std::array<int, 20> conflicts = {};

    constexpr OdometerWheel() = default;
    constexpr explicit OdometerWheel(int i) : idx(i) {}

    constexpr void add_conflict(int previous_wheel_number) {
        assert(num_conflicts < 20);
        conflicts[num_conflicts++] = previous_wheel_number;
    }
};

struct Odometer {
    std::array<OdometerWheel, 81> wheels;
    int num_wheels = 0;

    constexpr void add_wheel(const OdometerWheel& new_wheel) {
        assert(num_wheels < 81);
        wheels[num_wheels++] = new_wheel;
    }

    constexpr Odometer() = default;
};

struct Workspace {
    DanceMatrix mat;
    DanceMatrix mat2;
    size_t processed = 0;

    void begin_odometer_sudoku(const int grid[9][9]);
    void complete_odometer_sudoku(const Odometer& odometer);
    int count_solutions_to_odometer_sudoku();
};

