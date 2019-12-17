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

static constexpr int MIN_N = 18;
static constexpr int MAX_N = 22;
static const char FILENAME[] = "dek-out.txt";

enum ScoreType { Extra = 0, Max = 1 };
struct ScoreType_Extra { static constexpr int value = ScoreType::Extra; };
struct ScoreType_Max { static constexpr int value = ScoreType::Max; };

struct Util {
    static char to_digit(int i) {
        assert(0 <= i && i <= 35);
        return ".123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[i];
    }
    static int from_digit(char ch) {
        const char *alphabet = ".123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        const char *p = strchr(alphabet, ch);
        assert(p != nullptr);
        return (p - alphabet);
    }
    static bool same_abs(int x, int y) {
        return x == y || x == -y;
    }
    static bool attacks(int r, int c, int r2, int c2) {
        return r2==r || c2==c || same_abs(r2 - r, c2 - c);
    }
};

class SolutionVerifier {
    std::vector<std::vector<int>> data_;
    int c_, f_, g_;

public:
    template<size_t N>
    explicit SolutionVerifier(const std::array<std::array<int, N>, N>& a, int c) {
        for (size_t i=0; i < N; ++i) {
            data_.push_back(std::vector<int>(a[i].begin(), a[i].end()));
        }
        c_ = c;
        auto queens = army_sizes();
        f_ = queens.front();
        g_ = queens.back();
    }

    explicit SolutionVerifier(const std::string& bestString) {
        int n, c, f, g;
        int rc = std::sscanf(bestString.c_str(), "N=%d C=%d min=%d max=%d all=", &n, &c, &f, &g);
        assert(rc == 4 || !"malformed solution in input file");
        const char *p = strchr(bestString.c_str(), '\n');
        assert(p != nullptr);
        ++p;
        data_.resize(n, std::vector<int>(n));
        for (int i=0; i < n; ++i) {
            for (int k=0; k < n; ++k) {
                data_[i][k] = Util::from_digit(*p++);
            }
            assert(*p == '\n');
            ++p;
        }
        assert(*p == '\0');
        c_ = c;
        f_ = f;
        g_ = g;
    }

    bool hasNoBadQueens() const {
        int n = data_.size();
        for (int i=0; i < n; ++i) {
            for (int j=0; j < n; ++j) {
                if (data_[i][j] == 0) continue;
                for (int i2 = 0; i2 < n; ++i2) {
                    for (int j2 = 0; j2 < n; ++j2) {
                        if (data_[i2][j2] == 0 || data_[i2][j2] == data_[i][j]) continue;
                        if (Util::attacks(i, j, i2, j2)) return false;
                    }
                }
            }
        }
        return true;
    }

    bool hasCorrectFandG() const {
        auto queens = army_sizes();
        return (f_ == queens.front() && g_ == queens.back());
    }

private:
    std::vector<int> army_sizes() const {
        std::vector<int> queens(c_);
        int n = data_.size();
        for (int r=0; r<n; ++r) {
            for (int c=0; c<n; ++c) {
                int val = data_[r][c];
                if (val == 0) continue;   //empty cell
                assert(1 <= val && val <= c_);
                queens[val-1] += 1;
            }
        }
        std::sort(queens.begin(), queens.end());
        return queens;
    }
};

class A250000_Base {
protected:
    struct NCFG { int n, c, f, g; };
private:
    virtual void do_step() = 0;
    virtual void do_parseBestString(const std::string& bestString) = 0;
    virtual std::string do_getBestString() const = 0;
    virtual NCFG do_getNCFG() const = 0;
public:
    virtual ~A250000_Base() = default;
    void step() {
        this->do_step();
    }
    void parseBestString(const std::string& bestString) {
        auto sv = SolutionVerifier(bestString);
        assert(sv.hasNoBadQueens() || !"non-solution in input file");
        assert(sv.hasCorrectFandG() || !"miscounted solution in input file");
        this->do_parseBestString(bestString);
    }
    std::string getBestString() const {
        return this->do_getBestString();
    }
    int getF() const { auto ncfg = this->do_getNCFG(); return ncfg.f; }
    int getG() const { auto ncfg = this->do_getNCFG(); return ncfg.g; }
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

struct NC {
    // It is important to the output format that N=4,C=3 sorts less-than N=5,C=2.
    int n, c;
    friend bool operator<(NC a, NC b) { return a.n < b.n || (a.n == b.n && a.c < b.c); }
};

struct RCV {
    int r, c, val;
};

class Parrot : public A250000_Base {
    int n_;
    int c_;
    int bestMinQueens_;
    int bestMaxQueens_;
    std::string bestString_;

public:
    explicit Parrot(int n, int c) : n_(n), c_(c) {}

private:
    void do_step() override {}
    void do_parseBestString(const std::string& bestString) override {
        bestString_ = bestString;
        int n, c, f, g;
        int rc = std::sscanf(bestString.c_str(), "N=%d C=%d min=%d max=%d all=", &n, &c, &f, &g);
        assert(rc == 4 || !"malformed solution in input file");
        assert(n == n_ && c == c_);
        bestMinQueens_ = f;
        bestMaxQueens_ = g;
    }

    std::string do_getBestString() const override {
        return bestString_;
    }

    NCFG do_getNCFG() const override {
        return { n_, c_, bestMinQueens_, bestMaxQueens_ };
    }
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
    int randint0(int n) {
        return gen_() % n;
    }

    std::string do_getBestString() const override {
        return currentBestString_;
    }
    NCFG do_getNCFG() const override { return { N, C, currentBestMinQueens_, currentBestMaxQueens_ }; }

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
                            if (Util::attacks(r, c, r2, c2)) {
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
        std::shuffle(ind_.begin(), ind_.end(), gen_);
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
                    auto sv = SolutionVerifier(a, C);
                    if (sv.hasNoBadQueens()) {
                        currentBestString_ = prettyPrint(a);
                        bestScore_ = s.score;
                        currentBestMinQueens_ = s.minQueens();
                        currentBestMaxQueens_ = s.maxQueens();
                    } else {
                        printf("ERROR! This board contains bad queens!");
                        printf("%s\n", prettyPrint(a).c_str());
                    }
                }
            } else {
                auto s = StructuredScore<ScoreType_Max>::from_board(a);
                optimizeChangesFast(a, s);

                if (s.score >= bestScores_[q]) {
                    bestScores_[q] = s.score;
                    bestA[q] = a;
                }

                if (s.score > bestScore_) {
                    auto sv = SolutionVerifier(a, C);
                    if (sv.hasNoBadQueens()) {
                        currentBestString_ = prettyPrint(a);
                        bestScore_ = s.score;
                        currentBestMinQueens_ = s.minQueens();
                        currentBestMaxQueens_ = s.maxQueens();
                    } else {
                        printf("ERROR! This board contains bad queens!");
                        printf("%s\n", prettyPrint(a).c_str());
                    }
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

    static std::string prettyPrint(const Board<int>& a)
    {
        std::ostringstream result;

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
                result << Util::to_digit(a[i][k]);
            }
            result << '\n';
        }
        return std::move(result).str();
    }

    static Board<int> prettyUnprint(const std::string& bestString) {
        Board<int> a;
        const char *p = strchr(bestString.c_str(), '\n');
        assert(p != nullptr);
        ++p;
        for (int i=0; i < N; ++i) {
            for (int k=0; k < N; ++k) {
                a[i][k] = Util::from_digit(*p++);
            }
            assert(*p == '\n');
            ++p;
        }
        assert(*p == '\0');
        return a;
    }

    void do_parseBestString(const std::string& bestString) override {
        Board<int> a = prettyUnprint(bestString);
        auto s = StructuredScore<ScoreType_Extra>::from_board(a);
        bestA[0] = a;
        bestScores_[0] = s.score;

        auto sm = StructuredScore<ScoreType_Max>::from_board(a);
        bestA[Q] = a;
        bestScores_[Q] = sm.score;
        bestScore_ = sm.score;

        currentBestString_ = prettyPrint(a);
        currentBestMinQueens_ = s.minQueens();
        currentBestMaxQueens_ = s.maxQueens();
    }
};

template<int N, int C>
class A250000<N, C, false> : public A250000_Base {
    void do_step() override { exit(0); }
    std::string do_getBestString() const override { return ""; }
    NCFG do_getNCFG() const override { return { N, C, 0, 0 }; }
    void do_parseBestString(const std::string&) override {}
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
    return make_A250000_n(make_int_range<MIN_N, MAX_N>(), n, c);
}

template<class F>
std::string make_triangle(const std::map<NC, std::shared_ptr<A250000_Base>>& m, const F& getValue)
{
    std::ostringstream result;
    result << "    k=       1  2  3  4  5  6  ...\n";
    result << "          .\n";
    result << "    n=1   .  1  0\n";
    result << "    n=2   .  4  0  0\n";

    assert(!m.empty());
    int max_n_to_print = std::prev(m.end())->first.n;
    assert(max_n_to_print >= MAX_N);

    for (int n = 3; n <= max_n_to_print; ++n) {
        result << "    n=" << std::setw(2) << std::left << n << "   " << std::setw(3) << std::right << n*n;
        for (int c = 2; c <= n+1; ++c) {
            auto it = m.find(NC{n,c});
            if (it != m.end()) {
                int value = getValue(*it->second);
                result << ' ' << std::setw(2) << std::right << value;
            } else if (c == n) {
                int value = (n == 2 || n == 3) ? 0 : 1;
                result << ' ' << std::setw(2) << std::right << value;
            } else if (c > n) {
                result << std::setw(3) << std::right << 0;
            } else {
                result << std::setw(3) << std::right << '?';
            }
        }
        result << "\n";
    }
    return std::move(result).str();
}

void write_solutions_to_file(const char *filename, const std::map<NC, std::shared_ptr<A250000_Base>>& m)
{
    std::ofstream outfile(filename);
    outfile << make_triangle(m, [](const A250000_Base& a) { return a.getF(); }) << "\n\n";
    outfile << make_triangle(m, [](const A250000_Base& a) { return a.getG(); }) << "\n\n";
    for (auto&& kv : m) {
        outfile << kv.second->getBestString() << '\n';
    }
}

void maybe_read_solutions_from_file(const char *filename, std::map<NC, std::shared_ptr<A250000_Base>>& m)
{
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        return;
    }
    std::string line;
    bool seen_a_grid = false;
    while (std::getline(infile, line)) {
        puts(line.c_str());
        if (line.compare(0, 2, "N=") == 0) {
            int n, c, f, g;
            int rc = std::sscanf(line.c_str(), "N=%d C=%d min=%d max=%d all=", &n, &c, &f, &g);
            assert(rc == 4 || !"input file contained malformed lines");

            std::string bestString = line + "\n";
            for (int i=0; i < n; ++i) {
                std::getline(infile, line);
                puts(line.c_str());
                assert(bool(infile) || !"input file ended prematurely");
                bestString += line;
                bestString += "\n";
            }
            auto it = m.find(NC{n, c});
            if (it == m.end()) {
                it = m.emplace(NC{n, c}, std::make_shared<Parrot>(n, c)).first;
            }
            it->second->parseBestString(bestString);

            seen_a_grid = true;
        } else if (seen_a_grid && line != "") {
            assert(!"input file contained malformed lines after the first grid");
        }
    }
}

template<class Duration>
size_t in_ms(Duration d)
{
    return static_cast<size_t>(std::chrono::duration_cast<std::chrono::milliseconds>(d).count());
}

int main()
{
    auto prestart_time = std::chrono::high_resolution_clock::now();

    std::map<NC, std::shared_ptr<A250000_Base>> to_output;
    std::vector<std::shared_ptr<A250000_Base>> to_update;

    for (int n = MIN_N; n <= MAX_N; ++n) {
        for (int c = 2; c < n; ++c) {
            std::shared_ptr<A250000_Base> p = make_A250000(n, c);
            to_output.emplace(NC{n,c}, p);
            to_update.push_back(p);
        }
    }

    maybe_read_solutions_from_file(FILENAME, to_output);

    auto start_time = std::chrono::high_resolution_clock::now();
    printf("done setup in %zu ms\n", in_ms(start_time - prestart_time));

    for (int iteration = 1; true; ++iteration) {
        for (auto&& p : to_update) {
            for (int i=0; i < 8; ++i) {
                p->step();
            }
        }

        write_solutions_to_file(FILENAME, to_output);

        auto finish_time = std::chrono::high_resolution_clock::now();
        printf("%d iterations in %zu ms\n", iteration, in_ms(finish_time - start_time));
    }
}
