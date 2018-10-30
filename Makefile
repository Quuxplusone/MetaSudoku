
a.out: metasudoku.c sudoku.c sudoku.h dance.c dance.h
	$(CC) -O3 metasudoku.c sudoku.c dance.c
