/**
 * @file mdi_types.h
 * @brief GDI (MODUS Driver Interface) - 外设行为结构体定义
 *
 * 定义 GPIO / PWM / Stream / ADC / IIC / SPI 六类外设的标准化结构体。
 * 所有结构体包含 void *pPriv 指针用于底层传参。
 *
 * @note 不包含任何 OS 相关头文件，纯裸机设计。
 */
#ifndef __MDI_TYPES_H__
#define __MDI_TYPES_H__

#include <stdint.h>
#include <stdbool.h>

#ifndef MDI_LEGACY_STATUS_T
#define MDI_LEGACY_STATUS_T
typedef int32_t mdi_legacy_status_t;
#endif

/*============================================================================
 * GPIO (控制类)
 *===========================================================================*/

/**
 * @brief GPIO 电平枚举
 */
typedef enum {
    MDI_GPIO_LOW  = 0,
    MDI_GPIO_HIGH = 1,
} mdi_gpio_level_t;

/**
 * @brief GPIO 外设结构体
 *
 * 方法：Set(level), Get(), Toggle()
 */
typedef struct {
    void     *pPriv;    /**< 底层平台私有数据（如寄存器地址、引脚号等） */

    int32_t  (*fnSet)   (void *pPriv, mdi_gpio_level_t eLevel);
    int32_t  (*fnGet)   (void *pPriv);
    int32_t  (*fnToggle)(void *pPriv);
} mdi_gpio_t;

/*============================================================================
 * PWM (动力类)
 *===========================================================================*/

/**
 * @brief PWM 外设结构体
 *
 * 方法：SetDuty(val), Enable(en)
 */
typedef struct {
    void     *pPriv;

    int32_t  (*fnSetDuty)(void *pPriv, uint32_t wDuty);
    int32_t  (*fnEnable) (void *pPriv, bool bEnable);
} mdi_pwm_t;

/*============================================================================
 * Stream (通信类 - 主要面向 UART 字节流)
 *===========================================================================*/

/**
 * @brief Stream 外设结构体
 *
 * 方法：Write(data, len), Read(buf, len), IsBusy()
 * 
 */
typedef struct {
    void     *pPriv;

    int32_t  (*fnWrite) (void *pPriv, const uint8_t *pchData, uint32_t wLen);
    int32_t  (*fnRead)  (void *pPriv, uint8_t *pchBuf, uint32_t wLen);
    int32_t  (*fnIsBusy)(void *pPriv);
} mdi_stream_t;

/*============================================================================
 * ADC (传感类)
 *===========================================================================*/

/**
 * @brief ADC 外设结构体
 *
 * 方法：Read()
 */
typedef struct {
    void     *pPriv;

    int32_t  (*fnRead)(void *pPriv);
} mdi_adc_t;

/*============================================================================
 * IIC (总线外设访问)
 *===========================================================================*/

/**
 * @brief IIC 外设结构体
 *
 * 方法：Write(data, len), Read(buf, len), IsBusy()
 */
typedef struct {
    void     *pPriv;

    int32_t  (*fnWrite) (void *pPriv, const uint8_t *pchData, uint32_t wLen);
    int32_t  (*fnRead)  (void *pPriv, uint8_t *pchBuf, uint32_t wLen);
    int32_t  (*fnIsBusy)(void *pPriv);
} mdi_iic_t;

/*============================================================================
 * SPI (总线外设访问)
 *===========================================================================*/

/**
 * @brief SPI 外设结构体
 *
 * 方法：Write(data, len), Read(buf, len), IsBusy()
 */
typedef struct {
    void     *pPriv;

    int32_t  (*fnWrite) (void *pPriv, const uint8_t *pchData, uint32_t wLen);
    int32_t  (*fnRead)  (void *pPriv, uint8_t *pchBuf, uint32_t wLen);
    int32_t  (*fnIsBusy)(void *pPriv);
} mdi_spi_t;

/*============================================================================
 * Flash (存储类)
 *===========================================================================*/

/**
 * @brief Flash 外设结构体
 *
 * 方法：Erase(addr, size), Write(addr, data, len), Read(addr, buf, len),
 *       Unlock(), Lock()
 *
 * @note Flash 带地址参数，不走 _Generic 宏路由，使用专用 inline 函数。
 *       用法: mdi_flash_Write(HW.ptFlash, addr, data, len)
 */
typedef struct {
    void     *pPriv;

    int32_t  (*fnErase) (void *pPriv, uint32_t wAddr, uint32_t wSize);
    int32_t  (*fnWrite) (void *pPriv, uint32_t wAddr, const uint8_t *pchData, uint32_t wLen);
    int32_t  (*fnRead)  (void *pPriv, uint32_t wAddr, uint8_t *pchBuf, uint32_t wLen);
    int32_t  (*fnUnlock)(void *pPriv);
    int32_t  (*fnLock)  (void *pPriv);
} mdi_flash_t;

/*============================================================================
 * 全局硬件资源池 — 由各项目自行定义
 *
 * mdi_hardware_t 不在此处定义，由各项目在 port 层头文件中自定义，
 * 以匹配实际使用的外设数量和类型。
 *
 * 使用步骤：
 *   1. 在项目 port 目录下创建 mdi_hw.h，定义项目专属的 mdi_hardware_t
 *   2. 在项目 port 目录下创建 mdi_hw.c，实例化各外设并填充 HW
 *   3. 需要使用 GDI 的源文件 #include "mdi_hw.h"
 *
 * 示例 — 一个有 2 个 LED + 3 个串口 + 2 个按键的项目：
 *
 *   // mdi_hw.h
 *   #include "mdi/legacy/mdi.h"
 *
 *   typedef struct {
 *       mdi_gpio_t   *ptLedStatus;
 *       mdi_gpio_t   *ptLedError;
 *       mdi_gpio_t   *ptKeyUp;
 *       mdi_gpio_t   *ptKeyDown;
 *       mdi_stream_t *ptSerialDebug;
 *       mdi_stream_t *ptSerialComm;
 *       mdi_stream_t *ptSerialGps;
 *       mdi_adc_t    *ptBatSensor;
 *   } mdi_hardware_t;
 *
 *   extern const mdi_hardware_t HW;
 *
 * pPriv 复用技巧 — 多个同类 GPIO 可共享同一套函数，靠 pPriv 区分：
 *
 *   typedef struct { GPIO_TypeDef *port; uint16_t pin; } gpio_priv_t;
 *   static gpio_priv_t s_tLedPriv = { GPIOD, 13 };
 *   static gpio_priv_t s_tKeyPriv = { GPIOA, 0  };
 *
 *   static int32_t common_gpio_Set(void *pPriv, mdi_gpio_level_t eLevel) {
 *       gpio_priv_t *p = (gpio_priv_t *)pPriv;
 *       if (eLevel) p->port->SCR = (1UL << p->pin);
 *       else        p->port->CLR = (1UL << p->pin);
 *       return 0;
 *   }
 *
 *   static mdi_gpio_t s_tLed = { .pPriv = &s_tLedPriv, .fnSet = common_gpio_Set, ... };
 *   static mdi_gpio_t s_tKey = { .pPriv = &s_tKeyPriv, .fnSet = common_gpio_Set, ... };
 *
 * 参考模板: src/mdi/template_port.h, src/mdi/template_port.c
 *===========================================================================*/

#endif  /* __MDI_TYPES_H__ */
