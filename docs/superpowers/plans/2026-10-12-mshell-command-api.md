# MODUS MShell 命令参数 API 优化计划与实施记录

**状态：** 已实施；测试与构建结果见第 6 节。

**Goal：** 让 MODUS 命令作者直接使用框架的词匹配、参数转换和输入完整性检查，减少重复字符串解析。

**Architecture：** 保留 `MODUS_SHELL_CMD` 和 `void handler(const char *args)`。新增无状态的参数解析模块，以明确类型函数实现转换，C11 `_Generic` 提供便捷入口。语法解析由 MShell 负责，业务范围与动作由命令负责。

**Tech Stack：** C、C11 `_Generic`、现有 MShell IO、主机 C 测试。

实现保留原 Shell 注册与回调接口，并将 M448 DAC 命令作为框架 API 的
首个应用迁移示例。

## 1. 当前缺口与边界

检查到的框架证据：

- `src/mdebug/mshell.h` 只公开 IO 设置、命令注册和轮询，命令作者拿到原始参数字符串。
- `src/mdebug/mshell.c` 的 `trim_leading`、`parse_hex` 是私有函数；十六进制解析缺少完整 token 和溢出检查。
- `src/arch/cortex-m/mdebug_cm.c`、`src/arch/riscv/mdebug_riscv.c` 的 `peek/poke/stack` 重复解析参数。
- 当前收行逻辑会丢弃超长部分，再执行截断的命令；Tab 与解析器支持的空白规则也不一致。
- `shell_puts` 未公开，应用命令缺少直接通过当前 Shell IO 回复的简单入口。

因此 API 放在 MODUS 的 `src/mdebug`，文档和测试也随 MODUS 交付。M448
DAC 命令使用框架 API 并删除本地解析函数；频率范围仍由该命令校验。

## 2. 方案选择

| 方案 | 优点 | 代价 | 选择 |
|---|---|---|---|
| 只公开现有私有函数 | 改动少 | 现有语法和错误处理不足，类型扩展困难 | 不采用 |
| 统一解析契约 + 类型化函数 + 泛型入口 | 保持兼容，调用简洁，可独立验证 | 需要补齐边界语义和测试 | 推荐 |
| 改成 argc/argv 或声明式命令参数表 | 适合复杂选项系统 | 回调迁移、存储和描述规则增加 | 本轮不引入 |

## 3. 对外 API

### 3.1 字符串与 token

```c
typedef struct {
    const char *data;
    size_t length;
} mshell_token_t;

typedef enum {
    MSHELL_PARSE_OK = 0,
    MSHELL_PARSE_END,
    MSHELL_PARSE_INVALID,
    MSHELL_PARSE_RANGE,
    MSHELL_PARSE_BAD_ARGUMENT
} mshell_parse_result_t;

const char *mshell_SkipSpaces(const char *text);
bool mshell_ConsumeWord(const char **cursor, const char *word);
bool mshell_AtEnd(const char *cursor);
mshell_parse_result_t mshell_NextToken(const char **cursor,
                                     mshell_token_t *token);
```

- 空白统一为空格和 Tab；解析函数自动跳过前导空白，普通调用不必手动 SkipSpaces。
- ConsumeWord 区分大小写，匹配完整 token：`start` 不能匹配 `starter`。失败不移动游标；word 必须为非空且不含空白的字符串。
- NextToken 返回指针与长度，不复制、不修改原字符串，也不补写 NUL。token 有效期由原字符串决定；Shell 回调退出后不能保留引用。
- AtEnd 忽略尾部空白后判断结束，用来拒绝多余参数。
- SkipSpaces(NULL) 返回 NULL；AtEnd(NULL) 返回 false；ConsumeWord 遇到非法指针返回 false。带状态返回值的函数返回 BAD_ARGUMENT。

### 3.2 类型转换

明确类型的叶子函数使用以下命名与签名规则：

```c
mshell_parse_result_t mshell_ParseUint32(const char **cursor,
                                       uint32_t *value);
mshell_parse_result_t mshell_ParseInt32(const char **cursor,
                                      int32_t *value);
mshell_parse_result_t mshell_ParseBool(const char **cursor,
                                     bool *value);
mshell_parse_result_t mshell_ParseHexUint32(const char **cursor,
                                          uint32_t *value);
```

首版支持 int8/16/32/64、uint8/16/32/64、bool；十六进制支持上述四种无符号宽度。每个叶子函数均有真实类型的目标指针，不接受运行时类型码。

统一调用形式：

```c
uint32_t count = 0U;
int16_t offset = 0;
bool enabled = false;

/* 以下为三个独立调用示意；实际命令必须检查每次返回值。 */
mshell_Parse(&cursor, &count);
mshell_Parse(&cursor, &offset);
mshell_Parse(&cursor, &enabled);

/* 进制属于语法，调用处明确指定。 */
mshell_ParseHex(&cursor, &count);
```

`mshell_Parse` 根据输出指针类型分派到上述整数、bool 叶子函数；字符串使用 NextToken。`mshell_ParseHex` 只分派到无符号十六进制函数。

泛型实现约束：选择函数名后统一调用；实参只求值一次；不提供吞掉错误类型的 default 分支。不同时列出 `uint32_t *` 与可能兼容的 `unsigned int *`，避免重复兼容类型。typedef 别名不产生新的分派语义。泛型宏仅在 C11 C 编译下公开，其他环境可调用显式类型函数；不据此承诺整个 MODUS 的 C++ 兼容性。

### 3.3 统一解析契约

| 输入类别 | 规则 |
|---|---|
| 普通整数 | 十进制；前导 0 不表示八进制 |
| 有符号整数 | 允许一个前导 + 或 - |
| 无符号整数 | 不接受正负号 |
| 十六进制 | 允许裸十六进制及 0x/0X 前缀；至少一个数字；不接受符号 |
| bool | 精确接受 `true/false`、`on/off`、`1/0`，区分大小写 |
| 空输入 | 跳过空白后没有 token，返回 END |
| 非法 token | 如 `12abc`、`0x`，返回 INVALID |
| 超范围 | 合法数字文本超出目标类型，返回 RANGE |
| 成功 | 消费完整 token，游标停在其末尾分隔符或 NUL |
| 失败 | 本次调用的原游标和输出值均保持不变 |

先确定整个 token 的词法合法性；包含非法字符时返回 INVALID，即使其数字前缀已溢出。溢出检查必须发生在乘加之前；有符号最小值单独正确处理，避免对 INT_MIN 取负。

“失败不修改”针对单次解析调用。已经成功读取前一个参数后，后一个参数失败不会回滚整条命令；命令应先解析到局部变量，全部校验成功后再执行业务动作。

频率上限、地址允许范围、对象 ID 合法性、枚举含义仍由各命令判断。

### 3.4 命令回复

补充 `void mshell_Write(const char *text)`，复用当前 IO 写出 NUL 结尾文本，供 usage、解析错误和确认消息使用，不受日志等级控制。NULL 或尚无可用后端时不写出；不承诺异步发送完成或传输成功。

本轮不新增格式化输出引擎，也不全面重写现有命令输出。参数解析可以独立于 IO 测试。

## 4. 使用者看到的代码

通用示例 `check <uint32>`：

```c
static void cmd_check(const char *args)
{
    const char *cursor = args;
    uint32_t value = 0U;
    mshell_parse_result_t result = mshell_Parse(&cursor, &value);

    if (result != MSHELL_PARSE_OK || !mshell_AtEnd(cursor)) {
        mshell_Write("Usage: check <uint32>\r\n");
        return;
    }

    if (value == 0U) {
        mshell_Write("zero\r\n");
    } else {
        mshell_Write("nonzero\r\n");
    }
}

MODUS_SHELL_CMD(check, cmd_check, "Check an unsigned decimal integer");
```

命令作者只保留取参数、业务判断与业务动作，不再维护数字扫描、溢出判断、分隔符识别。带子命令时调用 ConsumeWord，字符串参数调用 NextToken。

## 5. 文件与实施顺序

MODUS 文件路径相对 MODUS 根目录；应用迁移路径相对 `M448_master` 根目录。

| 文件 | 工作 |
|---|---|
| 新增 `src/mdebug/mshell_parse.h` | 结果类型、token、叶子 API 和泛型宏 |
| 新增 `src/mdebug/mshell_parse.c` | 无 IO、无硬件依赖的解析实现 |
| 修改 `src/mdebug/mshell.h` | 在启用 MShell 时包含解析头，公开 Write |
| 修改 `src/mdebug/mshell.c` | Write、post 迁移、超长行与 Tab 处理 |
| 修改 `src/arch/cortex-m/mdebug_cm.c` | peek/poke/stack 复用解析 API |
| 修改 `src/arch/riscv/mdebug_riscv.c` | 同步迁移架构命令 |
| 修改 `modus.mk` | 按现有 MShell 开关纳入解析源文件 |
| 新增 `src/mdebug/tests/run_tests.py` | 独立主机测试入口，参考 MDI 测试组织 |
| 新增 `src/mdebug/tests/test_mshell_parse.c` | 解析语义与边界测试 |
| 新增 `src/mdebug/tests/test_mshell_io.c` | 假 IO 下收行与回复测试 |
| 新增 `src/mdebug/tests/stubs/mdebug/util_debug.h` | 隔离 Shell IO 测试的日志依赖 |
| 修改 `docs/mdebug/mshell.md` | API 契约、命令编写例子、迁移说明 |
| 修改 `M448_master/class/mircoelec.c` | DAC 命令改用公共解析 API，删除四个私有解析函数 |
| 修改 `M448_master/class/tests/fake/mdebug/mshell.h` | 暴露框架解析头给 Class 单元测试 |
| 修改 `M448_master/class/tests/run.ps1` | DAC Class 测试链接公共解析实现 |
| 修改 `M448_master/class/tests/business_test.c` | 验证初始化时 DAC 仍关闭 |

### 阶段一：固定契约，完成纯解析模块

- [x] 建立主机测试入口，使用临时构建目录，不依赖 M448 配置或芯片头文件。
- [x] 实现 token、词匹配、尾部检查；覆盖空输入、空白和前缀误匹配。
- [x] 实现整数、bool 和显式十六进制；共享私有扫描逻辑，保持公开函数类型明确。
- [x] 增加泛型宏，验证各输出类型分派、不支持类型编译失败、实参无重复求值。

### 阶段二：接入 MShell，先迁移框架消费者

- [x] 加入构建，公开 Write，保留现有回调与注册方式。
- [x] post 使用十六进制解析；保留 ID/event 非零要求，额外拒绝垃圾尾缀、溢出和多余参数。
- [x] 两种架构的 peek/poke 使用十六进制解析；所有参数验证完毕后才能读写内存。
- [x] stack 使用十进制解析：保留无参数默认深度和既有上限处理，拒绝原先误接受的非法文本。
- [x] log 的既有选项语法保持兼容；本轮不强行把组合选项改为 bool 参数。
- [x] 接收 Tab；超长行进入丢弃状态直到换行，报告一次错误且不执行截断前缀；下一行正常接收。

### 阶段三：文档与验收

- [x] 文档写清返回值、失败原子性、token 生命周期和泛型边界。
- [x] 提供十进制、有符号、hex、bool、子命令、字符串 token 的独立例子。
- [x] 修正文档中固定 RTT 与缓冲区长度的过时描述，以 IO 配置和宏默认值为准。
- [x] 将 DAC 命令迁移到新 API，并移除 `_mircoelec_SkipSpaces`、
      `_mircoelec_ConsumeWord`、`_mircoelec_ParseFrequency` 和
      `_mircoelec_HasNoExtraArguments`。

## 6. 验收标准

实施时从 MODUS 根目录执行新增入口：

```text
python src/mdebug/tests/run_tests.py
```

测试矩阵：

- 各整数类型最小值、最大值、边界外一位和很长的数字；UINT64_MAX 与 INT64_MIN 必测。
- `12abc`、`0x`、`-1` 转 unsigned、单独符号、空字符串、多余参数；每次失败都检查游标和目标值未改变。
- `start` 与 `starter`、多空白、Tab、尾空白、只读字符串；bool 全部支持值和大小写拒绝规则。
- hex 裸值和带前缀值一致；普通解析 `010` 得到十进制 10。
- 泛型成功编译全部受支持类型；`float *` 等不支持类型必须编译失败；显式类型 API 不依赖泛型宏。
- 假 IO 验证正常行、CR/LF、退格、Tab、恰好装满与超长行；超长行回调次数为零，下行恢复。
- 日志掩码关闭时，调用 Write 仍通过选定的假 IO 输出；不硬编码 RTT。
- 主机测试不执行真实 peek/poke/stack 内存访问；架构消费者采用交叉编译检查或桩函数隔离。
- 后续按项目构建入口检查 MShell 开启/关闭的集成与体积变化；构建结果不能替代真机验证。

验收结果应同时展示：公共 API 可独立使用、框架重复解析已减少、非法输入被拒绝、原有正常命令兼容。实施期间运行下述主机测试和固件构建；这些结果用于验证代码，不替代真机验证。

已执行并通过：

```text
MODUS 根目录：python src/mdebug/tests/run_tests.py
M448_master：powershell -NoProfile -ExecutionPolicy Bypass -File class/tests/run.ps1 -Module mircoelec
M448_master：.\make.bat
```

主机测试覆盖整数边界、错误 token、bool/hex、失败不改输入、C11 泛型、
C99 与 C++ 显式 API、假 IO 和超长行恢复；Clang 交叉编译检查通过
Cortex-M 与 RV32 命令。Class 测试验证 DAC 初始化保持关闭。固件构建退出码
为 0；构建输出有一条既有路径提示，但编译、链接和产物生成均完成。

## 7. 暂不纳入首版

浮点、带引号与转义字符串、任意进制、命令参数描述表、自动生成 help、运行时类型注册均不纳入首版。后续有具体需求时沿用相同游标和错误契约扩展；不会因为采用 `_Generic` 就自动获得新的文本语法。
