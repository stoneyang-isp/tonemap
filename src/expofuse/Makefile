GCC = gcc


fuse: fuse.o expofuse.o Matrix.o
	$(GCC) $^ -Wall -O3 -o $@ -lopencv_core -lopencv_imgproc -lopencv_highgui

gui: gui.o expofuse.o Matrix.o
	$(GCC) $^ -Wall -O3 -o $@ `pkg-config --cflags --libs gtk+-2.0` -lopencv_core -lopencv_imgproc -lopencv_highgui

test: test.c expofuse.o Matrix.o
	$(GCC) -c test.c -o test.o
	$(GCC) $^ -o test -lopencv_core -lopencv_imgproc -lopencv_highgui

gui.o: gui.c expofuse.h
	$(GCC) -O3 -Wall -c $< -o $@ -lpthread `pkg-config --cflags gtk+-2.0`

fuse.o: fuse.c expofuse.h Matrix.h
	$(GCC) -O3 -Wall -c $< -o $@

expofuse.o: cExpofuse.o asmExpofuse.o
	ld -r $^ -o $@

cExpofuse.o: expofuse.c Matrix.h asmExpofuse.o
	$(GCC) -Wall -O0 -c $< -o $@ `pkg-config --cflags opencv`

asmExpofuse.o: expofuse.asm
	nasm -felf64 -o $@ $<

Matrix.o: asmMatrix.o cMatrix.o
	ld -r $^ -o $@

cMatrix.o: Matrix.c Matrix.h
	$(GCC) -O2 -Wall -c $< -o $@

asmMatrix.o: Matrix.asm
	nasm -felf64 -O3 -o $@ $<

clean:
	rm -rf *.o gui fuse test
