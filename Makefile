CC=gcc
CFLAGS=-I. -O3 -lX11 -lGL -lGLU -Wall -Wextra

klimTris: main.c shapes.c
	$(CC) -o $@ $^ $(CFLAGS)
