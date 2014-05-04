all:gameoflife
gameoflife:gameoflife.o
	mpicc gameoflife.o -o gameoflife
gameoflife.o:gameoflife.c gameoflife.h
	mpicc gameoflife.c -c
clean:
	rm -rf *.o 