
/*
 *  Convert an image to grayscale.
 *
 *  To compile the code, we use
 *    mpicc -g -Wall -o color2gray stb_image/stb_image.h stb_image/stb_image_write.h color2gray_mpi.c -lm
 *  
 *  To run the code, type
 *    mpiexec -n ${process count} ./color2gray ${input image} ${output image} ${image type}
 *
 *    The format of images depends on its types.
 *    To specify image type, we have ${image type} as follows:
 *      1 is for .png file
 *      2 is for .jpg file
 *       
 *    For example,
 *      mpiexec -n 2 ./color2gray lena1.png lena2.png 1
 *	mpiexec -n 2 ./color2gray lizard1.jpg lizard2.jpg 1
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

const int IS_PNG = 1;
const int IS_JPG = 2;
const int DESIRED_CHANNELS = 3;
const int MAX_NAME_LENGTH = 500;

int main(int argc, char *argv[]) {
    int thread_count, my_rank, channels, type;
    int gray_channels;
    int width, height, my_height;
    char in_name[MAX_NAME_LENGTH], out_name[MAX_NAME_LENGTH];
    unsigned char* color_img, *my_color_img;
    unsigned char* gray_img, *my_gray_img;

    if (argc < 4){
        printf("Usage: color2Grayscale ${input color image file} ${output grayscale image file} ${image type}\n Image Types:\n\t1: PGN\n\t2: JPG");
        exit(-1);
    }
    
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &thread_count);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if(my_rank == 0){
	strcpy(in_name, argv[1]);
	strcpy(out_name, argv[2]);
	type = atoi(argv[3]);

	color_img = stbi_load(in_name, &width, &height, &channels, 0);
	if(color_img == NULL) {
	    printf("Error in loading the image\n");
	    exit(-1);
	}
	printf("Loaded image %s with a width of %dpx, a height of %dpx and %d channels\n", in_name, width, height, channels);

	gray_channels = channels == 4 ? 2 : 1;
	size_t gray_img_size = width * height * gray_channels;
    
	gray_img = (unsigned char *)malloc(gray_img_size);
	if(gray_img == NULL) {
	    printf("Unable to allocate memory for the gray image.\n");
	    exit(1);
	}
	printf("Create a image array with a width of %dpx, a height of %dpx and %d channels\n", width, height, gray_channels);

        my_height = height / thread_count;
    }

    MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&my_height, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&channels, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&gray_channels, 1, MPI_INT, 0, MPI_COMM_WORLD);    
    MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if(my_rank == thread_count-1)
        my_height += (height % thread_count);

    my_color_img = (unsigned char*)malloc(my_height * width * channels);
    my_gray_img = (unsigned char*)malloc(my_height * width * gray_channels);

    MPI_Scatter(color_img, my_height*width*channels, MPI_UNSIGNED_CHAR, my_color_img,
	my_height*width*channels, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    unsigned char pixel[DESIRED_CHANNELS];

    for (int row = 0; row < my_height; ++row)
    {
        for (int col = 0; col < width; ++col)
        {
            int greyOffset = row * width + col;
            int rgbOffset = greyOffset * DESIRED_CHANNELS;
            pixel[0] = my_color_img[rgbOffset];
            pixel[1] = my_color_img[rgbOffset + 1];
            pixel[2] = my_color_img[rgbOffset + 2];
 
            my_gray_img[greyOffset] = pixel[0] * 0.3 + pixel[1] * 0.58 + pixel[2] * 0.11;
        }
    }

    MPI_Gather(my_gray_img, my_height*width*gray_channels, MPI_UNSIGNED_CHAR, gray_img, 
	my_height*width*gray_channels, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    if(my_rank == 0){
	if (type == IS_PNG)
	    stbi_write_png(out_name, width, height, gray_channels, gray_img, width * gray_channels);
	else{ 
	    if (type == IS_JPG)
		stbi_write_jpg(out_name, width, height, gray_channels, gray_img, 100);
	}
	printf("Wrote image %s with a width of %dpx, a height of %dpx and %d channels\n", out_name, width, height, channels);
	stbi_image_free(gray_img);
	stbi_image_free(color_img);
    }
    free(my_color_img);
    free(my_gray_img);
    MPI_Finalize();
}
