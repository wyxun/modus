# MDI：面向应用的静态硬件接口

MDI（MODUS Device Interface）把 MCU 能直接完成的硬件工作抽象成**编译期资源**：
应用使用一个资源 token，编译器根据芯片实例绑定生成对应的寄存器访问。资源不通过
设备编号查找，也不保存对象、`void *` 上下文或运行时函数表，因此热路径可以展开为
与手写寄存器代码相同的 `static inline` 访问。

这套接口的目标是让上层调用硬件工作，而不是适配某一家 MCU 厂商的 HAL。GPIO、采样
帧、PWM、I2C、SPI 和 FOC 组合能力由公共契约描述；引脚、寄存器、触发源、DMA 和安全
状态由 `peripheral/<chip>/mdi/` 的芯片绑定提供。

框架的稳定设计规范见 [MDI 框架设计](DESIGN.md)；本文负责应用接入、API 速查和示例。

## 先看目录边界

```text
modus/src/mdi/
├── mdi.h                 应用默认公共入口
├── core/                 芯片无关契约与编译期绑定模板
├── feature/              可组合的协议/领域能力
├── examples/             芯片实例上的调用示例
├── tests/                最小契约、负编译和汇编检查
└── legacy/               旧对象/函数指针接口，仅供迁移期显式使用

peripheral/<chip>/mdi/
├── backend.h             芯片寄存器操作
├── instance.h            引脚、通道和资源 token 的绑定
├── pwm.h / i2c.h         芯片外设扩展和生命周期
└── fault.h/.c            芯片或板级安全故障源
```

职责边界是静态源码边界，不是运行时调用层。应用通常包含：

```c
#include "mdi/mdi.h"
#include "mdi/instance.h"       /* 由芯片工程提供 */
```

`mdi/mdi.h` 只汇总公共契约和可选 feature；它不会选择 STM32、GD32 或某一块板卡。
芯片的 `instance.h` 才选择具体资源。这样同一个应用调用可以换芯片绑定，而不用改成
厂商 HAL API。

## 公共接口按能力划分

| 能力 | 主要接口 | 语义 |
| --- | --- | --- |
| IO | `MDI_IO_Read`、`MDI_IO_Write`、`MDI_IO_WriteMasked` | 单 pin 或跨端口位向量，写入只影响绑定位 |
| IO 能力 | `MDI_IO_Width`、`MDI_IO_Capabilities` | 编译期宽度、输入/输出/开漏约束 |
| ADC/采样 | `MDI_Sample_Frame`、`MDI_Sample_Read` | 一次读取已经完成的整帧，不启动转换 |
| 稳定帧 | `MDI_Sample_ReadStable`、`MDI_Sample_ReadCompleted` | 序列号或硬件完成标志保护生产者所有权 |
| ADC/DMA feature | `MDI_ADC_Read`、`MDI_ADC_SetSampleFrequency` | 应用按通道读取；DMA 发布和均值处理留在 feature/芯片适配内部 |
| PWM | `MDI_PWM_SetDuty`、`MDI_PWM_Stage`、`MDI_PWM_Commit` | Q16 占空比或 timer tick，显式提交生效 |
| PWM 生命周期 | `MDI_PWM_Enable`、`MDI_PWM_SafeStop`、`MDI_PWM_ClearFault` | 输出门控、停机和故障锁存 |
| 总线 | `MDI_I2C_Transfer`、`MDI_SPI_Transfer` | 有界同步事务，缓冲区只在调用期间借用 |
| 设备组合 | `MDI_I2C_Reg8_Read`、`MDI_SPI_EEPROM_Read/Write` | 把寄存器命令、页边界和片选规则组合到总线事务 |
| FOC 周期 | `MDI_FOC_RunCycle`、`MDI_FOC_RunCycleFast` | 一次完成采样读取、三相占空比提交和 Commit |

接口按能力增加，而不是把所有设备塞进一个“大 MDI 对象”。没有能力的资源不会得到
成功 stub；错误在编译期或明确的 `mdi_status_t` 返回值中暴露。

## Core 与 feature 的扩展规则

`core/` 只保留跨芯片、跨应用都成立的硬件原语和稳定语义：编译期资源绑定、直接读写、
类型化帧、完成状态、所有权和错误码。它不放采样次数、滤波器参数、设备协议、FOC
策略或某块板卡的配置表。

`feature/` 用来组合这些原语，表达 DMA 采集组、tick 调度、均值/中值/IIR 滤波、软件
I2C、EEPROM 等可选能力。feature 可以通过绑定宏接受编译期参数，也可以维护自己的
同步状态，但不能把运行时对象分派重新引入热路径，更不能改变 core 接口的含义。

以 ADC 为例，`core/adc.h` 只负责单值读取；`adc_dma.h` 负责完成发布和缓冲区所有权；
`adc_mean.h` 负责批量均值。`SAMPLE_COUNT` 由芯片实例或构建配置在编译期提供，改变它
会同时改变 DMA 布局和生成代码。不同通道重复次数或新的滤波算法应新增 feature 或采集
组，而不是扩张 core。

`core/adc.h` 只定义最基本的单通道 ADC code 读取；ADC 扫描组、DMA 完成发布、多采样
均值和按通道视图属于可选的 `feature/adc_dma.h`、`feature/adc_mean.h`。应用通常只看
`MDI_ADC_Read` 和可选的 `MDI_ADC_SetSampleFrequency`，DMA flag、缓冲区所有权和均值
过程由 feature 与芯片后端内部完成；若采样由轮询 tick 驱动，应用还需在任务上下文
调用该芯片实例提供的服务函数完成调度和滤波发布。

### 编译期绑定不等于自动零开销

`MDI_*` 宏只是调用外观，不能单凭宏名称判断实现效率。一个错误的后端完全可能把
宏展开成下面这种隐藏链路：

```text
MDI_IO_Write(token, value)
    -> 资源对象查找
    -> void * 上下文
    -> 函数指针
    -> 通用分发器
    -> 厂商 HAL
    -> 寄存器
```

这种实现只是把低效调用藏在宏后面，不属于 MDI 的静态热路径。实时资源必须满足：

- token 在编译期确定，不能转换成运行时设备编号或描述符查找；
- provider 使用 `static inline`、编译期寄存器表达式或直接寄存器访问；
- 热路径不能经过 `void *`、函数指针、虚表、字符串命令或通用能力分发器；
- 多路操作应在一个类型化帧或一次资源操作中完成，不能让应用重复进入同一层；
- 对 `*_Fast` 路径必须检查最终汇编，确认没有 `bl`/间接调用和不需要的临时对象。

协议状态机、超时轮询、DMA 同步和故障处理本身可能需要真实指令和循环；这里要求的
是它们直接位于对应 provider/feature 中，而不是再套一层运行时对象分派。新增后端时，
至少同时做预处理展开、`-O2/-Os` 汇编检查和一次目标构建，不能只通过“宏能编译”来
宣称零开销。

## 芯片层接入步骤

芯片或板级工程按下面顺序接入 MDI：

1. 在 `instance.h` 中用 X-list 或绑定宏声明资源 token，例如 `phase_current`、
   `bridge`、`encoder_i2c`。
2. 在芯片后端把 token 映射到寄存器、触发标志、DMA 发布序列和输出门控。后端可以
   使用 CMSIS 寄存器，但公共 `core/` 不得包含厂商头文件。
3. 在板级启动代码完成时钟、GPIO 模式、外设使能、预装载和总线所有权。MDI 读写接口
   不会猜测这些初始化，也不会在第一次调用时偷偷初始化硬件。
4. 在实时中断中只调用已完成帧和快速路径：先读完整采样帧，再写整组 PWM，最后提交。
   前台任务负责 I2C/SPI 等有界事务；不要在 FOC 中断里轮询外部设备。
5. 故障路径使用 `MDI_PWM_SafeStop`、`MDI_PWM_FaultActive` 和
   `MDI_PWM_ClearFault`，由芯片后端决定 MOE、Break、COMP 和实际门控顺序。

以 G431 为例，`peripheral/stm32g431/mdi/instance.h` 绑定三相 ADC、TIM1 三相 PWM、
AS5600 I2C 设备和蜂鸣器；`mdi_stm32_g431_i2c1_Init()` 负责 I2C1 的时钟、PB7/PB8
开漏复用、时序和使能。这个初始化属于芯片后端，不会污染 `modus/src/mdi/core/`。
G431 的 FOC 三相电流当前使用 TIM1 触发的 ADC 注入组和 JEOS 完成边界，直接读取
JDR 寄存器，并不强制使用 DMA；DMA 扫描、批量均值和其他滤波属于可选 feature。它们都
可以通过同一个稳定帧契约提供给 FOC，选择哪一种由芯片实例的实时性和缓冲区所有权决定。

如果需要从零参考一个多外设芯片实例，可查看模板工程的
[`peripheral_template`](../../../peripheral_template/README.md)。它使用抽象的 32 位
寄存器模型，同时展示 GPIO、并口 DAC、三相 ADC、PWM、硬件/软件 I2C 和 SPI EEPROM
如何绑定到同一套 MDI 应用接口。

## 示例：应用如何调用

`examples/application.c` 是可编译的芯片实例调用示例。下面的代码展示了应用只关心
资源语义：

```c
/* 一次写入单 pin 和四位并口，均由 instance.h 绑定。 */
MDI_IO_Write(status_led, 1U);
MDI_IO_Write(dac_data, wCode);

/* 一次读取三相已完成帧；接口不启动 ADC，也不等待转换。 */
MDI_Sample_Frame(phase_current_completed) tCurrent = {0};
mdi_status_t eStatus = MDI_Sample_ReadCompleted(
    phase_current_completed, &tCurrent);

/* 普通路径做单位转换和范围检查。 */
MDI_PWM_DutyFrame(bridge) tDuty = {
    .u = 32768U, .v = 32768U, .w = 32768U
};
if (eStatus == MDI_OK) {
    eStatus = MDI_PWM_SetDuty(bridge, &tDuty);
    if (eStatus == MDI_OK) {
        eStatus = MDI_PWM_Commit(bridge);
    }
}
```

严格时限的 FOC 路径可以在进入中断前验证范围，随后使用 tick 帧快速提交：

```c
MDI_Sample_Frame(phase_current_completed) tCurrent = {0};
const MDI_PWM_Frame(bridge) tCompare = {
    .u = wUCompare, .v = wVCompare, .w = wWCompare
};

/* 读取未完成时返回 MDI_BUSY，PWM 保持不变。 */
(void)MDI_FOC_RunCycleFast(g431_foc_cycle, &tCurrent, &tCompare);
```

变频 PWM 使用频率和占空比分开的契约；频率属于定时器组，占空比属于通道组：

```c
const MDI_PWM_DutyFrame(buzzer) tDuty = {.value = 32768U};
(void)MDI_PWM_SetFrequency(buzzer, 2000U);
(void)MDI_PWM_SetDuty(buzzer, &tDuty);
```

AS5600 这类 8-bit 寄存器设备通过设备 feature 组合到 I2C：

```c
uint8_t achRaw[2] = {0U, 0U};
mdi_status_t eStatus = MDI_I2C_Reg8_Read(
    encoder_angle, 0x0CU, achRaw, 2U);
uint16_t hwAngle = (uint16_t)((achRaw[0] << 8U | achRaw[1]) & 0x0FFFU);
```

实际芯片工程仍需先完成自己的 I2C 初始化和总线所有权；设备 feature 不保存 I2C
对象，也不替应用管理并发。

## 调用开销和检查边界

- 资源 token 在预处理阶段拼接成具体函数名；绑定宏生成 `static inline` 函数和类型。
- 生产代码不经过 `void *`、对象索引或函数指针。使用 `-O2/-Os` 时，IO、采样和 PWM
  热路径可以与直接寄存器版本产生相同指令；`tests/run_tests.py` 会检查这一点。
- 带检查的接口会保留空指针、范围、状态和超时逻辑。`*_Fast` 只在调用者已经满足
  非空、范围、更新窗口和资源所有权前提时使用，不能用来绕过安全停机。
- 协议本身的工作不会消失：I2C 的 START/ACK/时钟拉伸、SPI 的传输、DMA 同步和故障
  处理仍然需要周期。MDI 消除的是抽象层调度和重复的对象访问，不是硬件本身的延迟。

## 迁移旧接口

旧的对象、`pPriv` 和函数指针实现位于 `legacy/`，默认入口不会包含它们。迁移时先让
芯片绑定提供同一语义资源，再把上层调用改成 `MDI_*` 宏；不要把旧的函数指针表重新
包一层宏继续放回实时路径。FOC 可以保持自己的独立端口契约，芯片适配器只把采样、
PWM、Fault 和位置读取绑定成直接调用。

## 验证和维护

`tests/run_tests.py` 是 MDI 的唯一测试入口，保留三类最小检查：

1. 主机契约测试：验证 IO、采样、PWM、软件 I2C 和 SPI EEPROM 行为。
2. 负编译测试：验证错误资源类型、单位、能力和物理映射在编译期失败。
3. Cortex-M 汇编检查：在 `-O2` 和 `-Os` 下比较直接实现与 MDI 实现，确认热路径不
   生成 `bl`/函数指针调用。

运行方式：

```text
python modus/src/mdi/tests/run_tests.py
```

新增芯片时应优先增加 `peripheral/<chip>/mdi/instance.h` 和后端验证，不要复制一套
新的公共 MDI 头文件。MODUS 的设计索引见
[modus/readme.md](../../../modus/readme.md)。

MDI 的 C/C++ 头文件、示例和文档统一使用 CRLF；宏续行反斜杠固定在第 82 列，便于
Windows 工具链和代码审查保持一致。
