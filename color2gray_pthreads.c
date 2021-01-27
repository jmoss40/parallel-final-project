
/*
 *  Convert an image to grayscale.
 *
 *  To compile the code, we use
 *    gcc -g -Wall -o color2gray stb_image/stb_image.h stb_image/stb_image_write.h color2gray_pthreads.c -lpthread -lm
 *
 *  To run the code, type
 *    ./color2gray ${input color image} ${output grayscale image} ${image type} ${thread count}
 *
 *    The format of images depends on its types.
 *    To specify image type, we have ${image type} as follows:
 *      1 is for .png file
 *      2 is for .jpg file
 *       
 *    For example,
 *      ./color2gray lena1.png lena2.png 1 4
 *      ./color2gray lizard1.jpg lizard2.jpg 2 4
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

void* colorToGrayscale(void* my_rank);

const int IS_PNG = 1;
const int IS_JPG = 2;
const int DESIRED_CHANNELS = 3;
const int MAX_NAME_LENGTH = 500;

int thread_count;
unsigned char* gray_img;
unsigned char* color_img;
int width, height, my_height;

int main(int argc, char *argv[]) {
    if (argc < 5){
        printf("Usage: color2Grayscale ${input color image file} ${output grayscale image file} ${image type} ${thread count}\n Image Types:\n\t1: PGN\n\t2: JPG");
	exit(-1);
    }

    int channels, type;
    char in_name[MAX_NAME_LENGTH], out_name[MAX_NAME_LENGTH];
    strcpy(in_name, argv[1]);
    strcpy(out_name, argv[2]);
    type = atoi(argv[3]);
    thread_count = strtol(argv[4], NULL, 10);

    color_img = stbi_load(in_name, &width, &height, &channels, 0); // load and conver the image to 3 channels (ignore the transparancy channel)
    if(color_img == NULL) {
        printf("Error in loading the image\n");
        exit(-1);
    }
    printf("Loaded image %s with a width of %dpx, a height of %dpx and %d channels\n", in_name, width, height, channels);

    // Convert the input image to gray
    int gray_channels = channels == 4 ? 2 : 1;
    size_t gray_img_size = width * height * gray_channels;
    
    gray_img = (unsigned char *)malloc(gray_img_size);
    if(gray_img == NULL) {
        printf("Unable to allocate memory for the gray image.\n");
        exit(1);
    }
    printf("Create a image array with a width of %dpx, a height of %dpx and %d channels\n", width, height, gray_channels);

    //create thread handler array
    pthread_t* thread_handler = malloc(thread_count * sizeof(pthread_t));

    my_height = height / thread_count;

    //create pthreads that run the colorToGrayscale function
    for(long thread = 0; thread < thread_count; thread++){
	pthread_create(&thread_handler[thread], NULL, colorToGrayscale, (void*)thread);
    }
    
    //join the pthreads
    for(long thread = 0; thread < thread_count; thread++){
	pthread_join(thread_handler[thread], NULL);
    }

    if (type == IS_PNG)
        stbi_write_png(out_name, width, height, gray_channels, gray_img, width * gray_channels);
    else {
        if (type == IS_JPG){
	    //The last parameter of the stbi_write_jpg function is a quality parameter that goes from 1 to 100.
	    //Since JPG is a lossy image format, you can chose how much data is dropped at save time. Lower quality 
	    //means smaller image size on disk and lower visual image quality.
            stbi_write_jpg(out_name, width, height, gray_channels, gray_img, 100);
	}
    }
    printf("Wrote image %s with a width of %dpx, a height of %dpx and %d channels\n", out_name, width, height, channels);

    stbi_image_free(gray_img);
    free(thread_handler);
}

void* colorToGrayscale(void* my_rank){
    unsigned char pixel[DESIRED_CHANNELS];

    long my_first_row = my_height * (long)my_rank;
    long my_last_row = my_first_row + my_height;
    //let the last thread pick up the rest of the work if it cannot be evenly divided
    if((long)my_rank == thread_count-1){
	my_last_row += (height % thread_count);
    }

    for (int row = my_first_row; row < my_last_row; ++row)
    {
        for (int col = 0; col < width; ++col)
        {
            //If the input image has a transparency channel this will be simply copied to the second channel of
	    //the gray image, while the first channel of the gray image will contain the gray pixel values. If
	    //the input image has three channels, the output image will have only one channel with the gray data.

            int greyOffset = row * width + col;
            int rgbOffset = greyOffset * DESIRED_CHANNELS;
            pixel[0] = color_img[rgbOffset];
            pixel[1] = color_img[rgbOffset + 1];
            pixel[2] = color_img[rgbOffset + 2];

            gray_img[greyOffset] = pixel[0] * 0.3 + pixel[1] * 0.58 + pixel[2] * 0.11;
        }
    }
    return NULL;
}
