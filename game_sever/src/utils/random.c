#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define RANDOM_ARAM 10000
float
random_float() {
	float a = (float)(rand() % RANDOM_ARAM); // [0, 9999]
	return a / (RANDOM_ARAM); // [0, 1)
}

int
random_int(int min, int max) {
	int area = rand() % (max - min);
	return min + area;
}