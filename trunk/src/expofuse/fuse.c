#include "opencv/cv.h"

#include "expofuse.h"

const char* CommandOption(int argc, char* argv[], const char* option);
int CommandOptionSet(int argc, char* argv[], const char* option);
void PrintHelp();
void PrintVersion();

int main(int argc, char* argv[])
{
	int k, n_samples;
	Matrix** weights;
	ColorImage** color_images;
//	ColorImage* naive_fused_image;
	ColorImage* fused_image;
	int extra_parameters = 0;
	const char* output_name = "fused_image.jpg";
	double contrast_weight = DEFAULT_CONTRAST_WEIGHT;
	double saturation_weight = DEFAULT_SATURATION_WEIGHT;
	double exposeness_weight = DEFAULT_EXPOSENESS_WEIGHT;
	
	if(argc<2)
	{
		printf("\n");
		printf("Error: At least one input image is required as argument\n");
		PrintHelp();
		return 0;
	}
	
	const char* option;
	
	// if version is required, print and exit program
	if ( CommandOptionSet(argc,argv,"-v") )
	{
		PrintVersion();
		extra_parameters += 1;
		return 0;
	}
	
	// if help is required, print and exit program
	if ( CommandOptionSet(argc,argv,"-h") )
	{
		PrintHelp();
		extra_parameters += 1;
		return 0;
	}
	
	// if output name is given
	option = CommandOption(argc,argv,"-o");
	if (option)
	{
		output_name = option;
		extra_parameters += 2;
	}
	
	// if contrast weight is given
	option = CommandOption(argc,argv,"-cw");
	if (option)
	{
		contrast_weight = strtod(option,NULL);
		extra_parameters += 2;
	}
	
	// if saturation weight is given
	option = CommandOption(argc,argv,"-sw");
	if (option)
	{
		saturation_weight = strtod(option,NULL);
		extra_parameters += 2;
	}
	
	// if exposeness weight is given
	option = CommandOption(argc,argv,"-ew");
	if (option)
	{
		exposeness_weight = strtod(option,NULL);
		extra_parameters += 2;
	}
	
	n_samples = argc-1-extra_parameters;
	
	printf("Loading %d images\n",n_samples);
	
	color_images = malloc(sizeof(ColorImage*)*n_samples);
	forn(k,n_samples)
		color_images[k] = LoadColorImage(argv[1+extra_parameters+k],0);
	
	printf("Generating weights with\n - contrast weight: %f\n - saturation weight %f\n - exposeness weight %f\n",contrast_weight,saturation_weight,exposeness_weight);
	
	weights = ConstructWeights(color_images,n_samples,contrast_weight,saturation_weight,exposeness_weight,SIGMA2);
	
//	printf("Fusing the naive way\n");
	
//	naive_fused_image = NaiveFusion(color_images,weights,n_samples);
//	SaveColorImage(naive_fused_image,"fused_image_naive.jpg");

	printf("Fusing images with weights\n");

	fused_image = Fusion(color_images,weights,n_samples);
	TruncateColorImage(fused_image);
	SaveColorImage(fused_image,output_name);

	// libero memoria
	printf("Freeing memory\n");
	
	forn(k,n_samples)
		DeleteMatrix(weights[k]);
	free(weights);
	
	forn(k,n_samples)
		DeleteColorImage(color_images[k]);
	free(color_images);
/*	
	printf("DeleteColorImage(naive_fused_image)\n");
	DeleteColorImage(naive_fused_image);
*/
	DeleteColorImage(fused_image);

	return 0;
}

void PrintHelp()
{
	printf("\n");
	PrintVersion(); printf("\n");
	printf("expofuse: fuses overlapping images with exposure fusion algoirithm\n\n");
	printf("Usage: expofuse [options] <input files>\n");
	printf("Valid options are:\n");
	printf("  -o output file name with valid extension. default: 'fused_image.jpg'\n");
	printf("  -cw contrast weight value as floating point number. default: 1.0\n");
	printf("  -sw saturation weight value as floating point number. default: 1.0\n");
	printf("  -ew exposeness weight value as floating point number. default: 1.0\n");
	printf("  -v display current version\n");
	printf("  -h display help (this text)\n");
	printf("\n");
}

void PrintVersion()
{
	printf("expofuse version %d.%d\n",VERSION,SUBVERSION);
}

const char* CommandOption(int argc, char* argv[], const char* option)
{
	int i;
	for(i=1;i<argc;i++)
		// if argv matches parameter
		if (!strcmp(argv[i],option))
			return argv[i+1];
	
	// if parameter doesnt matche any argv return -1
	return NULL;
}

int CommandOptionSet(int argc, char* argv[], const char* option)
{
	int i;
	for(i=1;i<argc;i++)
		// if argv matches parameter
		if (!strcmp(argv[i],option))
			return 1;
	
	// if parameter doesnt matche any argv return -1
	return 0;
}
