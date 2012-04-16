GCC = gcc


fuse: fuse.o expofuse.o Matrix.o
	$(GCC) $^ -Wall -g -o $@ -lopencv_core -lopencv_imgproc -lopencv_highgui

gui: gui.o expofuse.o Matrix.o
	$(GCC) $^ -Wall -g -o $@ `pkg-config --cflags --libs gtk+-2.0` -lopencv_core -lopencv_imgproc -lopencv_highgui

test: test.c expofuse.o Matrix.o
	$(GCC) -c test.c -o test.o
	$(GCC) $^ -o test -lopencv_core -lopencv_imgproc -lopencv_highgui

gui.o: gui.c expofuse.h
	$(GCC) -g -Wall -c $< -o $@ -lpthread `pkg-config --cflags gtk+-2.0`

fuse.o: fuse.c expofuse.h Matrix.h
	$(GCC) -g -Wall -c $< -o $@

expofuse.o: expofuse.c Matrix.h
	$(GCC) -g -Wall -c $< -o $@ `pkg-config --cflags opencv`

Matrix.o: asmMatrix.o cMatrix.o
	ld -r $^ -o $@

cMatrix.o: Matrix.c Matrix.h
	$(GCC) -g -Wall -c $< -o $@

asmMatrix.o: Matrix.asm
	nasm -felf64 -o $@ $<

clean:
	rm -rf *.o gui fuse test
