#ifndef MATRIX_H
#define MATRIX_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define min(x1,x2) ((x1) > (x2) ? (x2):(x1))
#define max(x1,x2) ((x1) > (x2) ? (x1):(x2))
#define forn(i,n) for((i)=0;(i)<(n);(i)++)
#define ELEM(A,i,j) (A)->data[ (i)*(A)->cols + (j) ]

typedef enum {SYMMETRIC,CIRCULAR,REPLICATE} BOUNDARY_OPTION;

typedef struct
{
	int cols;
	int rows;
	double* data;
} Matrix;

Matrix* NewMatrix(int rows, int cols);

void DeleteMatrix(Matrix* A);

Matrix* CopyMatrix(const Matrix* A);

Matrix* Substract(const Matrix* A, const Matrix* B);
Matrix* asmSubstract(const Matrix* A, const Matrix* B);
extern void _asmSubstract(const double* A, const double* B, const double* C, int rows, int cols);

Matrix* AddMatrix(const Matrix* A, const Matrix* B);
Matrix* asmAddMatrix(const Matrix* A, const Matrix* B);
extern void _asmAddMatrix(const double* A, const double* B, const double* C, int rows, int cols);

Matrix* AddEqualsMatrix(Matrix* A, const Matrix* B);
Matrix* asmAddEqualsMatrix(Matrix* A, const Matrix* B);
extern void _asmAddEqualsMatrix(double* A, const double* B, int rows, int cols);

Matrix* Convolve(const Matrix* A, const Matrix* kernel, const BOUNDARY_OPTION bound);
Matrix* ConvolveGauss1x5(const Matrix* A);
extern void _asmConvolve1x5(const double* A, double* C, int rows, int cols, const double* kern);
extern void _asmConvolve1x5Symm(const double* A, double* C, int rows, int cols, const double* kern);

Matrix* Downsample(const Matrix* I);
extern void _asmDownsample(double* A, const double* B, int Arows, int Acols, int Bcols);

Matrix* Upsample(const Matrix* I, const int odd_rows, const int odd_cols);

void PrintMatrix(const Matrix* A);

void PrintMatrixMatStyle(const Matrix* A);

extern const Matrix LAPLACIAN_KERN_3x3;
extern const Matrix GAUSS_KERN_5x1;
extern const Matrix GAUSS_KERN_1x5;

#endif
