EXTRA_DEFINES=-DJUST_COUNT_VIABLE_GRIDS=0 -DNUM_THREADS=4

a.out: metasudoku.cc sudoku.cc sudoku.h dance.cc dance.h odo-sudoku.h work-queue.h
	$(CXX) -std=c++14 -flto -O3 dance.cc -c
	$(CXX) -std=c++14 -flto -O3 sudoku.cc -c
	$(CXX) -std=c++14 -flto -O3 metasudoku.cc -c $(EXTRA_DEFINES)
	$(CXX) -std=c++14 -flto -O3 metasudoku.o sudoku.o dance.o

exhaustive-17clue: exhaustive-17clue.cc sudoku.cc sudoku.h dance.cc dance.h odo-sudoku.h work-queue.h
	$(CXX) -std=c++14 -flto -O3 dance.cc -c
	$(CXX) -std=c++14 -flto -O3 sudoku.cc -c
	$(CXX) -std=c++14 -flto -O3 exhaustive-17clue.cc -c $(EXTRA_DEFINES)
	$(CXX) -std=c++14 -flto -O3 exhaustive-17clue.o sudoku.o dance.o -o exhaustive-17clue
