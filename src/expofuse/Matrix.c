#include "Matrix.h"
#include <string.h>

double GAUSS_KERN_5x1_DATA[5] = {0.0625,0.2500,0.3750,0.2500,0.0625};
const Matrix GAUSS_KERN_5x1 = { .rows=5, .cols=1, .data = GAUSS_KERN_5x1_DATA };
const Matrix GAUSS_KERN_1x5 = { .rows=1, .cols=5, .data = GAUSS_KERN_5x1_DATA };

double LAPLACIAN_KERN_3x3_DATA[9] = {
	0.0,1.0,0.0,
	1.0,-4.0,1.0,
	0.0,1.0,0.0
};
const Matrix LAPLACIAN_KERN_3x3 = { .rows=3, .cols=3, .data = LAPLACIAN_KERN_3x3_DATA };

Matrix* NewMatrix(int rows, int cols) {
	Matrix* A;
	A = malloc(sizeof(Matrix));
	A->rows=rows;
	A->cols=cols;
	A->data = malloc(sizeof(double)*rows*cols);
	return A;
}

void DeleteMatrix(Matrix* A) {
	free(A->data);
	free(A);
}

Matrix* CopyMatrix(const Matrix* A) {
	Matrix* C = NewMatrix(A->rows, A->cols);
	memcpy(C->data, A->data, sizeof(double)*A->rows*A->cols);
	return C;
}

Matrix* Substract(const Matrix* A, const Matrix* B) {
	int i, j;
	Matrix* C;

	if (A->cols != B->cols || A->rows != B->rows) {
		printf("Error: Substract - matrix sizes must match!\n");
		exit(0);
	}

	C = NewMatrix(A->rows, A->cols);

	forn(i, C->rows) forn(j, C->cols) {
		int n = i*C->cols + j;
		C->data[n] = A->data[n] - B->data[n];
	}

	return C;
}

Matrix* asmSubstract(const Matrix* A, const Matrix* B) {
  if ( A->cols!=B->cols || A->rows!=B->rows )	{
		printf("Error: Substract - matrix sizes must match!\n");
		exit(0);
	}
	Matrix* C = NewMatrix(A->rows, A->cols);
	_asmSubstract(A->data, B->data, C->data, A->rows, A->cols);
	return C;
}

Matrix* AddMatrix(const Matrix* A, const Matrix* B) {
	int i, j;
	Matrix* C;

	if ( A->cols!=B->cols || A->rows!=B->rows ) {
		printf("Error: Add - matrix sizes must match!\n");
		exit(0);
	}

	C = NewMatrix(A->rows,A->cols);

	forn(i,C->rows) forn(j,C->cols) {
		int n = i*C->cols + j;
		C->data[n] = A->data[n] + B->data[n];
	}
	
	return C;
}

Matrix* asmAddMatrix(const Matrix* A, const Matrix* B) {
  if ( A->cols!=B->cols || A->rows!=B->rows )	{
		printf("Error: Add - matrix sizes must match!\n");
		exit(0);
	}
	Matrix* C = NewMatrix(A->rows, A->cols);
	_asmAddMatrix(A->data, B->data, C->data, A->rows, A->cols);
	return C;
}

Matrix* AddEqualsMatrix(Matrix* A, const Matrix* B) {
	int i, j;

	if ( A->cols!=B->cols || A->rows!=B->rows ) {
		printf("Error: AddEquals - matrix sizes must match!\n");
		exit(0);
	}

	forn(i,A->rows) forn(j,A->cols) {
		int n = i*A->cols + j;
		A->data[n] += B->data[n];
	}

	return A;
}

Matrix* asmAddEqualsMatrix(Matrix* A, const Matrix* B) {
  if ( A->cols!=B->cols || A->rows!=B->rows )	{
		printf("Error: AddEquals - matrix sizes must match!\n");
		exit(0);
	}
	_asmAddEqualsMatrix(A->data, B->data, A->rows, A->cols);
	return A;
}

Matrix* Convolve(const Matrix* A, const Matrix* kernel, const BOUNDARY_OPTION bound) {
	int i, j, ii, jj, k_center_row, k_center_col;
	Matrix* C;

	C = NewMatrix(A->rows,A->cols);
	k_center_row = kernel->rows/2;
	k_center_col = kernel->cols/2;

	for(i=0;i<A->rows;i++) for(j=0;j<A->cols;j++) {
		double sum = 0;

		for(ii=0;ii<kernel->rows;ii++) for(jj=0;jj<kernel->cols;jj++) {
			int r = i + (ii - k_center_row);
			int c = j  + (jj - k_center_col);
			
			if ( r<0 || A->rows <= r || c<0 || A->cols <= c ) {
				if (bound==SYMMETRIC) {
					if (r<0) r=-r;
					else if (A->rows <= r) r = 2*A->rows - r - 2;
					if (c<0) c=-c;
					else if (A->cols <= c) c = 2*A->cols - c - 2;
				} else if (bound==CIRCULAR) {
					while (r<0) r += A->rows;
					r %= A->rows;
					while (c<0) c += A->cols;
					c %= A->cols;
				} else if (bound==REPLICATE) {
					if (r<0) r=0;
					else if (A->rows <= r) r=A->rows-1;
					if (c<0) c=0;
					else if (A->cols <= c) c=A->cols-1;
				} else {
					printf("Unknown boundary option %d\n",bound);
					exit(0);
				}
			}

			sum += ELEM(A,r,c) * ELEM(kernel,ii,jj);
		}

		ELEM(C,i,j) = sum;
	}

	return C;
}

Matrix* ConvolveGauss1x5(const Matrix* A) {
	Matrix* C = NewMatrix(A->rows, A->cols);
	_asmConvolve1x5Symm(A->data, C->data, A->rows, A->cols, GAUSS_KERN_1x5.data);
	return C;
}

Matrix* Downsample(const Matrix* I) {
	Matrix* aux;
	Matrix* convolved;
	Matrix* downsampled;
	
	aux = Convolve(I, &GAUSS_KERN_5x1, SYMMETRIC);
	convolved = ConvolveGauss1x5(aux); // Convolve(aux, &GAUSS_KERN_1x5, SYMMETRIC);
	DeleteMatrix(aux);
	
	downsampled = NewMatrix(I->rows/2, I->cols/2);
	
	/*int i, j;
	forn(i,downsampled->rows) forn(j,downsampled->cols)
		ELEM(downsampled,i,j) = ELEM(convolved,2*i,2*j);*/
  _asmDownsample(downsampled->data, convolved->data, downsampled->rows, downsampled->cols, convolved->cols);
	
	DeleteMatrix(convolved);
	
	return downsampled;
}

// TODO: ASM
Matrix* Upsample(const Matrix* I, const int odd_rows, int odd_cols)
{
	Matrix* aux;
	Matrix* upsampled;
	
	if ((odd_rows!=1 && odd_rows!=0) || (odd_cols!=1 && odd_cols!=0)) {
		printf("Error: Upsample - odd_rows: %d odd_cols: %d\n", odd_rows,odd_cols);
		exit(0);
	}
	
	upsampled = NewMatrix(2*I->rows+odd_rows,2*I->cols+odd_cols);
	
	_asmUpsample(I->data, upsampled->data, I->rows, I->cols, upsampled->rows, upsampled->cols);
	/*int i, j;
	forn(i, I->rows) forn(j, I->cols) {
		ELEM(upsampled,2*i,2*j) = 4*ELEM(I,i,j);
		ELEM(upsampled,2*i,2*j+1) = 0;
		ELEM(upsampled,2*i+1,2*j) = 0;
		ELEM(upsampled,2*i+1,2*j+1) = 0;
	}
	
	if (odd_rows)
		forn(j,upsampled->cols)
			ELEM(upsampled,upsampled->rows-1,j) = ELEM(upsampled,upsampled->rows-3,j);
	
	if (odd_cols)
		forn(i,upsampled->rows)
			ELEM(upsampled,i,upsampled->cols-1) = ELEM(upsampled,i,upsampled->cols-3);*/
	
	aux = Convolve(upsampled, &GAUSS_KERN_5x1, SYMMETRIC);
	DeleteMatrix(upsampled);
	upsampled = ConvolveGauss1x5(aux); //Convolve(aux,&GAUSS_KERN_1x5,SYMMETRIC);
	DeleteMatrix(aux);
	
	return upsampled;
}

void PrintMatrix(const Matrix* A) {
	int i, j;
	for(i=0;i<A->rows;i++) {
		for(j=0;j<A->cols;j++) printf("%f ",ELEM(A,i,j));
		printf("\n");
	}
}

void PrintMatrixMatStyle(const Matrix* A) {
	int i, j;
	for(i=0;i<A->rows;i++) {
		for(j=0;j<A->cols;j++) {
			if (0<j) printf(",");
			printf("%.5f",ELEM(A,i,j));
		}
		printf("\n");
	}
}
