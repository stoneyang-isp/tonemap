#include "Matrix.h"

double GAUSS_KERN_5x1_DATA[5] = {0.0625,0.2500,0.3750,0.2500,0.0625};
const Matrix GAUSS_KERN_5x1 = { .rows=5, .cols=1, .data = GAUSS_KERN_5x1_DATA };
const Matrix GAUSS_KERN_1x5 = { .rows=1, .cols=5, .data = GAUSS_KERN_5x1_DATA };

double LAPLACIAN_KERN_3x3_DATA[9] =
{
	0.0,1.0,0.0,
	1.0,-4.0,1.0,
	0.0,1.0,0.0
};
const Matrix LAPLACIAN_KERN_3x3 = { .rows=3, .cols=3, .data = LAPLACIAN_KERN_3x3_DATA };

Matrix* NewMatrix(int rows, int cols)
{
	Matrix* A;
	
	A = malloc(sizeof(Matrix*));
	A->rows=rows;
	A->cols=cols;
	A->data = malloc(sizeof(double)*rows*cols);
	
	return A;
}

void DeleteMatrix(Matrix* A)
{
	free(A->data);
	free(A);
}

Matrix* CopyMatrix(const Matrix* A)
{
	int i, j;
	Matrix* C;
	
	C = NewMatrix(A->rows,A->cols);
	
	forn(i,C->rows) forn(j,C->cols)
	{
		int n = i*C->cols + j;
		C->data[n] = A->data[n];
	}
	
	return C;
}

Matrix* Substract(const Matrix* A, const Matrix* B)
{
	int i, j;
	Matrix* C;
	
	if ( A->cols!=B->cols || A->rows!=B->rows )
	{
		printf("Error: Substract - matrix sizes must match!\n");
		exit(0);
	}
	
	C = NewMatrix(A->rows,A->cols);
	
	forn(i,C->rows) forn(j,C->cols)
	{
		int n = i*C->cols + j;
		C->data[n] = A->data[n] - B->data[n];
	}
	
	return C;
}

Matrix* Add(const Matrix* A, const Matrix* B)
{
	int i, j;
	Matrix* C;
	
	if ( A->cols!=B->cols || A->rows!=B->rows )
	{
		printf("Error: Add - matrix sizes must match!\n");
		exit(0);
	}
	
	C = NewMatrix(A->rows,A->cols);
	
	forn(i,C->rows) forn(j,C->cols)
	{
		int n = i*C->cols + j;
		C->data[n] = A->data[n] + B->data[n];
	}
	
	return C;
}

Matrix* AddEqualsMatrix(Matrix* A, const Matrix* B)
{
	int i, j;
	
	if ( A->cols!=B->cols || A->rows!=B->rows )
	{
		printf("Error: AddEquals - matrix sizes must match!\n");
		exit(0);
	}
	
	forn(i,A->rows) forn(j,A->cols)
	{
		int n = i*A->cols + j;
		A->data[n] += B->data[n];
	}
	
	return A;
}

Matrix* Convolve(const Matrix* A, const Matrix* kernel, const BOUNDARY_OPTION bound)
{
	int i, j, ii, jj, k_center_row, k_center_col;
	Matrix* C;
	
	C = NewMatrix(A->rows,A->cols);
	k_center_row = kernel->rows/2;
	k_center_col = kernel->cols/2;
	
	for(i=0;i<A->rows;i++) for(j=0;j<A->cols;j++)
	{
		double sum = 0;
		
		for(ii=0;ii<kernel->rows;ii++) for(jj=0;jj<kernel->cols;jj++)
		{
			int r = i + (ii - k_center_row);
			int c = j  + (jj - k_center_col);
			
			if ( r<0 || A->rows <= r || c<0 || A->cols <= c )
			{
				if (bound==SYMMETRIC)
				{
					if (r<0) r=-r;
					else if (A->rows <= r) r = 2*A->rows - r - 2;
					if (c<0) c=-c;
					else if (A->cols <= c) c = 2*A->cols - c - 2;
				}
				else if (bound==CIRCULAR)
				{
					while (r<0) r += A->rows;
					r %= A->rows;
					while (c<0) c += A->cols;
					c %= A->cols;
				}	
				else if (bound==REPLICATE)
				{
					if (r<0) r=0;
					else if (A->rows <= r) r=A->rows-1;
					if (c<0) c=0;
					else if (A->cols <= c) c=A->cols-1;
				}	
				else
				{
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

void PrintMatrix(const Matrix* A)
{
	int i, j;
	for(i=0;i<A->rows;i++) 
	{
		for(j=0;j<A->cols;j++) printf("%f ",ELEM(A,i,j));
		printf("\n");
	}
}

void PrintMatrixMatStyle(const Matrix* A)
{
	int i, j;
	for(i=0;i<A->rows;i++) 
	{
		for(j=0;j<A->cols;j++)
		{
			if (0<j) printf(",");
			printf("%.5f",ELEM(A,i,j));
		}
		printf("\n");
	}
}
