//
// Created by Yilong Guo on 2019-06-20.
//

#include "Network.h"
#include "Utils.h"

long InputSpikes[INPUT_SIZE];
long OutputSpikes[OUTPUT_SIZE];
double Weights[INPUT_SIZE][OUTPUT_SIZE];


static double LIFMembranes[OUTPUT_SIZE];
static double LIFThresholds[OUTPUT_SIZE];

static int FiredNeuronHistory[TRACKER_SIZE];
static int FiredTimeHistory[TRACKER_SIZE];
static int TrackerHead;
static int TrackerSize;     // true size before the tracker is fully filled. TrackerSize <= TRACKER_SIZE

static int NetworkTime;


void InitNetwork() {

    // load data
    LoadMNIST();

    // network config
    NetworkTime = 0;

    // input layer
    InitLongArray(InputSpikes, INPUT_SIZE, 0);

    // output layer
    InitLongArray(OutputSpikes, OUTPUT_SIZE, 0);
    InitDoubleArray(LIFMembranes, OUTPUT_SIZE, 0.);
    InitDoubleArray(LIFThresholds, OUTPUT_SIZE, LIF_THRESHOLD_REST);

    // tracker
    TrackerHead = 0;
    TrackerSize = 0;

}