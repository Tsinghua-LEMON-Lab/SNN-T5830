//
// Created by Yilong Guo on 2019-06-21.
//

#include "Utils.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>


extern void InitLongArray(long* array, int size, long value) {
    int i;
    for (i = 0; i < size; ++i) {
        array[i] = value;
    }
}

extern void InitIntArray(int* array, int size, int value) {
    int i;
    for (i = 0; i < size; ++i) {
        array[i] = value;
    }
}

extern void InitDoubleArray(double* array, int size, double value) {
    int i;
    for (i = 0; i < size; ++i) {
        array[i] = value;
    }
}

extern double GetImageNorm(const double *image, int size) {
    double norm = 0;

    int i;
    for (i = 0; i < size; ++i) {
        norm += image[i];
    }

    return norm;
}

extern double RandDouble() {
    return (double)(rand() % 1000) / 1000.0;
}

extern char* Now() {
    time_t now;
    struct tm* time_info;
    char* buffer = (char*)malloc(sizeof(char) * 20);

    time(&now);
    time_info = localtime(&now);

    strftime(buffer, 20, "%Y-%m-%d-%H-%M-%S", time_info);
    return buffer;
}

extern const char* GetFilename(const char* base, const char* filename) {
    char* full = (char*)malloc(sizeof(char) * 500);

    strcpy(full, base);
    strcat(full, filename);

    return full;
}
