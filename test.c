//
// Created by Yilong Guo on 2019-06-19.
//

#include "test.h"
#include "DataLoader.h"
#include "Network.h"

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


int test_InitNetwork() {
    InitNetwork();

    printf("Output spikes: ");

    int i;
    for (i = 0; i < OUTPUT_SIZE; ++i) {
        printf("%ld ", OutputSpikes[i]);
    }

    printf("\n");

    return 1;
}


int main() {
    test_InitNetwork();

    return 0;
}