GCC = gcc

gui: gui.c
	gcc -Wall -g gui.c -o gui -lpthread `pkg-config --cflags gtk+-2.0` `pkg-config --libs gtk+-2.0`

fuse: fuse.o expofuse.o Matrix.o
	$(GCC)  $^ -Wall -g -o $@ `pkg-config --cflags --libs opencv`

fuse.o: fuse.c expofuse.h Matrix.h
	$(GCC) -g -Wall -c $< -o $@

expofuse.o: expofuse.c Matrix.h
	$(GCC) -g -Wall -c $< -o $@ `pkg-config --cflags opencv`

Matrix.o: Matrix.c Matrix.h
	$(GCC) -g -Wall -c $< -o $@

clean:
	rm -rf *.o
