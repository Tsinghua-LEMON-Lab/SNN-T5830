//
// Created by Yilong Guo on 2019-06-21.
//

#ifndef SNN_T5830_UTILS_H
#define SNN_T5830_UTILS_H


extern void InitLongArray(long* array, int size, long value);
extern void InitIntArray(int* array, int size, int value);
extern void InitDoubleArray(double* array, int size, double value);
extern double GetImageNorm(const double *image, int size);
extern double RandDouble();
extern char* Now();
extern const char* GetFilename(const char* base, const char* filename);


#endif //SNN_T5830_UTILS_H
