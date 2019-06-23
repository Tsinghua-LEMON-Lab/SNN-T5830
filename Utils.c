//
// Created by Yilong Guo on 2019-06-21.
//

#include <stdlib.h>
#include "Utils.h"


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