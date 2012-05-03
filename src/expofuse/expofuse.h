#ifndef EXPOFUSE_H
#define EXPOFUSE_H

// program version
#define VERSION 1
#define SUBVERSION 0

#define DEFAULT_CONTRAST_WEIGHT 1
#define DEFAULT_SATURATION_WEIGHT 1
#define DEFAULT_EXPOSENESS_WEIGHT 1
#define SIGMA2 .04 // representa SIGMA^2 cuando SIGMA = .2

#include "opencv/cv.h"
#include "opencv/highgui.h"
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "Matrix.h"

typedef struct
{
	Matrix* R;
	Matrix* G;
	Matrix* B;
} ColorImage;

ColorImage* NewColorImage();

void DeleteColorImage(ColorImage* image);

ColorImage* CopyColorImage(const ColorImage* image);

Matrix** ConstructWeights(/*const*/ ColorImage** color_images, const int n_samples, double contrast_weight, double saturation_weight, double exposeness_weight, double sigma);

ColorImage* AddColorImage(const ColorImage* A, const ColorImage* B);

ColorImage* AddEqualsColorImage(ColorImage* A, const ColorImage* B);

ColorImage* LoadColorImage(const char* filename, const int size);

void SaveColorImage(const ColorImage* I, const char* filename);

void TruncateColorImage(ColorImage* I);
extern void _asmTruncate(double* A, int rows, int cols);

// Matrix* LoadGrayscaleImage(const char* filename);

// void SaveGrayscaleImage(const Matrix* I, const char* filename);

Matrix* DesaturateImage(const ColorImage* I);

Matrix* Contrast(const Matrix* I);

Matrix* Saturation(const ColorImage* I);

Matrix* Exposeness(const ColorImage* I, double sigma);

Matrix* Weight(const Matrix* contrast, double contrast_weight, const Matrix* saturation, double saturation_weight, const Matrix* exposeness, double exposeness_weight);

void NormalizeWeights(Matrix** weights, const int n_samples);

void WeightColorImage(const ColorImage* color_image, const Matrix* weights);

ColorImage* NaiveFusion(/*const*/ ColorImage** color_images, /*const*/ Matrix** weights, const int n_samples);

ColorImage* Fusion(/*const*/ ColorImage** color_images, /*const*/ Matrix** weights, const int n_samples);

Matrix** GaussianPyramid(/*const*/ Matrix* I, const int levels);

Matrix** LaplacianPyramid(/*const*/ Matrix* I, const int levels);

ColorImage** ColorLaplacianPyramid(/*const*/ ColorImage* I, const int levels);

ColorImage* ReconstructFromPyramid(ColorImage** pyramid, const int n_levels);

#endif
