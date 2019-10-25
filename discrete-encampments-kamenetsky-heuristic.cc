// Computes the maximum number of queens with C colours that you can place on a NxN grid such that
// queens of different colour do not attack each other.
//
// This program can be used to compute the best known solutions for A250000, as well as solutions for the
// derivative sequences such as A308632 and A328283.
//
// The program uses a version of hill climbing. The key to the approach is a very fast mutation operator.
// Mutations involve changing the value of a single cell: empty to a queen, queen to an empty or
// queen of colour C1 to colour C2. Checking whether such a mutation improves the score takes time O(C),
// while making the actual change and updating auxiliary variables takes time O(N).
//
// The scoreType can play an important role and produce different results.
// I found that the "extra" options seems to be the best, but you need to play around with it.
// You can also change the number of random starting solutions Q in solveMany().
//
// Compilation: javac A250000.java
// Usage: java A250000 N C
// N is the grid size
// C is the number of colours
//
// Dmitry Kamenetsky, 17th of October 2019.

// Ported to C++ and refactored by Arthur O'Dwyer, 19 October 2019.

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

enum ScoreType { Extra = 0, Max = 1 };
struct ScoreType_Extra { static constexpr int value = ScoreType::Extra; };
struct ScoreType_Max { static constexpr int value = ScoreType::Max; };

class A250000_Base {
protected:
    struct CNFG { int c, n, f, g; };
private:
    virtual void do_step() = 0;
    virtual std::string do_getBestString() const = 0;
    virtual CNFG do_getCNFG() const = 0;
public:
    virtual ~A250000_Base() = default;
    void step() {
        this->do_step();
    }
    std::string getBestString() const {
        return this->do_getBestString();
    }
    std::pair<int, int> getCN() const { auto cnfg = this->do_getCNFG(); return {cnfg.c, cnfg.n}; }
    int getFCN() const { auto cnfg = this->do_getCNFG(); return cnfg.f; }
    int getGCN() const { auto cnfg = this->do_getCNFG(); return cnfg.g; }
};

struct RCV {
    int r, c, val;
};

struct xorshift128p {
    using result_type = uint64_t;

    static constexpr result_type min() { return 0; }
    static constexpr result_type max() { return uint64_t(-1); }

    // https://stackoverflow.com/a/34432126/1424877
    result_type operator()() {
        uint64_t a = m_state[0];
        uint64_t b = m_state[1];

        m_state[0] = b;
        a ^= a << 23;
        a ^= a >> 18;
        a ^= b;
        a ^= b >> 5;
        m_state[1] = a;

        return a + b;
    }

private:
    // splitmix64 seeded with "1"
    uint64_t m_state[2] = { 0x5692161D100B05E5uLL, 0x910A2DEC89025CC1uLL };
};

template<int N, int C, bool Interesting = (2 <= C && C < N)>
class A250000 : public A250000_Base {
    static constexpr int Q = 16;

    template<class T>
    using Board = std::array<std::array<T, N>, N>;

    xorshift128p gen_;
    std::vector<RCV> ind_;
    int bestScore_ = INT_MIN;
    int bestScores_[2*Q];
    Board<int> bestA[2*Q] {};
    std::string currentBestString_;
    int currentBestMinQueens_ = 0;
    int currentBestMaxQueens_ = 0;

public:
    A250000() {
        for (int r=0; r < N; ++r) {
            for (int c=0; c < N; ++c) {
                for (int val = 0; val <= C; ++val) {
                    ind_.push_back({r, c, val});
                }
            }
        }
        for (int& elt : bestScores_) {
            elt = INT_MIN;
        }
    }

private:
    static bool same_abs(int x, int y) {
        return x == y || x == -y;
    }
    static bool attacks(int r, int c, int r2, int c2) {
        return r2==r || c2==c || same_abs(r2 - r, c2 - c);
    }

    int randint0(int n) {
        return gen_() % n;
    }

    std::string do_getBestString() const override {
        return currentBestString_;
    }
    CNFG do_getCNFG() const override { return { C, N, currentBestMinQueens_, currentBestMaxQueens_ }; }

    template<class ST>
    struct StructuredScore {
        int score = 0;
        int bad = 0;
        int queens[C+1] {};
        int counts[C+1][N][N] {};

        static StructuredScore from_board(const Board<int>& a) {
            StructuredScore s;

            for (int r=0; r<N; r++) {
                for (int c=0; c<N; c++) {
                    int val = a[r][c];
                    if (val == 0) continue;   //empty cell

                    s.queens[val] += 1;

                    for (int r2=0; r2<N; r2++) {
                        for (int c2=0; c2<N; c2++) {
                            if (attacks(r, c, r2, c2)) {
                                s.counts[val][r2][c2] += 1;
                                if (a[r2][c2] != 0 && a[r2][c2] != val) {
                                    s.bad += 1;
                                }
                            }
                        }
                    }
                }
            }
            s.update_just_score();
            return s;
        }

        int minQueens() const {
            int r = INT_MAX;
            for (int i=1; i <= C; ++i) {
                if (queens[i] < r) r = queens[i];
            }
            return r;
        }
        int maxQueens() const {
            int r = INT_MIN;
            for (int i=1; i <= C; ++i) {
                if (queens[i] > r) r = queens[i];
            }
            return r;
        }

        void update_just_score() {
            int minq = minQueens();
            int maxq = maxQueens();

            int extra = 0;
            for (int i=1; i <= C; ++i) {
                if (queens[i] == minq) {
                    extra += 1;
                }
            }
            assert(1 <= extra && extra <= C);

            if (ST::value == ScoreType::Extra) {
                score = ((minq - bad) * 256*256) + (C - extra);
            } else if (ST::value == ScoreType::Max) {
                score = ((minq - bad) * 256*256) + C + maxq;
            } else {
                assert(false);
            }
        }

        void update_all_but_counts(const Board<int>& a, int r, int c, int val) {
            // We *are* the score of 'a' right now.
            // Update us to be the score of 'a' if 'a[r][c]' were replaced with 'val'.

            int old = a[r][c];
            if (old == val) return;

            queens[old]--;
            queens[val]++;

            if (old == 0 && val != 0) {
                //added queen
                for (int k=1; k <= C; ++k) {
                    if (k != val) bad += 2*counts[k][r][c];
                }
            } else if (old != 0 && val==0) {
                //removed queen
                for (int k=1; k<=C; ++k) {
                    if (k != old) bad -= 2*counts[k][r][c];
                }
            } else {
                //changed queen colours
                for (int k=1; k <= C; ++k) {
                    if (k != old) bad -= 2*counts[k][r][c];
                    if (k != val) bad += 2*counts[k][r][c];
                }
                bad -= 2;    //to handle double-counting itself
            }
            update_just_score();
        }

        void update_just_counts(const Board<int>& a, int r, int c, int val) {
            int old = a[r][c];
            assert(old != val);

            //horizontal
            for (int c2=0; c2<N; c2++) {
                if (c2 == c) continue;
                if (old != 0) counts[old][r][c2]--;
                if (val != 0) counts[val][r][c2]++;
            }

            //vertical
            for (int r2=0; r2<N; r2++) {
                if (r2==r) continue;
                if (old!=0) counts[old][r2][c]--;
                if (val!=0) counts[val][r2][c]++;
            }

            int min = std::min(r,c);
            for (int r2 = r-min, c2 = c-min; r2<N && c2<N; r2++, c2++) {
                if (r2==r && c2==c) continue;
                if (old != 0) counts[old][r2][c2]--;
                if (val != 0) counts[val][r2][c2]++;
            }

            min = std::min(r,N-1-c);
            for (int r2 = r-min, c2 = c+min; r2<N && c2>=0; r2++,c2--) {
                if (r2==r && c2==c) continue;
                if (old != 0) counts[old][r2][c2]--;
                if (val != 0) counts[val][r2][c2]++;
            }

            //same spot
            if (old != 0) counts[old][r][c]--;
            if (val != 0) counts[val][r][c]++;
        }
    };

    void do_step() override {
        for (int q=0; q < 2*Q; q++) {
            Board<int> a = bestA[q];

            // First make between 1 and 5 random edits to the board.
            make_random_edits(a, 1 + randint0(5));

            // Then, jiggle the solution until it cannot be improved by ANY single edit.
            if (q < Q) {
                auto s = StructuredScore<ScoreType_Extra>::from_board(a);
                optimizeChangesFast(a, s);

                if (s.score >= bestScores_[q]) {
                    bestScores_[q] = s.score;
                    bestA[q] = a;
                }
                if (s.score > bestScores_[Q+q]) {
                    bestScores_[Q+q] = s.score;
                    bestA[Q+q] = a;
                }

                if (s.score > bestScore_) {
                    currentBestString_ = prettyPrint(s.score, a);
                    bestScore_ = s.score;
                    currentBestMinQueens_ = s.minQueens();
                    currentBestMaxQueens_ = s.maxQueens();
                }
            } else {
                auto s = StructuredScore<ScoreType_Max>::from_board(a);
                optimizeChangesFast(a, s);

                if (s.score >= bestScores_[q]) {
                    bestScores_[q] = s.score;
                    bestA[q] = a;
                }

                if (s.score > bestScore_) {
                    currentBestString_ = prettyPrint(s.score, a);
                    bestScore_ = s.score;
                    currentBestMinQueens_ = s.minQueens();
                    currentBestMaxQueens_ = s.maxQueens();
                }
            }
        }
    }

    void make_random_edits(Board<int>& a, int num_changes) {
        for (int i=0; i < num_changes; ++i) {
            int r = randint0(N);
            int c = randint0(N);
            a[r][c] = (a[r][c] + 1 + randint0(C)) % (C+1);
        }
    }

    template<class StructuredScore>
    void optimizeChangesFast(Board<int>& a, StructuredScore& s)
    {
        while (true) {
            std::shuffle(ind_.begin(), ind_.end(), gen_);
            bool changed = false;

            for (const auto& rcv : ind_) {
                maybeAdjust(a, s, changed, rcv.r, rcv.c, rcv.val);
            }
            if (!changed) {
                break;
            }
        }
    }

    template<class StructuredScore>
    static void maybeAdjust(
        Board<int>& a,
        StructuredScore& s,
        bool& changed,
        int r, int c, int val)
    {
        // Try changing a[r][c] to val, and see if that improves this position.

        int old = a[r][c];

        if (old == val) {
            return;   //no change
        }
        StructuredScore s2 = s;
        s2.update_all_but_counts(a, r, c, val);

        if (s2.score >= s.score) {
            if (s2.score > s.score) {
                changed = true;
            }
            s = std::move(s2);
            s.update_just_counts(a, r, c, val);
            a[r][c] = val;
        }
    }

    static char to_digit(int i) {
        return ".123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[i];
    }

    static std::string prettyQueens(const int (&queens)[C+1])
    {
        int temp[C];
        std::copy(queens+1, queens+C+1, temp);
        std::sort(temp, temp+C);  // in ascending order
        std::ostringstream result;
        result << "min=" << temp[0] << " max=" << temp[C-1] << " all=";
        for (int i=0; i < C; ++i) {
            if (i != 0) result << '+';
            result << temp[i];
        }
        return std::move(result).str();
    }

    static std::string prettyPrint(int score, const Board<int>& a)
    {
        std::ostringstream result;
        result << "score " << score << "\n";

        int queens[C+1] {};
        int totalQueens = 0;

        for (int r=0; r<N; r++) {
            for (int c=0; c<N; c++) {
                queens[a[r][c]]++;
                if (a[r][c] > 0) totalQueens++;
            }
        }

        result << "N=" << N << " C=" << C << " " << prettyQueens(queens) << "\n";
        for (int i=0; i<N; i++) {
            for (int k=0; k<N; k++) {
                result << to_digit(a[i][k]);
            }
            result << '\n';
        }
        result << '\n';
        return std::move(result).str();
    }
};

template<int N, int C>
class A250000<N, C, false> : public A250000_Base {
    void do_step() override { exit(0); }
    std::string do_getBestString() const override { return ""; }
    CNFG do_getCNFG() const override { return { C, N, 0, 0 }; }
};

template<int Min, int... Is>
auto make_int_range_impl(std::integer_sequence<int, Is...>) {
    return std::integer_sequence<int, Is+Min...>{};
}

template<int Min, int Max, std::enable_if_t<(Min <= Max), int> = 0>
auto make_int_range() {
    return make_int_range_impl<Min>(std::make_integer_sequence<int, (Max - Min) + 1>());
}

template<int Min, int Max, std::enable_if_t<(Min > Max), int> = 0>
auto make_int_range() {
    return std::integer_sequence<int>{};
}
static_assert(std::is_same<decltype(make_int_range<2,5>()), std::integer_sequence<int,2,3,4,5> >::value, "");
static_assert(std::is_same<decltype(make_int_range<2,2>()), std::integer_sequence<int,2> >::value, "");

template<int N, int... Cs>
std::unique_ptr<A250000_Base> make_A250000_c(std::integer_sequence<int, Cs...>, int c) {
    struct {
        int c;
        std::unique_ptr<A250000_Base> (*action)();
    } array[] = {
        { Cs, []() -> std::unique_ptr<A250000_Base> { return std::make_unique<A250000<N, Cs>>(); } } ...
    };
    for (const auto& elt : array) {
        if (c == elt.c) return elt.action();
    }
    fprintf(stderr, "oops! n=%d c=%d\n", N, c);
    assert(false);
}

template<int... Ns>
std::unique_ptr<A250000_Base> make_A250000_n(std::integer_sequence<int, Ns...>, int n, int c) {
    struct {
        int n;
        std::unique_ptr<A250000_Base> (*action)(int);
    } array[] = {
        { Ns, [](int c) { return make_A250000_c<Ns>(make_int_range<2, Ns-1>(), c); } } ...
    };
    for (const auto& elt : array) {
        if (n == elt.n) return elt.action(c);
    }
    fprintf(stderr, "oops! n=%d c=%d\n", n, c);
    assert(false);
}

std::unique_ptr<A250000_Base> make_A250000(int n, int c) {
    return make_A250000_n(make_int_range<3, 16>(), n, c);
}

std::string make_triangle(const std::map<std::pair<int, int>, int>& fcn)
{
    std::ostringstream result;
    result << "    k=       1  2  3  4  5  6  ...\n";
    result << "          .\n";
    result << "    n=1   .  1  0\n";
    result << "    n=2   .  4  0  0\n";
    for (int n = 3; n <= 16; ++n) {
        result << "    n=" << std::setw(2) << std::left << n << "   " << std::setw(3) << std::right << n*n;
        for (int c = 2; c <= n+1; ++c) {
            auto it = fcn.find({c,n});
            int value = 0;
            if (it != fcn.end()) {
                value = it->second;
            } else if (n == c) {
                value = (n == 2 || n == 3) ? 0 : 1;
            }
            result << std::setw(3) << std::right << value;
        }
        result << "\n";
    }
    return std::move(result).str();
}

template<class Duration>
size_t in_ms(Duration d)
{
    return static_cast<size_t>(std::chrono::duration_cast<std::chrono::milliseconds>(d).count());
}

int main() {
    auto prestart_time = std::chrono::high_resolution_clock::now();

    std::vector<std::unique_ptr<A250000_Base>> all;
    for (int n = 3; n <= 16; ++n) {
        for (int c = 2; c < n; ++c) {
            all.push_back(make_A250000(n, c));
        }
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    printf("done setup in %zu ms\n", in_ms(start_time - prestart_time));

    std::map<std::pair<int, int>, int> fcn;
    std::map<std::pair<int, int>, int> gcn;
    for (int iteration = 1; true; ++iteration) {
        std::string output;
        for (const auto& a : all) {
            for (int i=0; i < 8; ++i) {
                a->step();
            }
            output += a->getBestString();
            fcn[a->getCN()] = a->getFCN();
            gcn[a->getCN()] = a->getGCN();
        }
        std::ofstream outfile("dek-out.txt");
        outfile << make_triangle(fcn) << "\n\n";
        outfile << make_triangle(gcn) << "\n\n";
        outfile << output;

        auto finish_time = std::chrono::high_resolution_clock::now();
        printf("%d iterations in %zu ms\n", iteration, in_ms(finish_time - start_time));
    }
}
