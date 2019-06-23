//
// Created by Yilong Guo on 2019-06-20.
//

#include "Network.h"
#include "Utils.h"

#include <string.h>
#include <stdio.h>

long InputSpikes[INPUT_SIZE];
long OutputSpikes[OUTPUT_SIZE];
double Weights[INPUT_SIZE][OUTPUT_SIZE];

static int NetworkPhase;        // TRAIN / INFERENCE
static int LocalTime;
static int PatternTime;
static int BackgroundTime;
static int CurrentPhase;        // PATTERN / BACKGROUND phase
static int ImageIndex;
static double CurrentImage[INPUT_SIZE];
static double CurrentImageNorm;
static double CurrentImageComplementary[INPUT_SIZE];
static double CurrentImageComplementaryNorm;

static double LIFMembranes[OUTPUT_SIZE];
static double LIFThresholds[OUTPUT_SIZE];
static double LIFRes = 50.0 / 30.0 * 1.0 / 0.15 * 1.0 / 1000.0;
static int LIFFiredIndex;
// (50uS/max_g) * (1.0V/0.15V) * (1nA/1uA);
// max conductance ratio * read voltage ratio * current unit conversion

// tracker (using ring buffers)
static int FiredNeuronHistory[TRACKER_SIZE];
static int FiredTimeHistory[TRACKER_SIZE];
static int TrackerTail;     // tail pointer for ring buffers
static int TrackerSize;     // true size before the tracker is fully filled. TrackerSize <= TRACKER_SIZE

static int NetworkTime;


extern void StartTrain() {
    // load train data
    LoadMNIST();

    // network config
    NetworkTime = 0;
    NetworkPhase = TRAIN;

    // input layer
    ImageIndex = 0;
    FetchTrainImage(ImageIndex);
    InitLongArray(InputSpikes, INPUT_SIZE, 0);

    // output layer
    InitLongArray(OutputSpikes, OUTPUT_SIZE, 0);
    InitDoubleArray(LIFThresholds, OUTPUT_SIZE, LIF_THRESHOLD_REST);
    LIFFiredIndex = -1;

    // tracker
    TrackerTail = -1;
    TrackerSize = 0;
}

extern void GetTrainInstruction() {
    refresh:    // label: refresh to obtain instructions

    // fetch next image?
    if ((CurrentPhase == PATTERN && PatternTime >= PATTERN_TIME) || (CurrentPhase == BACKGROUND && BackgroundTime >= BACKGROUND_TIME)) {
        // track and adapt
        // track
        TrackerTail = (TrackerTail + 1) % TRACKER_SIZE;
        FiredNeuronHistory[TrackerTail] = LIFFiredIndex;
        FiredTimeHistory[TrackerTail] = LocalTime;
        if (TrackerSize < TRACKER_SIZE) {
            TrackerSize++;
        }

        // adapt
        int duration = PATTERN_TIME + BACKGROUND_TIME;
        int fire_count[OUTPUT_SIZE];
        InitIntArray(fire_count, OUTPUT_SIZE, 0);

        int i, index, total_time = 0, fired_neuron;
        for (i = 0; i < TrackerSize; ++i) {
            index = (TrackerTail + TRACKER_SIZE - i) % TRACKER_SIZE;
            total_time += FiredTimeHistory[index];
            fired_neuron = FiredNeuronHistory[index];

            if (fired_neuron >= 0) {
                fire_count[fired_neuron]++;
            }
        }

        for (i = 0; i < OUTPUT_SIZE; ++i) {
            LIFThresholds[i] += 0.1 * ((double)fire_count[i] / (double)total_time
                                       - LIF_ADAPT_FACTOR / (double)OUTPUT_SIZE / (double)duration);
        }

        // fetch next image
        if (FetchTrainImage(++ImageIndex)) {
            goto refresh;
        } else {
            // TODO: return empty instruction (end of train)
            printf("End of train.\n");
            return;
        }
    }

    // Input layer fires
    int fired = FireInputSpikes();
    // Output layer leaks
    LeakOutputMembranes();

    if (fired) {
        // TODO: return instruction (READ for PATTERN phase, RESET for BACKGROUND phase)
        printf("Input fires, READ or RESET.\n");
        printf("Phase: %d\n", CurrentPhase);

        // ++time
        if (CurrentPhase == PATTERN) {
            ++PatternTime;
        } else {
            ++BackgroundTime;
        }
        ++LocalTime;
        ++NetworkTime;

        return;
    } else {
        // ++time
        if (CurrentPhase == PATTERN) {
            ++PatternTime;
        } else {
            ++BackgroundTime;
        }
        ++LocalTime;
        ++NetworkTime;

        goto refresh;
    }
}

extern void GetTrainFeedbackInstruction(int* currents) {
    // current unit: nA

    // no operation during BACKGROUND phase
    if (CurrentPhase == BACKGROUND) {
        // TODO: return empty feedback instruction
        printf("No LIF activity during BACKGROUND phase.\n");
        return;
    }

    // integrate
    int i;
    for (i = 0; i < OUTPUT_SIZE; ++i) {
        LIFMembranes[i] += LIFRes * (double)currents[i] / (double)INPUT_SIZE;
    }

    // fire
    FireOutputSpikes();
    if (LIFFiredIndex == -1) {
        // TODO: return empty feedback instruction
        printf("No feedback instruction since no LIF fires.\n");
        return;
    } else {
        // TODO: return feedback instruction (SET)
        printf("LIF fires during PATTERN phase, SET instruction needed.\n");

        // switch phase to BACKGROUND
        CurrentPhase = BACKGROUND;

        return;
    }

}

static int FetchTrainImage(int index) {
    if (index >= TRAIN_SIZE) {
        return 0;
    }

    printf("TRAIN: %d\n", index);

    // reset input layer
    LocalTime = 0;
    PatternTime = 0;
    BackgroundTime = 0;
    CurrentPhase = PATTERN;

    // reset output layer membrane potential
    InitDoubleArray(LIFMembranes, OUTPUT_SIZE, 0.);

    // get image data
    memcpy(CurrentImage, TrainImages[index], sizeof(CurrentImage));
    CurrentImageNorm = GetImageNorm(CurrentImage, INPUT_SIZE);

    // get complementary image for background phase
    int i;
    for (i = 0; i < INPUT_SIZE; ++i) {
        CurrentImageComplementary[i] = 1 - CurrentImage[i];
    }

    CurrentImageComplementaryNorm = GetImageNorm(CurrentImageComplementary, INPUT_SIZE);

    return 1;
}

static int FireInputSpikes() {
    // clear input spikes
    InitLongArray(InputSpikes, INPUT_SIZE, 0);

    double* image;
    double norm;
    double rate;

    if (CurrentPhase == PATTERN) {
        image = CurrentImage;
        norm = CurrentImageNorm;
        rate = PATTERN_RATE;
    } else {
        image = CurrentImageComplementary;
        norm = CurrentImageComplementaryNorm;
        rate = BACKGROUND_RATE;
    }

    // generate input spikes
    int rst = 0, fired;
    int i;
    double x;
    for (i = 0; i < INPUT_SIZE; ++i) {
        x = RandDouble();

        fired = (x <= image[i] / norm * rate);
        InputSpikes[i] = fired;

        if (fired) {
            rst = 1;
        }
    }

    return rst;
}

static void LeakOutputMembranes() {
    int i;
    for (i = 0; i < OUTPUT_SIZE; ++i) {
        LIFMembranes[i] -= LIFMembranes[i] / (double)LIF_TAU;
    }
}

static void FireOutputSpikes() {
    // clear output spikes
    InitLongArray(OutputSpikes, OUTPUT_SIZE, 0);

    LIFFiredIndex = -1;

    int i;
    double max_membrane = 0.;
    int index = -1;
    for (i = 0; i < OUTPUT_SIZE; ++i) {
        if (LIFMembranes[i] > max_membrane) {
            max_membrane = LIFMembranes[i];
            index = i;
        }
    }

    if (index == -1) {
        fprintf(stderr, "Error during LIF firing.\n");
        return;
    }

    if (max_membrane >= LIFThresholds[index]) {
        LIFFiredIndex = index;
        OutputSpikes[LIFFiredIndex] = 1;

        // reset LIF (Winner Take All)
        InitDoubleArray(LIFMembranes, OUTPUT_SIZE, 0.);
    }
}
