
a.out: metasudoku.cc sudoku.cc sudoku.h dance.cc dance.h odo-sudoku.h work-queue.h
	$(CXX) -std=c++14 -flto -O3 dance.cc -c
	$(CXX) -std=c++14 -flto -O3 sudoku.cc -c
	$(CXX) -std=c++14 -flto -O3 metasudoku.cc -c
	$(CXX) -std=c++14 -flto -O3 metasudoku.o sudoku.o dance.o
