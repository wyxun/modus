#include "mdebug_cm.h"
#include "mdebug/mshell.h"
#include "mdebug/util_debug.h"
#include <stdint.h>
#include <stdbool.h>

#if defined(__ARM_ARCH)

/*============================ INLINE ASM HELPERS ==============================*/

static uint32_t get_msp(void)
{
    uint32_t r;
    __asm__ volatile ("mrs %0, msp" : "=r" (r));
    return r;
}

static uint32_t get_psp(void)
{
    uint32_t r;
    __asm__ volatile ("mrs %0, psp" : "=r" (r));
    return r;
}

static uint32_t get_control(void)
{
    uint32_t r;
    __asm__ volatile ("mrs %0, control" : "=r" (r));
    return r;
}

static uint32_t get_primask(void)
{
    uint32_t r;
    __asm__ volatile ("mrs %0, primask" : "=r" (r));
    return r;
}

static uint32_t get_faultmask(void)
{
    uint32_t r;
    __asm__ volatile ("mrs %0, faultmask" : "=r" (r));
    return r;
}

static uint32_t get_basepri(void)
{
    uint32_t r;
    __asm__ volatile ("mrs %0, basepri" : "=r" (r));
    return r;
}

/*============================ PRIVATE FUNCTIONS ==============================*/

/*-- regs command: dump core registers ----------------------------------------*/

static void cmd_regs(const char *args)
{
    (void)args;

    uint32_t wMsp     = get_msp();
    uint32_t wPsp     = get_psp();
    uint32_t wControl = get_control();
    uint32_t wPrimask = get_primask();
    uint32_t wFaultmask = get_faultmask();
    uint32_t wBasepri = get_basepri();
    bool     bUsingPsp = (wControl & 2) != 0;
    uint32_t wSp = bUsingPsp ? wPsp : wMsp;

    MLOG(I, "\r\n=== Cortex-M Core Registers ===\r\n");
    MLOGF(I, "MSP     = 0x%08X\r\n", (unsigned)wMsp);
    MLOGF(I, "PSP     = 0x%08X\r\n", (unsigned)wPsp);
    MLOGF(I, "SP(now) = 0x%08X (%s)\r\n",
          (unsigned)wSp, bUsingPsp ? "PSP" : "MSP");
    MLOGF(I, "CONTROL  = 0x%08X (SPSEL=%u, nPRIV=%u)\r\n",
          (unsigned)wControl,
          (unsigned)((wControl >> 1) & 1),
          (unsigned)(wControl & 1));
    MLOGF(I, "PRIMASK   = 0x%08X\r\n", (unsigned)wPrimask);
    MLOGF(I, "FAULTMASK = 0x%08X\r\n", (unsigned)wFaultmask);
    MLOGF(I, "BASEPRI   = 0x%08X\r\n", (unsigned)wBasepri);

    uint32_t wLR;
    __asm__ volatile ("mov %0, lr" : "=r" (wLR));
    MLOGF(I, "LR      = 0x%08X\r\n", (unsigned)wLR);
}

/*-- peek command: read uint32 at any memory address ---------------------------*/

static void cmd_peek(const char *args)
{
    uintptr_t wAddr = 0U;
    const char *p = args;

    if (mshell_ParseHex(&p, &wAddr) != MSHELL_PARSE_OK ||
        !mshell_AtEnd(p)) {
        MLOGF(W, "Usage: peek <hex_addr>\r\n");
        return;
    }

    uint32_t wVal = *(volatile uint32_t *)(uintptr_t)wAddr;
    MLOGF(I, "@0x%08X = 0x%08X (%u)\r\n",
          (unsigned)wAddr, (unsigned)wVal, (unsigned)wVal);
}

/*-- poke command: write uint32 to any memory address -------------------------*/

static void cmd_poke(const char *args)
{
    uintptr_t wAddr = 0U;
    uint32_t wVal = 0U;
    const char *p = args;
    if (mshell_ParseHex(&p, &wAddr) != MSHELL_PARSE_OK ||
        mshell_ParseHex(&p, &wVal) != MSHELL_PARSE_OK ||
        !mshell_AtEnd(p)) {
        MLOGF(W, "Usage: poke <hex_addr> <hex_val>\r\n");
        return;
    }

    *(volatile uint32_t *)wAddr = wVal;
    MLOGF(I, "@0x%08X <- 0x%08X\r\n", (unsigned)wAddr, (unsigned)wVal);
}

/*-- stack command: dump stack memory around current SP ------------------------*/

static bool is_likely_code_addr(uint32_t wAddr)
{
    return (wAddr >= 0x08000000u && wAddr <= 0x080FFFFFu);
}

static void cmd_stack(const char *args)
{
    uint32_t wMsp     = get_msp();
    uint32_t wPsp     = get_psp();
    uint32_t wControl = get_control();
    bool     bUsingPsp = (wControl & 2) != 0;
    uint32_t wSp = bUsingPsp ? wPsp : wMsp;

    uint32_t wDepth = 16U;
    const char *p = args;
    if (!mshell_AtEnd(p)) {
        if (mshell_Parse(&p, &wDepth) != MSHELL_PARSE_OK ||
            !mshell_AtEnd(p)) {
            MLOGF(W, "Usage: stack [depth_words]\r\n");
            return;
        }
        if (wDepth > 256) wDepth = 256;
    }

    MLOGF(I, "\r\nSP(now) = 0x%08X (%s), dump %u words:\r\n",
          (unsigned)wSp, bUsingPsp ? "PSP" : "MSP", (unsigned)wDepth);

    for (unsigned i = 0; i < wDepth; i++) {
        uint32_t wAddr = wSp + i * 4;
        uint32_t wVal  = *(volatile uint32_t *)wAddr;

        if (i == 0) {
            MLOGF(I, "  +0x%04X  0x%08X  0x%08X <-- SP\r\n",
                  (unsigned)(i * 4), (unsigned)wAddr, (unsigned)wVal);
        } else if (is_likely_code_addr(wVal)) {
            MLOGF(I, "  +0x%04X  0x%08X  0x%08X <-- LR/PC?\r\n",
                  (unsigned)(i * 4), (unsigned)wAddr, (unsigned)wVal);
        } else {
            MLOGF(I, "  +0x%04X  0x%08X  0x%08X\r\n",
                  (unsigned)(i * 4), (unsigned)wAddr, (unsigned)wVal);
        }
    }
}

/*-- cfsr command: Configurable Fault Status Register -------------------------*/

static void cmd_cfsr(const char *args)
{
    (void)args;
    uint32_t wCfsr = (*((volatile uint32_t *)0xE000ED28u));
    uint32_t wHfsr = (*((volatile uint32_t *)0xE000ED2Cu));
    uint32_t wMmar = (*((volatile uint32_t *)0xE000ED34u));
    uint32_t wBfar = (*((volatile uint32_t *)0xE000ED38u));

    MLOGF(I, "\r\n=== Fault Status Registers ===\r\n");
    MLOGF(I, "CFSR = 0x%08X (UFSR=%02X BFSR=%02X MMFSR=%02X)\r\n",
          (unsigned)wCfsr,
          (unsigned)((wCfsr >> 16) & 0xFF),
          (unsigned)((wCfsr >> 8) & 0xFF),
          (unsigned)(wCfsr & 0xFF));
    MLOGF(I, "HFSR = 0x%08X\r\n", (unsigned)wHfsr);
    MLOGF(I, "MMAR = 0x%08X\r\n", (unsigned)wMmar);
    MLOGF(I, "BFAR = 0x%08X\r\n", (unsigned)wBfar);

    if (wCfsr & 1)         MLOGF(E, "  MMFSR: Instruction access violation\r\n");
    if (wCfsr & 2)         MLOGF(E, "  MMFSR: Data access violation\r\n");
    if (wCfsr & 0x80)      MLOGF(E, "  MMFSR: Unstacking error\r\n");
    if (wCfsr & 0x100)     MLOGF(E, "  BFSR: Bus fault on instruction fetch\r\n");
    if (wCfsr & 0x200)     MLOGF(E, "  BFSR: Bus fault on data access\r\n");
    if (wCfsr & 0x10000)   MLOGF(E, "  UFSR: Undefined instruction\r\n");
    if (wCfsr & 0x20000)   MLOGF(E, "  UFSR: Invalid state\r\n");
    if (wCfsr & 0x40000)   MLOGF(E, "  UFSR: InvPC load\r\n");
    if (wCfsr & 0x80000)   MLOGF(E, "  UFSR: No coprocessor\r\n");
    if (wCfsr & 0x1000000) MLOGF(E, "  UFSR: Divide by zero\r\n");
    if (wHfsr & 0x40000000) MLOGF(E, "  HFSR: Forced hard fault\r\n");
}

/*============================ PRIVATE DATA ===================================*/

MODUS_SHELL_CMD(regs,  cmd_regs,  "Dump core registers (MSP/PSP/CONTROL/PRIMASK/FAULTMASK/BASEPRI/LR)");
MODUS_SHELL_CMD(peek,  cmd_peek,  "Read memory word: peek <hex_addr>");
MODUS_SHELL_CMD(poke,  cmd_poke,  "Write memory word: poke <hex_addr> <hex_val>");
MODUS_SHELL_CMD(stack, cmd_stack, "Dump stack memory: stack [depth_words]");
MODUS_SHELL_CMD(cfsr,  cmd_cfsr,  "Show fault status registers (CFSR/HFSR/MMAR/BFAR)");

#endif /* defined(__ARM_ARCH) */
