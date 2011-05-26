GCC = gcc

expofuse: expofuse.o Matrix.o
	$(GCC)  $^ -Wall -g -o $@ `pkg-config --cflags --libs opencv`

expofuse.o: expofuse.c Matrix.h
	$(GCC) -g -Wall -c $< -o $@ `pkg-config --cflags opencv`

Matrix.o: Matrix.c Matrix.h
	$(GCC) -g -Wall -c $< -o $@

clean:
	rm -rf *.o
