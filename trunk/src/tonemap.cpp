#include<ImfRgbaFile.h>
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

// Struct que invente yo para guardar los 3 canales de una imagen y el tamanio
struct Img{
	int w;
	int h;
	float*R;
	float*G;
	float*B;
};

// Funcion que lee un HDR en formato EXR
// lo lee en un array de RGBA donde cada pixel son 4 HALFS (medio float)
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

// Funciones que calculan la luminance de un pixel
// No se por que hay papers que lo hacen de forma distinta..
float luminance(Rgba color){
	return .2126*color.r + .7152*color.g + .0722*color.b;
}
float luminance(float r, float g, float b){
	return .2126*r + .7152*g + .0722*b;
}
float luminance2(float r, float g, float b){
	return .299*r + .587*g + .114*b;
}

// Imprimir un error
void error(const char*err){
	cerr << err << endl;
	exit(0);
}

//Convertir una imagen RGBA de HALF en una imagen RGB de FLOAT
float* halfRgba2floatRgb(Rgba* img, int w, int h){
	float*out=(float*)malloc(w*h*sizeof(float)*3);
	forn(i,w*h){
		out[3*i]=img[i].r;
		out[3*i+1]=img[i].g;
		out[3*i+2]=img[i].b;
	}
	return out;
}

// Convertir una imagen RGBA de HALFS en una imagen del tipo struct Img (con canales RGB por separado, en floats)
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

// Convierte una imagen con luminosidades lineales a logaritmicas, para mostrar en la pantalla
// Usar esto cada vez que se quiere mostrar una imagen con luminosidades lineales
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

// Convertir una imagen Img (struct propia) a una Image que puede manejar la libreria ImageMagick
// la Image tiene los canales RGB mezcladosn en formato float
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

// funcion comparadora de floats para el sort
int floatcomp(const void * a, const void * b){
	if(*(float*)a < *(float*)b) return -1;
	return *(float*)a > *(float*)b;
}

// Median Filter para un canal
// es una matriz de convolucion que le asigna a cada pixel la MEDIANA de los vecinos (NO la media).
float* median_filter(float* img, int w, int h, int window){
	int win=2*window-1; //win es impar
	float*out=(float*)malloc(w*h*sizeof(float));
	float temparray[win*win];
	forn(i,h) forn(j,w){
		forn(ii,win) forn(jj,win){
			temparray[ii*win+jj]=img[(i-window+(i-window+ii<0?0:(i-window+ii>=h?h-1:ii)))*w + (j-window+(j-window+jj<0?0:(j-window+jj>=w?w-1:jj)))];
			//temparray[ii*win+jj]=img[(i-window+ii)*w + (j-window+jj)];
		}
		//ordeno los vecinos
		qsort(temparray,win*win,sizeof(float),floatcomp);
		//me quedo con la media (el del medio)
		out[i*w + j]=temparray[win*win/2];
	}
	return out;
}

// Le aplica un Median Filter a cada canal por separado
Img median_filter(Img img, int window){
	Img out;
	out.w=img.w;
	out.h=img.h;
	out.R=median_filter(img.R, img.w, img.h, window);
	out.G=median_filter(img.G, img.w, img.h, window);
	out.B=median_filter(img.B, img.w, img.h, window);
	return out;
}

// Operador de ToneMapping de K.K. Biswas
// Leer el paper de simple_tonemapping.pdf
Img biswas(Img img){
	float c=.15;
	float gamma=.4;
	int w=img.w;
	int h=img.h;
	
	//Calculo la luminance promedio de toda la imagen (GC)
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
	float GC=c*sum/(w*h);
	
	Img median=median_filter(img,5);
	
	Img out;
	out.w=w;
	out.h=h;
	out.R=(float*)malloc(w*h*sizeof(float));
	out.G=(float*)malloc(w*h*sizeof(float));
	out.B=(float*)malloc(w*h*sizeof(float));
	forn(i,h) forn(j,w){
		// Aplico el operador a cada pixel
		// del paper: YD(x,y) = Y(x,y)/[Y(x,y)+CL(x,y)]
		// donde: CL=YL[log(delta + YL/Y)] + GC
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
	readEXR(argv[1], pixels, width, height); //leer el EXR
	
	// Convertir a Img para trabajar
	Img img=halfRgba2Img(pixels,width,height);
	// Aplicar el operador Biswas
	Img bis=biswas(img);
	// Convertirlo a formato Magick
	Image out=Img2Image(bis);
	out.display();
	return 0;
}
