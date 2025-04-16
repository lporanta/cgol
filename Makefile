COMPILER = gcc
FLAGS = -lncurses -O3

cgol: cgol.c
	${COMPILER} ${FLAGS} -o $@ $^
