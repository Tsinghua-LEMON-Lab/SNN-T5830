//
// Created by Yilong Guo on 2019-06-20.
//

#include "Network.h"
#include "Utils.h"

#include "ATLio.h"

#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

long InputSpikes[INPUT_SIZE];
long OutputSpikes[OUTPUT_SIZE];
double Weights[INPUT_SIZE][OUTPUT_SIZE];
char ResultDir[500];


static int NetworkTime;
static int NetworkPhase;        // TRAIN / TEST
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
static double LIFRes = 5.0 / 30.0 * 1.0 / 0.15 * 1.0 / 1000.0;
static int LIFFiredIndex;
// (5.0/max_g) * (1.0V/0.15V) * (1n/1u);
// max conductance ratio * read voltage ratio * current unit conversion

// tracker (using ring buffers)
static int FiredNeuronHistory[TRACKER_SIZE];
static int FiredTimeHistory[TRACKER_SIZE];
static int TrackerTail;     // tail pointer for ring buffers
static int TrackerSize;     // true size before the tracker is fully filled. TrackerSize <= TRACKER_SIZE

// inference responses
static int SetFlag;         // TRAIN / TEST image set
static int TrainSetFiredNeurons[TRAIN_SIZE];
static int TrainSetFiredTimings[TRAIN_SIZE];
static int TestSetFiredNeurons[TEST_SIZE];
static int TestSetFiredTimings[TEST_SIZE];

static double NeuronScores[OUTPUT_SIZE][CATEGORIES];
static int NeuronLabels[OUTPUT_SIZE];
static int TrainLabelsPredict[TRAIN_SIZE];
static int TestLabelsPredict[TEST_SIZE];
static int TrainHit;
static int TestHit;


extern void GetSetPulseConfig(void* v_bl, void* v_sl, void* v_wl, void* pulse_width) {
    long bl = V_SET_BL;
    long sl = V_SET_SL;
    long wl = V_SET_WL;
    long width = SET_WIDTH;

    if (!WriteLong(v_bl, bl)) {
        fprintf(stderr, "Error: GetSetPulseConfig() @param v_bl\n");
        return;
    }

    if (!WriteLong(v_sl, sl)) {
        fprintf(stderr, "Error: GetSetPulseConfig() @param v_sl\n");
        return;
    }

    if (!WriteLong(v_wl, wl)) {
        fprintf(stderr, "Error: GetSetPulseConfig() @param v_wl\n");
        return;
    }

    if (!WriteLong(pulse_width, width)) {
        fprintf(stderr, "Error: GetSetPulseConfig() @param pulse_width\n");
        return;
    }
}

extern void GetResetPulseConfig(void* v_bl, void* v_sl, void* v_wl, void* pulse_width) {
    long bl = V_RESET_BL;
    long sl = V_RESET_SL;
    long wl = V_RESET_WL;
    long width = RESET_WIDTH;

    if (!WriteLong(v_bl, bl)) {
        fprintf(stderr, "Error: GetResetPulseConfig() @param v_bl\n");
        return;
    }

    if (!WriteLong(v_sl, sl)) {
        fprintf(stderr, "Error: GetResetPulseConfig() @param v_sl\n");
        return;
    }

    if (!WriteLong(v_wl, wl)) {
        fprintf(stderr, "Error: GetResetPulseConfig() @param v_wl\n");
        return;
    }

    if (!WriteLong(pulse_width, width)) {
        fprintf(stderr, "Error: GetResetPulseConfig() @param pulse_width\n");
        return;
    }
}

extern void GetReadPulseConfig(void* v_bl, void* v_sl, void* v_wl, void* pulse_width) {
    long bl = V_READ_BL;
    long sl = V_READ_SL;
    long wl = V_READ_WL;
    long width = READ_WIDTH;

    if (!WriteLong(v_bl, bl)) {
        fprintf(stderr, "Error: GetReadPulseConfig() @param v_bl\n");
        return;
    }

    if (!WriteLong(v_sl, sl)) {
        fprintf(stderr, "Error: GetReadPulseConfig() @param v_sl\n");
        return;
    }

    if (!WriteLong(v_wl, wl)) {
        fprintf(stderr, "Error: GetReadPulseConfig() @param v_wl\n");
        return;
    }

    if (!WriteLong(pulse_width, width)) {
        fprintf(stderr, "Error: GetReadPulseConfig() @param pulse_width\n");
        return;
    }
}

extern void StartTrain() {
    // load train data
    LoadMNIST();

    strcpy(ResultDir, RESULTS_LOC);
    strcat(ResultDir, Now());    // without trailing '/'
    strcat(ResultDir, "/");
    mkdir(ResultDir, 0777);

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

extern void GetTrainInstruction(void* end_of_train, void* operation_type, void* bl_enable, void* sl_enable) {
    int fired;
    WriteLong(end_of_train, 0);

    refresh:    // label: refresh to obtain instructions

    // fetch next image?
    if ((CurrentPhase == PATTERN && PatternTime >= PATTERN_TIME) || (CurrentPhase == BACKGROUND && BackgroundTime >= BACKGROUND_TIME)) {
        // track and adapt
        TrackAndAdapt();

        // fetch next image
        if (FetchTrainImage(++ImageIndex)) {
            goto refresh;
        } else {
            // return empty instruction (end of train)
            WriteLong(operation_type, EMPTY);
            WriteLong(end_of_train, 1);
            return;
        }
    }

    // Input layer fires
    fired = FireInputSpikes();
    // Output layer leaks
    LeakOutputMembranes();

    // ++time
    if (CurrentPhase == PATTERN) {
        ++PatternTime;
    } else {
        ++BackgroundTime;
    }
    ++LocalTime;
    ++NetworkTime;

    if (fired) {
        // return instruction (READ for PATTERN phase, RESET for BACKGROUND phase)
        if (CurrentPhase == PATTERN) {
            WriteLong(operation_type, READ);
            WriteLongArray(sl_enable, InputSpikes, INPUT_SIZE);

        } else {
            WriteLong(operation_type, RESET);
            WriteLongArray(sl_enable, InputSpikes, INPUT_SIZE);
            WriteLong(bl_enable, (long)LIFFiredIndex);
        }
        return;
    } else {
        goto refresh;
    }
}

extern void GetTrainFeedbackInstruction(void* bl_currents, void* operation_type, void* bl_enable, void* sl_enable) {
    // current unit: nA
    long currents[8];
    ReadLongArray(currents, bl_currents, OUTPUT_SIZE);

    // no operation during BACKGROUND phase
    if (CurrentPhase == BACKGROUND) {
        // return empty feedback instruction
        WriteLong(operation_type, EMPTY);
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
        // return empty feedback instruction
        WriteLong(operation_type, EMPTY);
        return;
    } else {
        // return feedback instruction (SET)
        WriteLong(operation_type, SET);
        WriteLongArray(sl_enable, InputSpikes, INPUT_SIZE);
        WriteLong(bl_enable, LIFFiredIndex);

        // switch phase to BACKGROUND
        CurrentPhase = BACKGROUND;

        return;
    }
}

extern void StartTest() {
    // load train data
    LoadMNIST();

    // network config
    NetworkTime = 0;
    NetworkPhase = TEST;

    // input layer
    ImageIndex = 0;
    FetchTrainImage(ImageIndex);
    InitLongArray(InputSpikes, INPUT_SIZE, 0);

    // output layer
    InitLongArray(OutputSpikes, OUTPUT_SIZE, 0);
    // load thresholds
    LIFFiredIndex = -1;

    // responses
    SetFlag = TRAIN;
    InitIntArray(TrainSetFiredNeurons, TRAIN_SIZE, -1);
    InitIntArray(TrainSetFiredTimings, TRAIN_SIZE, -1);
    InitIntArray(TestSetFiredNeurons, TEST_SIZE, -1);
    InitIntArray(TestSetFiredTimings, TEST_SIZE, -1);
}

extern void GetTestInstruction(void* operation_type, void* sl_enable) {
    int fired;

    refresh:    // label: refresh to obtain instructions

    // Input layer fires
    fired = FireInputSpikes();
    // Output layer leaks
    LeakOutputMembranes();

    // ++time
    if (CurrentPhase == PATTERN) {
        ++PatternTime;
    } else {
        ++BackgroundTime;
    }
    ++LocalTime;
    ++NetworkTime;

    if (fired) {
        // return instruction (READ)
        WriteLong(operation_type, READ);
        WriteLongArray(sl_enable, InputSpikes, INPUT_SIZE);

        return;
    } else {
        goto refresh;
    }
}

extern void GetTestFeedbackInstruction(void* bl_currents, void* end_of_test) {
    // current unit: nA
    long currents[8];
    ReadLongArray(currents, bl_currents, OUTPUT_SIZE);

    WriteLong(end_of_test, 0);

    // integrate
    int i;
    for (i = 0; i < OUTPUT_SIZE; ++i) {
        LIFMembranes[i] += LIFRes * (double)currents[i] / (double)INPUT_SIZE;
    }

    // fire
    FireOutputSpikes();
    if (LIFFiredIndex == -1) {
        return;
    } else {
        // save responses
        if (SetFlag == TRAIN) {
            TrainSetFiredNeurons[ImageIndex] = LIFFiredIndex;
            TrainSetFiredTimings[ImageIndex] = LocalTime;

            if (!FetchTrainImage(++ImageIndex)) {
                // get test set responses
                SetFlag = TEST;
                ImageIndex = 0;
                FetchTestImage(ImageIndex);
            }
        } else {
            TestSetFiredNeurons[ImageIndex] = LIFFiredIndex;
            TestSetFiredTimings[ImageIndex] = LocalTime;

            if (!FetchTestImage(++ImageIndex)) {
                WriteLong(end_of_test, 1);
                return;
            }
        }
    }
}

extern void EvaluateScore() {
    int i, j;
    for (i = 0; i < OUTPUT_SIZE; ++i) {
        for (j = 0; j < CATEGORIES; ++j) {
            NeuronScores[i][j] = 0.;
        }
    }

    // evaluate train set
    for (i = 0; i < TRAIN_SIZE; ++i) {
        NeuronScores[TrainSetFiredNeurons[i]][TrainLabels[i]] += 1. / (double)TrainSetFiredTimings[i];
    }

    // evaluate neuron label
    for (i = 0; i < OUTPUT_SIZE; ++i) {
        double max_score = NeuronScores[i][0];
        int index = 0;
        for (j = 1; j < CATEGORIES; ++j) {
            if (NeuronScores[i][j] > max_score) {
                max_score = NeuronScores[i][j];
                index = j;
            }
        }

        NeuronLabels[i] = index;
    }

    // train set score
    TrainHit = 0;
    for (i = 0; i < TRAIN_SIZE; ++i) {
        TrainLabelsPredict[i] = NeuronLabels[TrainSetFiredNeurons[i]];

        TrainHit += TrainLabelsPredict[i] == TrainLabels[i];
    }

    // test set score
    TestHit = 0;
    for (i = 0; i < TEST_SIZE; ++i) {
        TestLabelsPredict[i] = NeuronLabels[TestSetFiredNeurons[i]];

        TestHit += TestLabelsPredict[i] == TestLabels[i];
    }

    // save scores
    SaveScores(ResultDir);
}

extern void SaveArray(void* bl0_currents, void* bl1_currents, void* bl2_currents, void* bl3_currents, void* bl4_currents, void* bl5_currents, void* bl6_currents, void* bl7_currents) {
    // current unit: nA
    long bl0[INPUT_SIZE], bl1[INPUT_SIZE], bl2[INPUT_SIZE], bl3[INPUT_SIZE], bl4[INPUT_SIZE], bl5[INPUT_SIZE], bl6[INPUT_SIZE], bl7[INPUT_SIZE];
    ReadLongArray(bl0, bl0_currents, INPUT_SIZE);
    ReadLongArray(bl1, bl1_currents, INPUT_SIZE);
    ReadLongArray(bl2, bl2_currents, INPUT_SIZE);
    ReadLongArray(bl3, bl3_currents, INPUT_SIZE);
    ReadLongArray(bl4, bl4_currents, INPUT_SIZE);
    ReadLongArray(bl5, bl5_currents, INPUT_SIZE);
    ReadLongArray(bl6, bl6_currents, INPUT_SIZE);
    ReadLongArray(bl7, bl7_currents, INPUT_SIZE);

    int i;
    for (i = 0; i < INPUT_SIZE; ++i) {
        Weights[i][0] = (double)bl0[i] / (double)V_READ_SL;
        Weights[i][1] = (double)bl1[i] / (double)V_READ_SL;
        Weights[i][2] = (double)bl2[i] / (double)V_READ_SL;
        Weights[i][3] = (double)bl3[i] / (double)V_READ_SL;
        Weights[i][4] = (double)bl4[i] / (double)V_READ_SL;
        Weights[i][5] = (double)bl5[i] / (double)V_READ_SL;
        Weights[i][6] = (double)bl6[i] / (double)V_READ_SL;
        Weights[i][7] = (double)bl7[i] / (double)V_READ_SL;
    }
}

extern void Save() {
    char base[500];
    strcpy(base, ResultDir);

    if (NetworkPhase == TRAIN) {
        strcat(base, "train/");
        mkdir(base, 0777);
    } else {
        strcat(base, "test/");
        mkdir(base, 0777);
    }

    SaveWeights(base);
    SaveThresholds(base);
    SaveConfig(base);
    SaveLabels(base);

    if (NetworkPhase == TEST) {
        SaveResponses(ResultDir);
    }
}

extern void SaveThresholds(const char* path) {
    FILE* fp;
    if ((fp = fopen(GetFilename(path, "thresholds.txt"), "a")) == NULL) {
        fprintf(stderr, "Error saving thresholds.txt\n");
        return;
    }

    int i;
    for (i = 0; i < OUTPUT_SIZE; ++i) {
        fprintf(fp, "%lf ", LIFThresholds[i]);
    }
    fprintf(fp, "\n");

    fclose(fp);
}

extern void LoadThresholds(const char* path) {
    FILE* fp;
    if ((fp = fopen(GetFilename(path, "thresholds.txt"), "r")) == NULL) {
        fprintf(stderr, "Error loading thresholds.txt\n");
        return;
    }

    int i;
    for (i = 0; i < OUTPUT_SIZE; ++i) {
        fscanf(fp, "%lf", &LIFThresholds[i]);
    }

    fclose(fp);
}

extern void SaveWeights(const char* path) {
    FILE* fp;
    if ((fp = fopen(GetFilename(path, "weights.txt"), "a")) == NULL) {
        fprintf(stderr, "Error saving weights.txt\n");
        return;
    }

    int i, j;
    for (i = 0; i < INPUT_SIZE; ++i) {
        for (j = 0; j < OUTPUT_SIZE; ++j) {
            fprintf(fp, "%lf ", Weights[i][j]);
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "\n");

    fclose(fp);
}

extern void LoadWeights(const char* path) {
    FILE* fp;
    if ((fp = fopen(GetFilename(path, "weights.txt"), "r")) == NULL) {
        fprintf(stderr, "Error loading weights.txt\n");
        return;
    }

    int i, j;
    for (i = 0; i < INPUT_SIZE; ++i) {
        for (j = 0; j < OUTPUT_SIZE; ++j) {
            fscanf(fp, "%lf", &Weights[i][j]);
        }
    }

    fclose(fp);
}

extern void SaveConfig(const char* path) {
    FILE* fp;
    if ((fp = fopen(GetFilename(path, "config.txt"), "w")) == NULL) {
        fprintf(stderr, "Error saving config.txt\n");
        return;
    }

    fprintf(fp, "Path: %s\n", ResultDir);
    fprintf(fp, "\n");

    fprintf(fp, "SET: %d %d %d %dns\n", V_SET_BL, V_SET_SL, V_SET_WL, SET_WIDTH);
    fprintf(fp, "RESET: %d %d %d %dns\n", V_RESET_BL, V_RESET_SL, V_RESET_WL, RESET_WIDTH);
    fprintf(fp, "READ: %d %d %d %dns\n", V_READ_BL, V_READ_SL, V_READ_WL, READ_WIDTH);
    fprintf(fp, "\n");

    fprintf(fp, "Pattern time: %d\n", PATTERN_TIME);
    fprintf(fp, "Background time: %d\n", BACKGROUND_TIME);
    fprintf(fp, "Pattern rate: %f\n", PATTERN_RATE);
    fprintf(fp, "Background rate: %f\n", BACKGROUND_RATE);
    fprintf(fp, "Tracker size: %d\n", TRACKER_SIZE);
    fprintf(fp, "LIF resting threshold: %f\n", LIF_THRESHOLD_REST);
    fprintf(fp, "LIF tau: %d\n", LIF_TAU);
    fprintf(fp, "LIF adapt factor: %f\n", LIF_ADAPT_FACTOR);
    fprintf(fp, "LIF res factor: %f\n", LIFRes);
    fprintf(fp, "\n");

    fprintf(fp, "Network phase: %s\n", NetworkPhase ? "TEST" : "TRAIN");
    fprintf(fp, "Network timestep used: %d\n", NetworkTime);
    fprintf(fp, "Samples learned: %d\n", ImageIndex);

    fclose(fp);
}

extern void SaveLabels(const char* path) {
    FILE* fp;
    if ((fp = fopen(GetFilename(path, "train-labels.txt"), "w")) == NULL) {
        fprintf(stderr, "Error saving train-labels.txt\n");
        return;
    }

    int i;
    for (i = 0; i < TRAIN_SIZE; ++i) {
        fprintf(fp, "%d\n", TrainLabels[i]);
    }
    fclose(fp);

    if ((fp = fopen(GetFilename(path, "test-labels.txt"), "w")) == NULL) {
        fprintf(stderr, "Error saving test-labels.txt\n");
        return;
    }
    for (i = 0; i < TEST_SIZE; ++i) {
        fprintf(fp, "%d\n", TestLabels[i]);
    }
    fclose(fp);
}

extern void SaveScores(const char* path) {
    FILE* fp;
    if ((fp = fopen(GetFilename(path, "scores.txt"), "w")) == NULL) {
        fprintf(stderr, "Error saving scores.txt\n");
        return;
    }

    int i, j;
    fprintf(fp, "Neuron scores: \n");
    for (i = 0; i < OUTPUT_SIZE; ++i) {
        for (j = 0; j < CATEGORIES; ++j) {
            fprintf(fp, "%lf ", NeuronScores[i][j]);
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "\n");

    fprintf(fp, "Neuron labels: \n");
    for (i = 0; i < OUTPUT_SIZE; ++i) {
        fprintf(fp, "%d ", NeuronLabels[i]);
    }
    fprintf(fp, "\n");

    fprintf(fp, "Train predict labels: \n");
    for (i = 0; i < TRAIN_SIZE; ++i) {
        fprintf(fp, "%d ", TrainLabelsPredict[i]);
    }
    fprintf(fp, "\n");

    fprintf(fp, "Test predict labels: \n");
    for (i = 0; i < TEST_SIZE; ++i) {
        fprintf(fp, "%d ", TestLabelsPredict[i]);
    }
    fprintf(fp, "\n");

    fprintf(fp, "Accuracy: \n");
    fprintf(fp, "Train: %d/%d = %lf\n", TrainHit, TRAIN_SIZE, (double)TrainHit / (double)TRAIN_SIZE);
    fprintf(fp, "Test: %d/%d = %lf\n", TestHit, TEST_SIZE, (double)TestHit / (double)TEST_SIZE);
}

extern void SaveResponses(const char* path) {
    FILE* fp;
    if ((fp = fopen(GetFilename(path, "train-responses.txt"), "w")) == NULL) {
        fprintf(stderr, "Error saving train-responses.txt\n");
        return;
    }

    int i;
    for (i = 0; i < TRAIN_SIZE; ++i) {
        fprintf(fp, "%d %d\n", TrainSetFiredNeurons[i], TrainSetFiredTimings[i]);
    }
    fclose(fp);

    if ((fp = fopen(GetFilename(path, "test-responses.txt"), "w")) == NULL) {
        fprintf(stderr, "Error saving test-responses.txt\n");
        return;
    }

    for (i = 0; i < TEST_SIZE; ++i) {
        fprintf(fp, "%d %d\n", TestSetFiredNeurons[i], TestSetFiredTimings[i]);
    }
    fclose(fp);
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

static int FetchTestImage(int index) {
    if (index >= TEST_SIZE) {
        return 0;
    }

    printf("TEST: %d\n", index);

    // reset input layer
    LocalTime = 0;
    PatternTime = 0;
    BackgroundTime = 0;
    CurrentPhase = PATTERN;

    // reset output layer membrane potential
    InitDoubleArray(LIFMembranes, OUTPUT_SIZE, 0.);

    // get image data
    memcpy(CurrentImage, TestImages[index], sizeof(CurrentImage));
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

static void TrackAndAdapt() {
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
}