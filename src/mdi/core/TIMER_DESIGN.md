# MDI Timer Core 原语设计

## 目标

为需要稳定硬件节拍的上层 driver 提供芯片无关的静态 timer contract。调用方只看到定时器资源 token 和语义化的 `MDI_TIMER_*` 宏，不直接使用 `MDI_OP` 生成的 provider 符号，也不依赖 `void *`、函数指针或运行时对象。

## 分层

`core/timer.h` 只定义跨芯片成立的四个控制操作：

- `MDI_TIMER_SetFrequency(R, wHz)`：设置运行频率，单位 Hz。定时器运行时修改返回 `MDI_BUSY`；零频率或硬件不可表示返回 `MDI_RANGE`。
- `MDI_TIMER_Start(R)`：启动已经配置好的计数器和其约定的更新事件输出。启动不会偷偷完成时钟、GPIO、NVIC 或复位初始化。
- `MDI_TIMER_Stop(R)`：停止计数器并关闭该资源约定的更新事件输出；调用后资源可以再次 `SetFrequency` 和 `Start`。
- `MDI_TIMER_IsRunning(R)`：读取当前运行状态。

芯片后端在 `peripheral/<chip>/mdi/backend.h` 提供绑定宏，生成资源对应的 `_timer_*` provider 操作。后端负责分频计算、寄存器写入、状态位和硬件错误映射；`instance.h` 选择具体 timer、时钟源和 IRQ 资源。

## 中断边界

Timer core 不提供回调注册、上下文指针或运行时分发。板级 IRQ vector 负责确认芯片中断标志，然后直接调用具体 driver 的 `IsrStep()`。这样 DAC 高频路径保持：

```text
TMR IRQ -> clear update flag -> dac_driver_IsrStep()
```

Timer provider 的 `Start` 只负责启动计数和打开已经绑定的更新中断；它不保存 ISR 回调。若某个应用只轮询计时器，后端可以让同一 `Start/Stop/IsRunning` contract 不启用 IRQ，具体差异写入该后端绑定说明。

## 状态和并发

- `SetFrequency`、`Start`、`Stop` 在初始化线程或明确的控制上下文调用，不在高频 ISR 中重新配置定时器。
- `IsRunning` 可在状态查询路径读取；后端必须保证读取不产生副作用。
- `SetFrequency` 失败时不得修改已有周期配置。
- `Stop` 后必须清除或按后端定义处理待决更新标志，避免重新启动时立即产生陈旧事件。
- 频率是资源的计数节拍，不表示 PWM 输出频率；PWM 通道仍使用 `MDI_PWM_*` contract。

## 性能约束

`MDI_TIMER_*` 只是调用外观。绑定必须生成 `static inline` 或等价的直接寄存器访问；不能在 provider 中引入运行时 token 查找、函数指针或 HAL 分派。控制操作不属于 DAC 每次采样的热路径，允许包含频率计算和状态检查；高频 ISR 仍由 driver 直接执行。

## 示例

```c
#include "mdi/mdi.h"
#include "mdi/instance.h"

mdi_status_t eStatus = MDI_TIMER_SetFrequency(dac_sample_timer, 320000U);
if (eStatus == MDI_OK) {
    eStatus = MDI_TIMER_Start(dac_sample_timer);
}

/* 板级 IRQ vector 直接确认硬件标志并进入 driver。 */
void TMR1_IRQHandler(void)
{
    if (board_timer_update_pending()) {
        board_timer_clear_update_flag();
        dac_driver_IsrStep();
    }
}
```

## 不纳入本次 contract 的内容

- 回调注册、`pContext`、`void *` 和运行时函数表；
- timer 到 ADC/DAC/PWM 的业务组合；
- tick 计数器、软件调度器和延时服务；
- 自动初始化时钟、GPIO、NVIC 或外设复位；
- PWM 占空比、死区、刹车和故障门控。

这些内容分别属于板级初始化、对应 feature 或具体 driver。
