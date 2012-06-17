#include "opencv/cv.h"

#include "expofuse.h"

int main(int argc, char* argv[]){
  if (argc<2) return -1;
  ColorImage* I = LoadColorImage(argv[1], 0);
  SaveMatrix(I->R, "matrixR.jpg");
	return 0;
}
