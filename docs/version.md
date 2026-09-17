# MODUS 版本历史

| 版本 | 日期 | 状态 | 核心变更 |
| :--- | :--- | :--- | :--- |
| **v0.6.0.0** | 2026-09-17 | 稳定 | MDI 编译期能力接口落地；移除 waveform snapshot 与旧软定时器；完善 MStorage 错误处理 |
| **v0.5.0.7** | 2026-09-07 | 稳定 | 修复 `MWAVEFORM_SNAPSHOT_ENABLE=0` 时 `cmd_snapshot` status 分支引用被守卫成员导致的编译失败 |
| **v0.5.0.6** | 2026-08-07 | 稳定 | waveform V2.1：批量帧、快照帧、每变量刷新率、RTT 4096 与 10kHz 近无损连续流 |
| **v0.5.0.5** | 2026-07-26 | 稳定 | 修复序号 0xFD 保留值引起的隔帧判定失误 |
| **v0.5.0.4** | 2026-07-14 | 稳定 | Cortex-M 裸异常服务函数汇编改写，解决因局部变量压栈造成的 Hard Fault 现场寄存器解析偏移 |
| **v0.5.0.3** | 2026-07-02 | 稳定 | debug_transport 抽象层，modus.mk arch 子目录 include path，全局 disable_irq 为 0 时 static_assert 警告 |
| **v0.5.0.2** | 2026-07-01 | 稳定 | arch 重构：perfc_port/MSHELL 解耦、Cortex-M debug 迁入、cmsis_compiler 抽象、riscv_shim |
| **v0.5.0.1** | 2026-06-20 | 稳定 | 支持 userconfig.h 包含配置以及 mshell 内置命令溢出保护机制 |
| **v0.5.0.0** | 2026-06-02 | 稳定 | 引入对 RISC-V 双架构支持，并统一 perf_counter 移植文件 |
| **v0.4.0.4** | 2026-05-31 | 稳定 | 彻底消除 shadowed variable 隐患；实现 Poll 被动软定时器 MTimer 并支持 RingBuffer 自动静默绑定 |
| **v0.4.0.3** | 2026-05-27 | 稳定 | 更新.gitmodules；使用代理地址快速pull |
| **v0.4.0.2** | 2026-05-10 | 稳定 | 引入 MLIST_STATIC_INIT 静态初始化，解决未注册组件时的野指针隐患 |
| **v0.4.0.1** | 2026-05-07 | 稳定 | 命名规范与代码风格标准化，对齐 81 列续行符 |
| **v0.4.0.0** | 2026-05-07 | 稳定 | 全库重命名为 MODUS，核心目录结构重构，自建链表并替换 util_queue |
| **v0.3.0.4** | 2026-05-05 | 稳定 | modus.mk 统一构建系统，模块化编译开关 |
| **v0.3.0.3** | 2026-05-03 | 稳定 | 新增 -T 日志类别与 SuperWaveform INI 宏命令管理 |
| **v0.3.0.2** | 2026-04-23 | 稳定 | mwaveform 架构维护：回归 Ping-Pong 及文档补全 |
| **v0.3.0.1** | 2026-04-22 | 稳定 | mwaveform 架构重构：乒乓缓冲与 ISR 安全优化 |
| **v0.3.0.0** | 2026-04-18 | 稳定 | SuperWaveform 重构与测量系统；协议同步 with 稳定性优化 |
| **v0.2.1.0** | 2026-04-14 | 稳定 | 新增 mshell 调试 Shell；MLOG 运行期级别控制；MLOGF 补充 `%f` |
| **v0.2.0.1** | 2026-04-08 | 稳定 | MLOGF 支持 %lu 格式化打印 |
| **v0.2.0.0** | 2026-04-07 | 稳定 | 新增 MStorage 持久化存储模块 |
| **v0.1.0.0** | 2026-03-15 | 稳定 | 初始版本发布 |

## [0.6.0.0] - 2026-09-17

### MDI 编译期能力接口

- 新增 `src/mdi/mdi_static.h`，使用 C11 `_Generic` 提供类型化的编译期能力分派。
- 覆盖 PWM、GPIO、ADC 和 Stream 等语义明确的硬件操作。
- 保留直接函数或 `static inline` 实现，避免运行时函数指针和通用
  `read/write/control` 分派进入实时路径。

### Waveform 简化

- 移除 `MWAVEFORM_SNAPSHOT_ENABLE`、snapshot 环形缓冲、snapshot 帧和相关 API。
- 移除 `wave snap` Shell 命令及 snapshot 协议帧 `0xFA`。
- 保留连续 stream、批量帧和每通道刷新率；波形通道在应用初始化时注册和配置。
- 删除 Makefile 中对应的 snapshot 配置和编译参数。

### MODUS 基础组件清理

- 移除 `msoft_timer_t` 及 `mbase_TimerInit/Start/Poll` 旧软定时器接口。
- 清理 `mbase.c` 对 `perf_counter` 的无关依赖。
- 删除已废弃的旧版 MDI 规划文档，避免与现行能力接口重复。

### MStorage 可靠性改进

- 检查 Flash 设备是否配置。
- 检查 Flash 解锁、擦除、读写和加锁的返回值。
- 初始化读取失败和存储操作失败时输出具体错误码，并阻止错误结果继续提交。

## [0.5.0.5] - 2026-07-26
- **waveform 帧率异常问题修复**:
  - 修复序号 0xFD 保留值引起的隔帧判定失误

## [0.5.0.7] - 2026-09-07
- **快照禁用态编译修复**:
  - 修复 `MWAVEFORM_SNAPSHOT_ENABLE=0` 时 `mwaveform.c` 编译失败：
    `cmd_snapshot` 的 `status` 分支无条件引用被 `#if MWAVEFORM_SNAPSHOT_ENABLE`
    守卫的结构体成员 `s_tWave.hwSnapshotValidCount`，禁用快照构建时该成员
    不存在。
  - 修复：该分支加 `#if MWAVEFORM_SNAPSHOT_ENABLE` / `#else`，禁用态打印
    `Snapshot disabled (MWAVEFORM_SNAPSHOT_ENABLE=0)`，不再引用缺失成员。

## [0.5.0.6] - 2026-08-07
- **waveform 任意 ISR 频率支持**:
  - 新增 `AA 55 FE` 采样周期元数据帧，启动、改频和周期重同步时上报。
  - 新增 `AA 55 FC` 批量帧，每样本保留通道掩码，支持混合采样率通道。
  - 新增 `AA 55 FA` 快照帧，支持预触发环形缓冲与上位机触发抓取。
  - 批量帧使用 CRC16，旧单点帧保持 XOR 兼容。
  - 新增 `SetSamplePeriodNs` 与 `wave period` / `wave snap` 控制。
- **waveform V2.1 每变量刷新率**:
  - 新增 `AddVariable` 指针绑定与 `MWAVEFORM_VAR_FLOAT` / `MWAVEFORM_VAR_RAW`。
  - 新增 `SetChannelRate` / `wave chrate`，每个通道可按 stream 整数分频独立刷新。
  - 修复 batch 打包时未按环形缓冲当前偏移取数导致的 64 样本重复/跳变问题。
- **RTT/OpenOCD 调优**:
  - 当前工程 RTT 缓冲调整为 4096，OpenOCD 轮询保持 1ms。
  - 10kHz 连续流 60 秒实测 MCU 侧 0 丢帧，host 侧 1 次 CRC 偶发错误。
- **文档更新**:
  - 重写 `docs/mdebug/mwaveform.md`，删除旧 FIFO/单点帧描述。
  - 更新 MStudio 集成指南与 README 中的旧 API 示例。

## [0.5.0.4] - 2026-07-14

### 修复与优化
- **Cortex-M 异常寄存器提取解析**:
  - 重构并改写了 `src/arch/cortex-m/fault_cm.c` 中的裸异常中断服务入口（`HardFault_Handler` 等）。
  - 利用裸汇编（`__asm__ __volatile__`）直接读取并提取异常发生时的真实 MSP/PSP 栈帧地址，作为参数传入解析函数。
  - 彻底解决了在无优化（`-O0`）或存在局部变量时，由于 C 语言 Prologue 压栈引起的崩溃现场寄存器（如 R2, R12, LR 等）解析偏移与乱码问题。

## [0.5.0.3] - 2026-07-02

### 新增与重构
- **debug_transport 抽象层**: 新增 `src/arch/debug_transport.c/h`，统一调试输出入口。
  - 默认使用 SEGGER RTT 后端（Cortex-M / RISC-V 通用，纯共享内存机制不依赖外设）。
  - 与 `USERCONFIG_MSHELL_ON_SERIAL` 解耦——串口双输出在项目层 `TRACE_MCU_WRITE_STRING` 回调中自行组合。
- **modus.mk 补全 arch 子目录**: `MODUS_INCLUDES` 新增 `src/arch/cortex-m/` 和 `src/arch/riscv/`，消除外部引用 `mdebug_cm.h` 等头文件时的路径缺失。
- **global_define.h**: `disable_irq` 为 0 时 static_assert 警告。

## [0.5.0.2] - 2026-07-02

### 新增与重构
- **arch 目录体系**: 新建 `src/arch/` 平台架构抽象层，集中存放内核架构相关的底层移植代码。
  - `perfc_port.c/h` 从 `src/mdebug/` 迁移至 `src/arch/`，与 debug 子系统解耦。
  - `riscv/` 子目录存放 RISC-V 通用移植（`riscv_shim`、`mdebug_riscv`）。
  - `cortex-m/` 子目录存放 Cortex-M debug（`mdebug_cm`、`fault_cm`），从 modus_template `vendor/` 迁入，实现双架构对称。
  - `cmsis_compiler.h` 作为架构感知的编译器抽象 shim，统一 RISC-V（`fence`/`fence.i`/`csrci mstatus`）与 ARM（`dsb`/`isb`/`cpsid i`）内联 intrinsic。
- **riscv_shim 模块**: 新增 `src/arch/riscv/riscv_shim.c`，提供 RISC-V bare-metal compiler-rt builtin 及 libc 替代函数。支持 `MODUS_NO_RISCV_SHIM` 外部 opt-out。
- **mdebug_riscv 去硬编码**: 芯片内存地址范围改用链接脚本导出符号 `__flash_start/__flash_end/__sram_start/__sram_end`，消除 CH592 特判。
- **perfc_port 双方案**: RISC-V 默认软件 SysTick 64 位拓展计数器（兼容 mcycle 未实现的芯片），可通过 `MODUS_PERFC_USE_MCYCLE` 切换到硬件 cycle CSR 方案。Cortex-M 侧使用 SysTick LOAD/VAL 标准外设。
- **modus.mk 构建分层**:
  - `MODUS_SRCS_ARCH` — 始终编译（`perfc_port` + `riscv_shim`），由 `MODUS_USE_DEFAULT_PERFC_PORT` 控制。
  - `MODUS_SRCS_ARCH_DEBUG` — 由 `MSHELL_ENABLE` 门控（`mdebug_riscv` + `mdebug_cm` + `fault_cm`）。
  - `perfc_port` 不再依赖 MSHELL，作为核心计时基础设施独立运行。
- **Cortex-M debug 迁移**: `mdebug_cm.c`（regs/peek/poke/stack/cfsr 命令）和 `fault_cm.c`（HardFault/MemManage/BusFault/UsageFault 裸处理）从 modus_template `vendor/cortex-m/core_debug/` 迁入。使用 `MDEBUG_CM_FAULT_HANDLERS_ACTIVE` 哨兵替代旧的 `CORE_DEBUG_FAULT_HANDLERS_ACTIVE`。
- **外部库路径**: `modus.mk` 新增 `$(MODUS_ROOT)/src/arch` 到 include path。

## [0.5.0.1] - 2026-06-20

### 新增与重构
- **用户配置自定义**：在 `SEGGER_RTT_Conf.h` 和 `mshell.h` 中引入对 `MODUS_CFG_USER_CONFIG_INCLUSION` 的包含支持，允许在外部 `userconfig.h` 中覆盖配置。默认 `BUFFER_SIZE_DOWN` 与 `MSHELL_LINE_SIZE` 提升至 64 字节。
- **安全机制**：在 `mshell_Poll` 初始化中对内置命令的注册使用局部宏 `SAFE_REG_CMD` 保护，防止超过 `MSHELL_MAX_CMDS` 限制引发越界死机。

## [0.5.0.0] - 2026-06-02

### 新增与重构
- **双架构支持**：在 `modus` 中添加了对 RISC-V 双架构支持，进一步增强其跨平台可移植性。
- **移植统一**：将 Cortex-M 与 RISC-V 的 `perf_counter` 移植文件统一为 `perfc_port.c`，简化底层调用逻辑。

## [0.4.0.4] - 2026-05-31

### 已修复
- **作用域遮蔽（Shadowed Variable）隐患**：在 `src/mbase.c` 中移除了 `chErgodicTime` 的重声明，重构为直接比对哨兵尾节点 `&tListObject.tListEnd`，彻底清除了大并发遍历下的潜在死机隐患。

### 新增与重构
- **MTimer 软件定时器**：在 `src/mbase.c/h` 中基于 `perf_counter` 系统时钟实现了轻量级被动 poll 软定时器 `msoft_timer_t`，免去了老旧协程开销，实现完全非阻塞轮询。
- **RingBuffer 静默自动绑定**：在 `mbase_Init` 内部增加了自动校验，用户无需再手动显式调用 RingBuffer 配置绑定接口，大幅度精简了模块模板冗余初始化。
- **老旧 API 废弃与虚化**：使用 `__attribute__((deprecated))` 强力废弃了链表消息（MessageList）与弱类型共享内存（ShareMem）查表 API，强推高效无锁的 RingBuffer 管道与强类型依赖注入（DI）指针。虚化了 `mcoroutine` 为弱存根，推进框架轻量与 KISS 纯净性。

## [0.4.0.2] - 2026-05-10

### 已修复
- **mlist 稳定性增强**:
  - **引入 MLIST_STATIC_INIT**: 在 `mlist.h` 中新增了静态初始化宏，支持在编译时完成链表及其哨兵节点的初始化。
  - **解决野指针隐患**: 修复了 `mbase.c` 中 `tListObject` 因懒加载初始化导致的漏洞。现在即使在没有任何组件注册的情况下，`modus_Run()` 和 `modus_Clock()` 的链表遍历也能安全跳过，彻底杜绝了空链表遍历时的 NULL 指针解引用风险。
  - **架构精简**: 移除了 `mbase_Init()` 中冗余的运行时初始化计数逻辑，降低了运行时开销并提升了代码鲁棒性。

## [0.4.0.1] - 2026-05-07

### 更新与优化
- **命名规范与代码风格标准化**:
  - 在 `README.md` 中明确了单行不得超过 **81** 个字符的限制。
  - 强制要求续行符 `\` 必须统一对齐至第 **81** 列，提升代码美观度。
  - 全面清理了源码及文档中遗留的 `GStorage` 标识符，统一为 `MStorage`。
  - 整合了匈牙利命名法表格至核心命名规范章节。
- **源码格式化**:
  - 完成了 `src/` 目录下所有 `.c` 和 `.h` 文件的格式化审计，所有续行符已完成对齐。
- **版本更新**:
  - `MODUS_INTERFACE_VERSION` 升级至 `4`，`MODUS_MINOR_VERSION` 更新为 `1`。

## [0.4.0.0] - 2026-05-07

### 新增功能
- **modus.mk 统一构建系统**:
  - 在仓库根目录新增 `modus.mk`，外部项目只需 `include $(MODUS_ROOT)/modus.mk` 即可引用 MODUS 框架。
  - 所有可选模块默认关闭（保守策略），外部项目按需通过 Make 变量显式开启。
- **模块化编译开关**:
  - `MSHELL_ENABLE` / `MWAVEFORM_ENABLE`: 控制 mshell、trace、SEGGER_RTT、mwaveform 等调试模块的编译。
  - `MSTORAGE_ENABLE` / `MBLINFO_ENABLE`: 控制 mstorage、mblinfo 功能模块的编译。
  - `MODUS_USE_LOG` / `MODUS_USE_ASSERT`: 控制 MLOG/MLOGF 日志宏和 MODUS_ASSERT 断言宏。
  - 关闭时零开销：对应源码不参与编译，ROM/RAM 完全不占用。
- **BLM 示例双模式构建**:
  - `make`（默认）: `-O0` + 全调试功能开启，支持 F5 一键调试。
  - `make release`: `-Oz` + 所有调试模块剥离，产出精简发布固件。

### 更新与优化
- **modus.c**: 在 `modus_Run()` 和 `modus_Clock()` 中增加了 `#if MSHELL_ENABLE` / `#if MWAVEFORM_ENABLE` 条件编译守卫，关闭时可被链接器垃圾回收。
- **mshell.h**: 增加了 `MSHELL_ENABLE` 编译期开关，当值为 `0` 时 `MODUS_SHELL_CMD` 宏展开为空，shell 命令自动废弃。
- **mwaveform.h**: 当 `MWAVEFORM_ENABLE == 0` 时提供轻量 API 桩结构体，避免调用方编译报错。
- **util_debug.h**: `__NO_USE_LOG__` 定义时 `MLOG`/`MLOGF` 宏展开为真正的空操作 `do {} while(0)`，彻底消除对 TRACE 符号的间接依赖。
- **BLM main.c**: 增加了 `#if MSHELL_ENABLE` / `#if MWAVEFORM_ENABLE` 守卫，保护 TRACE 初始化和波形测试相关调用。


## [0.3.0.3] - 2026-05-03

### 新增功能
- **MODUS 调试系统**:
  - **新增 -T (Timer) 日志类别**: 专门用于周期性/轮询函数中的状态验证。支持通过 `log -T` 独立控制开关，解决了高频定时打印干扰开发视线的问题。
  - **MLOG 架构优化**: 重构了编译期 Gate 逻辑，确保 T 类别在默认 INFO 级别下也可编译包含，且运行时 Mask 依然解耦。
- **SuperWaveform (Host)**:
  - **动态宏命令管理**: 引入 INI 驱动的宏按钮系统，支持用户手写配置文件自定义快捷指令。
  - **配置持久化**: 支持加载外部 INI/TOML 扩展配置，并自动保存/恢复最后一次使用的路径。

## [0.3.0.2] - 2026-04-23

### 更新与优化
- **mwaveform (架构维护)**:
  - **稳定性回归**: 将驱动架构从实验性 FIFO 回归至 **Ping-Pong (最新优先)** 模型，确保高频采样下的低延迟特性。
  - **物理链路优化**: 经验证并保留了 8MHz 的高速 SWD 配置，显著提升 RTT 传输上限。
  - **文档补全**: 完成了 `doc/mdebug/mwaveform.md` 深度指南，包含协议细节说明并归档了无损 FIFO 演进思路。

## [0.3.0.1] - 2026-04-22

### 更新与优化
- **mwaveform (架构级重构)**:
  - **内存优化**: 舍弃了 8KB 的中间 Ring Buffer，改用双缓冲区 **Ping-Pong** 架构。总 RAM 开销从 ~16.5KB 降低至 **~1.1KB**，节省了约 94% 的内存。
  - **ISR 安全**: 实现了基于 `wWriteCount / wReadCount` 锁无关计数器的交接逻辑，确保在 FOC 等高频 ISR 中调用 `Step()` 是非阻塞且 100% 安全的。
  - **丢帧诊断**: 新增丢帧计数与丢帧率统计，支持通过 `wave drop` 实时监控传输健康度。
  - **控制增强**: 引入 `SetRate(0)` 外部驱动模式，支持用户在自定义高频中断（如 20kHz）中直接触发采样。
  - **API 统一**: 接口全面迁移至 `mwaveform.` 命名空间，并废弃了旧的 `mringbuf` 内部依赖。

## [0.3.0.0] - 2026-04-18

### 新增功能
- **SuperWaveform (Host)**: 彻底重构为高性能 C++ 版本，采用 ImGui/ImPlot 渲染引擎。
  - **测量系统**: 新增 `Space` 差值测量功能，支持 dX, dY 及频率实时计算。
  - **录制功能**: 实现本地 CSV 异步录制，支持 `rec_YYYYMMDD_HHMMSS` 自动命名。
  - **多窗口分析**: 支持多实例离线查看器，具备窗口色调自动区分与独立交互。
  - **视觉提升**: 引入蓝黑科技格调 UI 与虚线十字准心绘制。

### 更新与优化
- **架构优化 (Directory Refactoring)**:
  - **组件迁移**: 建立了 `modus/mdebug/` 统一调试目录。
  - **文件重组**: 将 `mshell`, `mwaveform`, `util_debug`, `trace` 等调试支撑组件迁移至该目录，使核心框架与调试工具链实现物理隔离，提高了模块化程度。
- **GWaveform**:
  - **同步优化**: 引入自适应虚拟时钟同步算法，消除 RTT 抖动导致的波形锯齿与阶梯感。
  - **协议稳定性**: 修复了通道掩码字节对齐（Mask Bytes Mismatch）导致的严重丢包 Bug。
  - **并发安全与性能**: 
    - 优化 `mringbuf` SPSC 环形缓冲区处理，消除采样中断与 Poll 循环之间的竞态冲突。
    - 通过扩大 RTT 单次读取块（从 128B 提升至 512B）减少了 RTT 操作频率，降低下位机 CPU 负载。
  - **接口升级**: `mshell` 的 `ver` 指令现在可以动态读回 v0.3.0.0 版本信息。

### 已修复
- **稳定性**: 修复了 ImPlot API 调用序列断言失败及 Tooltip 嵌套导致的闪退问题。
- **性能**: 修正了部分调试接口在极端带宽下的阻塞风险。

## [0.2.0.1] - 2026-04-08

### 新增功能

+ **MLOGF**：增加对`%lu`变量打印的支持，可以识别`long`型数值打印

## [0.2.0.0] - 2026-04-07

### 新增功能
- **MStorage**: 增加了持久化存储接口。基于 GDI (MODUS Driver Interface) 抽象层，支持自动 CRC 校验与 Flash 读写操作，实现了 RAM 数据的自动定时同步。

### 更新与优化
- **MLOG**: 更新了日志系统接口。采用了链式参数分发机制，支持更高效的多参数打印，降低了栈开销并移除了对 `snprintf` 的依赖。
- **MODUS 初始化**: 更新了 `modus_Init` 以支持全局默认 Flash 设备的注入。

### 已修复
- 修复了 `example/posix_uart` 和 `example/template` 在最新接口下的运行问题。
- 修复了 `mstorage` 的缓冲区溢出隐患。

