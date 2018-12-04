/******************************************************************************
* 
* dense_mm.c
* 
* This program implements a dense matrix multiply and can be used as a
* hypothetical workload. 
*
* Usage: This program takes a single input describing the size of the matrices
*        to multiply. For an input of size N, it computes A*B = C where each
*        of A, B, and C are matrices of size N*N. Matrices A and B are filled
*        with random values. 
*
* Written Sept 6, 2015 by David Ferry
* Amended for "Lab 3: Memory Management and Paging" by Brian Kocoloski
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <paging.h>
#define PAGE_SIZE sysconf(_SC_PAGESIZE)

const int num_expected_args = 2;
const unsigned sqrt_of_UINT32_MAX = 65536;
struct timeval start_map, end_map, start_mult, end_mult;

static void *
mmap_malloc(int    fd,
            size_t bytes)
{

    void * data;

    data = mmap(0, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        fprintf(stderr, "Could not mmap " DEV_NAME ": %s\n", strerror(errno));
        return NULL;
    }

    return data;

    //return malloc(bytes);
}

int main (int argc, char* argv[])
{
    int fd;
	unsigned index, row, col; //loop indicies
	unsigned matrix_size, squared_size;
	double *A, *B, *C;
	FILE *fp;
	char filename[1024];

	if (argc != num_expected_args) {
		printf("Usage: ./dense_mm <size of matrices>\n");
		exit(-1);
	}

	matrix_size = atoi(argv[1]);
	
	if (matrix_size > sqrt_of_UINT32_MAX) {
		printf("ERROR: Matrix size must be between zero and %u!\n", sqrt_of_UINT32_MAX);
		exit(-1);
	}

    fd = open(DEV_NAME, O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "Could not open " DEV_NAME ": %s\n", strerror(errno));
        return -1;
    }

	squared_size = matrix_size * matrix_size;

	gettimeofday(&start_map, NULL);
    A = (double *)mmap_malloc(fd, sizeof(double) * squared_size);
    B = (double *)mmap_malloc(fd, sizeof(double) * squared_size);
    C = (double *)mmap_malloc(fd, sizeof(double) * squared_size);
	gettimeofday(&end_map, NULL);

	gettimeofday(&start_mult, NULL);
	for (row = 0; row < matrix_size; row++) {
		for (col = 0; col < matrix_size; col++) {
			for (index = 0; index < matrix_size; index++){
                C[row*matrix_size + col] += A[row*matrix_size + index] *B[index*matrix_size + col];
			}	
		}
	}
	gettimeofday(&end_mult, NULL);

	int map_time = (end_map.tv_sec * 1e6 + end_map.tv_usec) - (start_map.tv_sec * 1e6 + start_map.tv_usec);
	int mult_time = (end_mult.tv_sec * 1e6 + end_mult.tv_usec) - (start_mult.tv_sec * 1e6 + start_mult.tv_usec);

	snprintf(filename, sizeof(filename), "results/output%s.csv", argv[1]);
	fp = fopen(filename, "a");
	fprintf(fp, "%d, %d\n", map_time, mult_time);
	fclose(fp);

    return 0;
}
