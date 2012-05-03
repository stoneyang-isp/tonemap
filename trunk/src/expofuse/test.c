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
  PrintMatrix(D); printf("\n");
  _asmUpsample(D->data, U->data, 3, 3, 7, 7);
  printf("U\n");PrintMatrix(U);
  
  int odd_rows = 1;
  int odd_cols = 1;
  Matrix* upsampled = NewMatrix(2*D->rows+odd_rows,2*D->cols+odd_cols);
  int j;
  forn(i, D->rows) forn(j, D->cols) {
		ELEM(upsampled,2*i,2*j) = 4*ELEM(D,i,j);
		ELEM(upsampled,2*i,2*j+1) = 0;
		ELEM(upsampled,2*i+1,2*j) = 0;
		ELEM(upsampled,2*i+1,2*j+1) = 0;
	}
	
	if (odd_rows)
		forn(j,upsampled->cols)
			ELEM(upsampled,upsampled->rows-1,j) = ELEM(upsampled,upsampled->rows-3,j);
	
	if (odd_cols)
		forn(i,upsampled->rows)
			ELEM(upsampled,i,upsampled->cols-1) = ELEM(upsampled,i,upsampled->cols-3);
  printf("ups\n");PrintMatrix(upsampled);
	return 0;
}
