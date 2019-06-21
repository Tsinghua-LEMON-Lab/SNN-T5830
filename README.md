# SNN-T5830

A shared library providing with greedy training algorithms for SNNs.

## Compile Options

- C90 standard
- Shared library

## Target Platform

- AdvanTest T5830 platform (linux)

## APIs

### Tested

- [ ] `void LoadMNIST()`
    - Load MNIST image data into arrays from hard-coded data file path.
- [ ] `void InitNetwork()`
    - Initialize the SNN, prepare input/output neurons and internal states

### TODOs

- `void NextSpike()`
    - Get the next spike of input neurons, for SL ports of the array.
- `void Feedback(void* current)`
    - Feed current read from array back to the SNN software.

## Change Log

- v0.1.0
    - New feature: `void InitNetwork()`.
    - Add utility functions.

- v0.0.1
    - Build program skeleton.
    - New feature: `void LoadMNIST()`, loading MNIST image data into arrays from hard-coded data file path.
