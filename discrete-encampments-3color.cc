#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

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

        SolutionState best_possible(int i) const {
            return SolutionState{ max_possible_red_encamped, max_possible_green_encamped, 0, 0, max_possible_blue_encamped };
        }

        int smallest_army() const { return std::min(std::min(red_encamped, green_encamped), max_possible_blue_encamped); }
        int biggest_army() const { return std::max(std::max(red_encamped, green_encamped), max_possible_blue_encamped); }
        int total_armies() const { return red_encamped + green_encamped + max_possible_blue_encamped; }
        friend bool operator<(const SolutionState& a, const SolutionState& b) {
            int as[3] = {a.smallest_army(), a.total_armies(), a.biggest_army()};
            as[1] -= (as[0] + as[2]);  // thus, size of the middle army
            int bs[3] = {b.smallest_army(), b.total_armies(), b.biggest_army()};
            bs[1] -= (bs[0] + bs[2]);  // thus, size of the middle army
            return std::lexicographical_compare(as, as+3, bs, bs+3);
        }
        friend bool operator>(const SolutionState& a, const SolutionState& b) { return (b < a); }
    };

    int w_;
    int h_;
    SolutionState best_solution_;
    std::vector<AttackCount> red_attacks_;
    std::vector<AttackCount> green_attacks_;
    std::vector<std::vector<int>> attack_vectors_;

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
        // Try coloring this spot RED...
        if (green_attacks_[i] == 0) {
            for (int a : attack_vectors_[i]) {
                if (red_attacks_[a] == 0 && green_attacks_[a] == 0) --s.max_possible_blue_encamped;
                if (red_attacks_[a] == 0) --s.max_possible_green_encamped;
                red_attacks_[a] += 1;
            }
            s.red_encamped += 1;
            if (i == 0) {
                found_solution(s);
            } else {
                if (s.best_possible(i-1) > best_solution_) {
                // We still have a chance to improve our best solution. Try recursing on this position.
                    backtracking_solve(i-1, s);
                }
            }
            s.red_encamped -= 1;
            for (int a : attack_vectors_[i]) {
                red_attacks_[a] -= 1;
                if (red_attacks_[a] == 0 && green_attacks_[a] == 0) ++s.max_possible_blue_encamped;
                if (red_attacks_[a] == 0) ++s.max_possible_green_encamped;
            }
        }

        // Try coloring this spot GREEN...
        if (red_attacks_[i] == 0) {
            for (int a : attack_vectors_[i]) {
                if (green_attacks_[a] == 0 && red_attacks_[a] == 0) --s.max_possible_blue_encamped;
                if (green_attacks_[a] == 0) --s.max_possible_red_encamped;
                green_attacks_[a] += 1;
            }
            s.green_encamped += 1;
            if (i == 0) {
                found_solution(s);
            } else {
                // We still have a chance to improve our best solution. Try recursing on this position.
                if (s.best_possible(i-1) > best_solution_) {
                    backtracking_solve(i-1, s);
                }
            }
            s.green_encamped -= 1;
            for (int a : attack_vectors_[i]) {
                green_attacks_[a] -= 1;
                if (green_attacks_[a] == 0 && red_attacks_[a] == 0) ++s.max_possible_blue_encamped;
                if (green_attacks_[a] == 0) ++s.max_possible_red_encamped;
            }
        }

        if (true) {
            // We still have a chance to improve our best solution. Try recursing on this position.
            if (i == 0) {
                found_solution(s);
            } else {
                //if (red_attacks_[i] == 0) s.max_possible_green_encamped -= 1;
                //if (green_attacks_[i] == 0) s.max_possible_red_encamped -= 1;
                if (s.best_possible(i-1) > best_solution_) {
                    backtracking_solve(i-1, s);
                }
                //if (red_attacks_[i] == 0) s.max_possible_green_encamped += 1;
                //if (green_attacks_[i] == 0) s.max_possible_red_encamped += 1;
            }
        }
    }

    void found_solution(const SolutionState& s)
    {
        if (s > best_solution_) {
            best_solution_ = s;
            printf("Found a solution with red=%d green=%d blue=%d\n", s.red_encamped, s.green_encamped, s.max_possible_blue_encamped);
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
                    putchar('B');
                } else if (is_green_army(j*w_+i)) {
                    putchar('G');
                } else if (is_red_army(j*w_+i)) {
                    putchar('R');
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
        best_solution_ = SolutionState::initial(w*h);
        for (int i=0; i < w*h; ++i) {
            for (int ci = 0; ci < w*h; ++ci) {
                if (is_attacking(i, ci)) {
                    attack_vectors_[i].push_back(ci);
                }
            }
        }
    }
};

void solve_encampments_for(int w, int h)
{
    Solver solver(w, h);
    Solver::SolutionState s = Solver::SolutionState::initial(w*h);
    solver.backtracking_solve(w*h - 1, s);
    printf("BOARD SIZE %d: ENCAMPMENT SIZE %d\n", w, solver.best_solution_.smallest_army());
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
