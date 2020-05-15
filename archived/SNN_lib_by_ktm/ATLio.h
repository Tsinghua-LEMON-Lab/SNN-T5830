//
// Created by Yilong Guo on 2019-06-25.
//

#ifndef SNN_T5830_ATLIO_H
#define SNN_T5830_ATLIO_H

#define TEST_ON_T5830 1


extern int WriteLong(void* dest, long src);
extern int WriteLongArray(void* dest, long* src, int size);
extern int ReadLongArray(long* dest, void* src, int size);

#endif //SNN_T5830_ATLIO_H
