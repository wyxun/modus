#ifndef __MODUS_MSHELL_H__
#define __MODUS_MSHELL_H__

#include <stdint.h>
#include <stdbool.h>
#include "global_define.h"

#ifdef MODUS_CFG_USER_CONFIG_INCLUSION
#   include MODUS_CFG_USER_CONFIG_INCLUSION
#endif


/*============================ COMPILE-TIME SWITCH ===========================*/

#ifndef MSHELL_ENABLE
#   define MSHELL_ENABLE            1
#endif

#if MSHELL_ENABLE

#include "mshell_parse.h"

/*============================ MACROS ========================================*/

/* 命令行缓冲区大小（含 '\0'），可在 userconfig.h 中覆盖 */
#ifndef MSHELL_LINE_SIZE
#   define MSHELL_LINE_SIZE     64
#endif

/* 最大注册命令数（含内置命令），可在 userconfig.h 中覆盖 */
#ifndef MSHELL_MAX_CMDS
#   define MSHELL_MAX_CMDS      16
#endif

/* 为 0 时不编译 RTT 默认后端，需手动调用 mshell_SetIO() */
#ifndef MSHELL_USE_RTT_DEFAULT
#   define MSHELL_USE_RTT_DEFAULT   1
#endif

/* 自动注册命令宏：在任何 .c 文件中使用，无需手动初始化。
 * 它会在 init_infos 章节中产生一个初始化项，由 modus_Init() 自动触发注册。
 */
#define MODUS_SHELL_CMD(name, handler, help_str)                                \
    static const mshell_cmd_t shell_cmd_##name = {                              \
        .pchName = #name,                                                       \
        .pfcnHandle = (handler),                                                \
        .pchHelp = (help_str),                                                  \
    };                                                                          \
    INIT_SECTION const modus_init_info_t shell_init_##name = {                  \
        .pfcnInitFunc = (init_func_t)mshell_RegisterCmd,                        \
        .wObjectAddr = (uintptr_t)&shell_cmd_##name,                            \
        .wConfigAddr = 0                                                        \
    }

/*============================ TYPES =========================================*/

/**
 * @brief mshell I/O 后端接口
 *   pfcnRead  — 非阻塞读，无数据返回 0
 *   pfcnWrite — 输出 hwSize 字节
 */
typedef struct {
    unsigned (*pfcnRead) (char *pchBuf, unsigned hwSize);
    void     (*pfcnWrite)(const char *pchBuf, unsigned hwSize);
} mshell_io_t;

/**
 * @brief 命令处理回调，args 为命令名后的参数字符串（已去前导空格），
 *        无参数时为空字符串 ""
 */
typedef void (*mshell_cmd_handler_t)(const char *args);

typedef struct {
    const char           *pchName;    /**< 命令名，如 "burn"           */
    mshell_cmd_handler_t  pfcnHandle; /**< 命令处理回调                 */
    const char           *pchHelp;    /**< 单行帮助文本                 */
} mshell_cmd_t;

/*============================ PROTOTYPES ====================================*/

/**
 * @brief 设置 I/O 后端（不调用则默认使用 RTT）
 * @param ptIO 指向静态/全局 mshell_io_t，生命周期须覆盖整个运行周期
 */
void mshell_SetIO(const mshell_io_t *ptIO);

/**
 * @brief Write a NUL-terminated string through the configured Shell backend.
 * @param pchText Text to write; NULL is ignored.
 * @return None.
 */
void mshell_Write(const char *pchText);

/**
 * @brief 注册用户自定义命令
 *        须在 modus_Init() 后、mshell_Poll() 首次调用前调用
 * @return MODUS_SUCCESS 或 MODUS_ENOMEM（命令表已满）
 */
int mshell_RegisterCmd(uintptr_t wAddr, uintptr_t wUnused);

/**
 * @brief 轮询 I/O 并处理命令；由 modus_Run() 内部自动调用，用户无需手动调用
 */
void mshell_Poll(void);

#else /* MSHELL_ENABLE == 0 */

/* 编译空桩：不生成任何 init 条目，mshell 代码被 gc-sections 自动剥离 */
#define MODUS_SHELL_CMD(name, handler, help_str)

#endif /* MSHELL_ENABLE */

#endif /* __MODUS_MSHELL_H__ */
