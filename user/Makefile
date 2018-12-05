CC = arm-linux-gnueabihf-gcc
INCLUDE_DIR=$(PWD)/../include
CFLAGS =-I$(INCLUDE_DIR) -Wall

all: dense_mm quicksort

clean:
	rm dense_mm quicksort

dense_mm: dense_mm.c
	$(CC) $(CFLAGS) dense_mm.c -o dense_mm

quicksort: quicksort.c
	$(CC) $(CFLAGS) quicksort.c -o quicksort
