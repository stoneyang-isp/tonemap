#include "opencv/cv.h"

#include "expofuse.h"

double Adata[9] = {1., 2., 3., 4., 5., 6., 7., 8., 9.};
double Bdata[9] = {9., 8., 7., 6., 5., 4., 3., 2., 1.};
double Cdata[9] = {2., 2., 2., 2., 2., 2., 2., 2., 2.};
double Gdata[9] = {2., 2., 2., 2., 2., 2., 2., 2., 2.};
Matrix A = { .rows=3, .cols=3, .data = Adata };
Matrix B = { .rows=3, .cols=3, .data = Bdata };
Matrix C = { .rows=3, .cols=3, .data = Cdata };

int main(int argc, char* argv[]){
  int i;
  Matrix* D = NewMatrix(3, 3);
  for(i=0; i<3*3; i++) D->data[i] = 2.;
  Matrix* U = NewMatrix(7, 7);
  for(i=0; i<7*7; i++) U->data[i] = i+1;
  Matrix* U2 = NewMatrix(7, 7);
  PrintMatrix(U); printf("\n");
  _asmConvolve1x5(U->data, U2->data, 7, 7, GAUSS_KERN_1x5.data);
  Matrix* U3 = Convolve(U, &GAUSS_KERN_1x5, SYMMETRIC);
  printf("U2\n");PrintMatrix(U2);
  printf("\nU3\n");PrintMatrix(U3);
	return 0;
}
