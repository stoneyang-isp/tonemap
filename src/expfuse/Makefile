GCC = gcc

laplacian: laplacian.o
	$(GCC)  $^ -Wall -g -o $@ `pkg-config --cflags --libs opencv`

laplacian.o: laplacian.c
	$(GCC) -g -Wall -c $< -o $@ `pkg-config --cflags opencv`

clean:
	rm -rf laplacian.o contrast_*  saturation_*  exposeness_*  weight_*  grey_*  
