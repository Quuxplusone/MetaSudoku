#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

struct Solver {
    using AttackCount = uint8_t;
    int w_;
    int h_;
    int best_solution_ = 0;
    std::vector<AttackCount> grid_;
    std::vector<std::vector<int>> attacks_;

    bool is_attacking(int i, int j) const
    {
        int xi = i % w_, yi = i / w_;
        int xj = j % w_, yj = j / w_;
        if (xi == xj || yi == yj) return true;
        if (xi + yi == xj + yj) return true;
        if (xi - yi == xj - yj) return true;
        return false;
    }

    void backtracking_solve(int i, int red_encamped, int green_encamped)
    {
        // Try coloring this spot RED...
        for (int a : attacks_[i]) {
            if (grid_[a] == 0) --green_encamped;
            grid_[a] += 1;
        }
        if (green_encamped > best_solution_) {
            // We still have a chance to improve our best solution. Try recursing on this position.
            if (i == 0) {
                found_solution(red_encamped, green_encamped);
            } else {
                backtracking_solve(i-1, red_encamped + 1, green_encamped);
            }
        }
        // Now try not coloring it...
        for (int a : attacks_[i]) {
            grid_[a] -= 1;
            if (grid_[a] == 0) ++green_encamped;
        }
        if (red_encamped + i > best_solution_) {
            // We still have a chance to improve our best solution. Try recursing on this position.
            if (i == 0) {
                found_solution(red_encamped, green_encamped);
            } else {
                backtracking_solve(i-1, red_encamped, green_encamped);
            }
        }
    }

    void found_solution(int red_encamped, int green_encamped)
    {
        int smaller_encampment = (red_encamped < green_encamped) ? red_encamped : green_encamped;
        if (smaller_encampment > best_solution_) {
            best_solution_ = smaller_encampment;
            printf("Found a solution with red=%d green=%d\n", red_encamped, green_encamped);
            print_grid();
        }
    }

    bool is_attacked_by_green(int i) const {
        for (int ci=0; ci < w_*h_; ++ci) {
            if (is_attacking(ci, i) && grid_[ci] == 0) {
                return true;
            }
        }
        return false;
    }

    void print_grid() const {
        for (int j=0; j < h_; ++j) {
            for (int i=0; i < w_; ++i) {
                if (grid_[j*w_+i] == 0) {
                    putchar('G');
                } else if (not is_attacked_by_green(j*w_+i)) {
                    putchar('R');
                } else {
                    putchar('.');
                }
            }
            putchar('\n');
        }
    }

    explicit Solver(int w, int h) :
        w_(w), h_(h), attacks_(w*h), grid_(w*h)
    {
        for (int i=0; i < w*h; ++i) {
            for (int ci = 0; ci < w*h; ++ci) {
                if (is_attacking(i, ci)) {
                    attacks_[i].push_back(ci);
                }
            }
        }
    }
};

void solve_encampments_for(int w, int h)
{
    Solver solver(w, h);
    solver.backtracking_solve(w*h - 1, 0, w*h);
    printf("BOARD SIZE %d: ENCAMPMENT SIZE %d\n", w, solver.best_solution_);
    fflush(stdout);
}

int main(int argc, char **argv) {
    int n = (argc != 2) ? 0 : atoi(argv[1]);
    if (n < 3 || n > 1000) {
        if (n == 0) {
            puts("Usage: discrete-encampments 12");
            puts("  to solve for a 12x12 grid, for example");
        }
        puts("  Use an N between 3 and 1000, please");
        exit(1);
    }
    for (int i=n; i < 100; ++i) {
        solve_encampments_for(i, i);
    }
}
