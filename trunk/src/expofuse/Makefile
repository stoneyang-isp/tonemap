GCC = gcc


fuse: fuse.o expofuse.o Matrix.o asm.o
	$(GCC) $^ -Wall -g -o $@ `pkg-config --cflags --libs opencv`

gui: gui.o expofuse.o Matrix.o asm.o
	$(GCC) $^ -Wall -g -o $@ `pkg-config --libs gtk+-2.0 opencv`

test: test.c asm.o expofuse.o Matrix.o
	$(GCC) -c test.c -o test.o
	$(GCC) $^ -o test `pkg-config --cflags --libs opencv`

gui.o: gui.c expofuse.h
	$(GCC) -g -Wall -c $< -o $@ -lpthread `pkg-config --cflags gtk+-2.0`

fuse.o: fuse.c expofuse.h Matrix.h
	$(GCC) -g -Wall -c $< -o $@

expofuse.o: expofuse.c Matrix.h
	$(GCC) -g -Wall -c $< -o $@ `pkg-config --cflags opencv`

Matrix.o: Matrix.c Matrix.h
	$(GCC) -g -Wall -c $< -o $@

asm.o: asm.asm
	nasm -felf -o $@ $<

clean:
	rm -rf *.o
