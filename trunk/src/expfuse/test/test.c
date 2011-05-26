#include "opencv/cv.h"
#include "opencv/highgui.h"
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "../Matrix.h"

Matrix* LoadMatrix(const char* filename);
void SaveMatrix(const Matrix* I, const char* filename);
Matrix** GaussianPyramid(/*const*/ Matrix* I, const int levels);
Matrix** LaplacianPyramid(/*const*/ Matrix* I, const int levels);
Matrix* Downsample(const Matrix* I);
Matrix* Upsample(const Matrix* I, const int odd_rows, const int odd_cols);

int main()
{
	int i;
	Matrix* I;
	Matrix** gauss_pyramid;
	Matrix** laplacian_pyramid;
	
	I = LoadMatrix("lena.png");
	
	int levels = (int)floor(log(min(I->rows,I->cols))/log(2));
	
	gauss_pyramid = GaussianPyramid(I,levels);
	laplacian_pyramid = LaplacianPyramid(I,levels);
	
	forn(i,levels)
	{
		char name[20];
		sprintf(name,"lena_gp_%d.jpg",i);
		SaveMatrix(gauss_pyramid[i],name);
		
		sprintf(name,"lena_lp_%d.jpg",i);
		SaveMatrix(laplacian_pyramid[i],name);
	}
	
	// liberando memoria
	
	forn(i,levels)
		DeleteMatrix(gauss_pyramid[i]);
	free(gauss_pyramid);
	
	DeleteMatrix(I);
	return 0;
}

Matrix* LoadMatrix(const char* filename)
{
	int i, j;
	IplImage* img;
	Matrix* C;

	img = cvLoadImage(filename,CV_LOAD_IMAGE_GRAYSCALE);
	if(!img)
	{
		printf("Could not load image file: %s\n",filename);
		exit(0);
	}
	C = NewMatrix(img->height,img->width);
	
	for(i=0;i<C->rows;i++) for(j=0;j<C->cols;j++)
	{
		unsigned char* src = (unsigned char*)(img->imageData + i*img->widthStep + j);
		
		ELEM(C,i,j) = (double)(src[0]/255.0);
	}
	
	return C;
}

void SaveMatrix(const Matrix* I, const char* filename)
{
	int i, j;
	
	IplImage* img;
	img = cvCreateImage(cvSize(I->cols,I->rows), IPL_DEPTH_8U, 1);
	
	for(i=0;i<I->rows;i++) for(j=0;j<I->cols;j++)
	{
		unsigned char* dst = (unsigned char*)(img->imageData + i*img->widthStep + j);
		dst[0] = (unsigned char)(ELEM(I,i,j)*255.0);
	}
	
	if(!cvSaveImage(filename,img,0))
		printf("Could not save: %s\n",filename);

	cvReleaseImage(&img);
}

Matrix** GaussianPyramid(/*const*/ Matrix* I, const int levels)
{
	int k;
	Matrix** pyramid;
	
	pyramid = malloc(sizeof(Matrix*)*levels);
	
	// copio el primer nivel como la imagen original
	pyramid[0] = CopyMatrix(I);
	
	for (k=1;k<levels;k++)
	{
		I = Downsample(I);
		pyramid[k] = I;
	}
	
	return pyramid;
}

Matrix* Downsample(const Matrix* I)
{
	int i, j;
	Matrix* gauss_kernel;
	Matrix* convolved;
	Matrix* downsampled;
	
	gauss_kernel = NewMatrix(5,5);
	ELEM(gauss_kernel,0,0)=0.0039; ELEM(gauss_kernel,0,1)=0.0156; ELEM(gauss_kernel,0,2)=0.0234; ELEM(gauss_kernel,0,3)=0.0156; ELEM(gauss_kernel,0,4)=0.0039;
	ELEM(gauss_kernel,1,0)=0.0156; ELEM(gauss_kernel,1,1)=0.0625; ELEM(gauss_kernel,1,2)=0.0938; ELEM(gauss_kernel,1,3)=0.0625; ELEM(gauss_kernel,1,4)=0.0156;
	ELEM(gauss_kernel,2,0)=0.0234; ELEM(gauss_kernel,2,1)=0.0938; ELEM(gauss_kernel,2,2)=0.1406; ELEM(gauss_kernel,2,3)=0.0938; ELEM(gauss_kernel,2,4)=0.0234;
	ELEM(gauss_kernel,3,0)=0.0156; ELEM(gauss_kernel,3,1)=0.0625; ELEM(gauss_kernel,3,2)=0.0938; ELEM(gauss_kernel,3,3)=0.0625; ELEM(gauss_kernel,3,4)=0.0156;
	ELEM(gauss_kernel,4,0)=0.0039; ELEM(gauss_kernel,4,1)=0.0156; ELEM(gauss_kernel,4,2)=0.0234; ELEM(gauss_kernel,4,3)=0.0156; ELEM(gauss_kernel,4,4)=0.0039;
	
	convolved = Convolve(I,gauss_kernel,REPLICATE);
	
	downsampled = NewMatrix(I->rows/2,I->cols/2);
	
	forn(i,downsampled->rows) forn(j,downsampled->cols)
		ELEM(downsampled,i,j) = ELEM(convolved,2*i,2*j);
	
	DeleteMatrix(gauss_kernel);
	DeleteMatrix(convolved);
	
	return downsampled;
}

Matrix** LaplacianPyramid(/*const*/ Matrix* I, const int levels)
{
	int k;
	Matrix* J;
	Matrix** pyramid;
	
	pyramid = malloc(sizeof(Matrix*)*levels);
	
	J = I;
	for (k=0;k<levels-1;k++)
	{
		I = Downsample(J);
		
		int odd_rows = J->rows - 2*I->rows;
		int odd_cols = J->cols - 2*I->cols;
		
		pyramid[k] = Substract(J,Upsample(I,odd_rows,odd_cols));
		
		J = I;
	}
	
	pyramid[levels-1] = J;
	
	return pyramid;
}

Matrix* Upsample(const Matrix* I, const int odd_rows, int odd_cols)
{
	int i, j;
	Matrix* gauss_kernel;
	Matrix* upsampled;
	
	gauss_kernel = NewMatrix(5,5);
	ELEM(gauss_kernel,0,0)=0.0039; ELEM(gauss_kernel,0,1)=0.0156; ELEM(gauss_kernel,0,2)=0.0234; ELEM(gauss_kernel,0,3)=0.0156; ELEM(gauss_kernel,0,4)=0.0039;
	ELEM(gauss_kernel,1,0)=0.0156; ELEM(gauss_kernel,1,1)=0.0625; ELEM(gauss_kernel,1,2)=0.0938; ELEM(gauss_kernel,1,3)=0.0625; ELEM(gauss_kernel,1,4)=0.0156;
	ELEM(gauss_kernel,2,0)=0.0234; ELEM(gauss_kernel,2,1)=0.0938; ELEM(gauss_kernel,2,2)=0.1406; ELEM(gauss_kernel,2,3)=0.0938; ELEM(gauss_kernel,2,4)=0.0234;
	ELEM(gauss_kernel,3,0)=0.0156; ELEM(gauss_kernel,3,1)=0.0625; ELEM(gauss_kernel,3,2)=0.0938; ELEM(gauss_kernel,3,3)=0.0625; ELEM(gauss_kernel,3,4)=0.0156;
	ELEM(gauss_kernel,4,0)=0.0039; ELEM(gauss_kernel,4,1)=0.0156; ELEM(gauss_kernel,4,2)=0.0234; ELEM(gauss_kernel,4,3)=0.0156; ELEM(gauss_kernel,4,4)=0.0039;
	
	upsampled = NewMatrix(2*I->rows,2*I->cols);
	
	forn(i,I->rows) forn(j,I->cols)
	{
		ELEM(upsampled,2*i,2*j) = 4*ELEM(I,i,j);
		ELEM(upsampled,2*i,2*j+1) = 0;
		ELEM(upsampled,2*i+1,2*j) = 0;
		ELEM(upsampled,2*i+1,2*j+1) = 0;
	}
	
	upsampled = Convolve(upsampled,gauss_kernel,REPLICATE);
	
	return upsampled;
}

