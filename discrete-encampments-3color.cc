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
        int max_possible_red_encamped;
        int max_possible_green_encamped;
        int max_possible_blue_encamped;

        static SolutionState initial(int n) {
            return SolutionState{0, 0, n, n, n};
        }

        static SolutionState worst_solution(int n) {
            if (n >= 13*13) return SolutionState{13-1, 12, 0, 0, 12};
            if (n >= 12*12) return SolutionState{11-1, 10, 0, 0, 10};
            if (n >= 11*11) return SolutionState{11-1, 8, 0, 0, 8};
            if (n >= 10*10) return SolutionState{8-1, 7, 0, 0, 7};
            if (n >= 9*9) return SolutionState{7-1, 6, 0, 0, 5};
            if (n >= 8*8) return SolutionState{6-1, 5, 0, 0, 4};
            return SolutionState{0, 0, 0, 0, 0};
        }

        SolutionState best_possible(int i) const {
            // This is the best possible outcome given the invariant that RED >= GREEN >= BLUE.
            return SolutionState{
                max_possible_red_encamped,
                std::min(max_possible_green_encamped, max_possible_red_encamped),
                0, 0,
                std::min(max_possible_blue_encamped, std::min(max_possible_green_encamped, max_possible_red_encamped))
            };
        }

        bool is_valid() const {
            return (red_encamped >= green_encamped && green_encamped >= max_possible_blue_encamped);
        }

        friend bool operator<(const SolutionState& a, const SolutionState& b) {
            if (a.max_possible_blue_encamped != b.max_possible_blue_encamped) return (a.max_possible_blue_encamped < b.max_possible_blue_encamped);
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
            if (green_attacks_[pos] == 0) {
                for (int a : attack_vectors_[pos]) {
                    if (red_attacks_[a] == 0 && green_attacks_[a] == 0) --s.max_possible_blue_encamped;
                    if (red_attacks_[a] == 0) --s.max_possible_green_encamped;
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
                    if (red_attacks_[a] == 0 && green_attacks_[a] == 0) ++s.max_possible_blue_encamped;
                    if (red_attacks_[a] == 0) ++s.max_possible_green_encamped;
                }
            }
        };

        auto try_green = [&]() {
            if (s.red_encamped != 0 && red_attacks_[pos] == 0) {
                for (int a : attack_vectors_[pos]) {
                    if (green_attacks_[a] == 0 && red_attacks_[a] == 0) --s.max_possible_blue_encamped;
                    if (green_attacks_[a] == 0) --s.max_possible_red_encamped;
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
                    if (green_attacks_[a] == 0 && red_attacks_[a] == 0) ++s.max_possible_blue_encamped;
                    if (green_attacks_[a] == 0) ++s.max_possible_red_encamped;
                }
            }
        };

        auto try_empty_or_blue = [&]() {
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

        if (i & 1) {
            try_red(); try_empty_or_blue(); try_green();
        } else {
            try_green(); try_empty_or_blue(); try_red();
        }
    }

    void found_solution(const SolutionState& s)
    {
        if (s.is_valid() && s > best_solution_) {
            // The solution might actually admit one or two more armies in unattacked places.
            int armies[3] = {};
            for (int i=0; i < w_*h_; ++i) {
                if (is_blue_army(i)) ++armies[0];
                else if (is_green_army(i)) ++armies[1];
                else if (is_red_army(i)) ++armies[2];
            }
            std::sort(armies, armies + 3);
            best_solution_.red_encamped = armies[2];
            best_solution_.green_encamped = armies[1];
            best_solution_.max_possible_blue_encamped = armies[0];
            printf("Found a solution with white=%d black=%d red=%d\n", armies[2], armies[1], armies[0]);
            print_grid();
        }
    }

    bool is_attacked_by_red(int i) const {
        return red_attacks_[i] != 0;
    }

    bool is_attacked_by_green(int i) const {
        return green_attacks_[i] != 0;
    }

    bool is_blue_army(int i) const {
        return !is_attacked_by_red(i) && !is_attacked_by_green(i);
    }

    bool is_attacked_by_blue(int i) const {
        for (int ci=0; ci < w_*h_; ++ci) {
            if (is_attacking(ci, i) && is_blue_army(ci)) {
                return true;
            }
        }
        return false;
    }

    bool is_red_army(int i) const {
        return !is_attacked_by_green(i) && !is_attacked_by_blue(i);
    }

    bool is_green_army(int i) const {
        return !is_attacked_by_red(i) && !is_attacked_by_blue(i);
    }

    void print_grid() const {
        for (int j=0; j < h_; ++j) {
            for (int i=0; i < w_; ++i) {
                if (is_blue_army(j*w_+i)) {
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
        w_(w), h_(h), attack_vectors_(w*h), red_attacks_(w*h), green_attacks_(w*h)
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
    printf("BOARD SIZE %d: ENCAMPMENT SIZE %d\n", w, solver.best_solution_.max_possible_blue_encamped);
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
