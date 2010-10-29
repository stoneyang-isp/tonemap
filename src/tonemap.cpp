#include<ImfRgbaFile.h>
#include<ImfStringAttribute.h>
#include<ImfMatrixAttribute.h>
#include<ImfArray.h>
#include<iostream>
#include<cstdlib>
#include<Magick++.h>
using namespace std;
using namespace Imf;
using namespace Imath;
using namespace Magick;

#define forn(i,n) for(int i=0;i<(int)(n);i++)
#define forsn(i,s,n) for(int i=(int)(s);i<(int)(n);i++)
#define forall(it,X) for(typeof((X).begin()) it=(X).begin();it!=(X).end();it++)

void readEXR(const char fileName[], Rgba* &pixels, int &width, int &height){
	// Read an RGBA image using class RgbaInputFile:
	//	- open the file
	//	- allocate memory for the pixels
	//	- describe the memory layout of the pixels
	//	- read the pixels from the file

	RgbaInputFile file(fileName);
	Box2i dw = file.dataWindow();

	width  = dw.max.x - dw.min.x + 1;
	height = dw.max.y - dw.min.y + 1;
	pixels=(Rgba*)malloc(width*height*sizeof(Rgba));
	clog<<"width: "<<width<<endl;
	clog<<"heigh: "<<height<<endl;

	file.setFrameBuffer(pixels-dw.min.x-dw.min.y*width, 1, width);
	file.readPixels(dw.min.y, dw.max.y);
	//forall(it,file.header()) clog<< it.name() <<endl;
}

float luminance(Rgba color){
	return .2126*color.r + .7152*color.g + .0722*color.b;
}
float luminance(float r, float g, float b){
	return .2126*r + .7152*g + .0722*b;
}
float luminance2(float r, float g, float b){
	return .299*r + .587*g + .114*b;
}

void error(const char*err){
	cerr << err << endl;
	exit(0);
}

struct Img{
	int w;
	int h;
	float*R;
	float*G;
	float*B;
};

float* halfRgba2floatRgb(Rgba* img, int w, int h){
	float*out=(float*)malloc(w*h*sizeof(float)*3);
	forn(i,w*h){
		out[3*i]=img[i].r;
		out[3*i+1]=img[i].g;
		out[3*i+2]=img[i].b;
	}
	return out;
}

Img halfRgba2Img(Rgba* img, int w, int h){
	Img out;
	out.w=w;
	out.h=h;
	out.R=(float*)malloc(w*h*sizeof(float));
	out.G=(float*)malloc(w*h*sizeof(float));
	out.B=(float*)malloc(w*h*sizeof(float));
	forn(i,w*h){
		out.R[i]=img[i].r;
		out.G[i]=img[i].g;
		out.B[i]=img[i].b;
	}
	return out;
}

Img gamma(Img img, float gamma){
	float maxlum=luminance(img.R[0], img.G[0], img.B[0]);
	float minlum=maxlum;
	double sum=0.;
	float lum;
	forn(i,img.w*img.h){
		lum=luminance(img.R[i], img.G[i], img.B[i]);
		sum+=lum;
		if(lum>maxlum) maxlum=lum;
		else if(lum<minlum) minlum=lum;
	}
	Img out;
	out.w=img.w;
	out.h=img.h;
	out.R=(float*)malloc(img.w*img.h*sizeof(float));
	out.G=(float*)malloc(img.w*img.h*sizeof(float));
	out.B=(float*)malloc(img.w*img.h*sizeof(float));
	forn(i,img.w*img.h){
		out.R[i]=pow(img.R[i]/maxlum, gamma);
		out.G[i]=pow(img.G[i]/maxlum, gamma);
		out.B[i]=pow(img.B[i]/maxlum, gamma);
	}
	return out;
}
Img gamma(Img img){
	return gamma(img,.4);
}

Image Img2Image(Img img){
	float*pixels=(float*)malloc(img.w*img.h*sizeof(float)*3);
	forn(i,img.w*img.h){
		pixels[3*i]  =img.R[i];
		pixels[3*i+1]=img.G[i];
		pixels[3*i+2]=img.B[i];
	}
	Image out(img.w, img.h,"RGB",FloatPixel,pixels);
	return out;
}

int floatcomp(const void * a, const void * b){
	if(*(float*)a < *(float*)b) return -1;
	return *(float*)a > *(float*)b;
}

float* median_filter(float* img, int w, int h, int window){
	int win=2*window-1; //win es impar
	float*out=(float*)malloc(w*h*sizeof(float));
	float temparray[win*win];
	forn(i,h) forn(j,w){
		forn(ii,win) forn(jj,win){
			temparray[ii*win+jj]=img[(i-window+(i-window+ii<0?0:(i-window+ii>=h?h-1:ii)))*w + (j-window+(j-window+jj<0?0:(j-window+jj>=w?w-1:jj)))];
			//temparray[ii*win+jj]=img[(i-window+ii)*w + (j-window+jj)];
		}
		qsort(temparray,win*win,sizeof(float),floatcomp);
		out[i*w + j]=temparray[win*win/2];
	}
	return out;
}

Img median_filter(Img img, int window){
	Img out;
	out.w=img.w;
	out.h=img.h;
	out.R=median_filter(img.R, img.w, img.h, window);
	out.G=median_filter(img.G, img.w, img.h, window);
	out.B=median_filter(img.B, img.w, img.h, window);
	return out;
}

Img biswas(Img img){
	int w=img.w;
	int h=img.h;
	float maxlum=luminance(img.R[0], img.G[0], img.B[0]);
	float minlum=maxlum;
	double sum=0.;
	float lum;
	forn(i,w*h){
		lum=luminance(img.R[i], img.G[i], img.B[i]);
		sum+=lum;
		if(lum>maxlum) maxlum=lum;
		else if(lum<minlum) minlum=lum;
	}
	float c=.15;
	float gamma=.4;
	float GC=c*sum/(w*h);
	
	Img median=median_filter(img,5);
	
	Img out;
	out.w=w;
	out.h=h;
	out.R=(float*)malloc(w*h*sizeof(float));
	out.G=(float*)malloc(w*h*sizeof(float));
	out.B=(float*)malloc(w*h*sizeof(float));
	forn(i,h) forn(j,w){
		float Y=luminance2(img.R[i*w+j],img.G[i*w+j],img.B[i*w+j]);
		float YL=luminance2(median.R[i*w+j],median.G[i*w+j],median.B[i*w+j]);
		float CL=GC+YL*log(1e-5+YL/Y);
		float YD=Y/(Y+CL);
		out.R[i*w+j]=pow(img.R[i*w+j]/Y,gamma)*YD;
		out.G[i*w+j]=pow(img.G[i*w+j]/Y,gamma)*YD;
		out.B[i*w+j]=pow(img.B[i*w+j]/Y,gamma)*YD;
	}
	return out;
}

int main(int argc,char**argv){
	if(argc<2) error("No Input File");
	InitializeMagick(*argv);
	Rgba* pixels;
	int width,height;
	readEXR(argv[1], pixels, width, height);
	
	Img img=halfRgba2Img(pixels,width,height);
	//Img median=median_filter(img,7);
	Img bis=biswas(img);
	//Img display=gamma(bis);
	Image out=Img2Image(bis);
	out.display();
	return 0;
}
