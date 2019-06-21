//
// Created by Yilong Guo on 2019-06-21.
//

#include "Utils.h"


void InitLongArray(long* array, int size, long value) {
    int i;
    for (i = 0; i < size; ++i) {
        array[i] = value;
    }
}

void InitIntArray(int* array, int size, int value) {
    int i;
    for (i = 0; i < size; ++i) {
        array[i] = value;
    }
}

void InitDoubleArray(double* array, int size, double value) {
    int i;
    for (i = 0; i < size; ++i) {
        array[i] = value;
    }
}