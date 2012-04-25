#include "opencv/cv.h"

#include "expofuse.h"

double Adata[9] = {1., 2., 3., 4., 5., 6., 7., 8., 9.};
double Bdata[9] = {9., 8., 7., 6., 5., 4., 3., 2., 1.};
double Cdata[9] = {2., 2., 2., 2., 2., 2., 2., 2., 2.};
Matrix A = { .rows=3, .cols=3, .data = Adata };
Matrix B = { .rows=3, .cols=3, .data = Bdata };
Matrix C = { .rows=3, .cols=3, .data = Cdata };

int main(int argc, char* argv[]){
  printf("%li\n", (long)C.data);
  printf("%li\n", (long)_asmAddEqualsMatrix(A.data, B.data, 3, 3));
  PrintMatrix(&A);
	return 0;
}
