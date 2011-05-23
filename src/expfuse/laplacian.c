#include "opencv/cv.h"
#include "opencv/highgui.h"
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define WC = 1
#define WS = 1
#define WE = 1
#define SIGMA .2

//#define PIX(image,i,j) ((i)*(image)->widthStep + (j)*(image)->nChannels)

void SaveImgs(double* data, int width, int height, int samples, char* prefix);
void SaveImg(double* data, int width, int height, int channels, char* name);
void ShowImg(IplImage* img);
void PrintM(double* data,int width,int height,int channels);
void PrintA(unsigned char* data,int width,int step,int height,int channels);

int main(int argc, char* argv[])
{
	IplImage* img = 0;
	int height,width,channels,step,depth,samples;
	int i,j,k;

	if(argc<2){
		printf("Usage: main <image-file-name>\n\7");
		exit(0);
	}

	samples = argc-1;
	
	// load an image
	// CV_LOAD_IMAGE_COLOR
	// CV_LOAD_IMAGE_GRAYSCALE
	// CV_LOAD_IMAGE_UNCHANGED
	
	double* data;
	
	for(k=0;k<samples;k++)
	{
		img = cvLoadImage(argv[1+k],CV_LOAD_IMAGE_UNCHANGED);
		
		if(!img){
			printf("Could not load image file: %s\n",argv[1+k]);
			exit(0);
		}
		
		if (k==0)
		{
			// get the image data
			height		= img->height;
			width		= img->width;
			channels	= img->nChannels;
			step		= img->widthStep;
			depth		= img->depth;
			
			data = malloc(sizeof(double)*height*width*channels*samples);
		}
		else
		{
			// check the image data
			assert(height == img->height);
			assert(width == img->width);
			assert(channels == img->nChannels);
			assert(step == img->widthStep);
			assert(depth == img->depth);
		}
		
		printf("Processing a %dx%d image with %d channels, %d depth, %d step\n",height,width,channels,depth,step);
		
		// normalizo los valores de los pixeles a doubles entre 0 y 1
		for(i=0;i<height;i++) for(j=0;j<width;j++)
		{
			unsigned char* src = (unsigned char*)(img->imageData + i*step + j*channels);
			double* dst = (double*)(data + i*width*channels*samples + j*channels*samples + k*channels);
			
			unsigned char b = src[0];
			unsigned char g = src[1];
			unsigned char r = src[2];
			
			dst[0] = ((double)r)/255;
			dst[1] = ((double)g)/255;
			dst[2] = ((double)b)/255;
		}

		cvReleaseImage(&img);
	}
	
	// genero la imagen en escala de grises
	printf("Generating grayscale data\n");
	double* data_grey = malloc(sizeof(double)*height*width*samples);
	for(i=0;i<height;i++) for(j=0;j<width;j++) for(k=0;k<samples;k++)
	{
		double* pix = data + ( i*width*channels*samples + j*channels*samples + k*channels );
		data_grey[ i*width*samples + j*samples + k ] = 
			( pix[0]*0.299 + pix[1]*0.587 + pix[2]*0.114 );
	}

	// guardo las imagenes en escala de grises para probar
	printf("Saving grayscale images\n");
	SaveImgs(data_grey,width,height,samples,"grey_");

	// CONTRAST (aplico el operador laplaciano)
	printf("Generating contrast data\n");
	// cvLaplace(img_grey,img_lap,1);

	double* contrast = malloc(sizeof(double)*height*width*samples);

	for(i=1;i<height-1;i++) for(j=1;j<width-1;j++) for(k=0;k<samples;k++)
	{
		unsigned int row_size = width*samples;
		unsigned int col_size = samples;
		unsigned int n = i*row_size + j*col_size + k;
		
		contrast[n] = 
			(
				0*data_grey[ n - row_size - col_size ] +
				1*data_grey[ n - row_size ] +
				0*data_grey[ n - row_size + col_size ] +
				
				1*data_grey[ n - col_size ] +
				-4*data_grey[ n ] +
				1*data_grey[ n + col_size ] +
				
				0*data_grey[ n + row_size - col_size ] +
				1*data_grey[ n + row_size ] +
				0*data_grey[ n + row_size + col_size ]
			);
	}

	printf("Saving contrast images\n");
	SaveImgs(contrast,width,height,samples,"contrast_");

	// SATURATION (standard deviation)
	printf("Generating saturation data\n");

	double* saturation = malloc(sizeof(double)*height*width*samples);

	for(i=0;i<height;i++) for(j=0;j<width;j++) for(k=0;k<samples;k++)
	{
		double* pix = data + (i*width*channels*samples + j*channels*samples + k*channels);
		
		double b = pix[0];
		double g = pix[1];
		double r = pix[2];

		// calculo la media (promedio)
		double mu = (b+g+r)/3;

		// calculo el desvio standard
		double sd = sqrt( ( pow(r-mu,2) + pow(g-mu,2) + pow(b-mu,2) ) / 3 );
		
		saturation[ i*width*samples + j*samples + k ] = sd;
	}

	printf("Saving saturation images\n");
	SaveImgs(saturation,width,height,samples,"saturation_");

	// WELL EXPOSENESS
	printf("Generating exposeness data\n");
	
	double* exposeness = malloc(sizeof(double)*height*width*samples);
	
	for(i=0;i<height;i++) for(j=0;j<width;j++) for(k=0;k<samples;k++)
	{
		double* pix = data + (i*width*channels*samples + j*channels*samples + k*channels);
		
		double ex = 1;
		int n;
		for(n=0;n<3;n++)
		{
			ex *= exp( -.5*pow((pix[n])-.5, 2) / SIGMA );
		}
		
		exposeness[ i*width*samples + j*samples + k ] = ex;
	}

	printf("Saving exposeness images\n");
	SaveImgs(exposeness,width,height,samples,"exposeness_");

	// WEIGHT MATRIX
	printf("Generating weight data\n");
	
	double* weight = malloc(sizeof(double)*height*width*samples);
	
	for(i=0;i<height;i++) for(j=0;j<width;j++) for(k=0;k<samples;k++)
	{
		unsigned int n = i*width*samples + j*samples + k;
		double w = contrast[n] * saturation[n] * exposeness[n];
		weight[n] = w;
	}
	
	printf("Normalizing weight data\n");
	
	for(i=0;i<height;i++) for(j=0;j<width;j++)
	{
		double sum = exp(-12);
		
		for(k=0;k<samples;k++)
			sum += weight[i*width*samples + j*samples + k];
		
		for(k=0;k<samples;k++)
			weight[i*width*samples + j*samples + k] /= sum;
	}
	
	printf("Saving weight images\n");
	SaveImgs(weight,width,height,samples,"weight_");
	
	// FINAL IMAGE
	double* final = malloc(sizeof(double)*height*width*channels);
	
	int c;
	for(i=0;i<height;i++) for(j=0;j<width;j++) for(c=0;c<channels;c++)
	{
		unsigned int n = i*width*channels + j*channels + c;
		final[n] = 0;
		
		for(k=0;k<samples;k++)
			final[n] +=
				data[ i*width*samples*channels + j*samples*channels + k*channels + c ] *
				weight[ i*width*samples + j*samples + k ];
	}
	
	SaveImg(final,width,height,channels,"final");
	
	return 0;
}

double* Downsample(unsigned char* data, int width, int height)
{
	int i, j;
	double* aux = malloc(sizeof(double)*height*width*samples);
	double* next = malloc(sizeof(double)*(height/2)*(width/2)*samples);
	
	// apply gaussian filter
	// 1D 5 tap low pass filter (used as 2D)
	double kernel[5] = {.0625, .25, .375, .25, .0625};
	
	for(i=0;i<height;i++) for(j=2;j<width-2;j++) for(k=0;k<samples-2;k++)
	{
		int n = i*width*samples + j*samples + k;
		
		if ( j<2 || width-3 < j )
			aux[n] = data[n];
		else
			aux[n] =
				data[n-2]*kernel[0] +
				data[n-1]*kernel[1] +
				data[n]*kernel[2] +
				data[n+1]*kernel[3] +
				data[n+2]*kernel[4];
	}
	
	for(i=2;i<height-2;i++) for(j=0;j<width;j++)
	{
		int n = i*width + j;
		aux[n] = aux[n-2*width] + aux[n-width] + aux[n] + aux[n+width] + aux[n+2*width];
	}
	
	for(i=0;i<height/2;i++) for(j=0;j<width/2;j++)
	{
		next[ i*width + j ] = aux[ 2*i*width + 2*j ];
	}
	
	return next;
}

void SaveImgs(double* data, int width, int height, int samples, char* prefix)
{
	int i, j, k;
	IplImage* img;
	
	for(k=0;k<samples;k++)
	{
		img = cvCreateImage(cvSize(width,height), IPL_DEPTH_8U, 1);
		
		for(i=0;i<height;i++) for(j=0;j<width;j++)
		{
			double* src = (double*)(data + i*width*samples + j*samples + k);
			unsigned char* dst = (unsigned char*)(img->imageData + i*img->widthStep + j);
			
			dst[0] = (unsigned char)(src[0]*255);
		}
		
		int size = strlen(prefix)+5;
		char* s = malloc(size);
		memset(s,0,size);
		strcpy(s,prefix);
		s[size-5] = (char)k + 48;
		strncat(s+6,".jpg",4);
		
		printf("Saving grayscale data %s\n",s);
		
		if(!cvSaveImage(s,img,0))
			printf("Could not save: %s\n",s);

		cvReleaseImage(&img);
	}
}

void SaveImg(double* data, int width, int height, int channels, char* name)
{
	int i, j;
	IplImage* img;
	
	img = cvCreateImage(cvSize(width,height), IPL_DEPTH_8U, channels);
	
	for(i=0;i<height;i++) for(j=0;j<width;j++)
	{
		double* src = (double*)(data + i*width*channels + j*channels);
		unsigned char* dst = (unsigned char*)(img->imageData + i*img->widthStep + j*channels);
		
		dst[2] = (unsigned char)(src[0]*255);
		dst[1] = (unsigned char)(src[1]*255);
		dst[0] = (unsigned char)(src[2]*255);
	}
	
	int size = strlen(name)+4;
	char* s = malloc(size);
	memset(s,0,size);
	strcpy(s,name);
	strncat(s,".jpg",4);
	
	printf("Saving color data %s\n",s);
	
	if(!cvSaveImage(s,img,0))
		printf("Could not save: %s\n",s);

	cvReleaseImage(&img);
}

void ShowImg(IplImage* img)
{
	// create a window
	cvNamedWindow("mainWin", CV_WINDOW_AUTOSIZE); 
	cvMoveWindow("mainWin", 100, 100);

	// show the image
	cvShowImage("mainWin", img );

	// wait for a key
	cvWaitKey(0);
}

void PrintM(double* data,int width,int height,int channels)
{
	int i,j;
	for(i=0;i<height;i++) for(j=0;j<width;j++)
	{
		double* src = (double*)(data + i*width*channels + j*channels);
		
		unsigned char r = (unsigned char)src[0];
		unsigned char g = (unsigned char)src[1];
		unsigned char b = (unsigned char)src[2];
		
		printf("(%02X%02X%02X) ",r,g,b);
		
		if (j==width-1)
			printf("\n");
	}
}

void PrintA(unsigned char* data,int width,int step,int height,int channels)
{
	int i,j;
	for(i=0;i<height;i++) for(j=0;j<width;j++)
	{
		unsigned char* src = (unsigned char*)(data + i*step + j*channels);
		
		printf("(%02X%02X%02X) ",src[2],src[1],src[0]);
		
		if (j==width-1)
			printf("\n");
	}
}
