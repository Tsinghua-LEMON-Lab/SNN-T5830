# SNN-T5830

A shared library providing with greedy training algorithms for SNNs.

## Compile Options

- C90 standard
- Shared library

## Target Platform

- AdvanTest T5830 platform (linux)

## Guidance

- [ATL与C交互](ATLxC-guidance/atl_en_clink_1.html)
- [在测试平台编译C shared library for ATL](ATLxC-guidance/atl_en_clink_1.5.html)
- [样例程序](ATLxC-guidance/atl_en_clink_1.6.html)

## Plaform-dependent MACROs

1. `#define TEST_ON_T5830 1` ([ATLio.h @line 8](ATLio.h))
2. `#define TRAIN_DATA_LOC "path/to/train/data.txt"` ([DataLoader.h @line 9](DataLoader.h))
3. `#define TEST_DATA_LOC "path/to/test/data.txt"` ([DataLoader.h @line 10](DataLoader.h))
4. `#define RESULTS_LOC "path/to/results/dir/` 需以`'/'`结尾 ([Network.h @line 11](Network.h))

## APIs

- [ ] `void GetSetPulseConfig(void* v_bl, void* v_sl, void* v_wl, void* pulse_width)`
    - @desc Return SET pulse config. Voltage unit: mV, width unit: ns.
    - @param `v_bl` <@ATL_type `INTEGER[1]`>: BL voltage.
    - @param `v_sl` <@ATL_type `INTEGER[1]`>: SL voltage.
    - @param `v_wl` <@ATL_type `INTEGER[1]`>: WL voltage.
    - @param `pulse_width` <@ATL_type `INTEGER[1]`>: pulse width.

- [ ] `void GetResetPulseConfig(void* v_bl, void* v_sl, void* v_wl, void* pulse_width)`
    - @desc Return RESET pulse config. Voltage unit: mV, width unit: ns.
    - @param `v_bl` <@ATL_type `INTEGER[1]`>: BL voltage.
    - @param `v_sl` <@ATL_type `INTEGER[1]`>: SL voltage.
    - @param `v_wl` <@ATL_type `INTEGER[1]`>: WL voltage.
    - @param `pulse_width` <@ATL_type `INTEGER[1]`>: pulse width.

- [ ] `void GetReadPulseConfig(void* v_bl, void* v_sl, void* v_wl, void* pulse_width)`
    - @desc Return READ pulse config. Voltage unit: mV, width unit: ns.
    - @param `v_bl` <@ATL_type `INTEGER[1]`>: BL voltage.
    - @param `v_sl` <@ATL_type `INTEGER[1]`>: SL voltage.
    - @param `v_wl` <@ATL_type `INTEGER[1]`>: WL voltage.
    - @param `pulse_width` <@ATL_type `INTEGER[1]`>: pulse width.

- [ ] `void StartTrain()`
    - @desc 初始化, 准备开始训练.
    
- [ ] `void GetTrainInstruction(void* end_of_train, void* operation_type, void* bl_enable, void* sl_enable)`
    - @desc 获取前向操作指令, 操作类型可能为`READ`, `RESET`或`EMPTY`.
    - @param `end_of_train` <@ATL_type `INTEGER[1]`>: 训练结束则`end_of_train=1`, 否则为`0`.
    - @param `operation_type` <@ATL_type `INTEGER[1]`>: 操作类型.
        - `SET = 0`: 此函数不会返回`SET`操作.
        - `RESET = 1`: 选中**若干行SL**以及**一行BL**进行`RESET`操作. 
            - 选中的`SL`信息返回至`sl_enable[128]`数组, 选中的置为`1`, 未选中的置为`0`.
            - 选中的`BL`返回至`bl_enable`整数型变量, 范围0~7.
        - `READ = 2`: 选中选中**若干行SL**进行`READ`操作 (`SL`加0.15V读电压, 从`BL`端口读电流), `BL`全部打开, 得到8个`BL`的累加电流 (nA), 暂存.
            - 选中的`SL`信息返回至`sl_enable[128]`数组, 选中的置为`1`, 未选中的置为`0`.
            - `BL`硬编码为全部打开, `bl_enable`不返回有意义的值.
        - `EMPTY = 3`: 空操作, 当且仅当训练结束时会返回空操作.
    - @param `bl_enable` <@ATL_type `INTEGER[1]`>: 当且仅当`operation_type = RESET`时, 此变量返回需要进行`RESET`的`BL`索引, 范围0~7.
    - @param `sl_enable` <@ATL_type `INTEGER[1] (128)`>: 长度为128的数组, 值非`0`即`1`, 表示对应`SL`是否打开.

- [ ] `void GetTrainFeedbackInstruction(void* bl_currents, void* operation_type, void* bl_enable, void* sl_enable)`
    - @desc 将前向指令`READ`操作读取到的各`BL`电流反馈给网络, 并获取反向操作指令, 操作类型可能为`SET`或`EMPTY`.
    - @param `bl_currents` <@ATL_type `INTEGER[1] (8)`>: 长度为8的数组, 将前向`READ`操作读取到的电流值存储在此变量, 供网络模型读取. 如果前向操作指令为`RESET`, 则不关心此变量的具体值.
    - @param `operation_type` <@ATL_type `INTEGER[1]`>: 操作类型.
        - `SET = 0`: 选中**若干行SL**以及**一行BL**进行`SET`操作. 
            - 选中的`SL`信息返回至`sl_enable[128]`数组, 选中的置为`1`, 未选中的置为`0`.
            - 选中的`BL`返回至`bl_enable`整数型变量, 范围0~7.
        - `RESET = 1`: 此函数不会返回`RESET`操作.
        - `READ = 2`: 此函数不会返回`READ`操作.
        - `EMPTY = 3`: 空操作, 此函数返回空操作**不**代表训练结束.

- [ ] `void StartTest()`
    - @desc 训练结束后调用, 准备开始inference.

- [ ] `void GetTestInstruction(void* operation_type, void* sl_enable)`
    - @desc 获取前向操作指令, 操作类型只可能为`READ`.
    - @param `sl_enable` <@ATL_type `INTEGER[1] (128)`>: 长度为128的数组, 值非`0`即`1`, 表示对应`SL`是否打开.

- [ ] `void GetTestFeedbackInstruction(void* bl_currents, void* end_of_test)`
    - @desc 将前向读取的电流反馈给网络, 并判断inference是否结束.
    - @param `bl_currents` <@ATL_type `INTEGER[1] (8)`>: 长度为8的数组, 将前向`READ`操作读取到的电流值存储在此变量, 供网络模型读取.
    - @param `end_of_test` <@ATL_type `INTEGER[1]`>: 推理结束则`end_of_test=1`, 否则为`0`.

- [ ] `void SaveArray(void* bl0_currents, void* bl1_currents, void* bl2_currents, void* bl3_currents, void* bl4_currents, void* bl5_currents, void* bl6_currents, void* bl7_currents)`
    - @desc 保存当前阵列各cell状态.
    - @param `blx_currents` <@ATL_type `INTEGER[1] (128)`>: 长度为128的数组, 第x行BL的各cell电流值(nA).

- [ ] `void Save()`
    - @desc 保存模型到文件.

- [ ] `void EvaluateScore()`
    - @desc 推理结束后调用, 计算准确率并保存至文件.

## Workflow

1. 标准FORMING, 未提供接口: 300nA - 4000nA

2. 获取脉冲参数配置: `LoadSetPulseConfig()`; `LoadResetPulseConfig()`; `LoadReadPulseConfig()`

3. 开始训练: `StartTrain()`

4. `while True:`

    1. `GetTrainInstruction()`
    1. 硬件操作阵列
    2. `if end_of_train: break`
    3. `GetTrainFeedbackInstruction()`
    1. 硬件操作阵列

5. 保存阵列状态: `SaveArray()`; 保存网络模型: `Save()`

6. 开始推理: `StartTest()`

7. `while True:`

    1. `GetTestInstruction()`
    1. 硬件操作阵列
    2. `GetTestFeedbackInstruction()`
    1. 硬件操作阵列
    3. `if end_of_test: break`

8. 保存阵列状态: `SaveArray()`; 保存网络模型: `Save()`

9. 计算准确率: `EvaluateScore()`

## Change Log

- v0.2.0
    - Refactor APIs.
    - Adapt to ATL interfaces.

- v0.1.0
    - New feature: `void InitNetwork()`.
    - Add utility functions.

- v0.0.1
    - Build program skeleton.
    - New feature: `void LoadMNIST()`, loading MNIST image data into arrays from hard-coded data file path.
