#include "expofuse.h"

ColorImage* NewColorImage()
{
	return malloc(sizeof(ColorImage));
}

void DeleteColorImage(ColorImage* image)
{
	DeleteMatrix(image->R);
	DeleteMatrix(image->G);
	DeleteMatrix(image->B);
	free(image);
}

ColorImage* CopyColorImage(const ColorImage* image)
{
	ColorImage* copy = NewColorImage();
	copy->R = CopyMatrix(image->R);
	copy->G = CopyMatrix(image->G);
	copy->B = CopyMatrix(image->B);
	return copy;
}

Matrix** ConstructWeights(/*const*/ ColorImage** color_images, int n_samples,  double contrast_weight,  double saturation_weight,  double exposeness_weight, double sigma)
{
	Matrix** grey_images;
	Matrix** contrast;
	Matrix** saturation;
	Matrix** exposeness;
	Matrix** weights;
	int k;
	
	grey_images = malloc(sizeof(Matrix*) * n_samples);
	forn(k, n_samples)
		grey_images[k] = DesaturateImage(color_images[k]);
	
	contrast = malloc(sizeof(Matrix*) * n_samples);
	forn(k, n_samples)
		contrast[k] = Contrast(grey_images[k]);
	
	saturation = malloc(sizeof(Matrix*) * n_samples);
	forn(k, n_samples)
		saturation[k] = Saturation(color_images[k]);
	
	exposeness = malloc(sizeof(Matrix*)*n_samples);
	forn(k,n_samples)
		exposeness[k] = Exposeness(color_images[k], sigma);

	weights = malloc(sizeof(Matrix*)*n_samples);
	forn(k, n_samples)
		weights[k] = Weight(contrast[k], contrast_weight, saturation[k],
		                    saturation_weight, exposeness[k], exposeness_weight);
	
	NormalizeWeights(weights, n_samples);
	
	// Free memory
	
	forn(k,n_samples) DeleteMatrix(grey_images[k]);
	free(grey_images);
	
	forn(k,n_samples) DeleteMatrix(contrast[k]);
	free(contrast);
	
	forn(k,n_samples) DeleteMatrix(saturation[k]);
	free(saturation);
	
	forn(k,n_samples) DeleteMatrix(exposeness[k]);
	free(exposeness);
	
	return weights;
}

ColorImage* AddColorImage(const ColorImage* A, const ColorImage* B)
{
	ColorImage* C;
	C = NewColorImage();
	
	C->R = AddMatrix(A->R,B->R);
	C->G = AddMatrix(A->G,B->G);
	C->B = AddMatrix(A->B,B->B);
	
	return C;
}

ColorImage* AddEqualsColorImage(ColorImage* A, const ColorImage* B)
{
	AddEqualsMatrix(A->R,B->R);
	AddEqualsMatrix(A->G,B->G);
	AddEqualsMatrix(A->B,B->B);
	
	return A;
}

Matrix** GaussianPyramid(/*const*/ Matrix* I, int levels) {
	int k;
	Matrix** pyramid;
	
	pyramid = malloc(sizeof(Matrix*)*levels);
	
	// copio el primer nivel como la imagen original
	pyramid[0] = CopyMatrix(I);
	
	for (k=1; k<levels; k++) {
		I = Downsample(I);
		pyramid[k] = I;
	}
	
	return pyramid;
}

ColorImage** ColorLaplacianPyramid(/*const*/ ColorImage* color_image, int n_levels)
{
	int k;
	Matrix** R_pyramid;
	Matrix** G_pyramid;
	Matrix** B_pyramid;
	ColorImage** pyramid;
	
	R_pyramid = LaplacianPyramid(color_image->R,n_levels);
	G_pyramid = LaplacianPyramid(color_image->G,n_levels);
	B_pyramid = LaplacianPyramid(color_image->B,n_levels);
	
	pyramid = malloc(sizeof(ColorImage*)*n_levels);
	forn(k, n_levels) {
		pyramid[k] = NewColorImage();
		pyramid[k]->R = R_pyramid[k];
		pyramid[k]->G = G_pyramid[k];
		pyramid[k]->B = B_pyramid[k];
	}
	
	return pyramid;
}

Matrix** LaplacianPyramid(/*const*/ Matrix* I, int levels)
{
	int k;
	Matrix* current_level;
	Matrix* aux;
	Matrix* downsampled;
	Matrix** pyramid;
	
	pyramid = malloc(sizeof(Matrix*)*levels);
	
	current_level = CopyMatrix(I);
	for (k=0; k<levels-1; k++) {
		downsampled = Downsample(current_level);
		
		int odd_rows = current_level->rows - 2*downsampled->rows;
		int odd_cols = current_level->cols - 2*downsampled->cols;
		
		aux = Upsample(downsampled,odd_rows,odd_cols);
		pyramid[k] = Substract(current_level, aux);
		
		// free some memory
		DeleteMatrix(current_level);
		DeleteMatrix(aux);
		
		current_level = downsampled;
	}
	
	pyramid[levels-1] = current_level;
	
	return pyramid;
}

ColorImage* LoadColorImage(const char* filename, int size)
{
	int i, j;
	IplImage* img;
	Matrix* R; Matrix* G; Matrix* B;
	ColorImage* I;

	printf("loading image %s\n",filename);
	img = cvLoadImage(filename,CV_LOAD_IMAGE_UNCHANGED);
	if(!img){
		printf("Could not load image file: %s\n",filename);
		exit(0);
	}
	
	int w,h;
	if(size>0) {
		if(img->height > img->width){
			h = size;
			w = img->width * size / img->height;
		}else{
			w = size;
			h = img->height * size / img->width;
		}
	} else {
    w = img->width;
	  h = img->height;
	}
	IplImage* small=cvCreateImage(cvSize(w,h), img->depth, img->nChannels);
	cvResize(img, small, CV_INTER_CUBIC);
	cvReleaseImage(&img);
	img=small;
	
	I = malloc(sizeof(ColorImage));
	
	R = NewMatrix(img->height,img->width); I->R = R;
	G = NewMatrix(img->height,img->width); I->G = G;
	B = NewMatrix(img->height,img->width); I->B = B;
	
	for(i=0; i < R->rows; i++) for(j=0; j < R->cols; j++) {
		unsigned char* src = (unsigned char*)(img->imageData + i*img->widthStep + j*img->nChannels);
		
		ELEM(B,i,j) = (double)(src[0]/255.0);
		ELEM(G,i,j) = (double)(src[1]/255.0);
		ELEM(R,i,j) = (double)(src[2]/255.0);
	}
	
	cvReleaseImage(&img);
	
	return I;
}

void SaveColorImage(const ColorImage* I, const char* filename)
{
	int i, j;
	
	IplImage* img;
	img = cvCreateImage(cvSize(I->R->cols,I->R->rows), IPL_DEPTH_8U, 3);
	
	forn(i, I->R->rows) forn(j, I->R->cols) {
		unsigned char* dst = (unsigned char*)(img->imageData + i*img->widthStep + j*img->nChannels);
		
		if ( ELEM(I->R,i,j)<0 || ELEM(I->G,i,j)<0 || ELEM(I->B,i,j)<0 )
			printf("WARNING: negative values present!\n");
		
		if ( 1<ELEM(I->R,i,j) || 1<ELEM(I->G,i,j) || 1<ELEM(I->B,i,j) )
			printf("WARNING: values greater than 1 present!\n");
		
		dst[2] = (unsigned char)(ELEM(I->R,i,j)*(ELEM(I->R,i,j)<0?-1:1)*255.0);
		dst[1] = (unsigned char)(ELEM(I->G,i,j)*(ELEM(I->G,i,j)<0?-1:1)*255.0);
		dst[0] = (unsigned char)(ELEM(I->B,i,j)*(ELEM(I->B,i,j)<0?-1:1)*255.0);
	}
	
	printf("saving image %s\n",filename);
	if(!cvSaveImage(filename,img,0))
		printf("Could not save: %s\n",filename);

	cvReleaseImage(&img);
}

void TruncateColorImage(ColorImage* I)
{
	
	#ifdef NO_ASM
	  int i, j;
	  forn(i,I->R->rows) forn(j,I->R->cols) {
		  if ( ELEM(I->R,i,j)<0 ) ELEM(I->R,i,j)=0;
		  if ( 1<ELEM(I->R,i,j) ) ELEM(I->R,i,j)=1;
		
		  if ( ELEM(I->G,i,j)<0 ) ELEM(I->G,i,j)=0;
		  if ( 1<ELEM(I->G,i,j) ) ELEM(I->G,i,j)=1;
		
		  if ( ELEM(I->B,i,j)<0 ) ELEM(I->B,i,j)=0;
		  if ( 1<ELEM(I->B,i,j) ) ELEM(I->B,i,j)=1;
	  }
	#else
    int rows = I->R->rows;
    int cols = I->R->cols;
	  _asmTruncate(I->R->data, rows, cols);
	  _asmTruncate(I->G->data, rows, cols);
	  _asmTruncate(I->B->data, rows, cols);
	#endif
}

Matrix* DesaturateImage(const ColorImage* color_image)
{
	Matrix* J;

	J = NewMatrix(color_image->R->rows,color_image->R->cols);
	
	#ifdef NO_ASM
	  int i, j;
	  forn(i,J->rows) forn(j,J->cols)
		  ELEM(J,i,j) = (
			  ELEM(color_image->R,i,j)*0.299 +
			  ELEM(color_image->G,i,j)*0.587 +
			  ELEM(color_image->B,i,j)*0.114
		  );
	#else
  	_asmDesaturate(J->data, color_image->R->data, color_image->G->data, color_image->B->data, J->rows, J->cols);
	#endif
	
	return J;
}

Matrix* Contrast(const Matrix* I)
{
	Matrix* J;
	
	J = Convolve(I, &LAPLACIAN_KERN_3x3, REPLICATE);
	
  #ifdef NO_ASM
	  int i, j;
	  forn(i,J->rows) forn(j,J->cols)
		  if (ELEM(J,i,j)<0) ELEM(J,i,j) *= -1;
	#else
	  _asmAbs(J->data, J->rows, J->cols);
	#endif
	
	return J;
}

void SContrast(ColorImage* A, double strength) {
	ContrastCurve(A->R, strength);
	ContrastCurve(A->G, strength);
	ContrastCurve(A->B, strength);
}

Matrix* Saturation(const ColorImage* I)
{
	Matrix* J;
	
	J = NewMatrix(I->R->rows,I->R->cols);
	
	#ifdef NO_ASM
	  int i, j;
	  forn(i,J->rows) forn(j,J->cols) {
		  double r = ELEM(I->R,i,j);
		  double g = ELEM(I->G,i,j);
		  double b = ELEM(I->B,i,j);
		
		  // calculo la media (promedio)
		  double mu = (b+g+r)/3;

		  // calculo el desvio standard
		  double sd = sqrt( ( pow(r-mu,2) + pow(g-mu,2) + pow(b-mu,2) ) / 3 );
		
		  ELEM(J,i,j) = sd;
	  }
	#else
  	_asmSaturation(J->data, I->R->data, I->G->data, I->B->data, J->rows, J->cols);
	#endif
	
	return J;
}

Matrix* Exposeness(const ColorImage* I, double sigma)
{
	double sigma2 = sigma*sigma;
	Matrix* J;
	
	J = NewMatrix(I->R->rows,I->R->cols);
	
	#ifdef NO_ASM
	  int i, j;
	  forn(i,J->rows) forn(j,J->cols) {
		  double ex = 1.0;
		  ex *= exp( -.5*pow((ELEM(I->R,i,j))-.5, 2) / sigma2 );
		  ex *= exp( -.5*pow((ELEM(I->G,i,j))-.5, 2) / sigma2 );
		  ex *= exp( -.5*pow((ELEM(I->B,i,j))-.5, 2) / sigma2 );
		  ELEM(J,i,j) = ex;
	  }
	#else
  	_asmExposeness(J->data, I->R->data, I->R->data, I->R->data, J->rows, J->cols, sigma2);
	#endif
	
	return J;
}

Matrix* Weight(const Matrix* contrast, double contrast_weight, const Matrix* saturation, double saturation_weight, const Matrix* exposeness, double exposeness_weight)
{
	int i, j;
	Matrix* J;
	
	J = NewMatrix(contrast->rows,contrast->cols);
	
	forn(i,J->rows) forn(j,J->cols)
		ELEM(J,i,j) =
			pow(ELEM(contrast,i,j),contrast_weight) +
			pow(ELEM(saturation,i,j),saturation_weight) +
			pow(ELEM(exposeness,i,j),exposeness_weight);
	
	return J;
}

void NormalizeWeights(Matrix** weights, int n_samples)
{
	double expm12 = exp(-12);

	int i, j, k;
	forn(i, weights[0]->rows) forn(j, weights[0]->cols) {
		double sum = expm12;
		
		forn(k,n_samples)
			sum += ELEM(weights[k],i,j);
		
		forn(k,n_samples)
			ELEM(weights[k],i,j) /= sum;
	}
}

void WeightColorImage(const ColorImage* color_image, const Matrix* weights)
{
	int i, j;
	
	forn(i, weights->rows) forn(j, weights->cols) {
		ELEM(color_image->R,i,j) *= ELEM(weights,i,j);
		ELEM(color_image->G,i,j) *= ELEM(weights,i,j);
		ELEM(color_image->B,i,j) *= ELEM(weights,i,j);
	}
}

ColorImage* NaiveFusion(/*const*/ ColorImage** color_images, /*const*/ Matrix** weights, int n_samples)
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
	
	forn(i, rows) forn(j, cols) {
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

ColorImage* Fusion(/*const*/ ColorImage** color_images, /*const*/ Matrix** weights, int n_samples)
{
	int i, j, n_levels;
	ColorImage* aux;
	ColorImage** fused_pyramid;
	
	n_levels = (int)floor(log(min(weights[0]->rows,weights[0]->cols))/log(2));
	
	fused_pyramid = malloc(sizeof(ColorImage*)*n_levels);
	
	forn(i,n_samples) {
		// construyo piramide por cada imagen
		Matrix** weight_gauss_pyramid = GaussianPyramid(weights[i], n_levels);
		ColorImage** laplacian_image_pyramid = ColorLaplacianPyramid(color_images[i], n_levels);
		
		// Save Pyramids as Images
    /*int c;
    char filename[50];
	  for(c = 0; c < n_samples; c++) {
	    sprintf(filename, "gauss_sample%d_level%d.jpg", i, c);
	    SaveMatrix(weight_gauss_pyramid[c], filename);
	    sprintf(filename, "laplace_sample%d_level%d.jpg", i, c);
	    SaveColorImage(laplacian_image_pyramid[c], filename);
	  }*/
		
		// agrego cada nivel al resultado compuesto
		forn(j, n_levels) {
			WeightColorImage(laplacian_image_pyramid[j], weight_gauss_pyramid[j]);
			if (i==0)
				fused_pyramid[j] = CopyColorImage(laplacian_image_pyramid[j]);
			else
				AddEqualsColorImage(fused_pyramid[j], laplacian_image_pyramid[j]);
		}
		// Save Pyramid as Images
    /*int c;
    char filename[50];
	  for(c = 0; c < n_levels; c++) {
	    sprintf(filename, "fused_pyramid_sample%d_level%d.jpg", i, c);
	    SaveColorImage(fused_pyramid[c], filename);
	  }*/
		
		// free some memory
		forn(j,n_levels)
			DeleteMatrix(weight_gauss_pyramid[j]);
		free(weight_gauss_pyramid);
		
		forn(j,n_levels)
			DeleteColorImage(laplacian_image_pyramid[j]);
		free(laplacian_image_pyramid);
	}
	
	aux = ReconstructFromPyramid(fused_pyramid,n_levels);
	
	// free some memory
	
	forn(j,n_levels)
		DeleteColorImage(fused_pyramid[j]);
	free(fused_pyramid);
	
	return aux;
}

ColorImage* ReconstructFromPyramid(ColorImage** pyramid, int n_levels)
{
	int k;
	Matrix* aux_r;
	Matrix* aux_g;
	Matrix* aux_b;
	ColorImage* color_image;
	color_image = pyramid[n_levels-1];
	for (k=n_levels-2;k>=0;k--)
	{
		int odd_rows = pyramid[k]->R->rows - 2*color_image->R->rows;
		int odd_cols = pyramid[k]->R->cols - 2*color_image->R->cols;
		
		aux_r = Upsample(color_image->R,odd_rows,odd_cols);
		aux_g = Upsample(color_image->G,odd_rows,odd_cols);
		aux_b = Upsample(color_image->B,odd_rows,odd_cols);
		
		// if it isnt the first loop,
		// where color_image belongs to the pyramid,
		// free some memory
		if (k!=n_levels-2)
			DeleteColorImage(color_image);
		color_image = NewColorImage();
		
		color_image->R = AddMatrix(pyramid[k]->R, aux_r);
		color_image->G = AddMatrix(pyramid[k]->G, aux_g);
		color_image->B = AddMatrix(pyramid[k]->B, aux_b);
		
		DeleteMatrix(aux_r);
		DeleteMatrix(aux_g);
		DeleteMatrix(aux_b);
	}
	
	return color_image;
}

void SaveMatrix(Matrix* I, char* filename) {
	int i, j;
	
	IplImage* img;
	img = cvCreateImage(cvSize(I->cols,I->rows), IPL_DEPTH_8U, 1);
	
	forn(i, I->rows) forn(j, I->cols) {
		unsigned char* dst = (unsigned char*)(img->imageData + i*img->widthStep + j);
		
		if (ELEM(I,i,j) < 0) printf("WARNING: negative values present!\n");
		if (1 < ELEM(I,i,j)) printf("WARNING: values greater than 1 present!\n");
		
		*dst = (unsigned char)((ELEM(I,i,j)>1?1:ELEM(I,i,j))*(ELEM(I,i,j)<0?0:1)*255.0);
	}
	
	printf("saving image %s\n", filename);
	if (!cvSaveImage(filename, img, 0)) printf("Could not save: %s\n",filename);

	cvReleaseImage(&img);
}
