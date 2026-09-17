# 下位机集成指南

本指南介绍如何在嵌入式固件中集成 `mwaveform`，把实时数据推送到 MStudio。

## 1. 软件依赖

- `mwaveform.c/h`
- `mwaveform_protocol.c/h`
- `segger_rtt/`

## 2. 初始化

```c
#include "mdebug/mwaveform.h"

static volatile int16_t s_raw_iu;
static volatile float   s_id;

void app_waveform_init(void)
{
    mwaveform.Init(NULL);

    uint8_t ch_iu = mwaveform.AddVariable(
        "Iu", 1.0f, (void *)&s_raw_iu, MWAVEFORM_VAR_RAW);
    uint8_t ch_id = mwaveform.AddVariable(
        "Id", 1000.0f, (void *)&s_id, MWAVEFORM_VAR_FLOAT);

    mwaveform.SetRate(0);
    mwaveform.SetStreamRate(50000, 10000);
    mwaveform.SetChannelRate(ch_id, 1000);
    mwaveform.Start();
}
```

如果变量是临时计算值，也可以使用：

```c
uint8_t ch_temp = mwaveform.AddChannel("Temp", 100.0f);
mwaveform.Push(ch_temp, temperature);
```

## 3. 高频 ISR

```c
void HighFrequencyISR(void)
{
    s_raw_iu = read_current_u();
    s_id     = controller_id_output();

    mwaveform.Step();
}
```

## 4. 主循环

`modus_Run()` 会自动调用 `mwaveform.Poll()`。如果没有使用 MODUS 主循环，需要手动调用：

```c
while (1) {
    mwaveform.Poll();
}
```

## 5. 调试器端口

OpenOCD 需要把 RTT 服务映射到：

- `9090`：RTT Channel 0，Shell
- `9091`：RTT Channel 1，Waveform

```text
rtt server start 9090 0
rtt server start 9091 1
```

## 6. 更多用法

stream、每变量刷新率、诊断和协议细节请参考：

- [MODUS 实时波形采集深度指南](../mdebug/mwaveform.md)
