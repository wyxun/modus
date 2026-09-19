#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include "example.h"
#include "template.h"
#include "mstorage.h"
#include "mdebug/util_debug.h"

/*============================ MSHELL 使用示例 (不影响编译) ===================*/
/*
 * mshell 已通过 modus_Run() 自动轮询，无需手动调用。
 * 只需在 modus_Init() 之后注册自定义命令：
 *
 *   #include "mdebug/mshell.h"
 *
 *   static void cmd_mytest(const char *args) {
 *       MLOGF(I, "args: %s\r\n", args);
 *   }
 *   static const mshell_cmd_t s_tCmdMyTest = {
 *       "mytest", cmd_mytest, "Run my test"
 *   };
 *
 *   modus_Init(&tModus);
 *   mshell_RegisterCmd(&s_tCmdMyTest);
 *
 * 运行期控制日志级别（通过 RTT Viewer 输入）：
 *   log -I      关闭/恢复 INFO 打印（toggle）
 *   log -D      关闭/恢复 DEBUG 打印（toggle）
 *   log -I -D   同时切换多个级别
 *   log         查看当前各级别开关状态
 *
 * g_chGLogMask 启动时默认值由 MLOG_MASK_DEFAULT 决定（默认全开 0x1F）；
 * 可在 userconfig.h 中覆盖（如仅保留 E+W：#define MLOG_MASK_DEFAULT 0x03u）。
 *
 * 替换为 UART 后端（非 RTT）：
 *   static unsigned uart_read(char *buf, unsigned size) { ... }  // 非阻塞
 *   static void uart_write(const char *buf, unsigned size) { ... }
 *   static const mshell_io_t s_tUartIO = { uart_read, uart_write };
 *   mshell_SetIO(&s_tUartIO);   // 在 modus_Init() 前或后均可
 */

/*============================ VIRTUAL FLASH FOR PC ==========================*/
static uint8_t s_chVirtualFlash[2048];

static int32_t pc_flash_erase(void *pPriv, uint32_t wAddr, uint32_t wSize)
{
    if (wAddr + wSize > sizeof(s_chVirtualFlash)) return -1;
    memset(&s_chVirtualFlash[wAddr], 0xFF, wSize);
    return 0;
}

static int32_t pc_flash_write(void *pPriv, uint32_t wAddr, const uint8_t *pchData, uint32_t wLen)
{
    if (wAddr + wLen > sizeof(s_chVirtualFlash)) return -1;
    memcpy(&s_chVirtualFlash[wAddr], pchData, wLen);
    return 0;
}

static int32_t pc_flash_read(void *pPriv, uint32_t wAddr, uint8_t *pchBuf, uint32_t wLen)
{
    if (wAddr + wLen > sizeof(s_chVirtualFlash)) return -1;
    memcpy(pchBuf, &s_chVirtualFlash[wAddr], wLen);
    return 0;
}

static int32_t pc_flash_unlock(void *pPriv) { return 0; }
static int32_t pc_flash_lock(void *pPriv)   { return 0; }

static mdi_flash_t s_tFlash = {
    .pPriv    = NULL,
    .fnErase  = pc_flash_erase,
    .fnWrite  = pc_flash_write,
    .fnRead   = pc_flash_read,
    .fnUnlock = pc_flash_unlock,
    .fnLock   = pc_flash_lock,
};

/*============================ STORAGE OBJECT ================================*/
/* Note: mstorage appends 2 bytes of CRC at the end of the buffer */
uint16_t g_hwSystemDataArrary[16 + 1] = {0};

mstorage_data_t tSysData = {
    .ptFlash             = &s_tFlash,
    .wFlashAddr          = 0,
    .pchStorageStartAddr = (uint8_t *)g_hwSystemDataArrary,
    .hwStorageLength     = 16 * sizeof(uint16_t),
};

MODUS_DECLARE_OBJECT(mstorage, MStorage, 
    .ptStorageObject  = &tSysData,
    .hwStorageTimeOut = 100, // 100ms cycle
);

/* GLOBAL MODUS CONFIGURATION */
modus_t tModus = {
    .ptAppFlash = &s_tFlash,
};

int main()
{   
    // example hardware init
    MLOG(I, "Starting Template Example...\n");
    
    modus_Init(&tModus);
    MLOG(I, "Entering main loop.\n");
    while (1)
    {
        modus_Run();
    }

    return MODUS_SUCCESS;
}

void timer_handler(int signum)
{
    modus_Clock();
}
