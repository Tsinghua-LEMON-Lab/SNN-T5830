//
// Created by Yilong Guo on 2019-06-19.
//

#include "test.h"
#include "DataLoader.h"
#include "Network.h"
#include "Utils.h"

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


int test_train() {
    StartTrain();
    int currents[8];

    int i, j;

    for (i = 0; i < 10000; ++i) {
        GetTrainInstruction();

        for (j = 0; j < 8; ++j) {
            currents[j] = (int)(RandDouble() * 4000);
        }
        GetTrainFeedbackInstruction(currents);
    }

    return 1;
}


int main() {
    test_train();

    return 0;
}