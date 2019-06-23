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
    int end_of_train = 0;

    StartTrain();
    int currents[OUTPUT_SIZE];

    int i;
    while (!end_of_train) {
        GetTrainInstruction(&end_of_train);

        for (i = 0; i < OUTPUT_SIZE; ++i) {
            currents[i] = (int)(RandDouble() * 4000);
        }
        GetTrainFeedbackInstruction(currents);
    }

    Save();

    return 1;
}

int test_test() {
    int end_of_test = 0;

    StartTest();
    int currents[OUTPUT_SIZE];

    int i;
    while (!end_of_test) {
        GetTestInstruction();

        for (i = 0; i < OUTPUT_SIZE; ++i) {
            currents[i] = (int)(RandDouble() * 4000);
        }
        GetTestFeedbackInstruction(currents, &end_of_test);
    }

    printf("%s", Now());
    Save();

    return 1;
}

int main() {
    test_train();

    return 0;
}