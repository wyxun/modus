# MDI feature：可组合能力层

`feature/` 放置建立在 `core/` 契约上的可选组合能力。它们描述协议或领域语义，但不
选择芯片，也不保存运行时设备对象。

- `adc_dma.h`：把 ADC/DMA 采集组的触发、完成发布、缓冲区所有权和通道视图组合起来；
  DMA 中断只发布完成计数，不执行均值或滤波。
- `adc_mean.h`：在通道读取或内部控制适配器需要时处理最近完成的交错 ADC 块，发布稳定
  的每通道均值快照；均值计算不进入 DMA 中断。需要严格控制处理时机时使用
  `MDI_ADC_CHANNEL_VIEW_BIND`，由外部服务先调用更新，再读取已发布快照。
- `foc.h`：把完成的采样帧、三相 PWM duty/tick 和一次 Commit 组合成 FOC 周期。
- `soft_i2c_edges.h` / `soft_i2c_master.h`：从开漏线动作组成有界软件 I2C 主机。
- `i2c_reg8.h`：把 8-bit 寄存器读写组合到通用 I2C transfer。
- `spi_eeprom_25xx.h`：把 25xx EEPROM 的片选、页边界和 WIP 轮询组合到 SPI transfer。

feature 只消费芯片实例提供的资源。例如
`MDI_I2C_REG8_BIND(encoder_angle, encoder_i2c, 0x36U)` 固定设备地址和寄存器命令，
但真正的 I2C 时序、GPIO 和总线所有权仍由 `peripheral/<chip>/mdi/` 提供。

应用只应包含 `mdi/mdi.h`，除非需要单独审查某个 feature 的编译依赖。新增 feature 时，
优先组合现有能力；只有当协议边界、缓冲区生命周期或实时性承诺确实不同，才新增公共
契约。

## ADC 采样次数与扩展方式

均值 feature 的 `SAMPLE_COUNT` 是绑定宏的编译期参数：

```c
#define APP_ADC_SAMPLE_COUNT 8U

MDI_ADC_MEAN_GROUP_BIND(
    adc_mean, adc_dma, g_awRaw,
    APP_ADC_SAMPLE_COUNT, APP_ADC_CHANNEL_COUNT, APP_ADC_CHANNELS,
    g_awMean, g_wMeanSequence, g_bMeanValid,
    ADC1->RATE_HZ, 80000000U)
```

它表示同一采集组中每个通道的重复次数，同时决定 DMA 缓冲区布局和均值循环次数。它
可以由芯片工程在 `instance.h` 或构建配置中改成 4、8、16 等值，但不是运行时切换的
过滤档位。若不同通道需要不同重复次数，应建立不同采集组，或新增一个专门描述通道
窗口、步长和计数的 feature；不要为了这个场景修改 `core/adc.h`。

均值只是一个 feature 实现。中值、IIR、限幅平均等算法可以复用同一个 DMA 发布和稳定
快照契约，分别提供新的 feature 绑定；DMA ISR 仍只发布完成状态，算法在外部服务上下文
执行。这样扩展算法不会增加 core 的公共接口，也不会改变普通 ADC 读取和三相帧读取的
语义。
