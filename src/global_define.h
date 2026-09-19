#ifndef __GLOBAL_DEFINE_H__
#define __GLOBAL_DEFINE_H__

/*============================ MODUS VERSION ==================================*/
/* MODUS_VERSION encodes the four-part release as {purpose, interface, major,
 * minor}; the current release is 0.6.1.1. */
#define GENERAL_PURPOSE                 0               //!< General purpose 
#define MODUS_PURPOSE                    GENERAL_PURPOSE //!< MODUS purpose   
#define MODUS_INTERFACE_VERSION          6               //!< MODUS interface version
#define MODUS_MAJOR_VERSION              1               //!< MODUS major version
#define MODUS_MINOR_VERSION              1               //!< MODUS patch/minor version

#define MODUS_VERSION                 {                                         \
                                        MODUS_PURPOSE,                          \
                                        MODUS_INTERFACE_VERSION,                \
                                        MODUS_MAJOR_VERSION,                    \
                                        MODUS_MINOR_VERSION,                    \
                                    }


// Include necessary libraries
#include <stdbool.h>
#include <stdint.h>

// Define types
#define MODUS_ID_BASEOFFSET      8
// HARDWARE
#define MODUS_ID_UART        1
#define MODUS_ID_IIC         2
#define MODUS_ID_SPI         3
#define MODUS_ID_CAN         4
#define MODUS_ID_INPUT       5
#define MODUS_ID_SENSOR      6
#define MODUS_ID_TIMER       7
#define MODUS_ID_IO          8
#define MODUS_ID_STORAGE     9
#define MODUS_ID_BLM         10
// SOFTWARE
#define MODUS_ID_CLASS       100
#define MODUS_ID_MOCK        101

// Define error codes
#define MODUS_SUCCESS        0
#define MODUS_EPERM          -1
#define MODUS_ENOENT         -2
#define MODUS_ESRCH          -3  
#define MODUS_EINTR          -4
#define MODUS_EIO            -5
#define MODUS_ENXIO          -6
#define MODUS_E2BIG          -7
#define MODUS_EINVAL         -8
#define MODUS_EBADF          -9
#define MODUS_ECHILD         -10
#define MODUS_EAGAIN         -11
#define MODUS_ENOMEM         -12
#define MODUS_EACCES         -13
#define MODUS_EFAULT         -14
#define MODUS_ENOTBLK        -15
#define MODUS_EBUSY          -16
#define MODUS_EEXIST         -17
#define MODUS_EXDEV          -18
#define MODUS_ENODEV         -19
#define MODUS_EFAIL          -20
#define MODUS_EMAX           -21

// Define events
typedef enum {
    Modus_Event00 = 1 << 0,
    Modus_Event01 = 1 << 1,
    Modus_Event02 = 1 << 2,
    Modus_Event03 = 1 << 3,
    Modus_Event04 = 1 << 4,
    Modus_Event05 = 1 << 5,
    Modus_Event06 = 1 << 6,
    Modus_Event07 = 1 << 7,
    Modus_Event08 = 1 << 8,
    Modus_Event09 = 1 << 9,
    Modus_Event10 = 1 << 10,
    Modus_Event11 = 1 << 11,
    Modus_Event12 = 1 << 12,
    Modus_Event13 = 1 << 13,
    Modus_Event14 = 1 << 14,
    Modus_Event15 = 1 << 15,
    Modus_Event16 = 1 << 16,
    Modus_Event17 = 1 << 17,
    Modus_Event18 = 1 << 18,
    Modus_Event19 = 1 << 19,
    Modus_Event20 = 1 << 20,
    Modus_Event21 = 1 << 21,
    Modus_Event22 = 1 << 22,
    Modus_Event23 = 1 << 23,
    Modus_Event24 = 1 << 24,
    Modus_Event25 = 1 << 25,
    Modus_Event26 = 1 << 26,
    Modus_Event27 = 1 << 27,
    Modus_Event28 = 1 << 28,
    Modus_Event29 = 1 << 29,
    Modus_Event_Timeout = 1 << 30,
    Modus_Event_Transition = (uint32_t)(1 << 31),
} Modus_Event;

// Define MODUS IDs
#define MODUS_STORAGE        ((MODUS_ID_STORAGE<<8)+1)
typedef enum {
    Event_Storage      = Modus_Event00,  /**< 保存：把当前 RAM 数据写入 Flash              */
    Event_GetData      = Modus_Event01,  /**< 加载：从 Flash 读回数据到 RAM                */
    Event_ResetDefault = Modus_Event02,  /**< 重置：只清空 Flash，不动 RAM，重启后生效默认值 */
} MODUS_STORAGE_Event;
#define MODUS_INPUT          ((MODUS_ID_IO<<8)+1)

#define GET_OBJECT_POINT(OBJECT, ADDR)    (OBJECT *)(ADDR)

// MBLINFO shared memory address
#define MBLINFO_SHARED_ADDR     0x08002000
#define MBLINFO_SHARED_SIZE     0x400       // 1KB
#define MBLINFO_MAGIC           0x424C4946  // "BLIF"

#define MODUS_MBLINFO            ((MODUS_ID_BLM<<8)+1)

/*============================ AUTO INIT =====================================*/
#if defined(__clang__) || defined(__ARMCOMPILER_VERSION) || defined(__ARMCC_VERSION)
  #if defined(__has_attribute)
    #if __has_attribute(section)
      #define INIT_SECTION __attribute__((section("init_infos"), used))
    #else
      #define INIT_SECTION __attribute__((section("init_infos"), used))
    #endif
  #else
    /* assume section attribute exists on clang/armclang when __has_attribute is not available */
    #define INIT_SECTION __attribute__((section("init_infos"), used))
  #endif
#elif defined(__GNUC__)
  #define INIT_SECTION __attribute__((section("init_infos"), used))
#else
  #define INIT_SECTION
#endif

typedef int (*init_func_t)(uintptr_t wObjectAddr, uintptr_t wObjectCfgAddr);

typedef struct {
    init_func_t pfcnInitFunc;
    uintptr_t wObjectAddr;
    uintptr_t wConfigAddr;
} modus_init_info_t;

#if defined(__riscv)
#undef __disable_irq
#define __disable_irq()   __asm__ __volatile__("csrc mstatus, 8")
#undef __enable_irq
#define __enable_irq()    __asm__ __volatile__("csrs mstatus, 8")
#endif

#endif
