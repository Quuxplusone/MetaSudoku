#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <random>

struct Solver {
    using AttackCount = uint8_t;

    struct SolutionState {
        int red_encamped;
        int green_encamped;
        int blue_encamped;
        int max_possible_red_encamped;
        int max_possible_green_encamped;
        int max_possible_blue_encamped;
        int max_possible_magenta_encamped;

        static SolutionState initial(int n) {
            return SolutionState{0, 0, 0, n, n, n, n};
        }

        static SolutionState worst_solution(int n) {
            int i = 0;
            for (i=0; i*i < n; ++i) {}
            if (i >= 14 && i % 2 == 0) return SolutionState{i-2, i-2, i-2, 0, 0, 0, i-3};

            if (n >= 12*12) return SolutionState{7, 7, 7, 0, 0, 0, 7-1};
            if (n >= 10*10) return SolutionState{5, 5, 5, 0, 0, 0, 5-1};
            if (n >= 9*9) return SolutionState{4, 4-1, 3, 0, 0, 0, 3};
            if (n >= 8*8) return SolutionState{4-1, 3, 3, 0, 0, 0, 3};
            if (n >= 7*7) return SolutionState{3, 3-1, 2, 0, 0, 0, 2};
            return SolutionState{0, 0, 0, 0, 0, 0, 0};
        }

        SolutionState best_possible(int i) const {
            // This is the best possible outcome given the invariant that RED >= GREEN >= BLUE >= MAGENTA.
            return SolutionState{
                max_possible_red_encamped,
                std::min(max_possible_green_encamped, max_possible_red_encamped),
                std::min(max_possible_blue_encamped, std::min(max_possible_green_encamped, max_possible_red_encamped)),
                0, 0, 0,
                std::min(max_possible_magenta_encamped, std::min(max_possible_blue_encamped, std::min(max_possible_green_encamped, max_possible_red_encamped)))
            };
        }

        bool is_valid() const {
            return (red_encamped >= green_encamped && green_encamped >= blue_encamped && blue_encamped >= max_possible_magenta_encamped);
        }

        friend bool operator<(const SolutionState& a, const SolutionState& b) {
            if (a.max_possible_magenta_encamped != b.max_possible_magenta_encamped) return (a.max_possible_magenta_encamped < b.max_possible_magenta_encamped);
            if (a.blue_encamped != b.blue_encamped) return (a.blue_encamped < b.blue_encamped);
            if (a.green_encamped != b.green_encamped) return (a.green_encamped < b.green_encamped);
            if (a.red_encamped != b.red_encamped) return (a.red_encamped < b.red_encamped);
            return false;
        }
        friend bool operator>(const SolutionState& a, const SolutionState& b) { return (b < a); }

        bool could_still_beat(int i, const SolutionState& rhs) {
            return this->best_possible(i) > rhs;
        }
    };

    int w_;
    int h_;
    SolutionState best_solution_;
    std::vector<AttackCount> red_attacks_;
    std::vector<AttackCount> green_attacks_;
    std::vector<AttackCount> blue_attacks_;
    std::vector<std::vector<int>> attack_vectors_;
    std::vector<int> position_lut_;

    bool is_attacking(int i, int j) const
    {
        int xi = i % w_, yi = i / w_;
        int xj = j % w_, yj = j / w_;
        if (xi == xj || yi == yj) return true;
        if (xi + yi == xj + yj) return true;
        if (xi - yi == xj - yj) return true;
        return false;
    }

    void backtracking_solve(int i, SolutionState s)
    {
        int pos = position_lut_[i];

        auto try_red = [&]() {
            if (green_attacks_[pos] == 0 && blue_attacks_[pos] == 0) {
                for (int a : attack_vectors_[pos]) {
                    if (red_attacks_[a] == 0 && green_attacks_[a] == 0 && blue_attacks_[a] == 0) --s.max_possible_magenta_encamped;
                    if (red_attacks_[a] == 0 && green_attacks_[a] == 0) --s.max_possible_blue_encamped;
                    if (red_attacks_[a] == 0 && blue_attacks_[a] == 0) --s.max_possible_green_encamped;
                    red_attacks_[a] += 1;
                }
                s.red_encamped += 1;
                if (i == 0) {
                    found_solution(s);
                } else {
                    if (s.could_still_beat(i-1, best_solution_)) {
                    // We still have a chance to improve our best solution. Try recursing on this position.
                        backtracking_solve(i-1, s);
                    }
                }
                s.red_encamped -= 1;
                for (int a : attack_vectors_[pos]) {
                    red_attacks_[a] -= 1;
                    if (red_attacks_[a] == 0 && green_attacks_[a] == 0 && blue_attacks_[a] == 0) ++s.max_possible_magenta_encamped;
                    if (red_attacks_[a] == 0 && green_attacks_[a] == 0) ++s.max_possible_blue_encamped;
                    if (red_attacks_[a] == 0 && blue_attacks_[a] == 0) ++s.max_possible_green_encamped;
                }
            }
        };

        auto try_green = [&]() {
            if (s.red_encamped != 0) {
            if (red_attacks_[pos] == 0 && blue_attacks_[pos] == 0) {
                for (int a : attack_vectors_[pos]) {
                    if (blue_attacks_[a] == 0 && green_attacks_[a] == 0 && red_attacks_[a] == 0) --s.max_possible_magenta_encamped;
                    if (green_attacks_[a] == 0 && red_attacks_[a] == 0) --s.max_possible_blue_encamped;
                    if (green_attacks_[a] == 0 && blue_attacks_[a] == 0) --s.max_possible_red_encamped;
                    green_attacks_[a] += 1;
                }
                s.green_encamped += 1;
                if (i == 0) {
                    found_solution(s);
                } else {
                    // We still have a chance to improve our best solution. Try recursing on this position.
                    if (s.could_still_beat(i-1, best_solution_)) {
                        backtracking_solve(i-1, s);
                    }
                }
                s.green_encamped -= 1;
                for (int a : attack_vectors_[pos]) {
                    green_attacks_[a] -= 1;
                    if (blue_attacks_[a] == 0 && green_attacks_[a] == 0 && red_attacks_[a] == 0) ++s.max_possible_magenta_encamped;
                    if (green_attacks_[a] == 0 && red_attacks_[a] == 0) ++s.max_possible_blue_encamped;
                    if (green_attacks_[a] == 0 && blue_attacks_[a] == 0) ++s.max_possible_red_encamped;
                }
            }
            }
        };

        auto try_blue = [&]() {
            if (s.red_encamped != 0 && s.green_encamped != 0) {
            if (red_attacks_[pos] == 0 && green_attacks_[pos] == 0) {
                for (int a : attack_vectors_[pos]) {
                    if (blue_attacks_[a] == 0 && green_attacks_[a] == 0 && red_attacks_[a] == 0) --s.max_possible_magenta_encamped;
                    if (blue_attacks_[a] == 0 && green_attacks_[a] == 0) --s.max_possible_red_encamped;
                    if (blue_attacks_[a] == 0 && red_attacks_[a] == 0) --s.max_possible_green_encamped;
                    blue_attacks_[a] += 1;
                }
                s.blue_encamped += 1;
                if (i == 0) {
                    found_solution(s);
                } else {
                    // We still have a chance to improve our best solution. Try recursing on this position.
                    if (s.could_still_beat(i-1, best_solution_)) {
                        backtracking_solve(i-1, s);
                    }
                }
                s.blue_encamped -= 1;
                for (int a : attack_vectors_[pos]) {
                    blue_attacks_[a] -= 1;
                    if (blue_attacks_[a] == 0 && green_attacks_[a] == 0 && red_attacks_[a] == 0) ++s.max_possible_magenta_encamped;
                    if (blue_attacks_[a] == 0 && green_attacks_[a] == 0) ++s.max_possible_red_encamped;
                    if (blue_attacks_[a] == 0 && red_attacks_[a] == 0) ++s.max_possible_green_encamped;
                }
            }
            }
        };

        auto try_empty_or_magenta = [&]() {
            if (true) {
                // We still have a chance to improve our best solution. Try recursing on this position.
                if (i == 0) {
                    found_solution(s);
                } else {
                    if (s.could_still_beat(i-1, best_solution_)) {
                        backtracking_solve(i-1, s);
                    }
                }
            }
        };

        switch (i % 4) {
            case 0: try_red(); try_empty_or_magenta(); try_green(); try_blue(); break;
            case 1: try_green(); try_empty_or_magenta(); try_blue(); try_red(); break;
            case 2: try_blue(); try_empty_or_magenta(); try_red(); try_green(); break;
            case 3: try_empty_or_magenta(); try_red(); try_green(); try_blue(); break;
        }
    }

    void found_solution(const SolutionState& s)
    {
        if (s.is_valid() && s > best_solution_) {
            // The solution might actually admit one or two more armies in unattacked places.
            int armies[4] = {};
            for (int i=0; i < w_*h_; ++i) {
                if (is_magenta_army(i)) ++armies[0];
                else if (is_blue_army(i)) ++armies[1];
                else if (is_green_army(i)) ++armies[2];
                else if (is_red_army(i)) ++armies[3];
            }
            std::sort(armies, armies + 4);
            best_solution_.red_encamped = armies[3];
            best_solution_.green_encamped = armies[2];
            best_solution_.blue_encamped = armies[1];
            best_solution_.max_possible_magenta_encamped = armies[0];
            printf("Found a solution with white=%d black=%d red=%d green=%d\n", armies[3], armies[2], armies[1], armies[0]);
            print_grid();
        }
    }

    bool is_attacked_by_red(int i) const {
        return red_attacks_[i] != 0;
    }

    bool is_attacked_by_green(int i) const {
        return green_attacks_[i] != 0;
    }

    bool is_attacked_by_blue(int i) const {
        return blue_attacks_[i] != 0;
    }

    bool is_magenta_army(int i) const {
        return !is_attacked_by_red(i) && !is_attacked_by_green(i) && !is_attacked_by_blue(i);
    }

    bool is_attacked_by_magenta(int i) const {
        for (int ci=0; ci < w_*h_; ++ci) {
            if (is_attacking(ci, i) && is_magenta_army(ci)) {
                return true;
            }
        }
        return false;
    }

    bool is_red_army(int i) const {
        return !is_attacked_by_green(i) && !is_attacked_by_blue(i) && !is_attacked_by_magenta(i);
    }

    bool is_green_army(int i) const {
        return !is_attacked_by_red(i) && !is_attacked_by_blue(i) && !is_attacked_by_magenta(i);
    }

    bool is_blue_army(int i) const {
        return !is_attacked_by_green(i) && !is_attacked_by_red(i) && !is_attacked_by_magenta(i);
    }

    void print_grid() const {
        for (int j=0; j < h_; ++j) {
            for (int i=0; i < w_; ++i) {
                if (is_magenta_army(j*w_+i)) {
                    putchar('G');
                } else if (is_blue_army(j*w_+i)) {
                    putchar('R');
                } else if (is_green_army(j*w_+i)) {
                    putchar('B');
                } else if (is_red_army(j*w_+i)) {
                    putchar('W');
                } else {
                    putchar('.');
                }
            }
            putchar('\n');
        }
    }

    explicit Solver(int w, int h) :
        w_(w), h_(h), attack_vectors_(w*h), red_attacks_(w*h), green_attacks_(w*h), blue_attacks_(w*h)
    {
        best_solution_ = SolutionState::worst_solution(w*h);
        for (int i=0; i < w*h; ++i) {
            position_lut_.push_back(i);
            for (int ci = 0; ci < w*h; ++ci) {
                if (is_attacking(i, ci)) {
                    attack_vectors_[i].push_back(ci);
                }
            }
        }
        std::mt19937 g(std::random_device{}());
        std::shuffle(position_lut_.begin(), position_lut_.end(), g);
    }
};

void solve_encampments_for(int w, int h)
{
    Solver solver(w, h);
    Solver::SolutionState s = Solver::SolutionState::initial(w*h);
    solver.backtracking_solve(w*h - 1, s);
    printf("BOARD SIZE %d: ENCAMPMENT SIZE %d\n", w, solver.best_solution_.max_possible_magenta_encamped);
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
    solve_encampments_for(n, n);
}
