//
// Created by Yilong Guo on 2019-06-19.
//

#include "test.h"
#include "DataLoader.h"

#include <stdio.h>


int test_LoadMNIST() {
    LoadMNIST();

    printf("Data: ");

    int i;
    int target = TRAIN_SIZE - 1;
    for (i = 0; i < INPUT_SIZE; ++i) {
        printf("%lf ", TrainImages[target][i]);
    }

    printf("\nLabel: %d\n", TrainLabels[target]);

    return 1;
}


int main() {
    test_LoadMNIST();

    return 0;
}