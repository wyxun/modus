/**
 * @file template_port.h
 * @brief GDI 适配层模板 — 头文件
 *
 * 定义项目专属的 mdi_hardware_t 结构体和 HW 全局声明。
 * 开发者复制此文件到具体项目的 port 目录中，改名为 mdi_hw.h，
 * 按实际外设增删成员即可。
 *
 * @usage
 *   1. 复制 template_port.h → 项目/port/mdi_hw.h
 *   2. 复制 template_port.c → 项目/port/mdi_hw.c
 *   3. 修改 mdi_hardware_t 成员以匹配实际外设
 *   4. 在需要使用 GDI 的 .c 文件中 #include "mdi_hw.h"
 */
#ifndef __MDI_HW_H__
#define __MDI_HW_H__

#include "mdi/legacy/mdi.h"

/*============================================================================
 * 项目硬件资源池定义
 *
 * 按需增删成员。每个指针对应一个外设实例。
 * 命名建议：pt + 功能名（如 ptLedStatus, ptSerialDebug）
 *===========================================================================*/

typedef struct {
    /* ---------- GPIO ---------- */
    mdi_gpio_t   *ptLedStatus;      /**< 状态 LED            */
    mdi_gpio_t   *ptLedError;       /**< 错误 LED (可选)     */
    mdi_gpio_t   *ptKeyInput;       /**< 按键输入            */
    /* 按需添加更多 GPIO:
     * mdi_gpio_t   *ptKeyUp;
     * mdi_gpio_t   *ptKeyDown;
     * mdi_gpio_t   *ptRelay1;
     */

    /* ---------- Stream (UART / USB CDC) ---------- */
    mdi_stream_t *ptSerialDebug;    /**< 调试串口            */
    mdi_stream_t *ptSerialComm;     /**< 通信串口 (可选)     */
    /* 按需添加更多串口:
     * mdi_stream_t *ptSerialGps;
     */

    /* ---------- PWM ---------- */
    mdi_pwm_t    *ptMotorMain;      /**< 主电机 PWM (可选)   */
    /* mdi_pwm_t    *ptBuzzer; */

    /* ---------- ADC ---------- */
    mdi_adc_t    *ptBatSensor;      /**< 电池电压 ADC (可选) */
    /* mdi_adc_t    *ptTempSensor; */

    /* ---------- IIC ---------- */
    /* mdi_iic_t    *ptIicSensor; */

    /* ---------- SPI ---------- */
    /* mdi_spi_t    *ptSpiFlash; */

    /* ---------- Flash ---------- */
    /* mdi_flash_t  *ptAppFlash; */

} mdi_hardware_t;

/**
 * @brief 全局硬件资源池声明（在 mdi_hw.c / port_mdi.c 中实例化）
 */
extern const mdi_hardware_t HW;

#endif  /* __MDI_HW_H__ */
