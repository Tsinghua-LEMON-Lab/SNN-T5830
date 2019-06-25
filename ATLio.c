//
// Created by Yilong Guo on 2019-06-25.
//

#include "ATLio.h"

#if TEST_ON_T5830

#include "UTATL.h"

extern int WriteLong(void* dest, long src) {
    return UTATL_CLINK_OK == UTATL_ClinkWriteVar(dest, (void*)&src, UTATL_VarTypeInte1);
}

extern int WriteLongArray(void* dest, long* src, int size) {
    return UTATL_CLINK_OK == UTATL_ClinkWriteDim(dest, (void*)src, UTATL_VarTypeInte1, size);
}

extern int ReadLongArray(long* dest, void* src, int size) {
    return UTATL_CLINK_OK == UTATL_ClinkReadDim(src, (void*)dest, UTATL_VarTypeInte1, NULL, size);
}

#else

#include <string.h>

extern int WriteLong(void* dest, long src) {
    *(long*)dest = src;

    return 1;
}

extern int WriteLongArray(void* dest, long* src, int size) {
    memcpy((long*)dest, src, size);

    return 1;
}

extern int ReadLongArray(long* dest, void* src, int size) {
    memcpy(dest, (long*)src, size);

    return 1;
}

#endif