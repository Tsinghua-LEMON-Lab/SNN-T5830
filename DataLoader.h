//
// Created by Yilong Guo on 2019-06-19.
//

#ifndef SNN_T5830_DATALOADER_H
#define SNN_T5830_DATALOADER_H

// MNIST data file location
#define TRAIN_DATA_LOC "/Users/Nuullll/Projects/SNN-T5830/mnist/121-size-012-cat/train.txt"
#define TEST_DATA_LOC "/Users/Nuullll/Projects/SNN-T5830/mnist/121-size-012-cat/test.txt"

// MNIST subset size (category {0,1,2} only)
#define TRAIN_SIZE 18623
#define TEST_SIZE 3147
#define INPUT_SIZE 121
#define OUTPUT_SIZE 8
#define CATEGORIES 3


extern double TrainImages[TRAIN_SIZE][INPUT_SIZE];
extern int TrainLabels[TRAIN_SIZE];
extern double TestImages[TEST_SIZE][INPUT_SIZE];
extern int TestLabels[TEST_SIZE];


extern void LoadMNIST();


#endif //SNN_T5830_DATALOADER_H
