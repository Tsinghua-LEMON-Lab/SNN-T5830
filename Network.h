//
// Created by Yilong Guo on 2019-06-20.
//

#ifndef SNN_T5830_NETWORK_H
#define SNN_T5830_NETWORK_H

#include "DataLoader.h"

// results folder
#define RESULTS_LOC "/Users/Nuullll/Projects/SNN-T5830/results/"

// pulse config (voltage unit: mV, pulse width unit: ns)
#define V_SET_BL 2000
#define V_SET_SL 0
#define V_SET_WL 2000
#define SET_WIDTH 50
#define V_RESET_BL 0
#define V_RESET_SL 2000
#define V_RESET_WL 3500
#define RESET_WIDTH 50
#define V_READ_BL 0
#define V_READ_SL 150
#define V_READ_WL 4500
#define READ_WIDTH 500

// Pattern/background phase
#define PATTERN_TIME 200
#define BACKGROUND_TIME 3
#define PATTERN_RATE 1.0
#define BACKGROUND_RATE 7.0

// Tracker for LIF threshold adaptation
#define TRACKER_SIZE 1000

#define LIF_THRESHOLD_REST 0.4
#define LIF_TAU 200
#define LIF_ADAPT_FACTOR 1.1

enum {PATTERN, BACKGROUND};
enum {TRAIN, TEST};


extern long InputSpikes[INPUT_SIZE];
extern long OutputSpikes[OUTPUT_SIZE];
extern double Weights[INPUT_SIZE][OUTPUT_SIZE];
extern char ResultDir[500];


extern void StartTrain();
extern void GetTrainInstruction(void* end_of_train);
extern void GetTrainFeedbackInstruction(int* currents);

extern void StartTest();
extern void GetTestInstruction();
extern void GetTestFeedbackInstruction(int* currents, void* end_of_test);

extern void EvaluateScore();

// Save/load model
extern void Save();
extern void SaveThresholds(const char* path);
extern void LoadThresholds(const char* path);
extern void SaveWeights(const char* path);
extern void LoadWeights(const char* path);
extern void SaveConfig(const char* path);
extern void SaveLabels(const char* path);
extern void SaveResponses(const char* path);

// private functions
static int FetchTrainImage(int index);
static int FireInputSpikes();
static void LeakOutputMembranes();
static void FireOutputSpikes();
static void TrackAndAdapt();
static int FetchTestImage(int index);


#endif //SNN_T5830_NETWORK_H
