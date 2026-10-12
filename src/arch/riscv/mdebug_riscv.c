#include "mdebug_riscv.h"
#include "mdebug/mshell.h"
#include "mdebug/util_debug.h"
#include <stdint.h>
#include <stdbool.h>

#if defined(__riscv)

/* 由 Linker script 导出的内存范围符号 */
extern uint8_t __flash_start[];
extern uint8_t __flash_end[];
extern uint8_t __sram_start[];
extern uint8_t __sram_end[];

/*============================ INLINE ASM HELPERS ==============================*/

static uintptr_t get_sp(void)
{
    uintptr_t r;
    __asm__ volatile ("mv %0, sp" : "=r" (r));
    return r;
}

static uintptr_t get_ra(void)
{
    uintptr_t r;
    __asm__ volatile ("mv %0, ra" : "=r" (r));
    return r;
}

static uintptr_t get_fp(void)
{
    uintptr_t r;
    __asm__ volatile ("mv %0, s0" : "=r" (r));
    return r;
}

static uint32_t get_mstatus(void)
{
    uint32_t r;
    __asm__ volatile ("csrr %0, mstatus" : "=r" (r));
    return r;
}

static uint32_t get_mcause(void)
{
    uint32_t r;
    __asm__ volatile ("csrr %0, mcause" : "=r" (r));
    return r;
}

static uint32_t get_mepc(void)
{
    uint32_t r;
    __asm__ volatile ("csrr %0, mepc" : "=r" (r));
    return r;
}

static uint32_t get_mtval(void)
{
    uint32_t r;
    __asm__ volatile ("csrr %0, mtval" : "=r" (r));
    return r;
}

static uint32_t get_mie(void)
{
    uint32_t r;
    __asm__ volatile ("csrr %0, mie" : "=r" (r));
    return r;
}

static uint32_t get_mip(void)
{
    uint32_t r;
    __asm__ volatile ("csrr %0, mip" : "=r" (r));
    return r;
}

/*============================ PRIVATE FUNCTIONS ==============================*/

/*-- Address range helpers ----------------------------------------------------*/

static bool is_likely_code_addr(uint32_t wAddr)
{
    uintptr_t flash_start = (uintptr_t)__flash_start;
    uintptr_t flash_end   = (uintptr_t)__flash_end;
    return (wAddr >= flash_start && wAddr < flash_end);
}

static bool is_valid_sram_addr(uintptr_t wAddr)
{
    uintptr_t sram_start = (uintptr_t)__sram_start;
    uintptr_t sram_end   = (uintptr_t)__sram_end;
    return (wAddr >= sram_start && wAddr < (sram_end - 4));
}

/*-- regs command: dump core registers ----------------------------------------*/

static void cmd_regs(const char *args)
{
    (void)args;

    uintptr_t wSp      = get_sp();
    uintptr_t wRa      = get_ra();
    uintptr_t wFp      = get_fp();
    uint32_t  wMstatus = get_mstatus();
    uint32_t  wMcause  = get_mcause();
    uint32_t  wMepc    = get_mepc();
    uint32_t  wMtval   = get_mtval();
    uint32_t  wMie     = get_mie();
    uint32_t  wMip     = get_mip();

    MLOG(I, "\r\n=== RISC-V Core Registers ===\r\n");
    MLOGF(I, "SP (x2)  = 0x%08X\r\n", (unsigned)wSp);
    MLOGF(I, "RA (x1)  = 0x%08X\r\n", (unsigned)wRa);
    MLOGF(I, "FP (x8)  = 0x%08X\r\n", (unsigned)wFp);
    MLOGF(I, "MSTATUS  = 0x%08X (MIE=%u, MPIE=%u)\r\n",
          (unsigned)wMstatus,
          (unsigned)((wMstatus >> 3) & 1),
          (unsigned)((wMstatus >> 7) & 1));
    MLOGF(I, "MCAUSE   = 0x%08X (Interrupt=%u, ExceptionCode=%u)\r\n",
          (unsigned)wMcause,
          (unsigned)((wMcause >> 31) & 1),
          (unsigned)(wMcause & 0x7FFFFFFF));
    MLOGF(I, "MEPC     = 0x%08X\r\n", (unsigned)wMepc);
    MLOGF(I, "MTVAL    = 0x%08X\r\n", (unsigned)wMtval);
    MLOGF(I, "MIE      = 0x%08X\r\n", (unsigned)wMie);
    MLOGF(I, "MIP      = 0x%08X\r\n", (unsigned)wMip);
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

static void cmd_stack(const char *args)
{
    uintptr_t wSp = get_sp();
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

    MLOGF(I, "\r\nSP(now) = 0x%08X, dump %u words:\r\n",
          (unsigned)wSp, (unsigned)wDepth);

    for (unsigned i = 0; i < wDepth; i++) {
        uintptr_t wAddr = wSp + i * 4;
        uint32_t wVal  = *(volatile uint32_t *)wAddr;

        if (i == 0) {
            MLOGF(I, "  +0x%04X  0x%08X  0x%08X <-- SP\r\n",
                  (unsigned)(i * 4), (unsigned)wAddr, (unsigned)wVal);
        } else if (is_likely_code_addr(wVal)) {
            MLOGF(I, "  +0x%04X  0x%08X  0x%08X <-- RA/PC?\r\n",
                  (unsigned)(i * 4), (unsigned)wAddr, (unsigned)wVal);
        } else {
            MLOGF(I, "  +0x%04X  0x%08X  0x%08X\r\n",
                  (unsigned)(i * 4), (unsigned)wAddr, (unsigned)wVal);
        }
    }
}

/*-- trap command: Configurable Trap/Fault Status -----------------------------*/

static void cmd_trap(const char *args)
{
    (void)args;
    uint32_t wMcause = get_mcause();
    uint32_t wMepc   = get_mepc();
    uint32_t wMtval  = get_mtval();

    MLOGF(I, "\r\n=== RISC-V Exception CSR Status ===\r\n");
    MLOGF(I, "MCAUSE = 0x%08X  MEPC = 0x%08X  MTVAL = 0x%08X\r\n",
          (unsigned)wMcause, (unsigned)wMepc, (unsigned)wMtval);

    bool bIsInterrupt = (wMcause >> 31) & 1;
    uint32_t wCode = wMcause & 0x7FFFFFFF;

    if (bIsInterrupt) {
        MLOGF(I, "Type: Asynchronous Interrupt\r\n");
        switch (wCode) {
            case 3:  MLOG(I, "  Cause: Machine Software Interrupt (MSI)\r\n"); break;
            case 7:  MLOG(I, "  Cause: Machine Timer Interrupt (MTI)\r\n"); break;
            case 11: MLOG(I, "  Cause: Machine External Interrupt (MEI)\r\n"); break;
            default: MLOGF(I, "  Cause: Reserved Interrupt (%u)\r\n", (unsigned)wCode); break;
        }
    } else {
        MLOGF(E, "Type: Synchronous Exception (Fault)\r\n");
        switch (wCode) {
            case 0:  MLOG(E, "  Instruction address misaligned\r\n"); break;
            case 1:  MLOG(E, "  Instruction access fault\r\n"); break;
            case 2:  MLOG(E, "  Illegal instruction\r\n"); break;
            case 3:  MLOG(E, "  Breakpoint (ebreak)\r\n"); break;
            case 4:  MLOG(E, "  Load address misaligned\r\n"); break;
            case 5:  MLOG(E, "  Load access fault\r\n"); break;
            case 6:  MLOG(E, "  Store/AMO address misaligned\r\n"); break;
            case 7:  MLOG(E, "  Store/AMO access fault\r\n"); break;
            case 8:  MLOG(E, "  Environment call from U-mode (ecall)\r\n"); break;
            case 11: MLOG(E, "  Environment call from M-mode (ecall)\r\n"); break;
            case 12: MLOG(E, "  Instruction page fault\r\n"); break;
            case 13: MLOG(E, "  Load page fault\r\n"); break;
            case 15: MLOG(E, "  Store/AMO page fault\r\n"); break;
            default: MLOGF(E, "  Reserved / Custom Fault (%u)\r\n", (unsigned)wCode); break;
        }
    }
}

/*============================ PUBLIC FUNCTIONS ==============================*/

void mdebug_riscv_DumpException(uintptr_t sp, uint32_t mepc, uint32_t mcause, uint32_t mtval)
{
    MLOG(E, "\r\n");
    MLOG(E, "========================================\r\n");
    MLOG(E, "      !! RISC-V HARDWARE FAULT !!       \r\n");
    MLOG(E, "========================================\r\n");

    MLOGF(E, "MCAUSE   = 0x%08X  (Exception Code: %u)\r\n",
          (unsigned)mcause, (unsigned)(mcause & 0x7FFFFFFF));
    MLOGF(E, "MEPC(PC) = 0x%08X\r\n", (unsigned)mepc);
    MLOGF(E, "MTVAL    = 0x%08X\r\n", (unsigned)mtval);
    MLOGF(E, "SP (x2)  = 0x%08X\r\n", (unsigned)sp);

    uint32_t wCode = mcause & 0x7FFFFFFF;
    MLOG(E, "\r\n[Fault Decode]:\r\n");
    switch (wCode) {
        case 0:  MLOG(E, "  Instruction address misaligned\r\n"); break;
        case 1:  MLOG(E, "  Instruction access fault\r\n"); break;
        case 2:  MLOGF(E, "  Illegal instruction (opcode: 0x%08X)\r\n", (unsigned)mtval); break;
        case 3:  MLOG(E, "  Breakpoint (ebreak)\r\n"); break;
        case 4:  MLOGF(E, "  Load address misaligned (addr: 0x%08X)\r\n", (unsigned)mtval); break;
        case 5:  MLOGF(E, "  Load access fault (addr: 0x%08X)\r\n", (unsigned)mtval); break;
        case 6:  MLOGF(E, "  Store/AMO address misaligned (addr: 0x%08X)\r\n", (unsigned)mtval); break;
        case 7:  MLOGF(E, "  Store/AMO access fault (addr: 0x%08X)\r\n", (unsigned)mtval); break;
        case 12: MLOG(E, "  Instruction page fault\r\n"); break;
        case 13: MLOG(E, "  Load page fault\r\n"); break;
        case 15: MLOG(E, "  Store/AMO page fault\r\n"); break;
        default: MLOGF(E, "  Unknown / Custom Fault (Code: %u)\r\n", (unsigned)wCode); break;
    }

    MLOG(E, "\r\n[Stack Context (Around SP)]:\r\n");
    for (int i = 0; i < 16; i++) {
        uintptr_t wAddr = sp + i * 4;
        if (is_valid_sram_addr(wAddr)) {
            uint32_t wVal = *(volatile uint32_t *)wAddr;
            if (i == 0) {
                MLOGF(E, "  sp+0x%02X [0x%08X] = 0x%08X\r\n",
                      i*4, (unsigned)wAddr, (unsigned)wVal);
            } else if (is_likely_code_addr(wVal)) {
                MLOGF(E, "  sp+0x%02X [0x%08X] = 0x%08X <-- RA/PC?\r\n",
                      i*4, (unsigned)wAddr, (unsigned)wVal);
            } else {
                MLOGF(E, "  sp+0x%02X [0x%08X] = 0x%08X\r\n",
                      i*4, (unsigned)wAddr, (unsigned)wVal);
            }
        }
    }

    MLOG(E, "========================================\r\n");
}

/*============================ PRIVATE DATA ===================================*/

MODUS_SHELL_CMD(regs,  cmd_regs,  "Dump RISC-V core registers (SP, RA, FP, CSRs)");
MODUS_SHELL_CMD(peek,  cmd_peek,  "Read memory word: peek <hex_addr>");
MODUS_SHELL_CMD(poke,  cmd_poke,  "Write memory word: poke <hex_addr> <hex_val>");
MODUS_SHELL_CMD(stack, cmd_stack, "Dump stack memory: stack [depth_words]");
MODUS_SHELL_CMD(trap,  cmd_trap,  "Show current trap cause and status registers");

#endif /* defined(__riscv) */
