#include "opencv/cv.h"

#include "expofuse.h"

double Adata[9] = {1., 2., 3., 4., 5., 6., 7., 8., 9.};
Matrix A = { .rows=3, .cols=3, .data = Adata };

int main(int argc, char* argv[]){
	asmInvertir(&A);
	PrintMatrix(&A);
	return 0;
}
