#include "opencv/cv.h"
#include "opencv/highgui.h"
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "Matrix.h"

#define WC 1
#define WS 1
#define WE 1
#define SIGMA2 .04 // representa SIGMA^2 cuando SIGMA = .2

typedef struct
{
	Matrix* R;
	Matrix* G;
	Matrix* B;
} ColorImage;

Matrix** ConstructWeights(/*const*/ ColorImage** color_images, const int n_samples);
ColorImage* AddEqualsColorImage(ColorImage* A, const ColorImage* B);
ColorImage* LoadColorImage(const char* filename);
void SaveColorImage(const ColorImage* I, const char* filename);
void TruncateColorImage(ColorImage* I);
Matrix* LoadGrayscaleImage(const char* filename);
void SaveGrayscaleImage(const Matrix* I, const char* filename);
Matrix* DesaturateImage(const ColorImage* I);
Matrix* Contrast(const Matrix* I);
Matrix* Saturation(const ColorImage* I);
Matrix* Exposeness(const ColorImage* I);
Matrix* Weight(const Matrix* contrast, const Matrix* saturation, const Matrix* exposeness);
void NormalizeWeights(Matrix** weights, const int n_samples);
void WeightColorImage(const ColorImage* color_image, const Matrix* weights);
ColorImage* NaiveFusion(/*const*/ ColorImage** color_images, /*const*/ Matrix** weights, const int n_samples);
ColorImage* Fusion(/*const*/ ColorImage** color_images, /*const*/ Matrix** weights, const int n_samples);
Matrix** GaussianPyramid(/*const*/ Matrix* I, const int levels);
Matrix** LaplacianPyramid(/*const*/ Matrix* I, const int levels);
ColorImage** ColorLaplacianPyramid(/*const*/ ColorImage* I, const int levels);
Matrix* Downsample(const Matrix* I);
Matrix* Upsample(const Matrix* I, const int odd_rows, const int odd_cols);
ColorImage* ReconstructFromPyramid(ColorImage** pyramid, const int n_levels);

int main(int argc, char* argv[])
{
	int k, n_samples;
	Matrix** weights;
	ColorImage** color_images;
	ColorImage* naive_fused_image;
	ColorImage* fused_image;
	
	if(argc<2){
		printf("Usage: expofuse [FILE]\n\7");
		exit(0);
	}
	
	n_samples = argc-1;
	
	printf("Loading %d images\n",n_samples);
	
	color_images = malloc(sizeof(ColorImage*)*n_samples);
	forn(k,n_samples)
		color_images[k] = LoadColorImage(argv[1+k]);
	
	printf("Generating weights\n");
	
	weights = ConstructWeights(color_images,n_samples);
	
	printf("Fusing the naive way\n");
	
	naive_fused_image = NaiveFusion(color_images,weights,n_samples);
	SaveColorImage(naive_fused_image,"fused_image_naive.jpg");

	printf("Fusing images with weights\n");

	fused_image = Fusion(color_images,weights,n_samples);
	TruncateColorImage(fused_image);
	SaveColorImage(fused_image,"fused_image.jpg");

	return 0;
}

Matrix** ConstructWeights(/*const*/ ColorImage** color_images, const int n_samples)
{
	Matrix** grey_images;
	Matrix** contrast;
	Matrix** saturation;
	Matrix** exposeness;
	Matrix** weights;
	int i;
	
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
	
	return weights;
}

void LaplacianTest()
{
	int j;
	ColorImage* color_image;
	//Matrix* G;
	
	color_image = LoadColorImage("C.jpg");
	//G = DesaturateImage(color_image);
	
	int n_levels = (int)floor(log(min(color_image->R->rows,color_image->R->cols))/log(2));
	ColorImage** laplacian_image_pyramid = ColorLaplacianPyramid(color_image,n_levels);

	forn(j,n_levels)
	{
		char name[30];
		sprintf(name,"image_pyramid_%d.jpg",j);
		SaveColorImage(laplacian_image_pyramid[j],name);
	}
}

ColorImage* AddEqualsColorImage(ColorImage* A, const ColorImage* B)
{
	AddEqualsMatrix(A->R,B->R);
	AddEqualsMatrix(A->G,B->G);
	AddEqualsMatrix(A->B,B->B);
	
	return A;
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
		dst[0] = (unsigned char)(ELEM(I,i,j)*(ELEM(I,i,j)<0?-1:1)*255.0);
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
	Matrix* convolved;
	Matrix* downsampled;
	
	convolved = Convolve(I,&GAUSS_KERN_5x1,SYMMETRIC);
	convolved = Convolve(convolved,&GAUSS_KERN_1x5,SYMMETRIC);
	
	downsampled = NewMatrix(I->rows/2,I->cols/2);
	
	forn(i,downsampled->rows) forn(j,downsampled->cols)
		ELEM(downsampled,i,j) = ELEM(convolved,2*i,2*j);
	
	DeleteMatrix(convolved);
	
	return downsampled;
}

ColorImage** ColorLaplacianPyramid(/*const*/ ColorImage* color_image, const int n_levels)
{
	int k;
	//ColorImage* J;
	Matrix** R_pyramid;
	Matrix** G_pyramid;
	Matrix** B_pyramid;
	ColorImage** pyramid;
	
	R_pyramid = LaplacianPyramid(color_image->R,n_levels);
	G_pyramid = LaplacianPyramid(color_image->G,n_levels);
	B_pyramid = LaplacianPyramid(color_image->B,n_levels);
	
	pyramid = malloc(sizeof(ColorImage*)*n_levels);
	forn(k,n_levels)
	{
		pyramid[k] = malloc(sizeof(ColorImage));
		pyramid[k]->R = R_pyramid[k];
		pyramid[k]->G = G_pyramid[k];
		pyramid[k]->B = B_pyramid[k];
	}
	
/*
	J = malloc(sizeof(ColorImage));
	pyramid = malloc(sizeof(ColorImage*)*levels);
	
	J->R = I->R;
	J->G = I->G;
	J->B = I->B;
	
	for (k=0;k<levels-1;k++)
	{
		I->R = Downsample(J->R);
		I->G = Downsample(J->G);
		I->B = Downsample(J->B);
		
		int odd_rows = J->R->rows - 2*I->R->rows;
		int odd_cols = J->R->cols - 2*I->R->cols;
		
		pyramid[k] = malloc(sizeof(ColorImage));
		pyramid[k]->R = Substract(J->R,Upsample(I->R,odd_rows,odd_cols));
		pyramid[k]->G = Substract(J->G,Upsample(I->G,odd_rows,odd_cols));
		pyramid[k]->B = Substract(J->B,Upsample(I->B,odd_rows,odd_cols));
		
		J->R = I->R;
		J->G = I->G;
		J->B = I->B;
	}
	
	pyramid[levels-1] = J;
*/
	
	return pyramid;
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
	Matrix* upsampled;
	
	if ((odd_rows!=1 && odd_rows!=0) || (odd_cols!=1 && odd_cols!=0))
	{
		printf("Error: Upsample - odd_rows: %d odd_cols: %d\n",odd_rows,odd_cols);
		exit(0);
	}
	
	upsampled = NewMatrix(2*I->rows+odd_rows,2*I->cols+odd_cols);
	
	forn(i,I->rows) forn(j,I->cols)
	{
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
			ELEM(upsampled,i,upsampled->cols-1) = ELEM(upsampled,i,upsampled->cols-3);
	
	upsampled = Convolve(upsampled,&GAUSS_KERN_5x1,SYMMETRIC);
	upsampled = Convolve(upsampled,&GAUSS_KERN_1x5,SYMMETRIC);
	
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
	
	forn(i,I->R->rows) forn(j,I->R->cols)
	{
		unsigned char* dst = (unsigned char*)(img->imageData + i*img->widthStep + j*img->nChannels);
		
		if ( ELEM(I->R,i,j)<0 || ELEM(I->G,i,j)<0 || ELEM(I->B,i,j)<0 )
			printf("WARNING: negative values present!\n");
		
		if ( 1<ELEM(I->R,i,j) || 1<ELEM(I->G,i,j) || 1<ELEM(I->B,i,j) )
			printf("WARNING: values greater than 1 present!\n");
		
		dst[2] = (unsigned char)(ELEM(I->R,i,j)*(ELEM(I->R,i,j)<0?-1:1)*255.0);
		dst[1] = (unsigned char)(ELEM(I->G,i,j)*(ELEM(I->G,i,j)<0?-1:1)*255.0);
		dst[0] = (unsigned char)(ELEM(I->B,i,j)*(ELEM(I->B,i,j)<0?-1:1)*255.0);
	}
	
	if(!cvSaveImage(filename,img,0))
		printf("Could not save: %s\n",filename);

	cvReleaseImage(&img);
}

void TruncateColorImage(ColorImage* I)
{
	int i, j;
	
	forn(i,I->R->rows) forn(j,I->R->cols)
	{
		if ( ELEM(I->R,i,j)<0 ) ELEM(I->R,i,j)=0;
		if ( 1<ELEM(I->R,i,j) ) ELEM(I->R,i,j)=1;
		
		if ( ELEM(I->G,i,j)<0 ) ELEM(I->G,i,j)=0;
		if ( 1<ELEM(I->G,i,j) ) ELEM(I->G,i,j)=1;
		
		if ( ELEM(I->B,i,j)<0 ) ELEM(I->B,i,j)=0;
		if ( 1<ELEM(I->B,i,j) ) ELEM(I->B,i,j)=1;
	}
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
	Matrix* J;
	
	J = Convolve(I,&LAPLACIAN_KERN_3x3,REPLICATE);
	
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
			pow(ELEM(contrast,i,j),WC) +
			pow(ELEM(saturation,i,j),WS) +
			pow(ELEM(exposeness,i,j),WE);
	
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

void WeightColorImage(const ColorImage* color_image, const Matrix* weights)
{
	int i, j;
	
	forn(i,weights->rows) forn(j,weights->cols)
	{
		ELEM(color_image->R,i,j) *= ELEM(weights,i,j);
		ELEM(color_image->G,i,j) *= ELEM(weights,i,j);
		ELEM(color_image->B,i,j) *= ELEM(weights,i,j);
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

ColorImage* Fusion(/*const*/ ColorImage** color_images, /*const*/ Matrix** weights, const int n_samples)
{
	int i, j, n_levels;
	ColorImage** fused_pyramid;
	
	n_levels = (int)floor(log(min(weights[0]->rows,weights[0]->cols))/log(2));
	
	fused_pyramid = malloc(sizeof(ColorImage*)*n_levels);
	
	forn(i,n_samples)
	{
		// construyo piramide por cada imagen
		Matrix** weight_gauss_pyramid = GaussianPyramid(weights[i],n_levels);
		ColorImage** laplacian_image_pyramid = ColorLaplacianPyramid(color_images[i],n_levels);
		
		// agrego cada nivel al resultado compuesto
		forn(j,n_levels)
		{
			WeightColorImage(laplacian_image_pyramid[j],weight_gauss_pyramid[j]);
			if (i==0)
				fused_pyramid[j] = laplacian_image_pyramid[j];
			else
				AddEqualsColorImage(fused_pyramid[j],laplacian_image_pyramid[j]);
		}
	}
	
	return ReconstructFromPyramid(fused_pyramid,n_levels);
}

ColorImage* ReconstructFromPyramid(ColorImage** pyramid, const int n_levels)
{
	int k;
	ColorImage* color_image;
	color_image = pyramid[n_levels-1];
	for (k=n_levels-2;k>=0;k--)
	{
		int odd_rows = pyramid[k]->R->rows - 2*color_image->R->rows;
		int odd_cols = pyramid[k]->R->cols - 2*color_image->R->cols;
		
		color_image->R = Add(pyramid[k]->R,Upsample(color_image->R,odd_rows,odd_cols));
		color_image->G = Add(pyramid[k]->G,Upsample(color_image->G,odd_rows,odd_cols));
		color_image->B = Add(pyramid[k]->B,Upsample(color_image->B,odd_rows,odd_cols));
	}
	
	return color_image;
}

