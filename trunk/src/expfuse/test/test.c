#include "opencv/cv.h"
#include "opencv/highgui.h"
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "../Matrix.h"

#define WC = 1
#define WS = 1
#define WE = 1
#define SAMPLES 4
#define SIGMA2 .04 // representa SIGMA^2 cuando SIGMA = .2

typedef struct
{
	Matrix* R;
	Matrix* G;
	Matrix* B;
} ColorImage;

ColorImage* LoadColorImage(const char* filename);
void SaveColorImage(const ColorImage* I, const char* filename);
Matrix* LoadGrayscaleImage(const char* filename);
void SaveGrayscaleImage(const Matrix* I, const char* filename);
Matrix* DesaturateImage(const ColorImage* I);
Matrix* Contrast(const Matrix* I);
Matrix* Saturation(const ColorImage* I);
Matrix* Exposeness(const ColorImage* I);
Matrix* Weight(const Matrix* contrast, const Matrix* saturation, const Matrix* exposeness);
void NormalizeWeights(Matrix** weights, const int n_samples);
ColorImage* NaiveFusion(/*const*/ ColorImage** color_images, /*const*/ Matrix** weights, const int n_samples);
Matrix** GaussianPyramid(/*const*/ Matrix* I, const int levels);
Matrix** LaplacianPyramid(/*const*/ Matrix* I, const int levels);
Matrix* Downsample(const Matrix* I);
Matrix* Upsample(const Matrix* I, const int odd_rows, const int odd_cols);

int main()
{
/*
	int i;
	Matrix* I;
	Matrix** gauss_pyramid;
	Matrix** laplacian_pyramid;
	
	I = LoadGrayscaleImage("lena.png");
	
	int levels = (int)floor(log(min(I->rows,I->cols))/log(2));
	
	gauss_pyramid = GaussianPyramid(I,levels);
	laplacian_pyramid = LaplacianPyramid(I,levels);
	
	forn(i,levels)
	{
		char name[20];
		sprintf(name,"lena_gp_%d.jpg",i);
		SaveGrayscaleImage(gauss_pyramid[i],name);
		
		sprintf(name,"lena_lp_%d.jpg",i);
		SaveGrayscaleImage(laplacian_pyramid[i],name);
	}
	
	// liberando memoria
	
	forn(i,levels)
		DeleteMatrix(gauss_pyramid[i]);
	free(gauss_pyramid);
	
	DeleteMatrix(I);
*/
	ColorImage** color_images;
	Matrix** grey_images;
	Matrix** contrast;
	Matrix** saturation;
	Matrix** exposeness;
	Matrix** weights;
	ColorImage* fused;
	int i, n_samples;
	
	n_samples = SAMPLES;
	
	color_images = malloc(sizeof(ColorImage*)*n_samples);
	color_images[0] = LoadColorImage("A.jpg");
	color_images[1] = LoadColorImage("B.jpg");
	color_images[2] = LoadColorImage("C.jpg");
	color_images[3] = LoadColorImage("D.jpg");
	
	grey_images = malloc(sizeof(Matrix*)*n_samples);
	forn(i,n_samples)
		grey_images[i] = DesaturateImage(color_images[i]);
	
	contrast = malloc(sizeof(Matrix*)*n_samples);
	forn(i,n_samples)
		contrast[i] = Contrast(grey_images[i]);
	
	saturation = malloc(sizeof(Matrix*)*n_samples);
	forn(i,n_samples)
		saturation[i] = Saturation(color_images[i]);
	
	exposeness = malloc(sizeof(Matrix*)*n_samples);
	forn(i,n_samples)
		exposeness[i] = Exposeness(color_images[i]);

	weights = malloc(sizeof(Matrix*)*n_samples);
	forn(i,n_samples)
		weights[i] = Weight(contrast[i],saturation[i],exposeness[i]);
	
	NormalizeWeights(weights,n_samples);
	
	fused = NaiveFusion(color_images,weights,n_samples);
	
	SaveColorImage(fused,"final.jpg");

	return 0;
}

Matrix* LoadGrayscaleImage(const char* filename)
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

void SaveGrayscaleImage(const Matrix* I, const char* filename)
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

ColorImage* LoadColorImage(const char* filename)
{
	int i, j;
	IplImage* img;
	Matrix* R; Matrix* G; Matrix* B;
	ColorImage* I;

	img = cvLoadImage(filename,CV_LOAD_IMAGE_UNCHANGED);
	if(!img)
	{
		printf("Could not load image file: %s\n",filename);
		exit(0);
	}
	
	I = malloc(sizeof(ColorImage));
	R = NewMatrix(img->height,img->width); I->R = R;
	G = NewMatrix(img->height,img->width); I->G = G;
	B = NewMatrix(img->height,img->width); I->B = B;
	
	for(i=0;i<R->rows;i++) for(j=0;j<R->cols;j++)
	{
		unsigned char* src = (unsigned char*)(img->imageData + i*img->widthStep + j*img->nChannels);
		
		ELEM(B,i,j) = (double)(src[0]/255.0);
		ELEM(G,i,j) = (double)(src[1]/255.0);
		ELEM(R,i,j) = (double)(src[2]/255.0);
	}
	
	return I;
}

void SaveColorImage(const ColorImage* I, const char* filename)
{
	int i, j;
	
	IplImage* img;
	img = cvCreateImage(cvSize(I->R->cols,I->R->rows), IPL_DEPTH_8U, 3);
	
	for(i=0;i<I->R->rows;i++) for(j=0;j<I->R->cols;j++)
	{
		unsigned char* dst = (unsigned char*)(img->imageData + i*img->widthStep + j*img->nChannels);
		
		dst[2] = (unsigned char)(ELEM(I->R,i,j)*255.0);
		dst[1] = (unsigned char)(ELEM(I->G,i,j)*255.0);
		dst[0] = (unsigned char)(ELEM(I->B,i,j)*255.0);
	}
	
	if(!cvSaveImage(filename,img,0))
		printf("Could not save: %s\n",filename);

	cvReleaseImage(&img);
}

Matrix* DesaturateImage(const ColorImage* I)
{
	int i, j;
	Matrix* J;

	J = NewMatrix(I->R->rows,I->R->cols);
	
	forn(i,J->rows) forn(j,J->cols)
		ELEM(J,i,j) = (
			ELEM(I->R,i,j)*0.299 +
			ELEM(I->G,i,j)*0.587 +
			ELEM(I->B,i,j)*0.114
		);
	
	return J;
}

Matrix* Contrast(const Matrix* I)
{
	int i, j;
	Matrix* lap_kernel;
	Matrix* J;
	
	lap_kernel = NewMatrix(3,3);
	ELEM(lap_kernel,0,0)=0.0; ELEM(lap_kernel,0,1)=1.0; ELEM(lap_kernel,0,2)=0.0;
	ELEM(lap_kernel,1,0)=1.0; ELEM(lap_kernel,1,1)=-4.0; ELEM(lap_kernel,1,2)=1.0;
	ELEM(lap_kernel,2,0)=0.0; ELEM(lap_kernel,2,1)=1.0; ELEM(lap_kernel,2,2)=0.0;
	
	J = Convolve(I,lap_kernel,REPLICATE);
	
	forn(i,J->rows) forn(j,J->cols)
		if (ELEM(J,i,j)<0) ELEM(J,i,j) *= -1;
	
	return J;
}

Matrix* Saturation(const ColorImage* I)
{
	int i, j;
	Matrix* J;
	
	J = NewMatrix(I->R->rows,I->R->cols);
	
	forn(i,J->rows) forn(j,J->cols)
	{
		double r = ELEM(I->R,i,j);
		double g = ELEM(I->G,i,j);
		double b = ELEM(I->B,i,j);
		
		// calculo la media (promedio)
		double mu = (b+g+r)/3;

		// calculo el desvio standard
		double sd = sqrt( ( pow(r-mu,2) + pow(g-mu,2) + pow(b-mu,2) ) / 3 );
		
		ELEM(J,i,j) = sd;
	}
	
	return J;
}

Matrix* Exposeness(const ColorImage* I)
{
	int i, j;
	Matrix* J;
	
	J = NewMatrix(I->R->rows,I->R->cols);
	
	forn(i,J->rows) forn(j,J->cols)
	{
		double ex = 1.0;
		ex *= exp( -.5*pow((ELEM(I->R,i,j))-.5, 2) / SIGMA2 );
		ex *= exp( -.5*pow((ELEM(I->G,i,j))-.5, 2) / SIGMA2 );
		ex *= exp( -.5*pow((ELEM(I->B,i,j))-.5, 2) / SIGMA2 );
		
		ELEM(J,i,j) = ex;
	}
	
	return J;
}

Matrix* Weight(const Matrix* contrast, const Matrix* saturation, const Matrix* exposeness)
{
	int i, j;
	Matrix* J;
	
	J = NewMatrix(contrast->rows,contrast->cols);
	
	forn(i,J->rows) forn(j,J->cols)
		ELEM(J,i,j) =
			ELEM(contrast,i,j) +
			ELEM(saturation,i,j) +
			ELEM(exposeness,i,j);
	
	return J;
}

void NormalizeWeights(Matrix** weights, const int n_samples)
{
	int i, j, k;
	
	forn(i,weights[0]->rows) forn(j,weights[0]->cols)
	{
		double sum = exp(-12);;
		
		forn(k,n_samples)
			sum += ELEM(weights[k],i,j);
		
		forn(k,n_samples)
			ELEM(weights[k],i,j) /= sum;
	}
}

ColorImage* NaiveFusion(/*const*/ ColorImage** color_images, /*const*/ Matrix** weights, const int n_samples)
{
	int i, j, k, rows, cols;
	Matrix* R; Matrix* G; Matrix* B;
	ColorImage* fusioned_image;
	
	rows = color_images[0]->R->rows;
	cols = color_images[0]->R->cols;
	
	fusioned_image = malloc(sizeof(ColorImage));
	R = NewMatrix(rows,cols); fusioned_image->R = R;
	G = NewMatrix(rows,cols); fusioned_image->G = G;
	B = NewMatrix(rows,cols); fusioned_image->B = B;
	
	forn(i,rows) forn(j,cols)
	{
		ELEM(R,i,j) = 0;
		ELEM(G,i,j) = 0;
		ELEM(B,i,j) = 0;
		
		forn(k,n_samples)
		{
			ELEM(R,i,j) += ELEM(color_images[k]->R,i,j) * ELEM(weights[k],i,j);
			ELEM(G,i,j) += ELEM(color_images[k]->G,i,j) * ELEM(weights[k],i,j);
			ELEM(B,i,j) += ELEM(color_images[k]->B,i,j) * ELEM(weights[k],i,j);
		}
	}
	
	return fusioned_image;
}
