/**
 * @file mdi.h
 * @brief GDI (MODUS Driver Interface) - 统一调度接口
 *
 * 利用 C11 _Generic 实现类"函数重载"的多态宏，使用户无需关心具体设备类型。
 * 通过参数计数宏区分不同参数形式（2参 vs 3参）。
 *
 * @usage
 *   MDI_Write(HW.ptLedStatus, MDI_GPIO_HIGH);       // GPIO Set
 *   MDI_Write(HW.ptSerialDebug, buf, len);           // Stream Write
 *   MDI_Read(HW.ptBatSensor);                        // ADC Read
 *   MDI_Toggle(HW.ptLedStatus);                      // GPIO Toggle
 *   MDI_Enable(HW.ptMotorMain, true);                // PWM Enable
 *
 * @note 需要 -std=c11 或 -std=gnu11 编译器支持。
 * @note 不包含任何 OS 相关头文件，纯裸机设计。
 */
#ifndef __MDI_H__
#define __MDI_H__

#include "mdi_types.h"
#include <stddef.h>     /* NULL */

/*============================================================================
 * Inline 包装函数
 *
 * 所有包装函数负责：①空指针检查 ②解引用函数指针并调用
 * 返回 int32_t，失败返回 -1（MODUS_EPERM 级别）
 *===========================================================================*/

/* ---- GPIO ---- */

static inline int32_t mdi_gpio_Set(mdi_gpio_t *ptDev, mdi_gpio_level_t eLevel)
{
    if (NULL == ptDev || NULL == ptDev->fnSet) { return -1; }
    return ptDev->fnSet(ptDev->pPriv, eLevel);
}

static inline int32_t mdi_gpio_Get(mdi_gpio_t *ptDev)
{
    if (NULL == ptDev || NULL == ptDev->fnGet) { return -1; }
    return ptDev->fnGet(ptDev->pPriv);
}

static inline int32_t mdi_gpio_Toggle(mdi_gpio_t *ptDev)
{
    if (NULL == ptDev || NULL == ptDev->fnToggle) { return -1; }
    return ptDev->fnToggle(ptDev->pPriv);
}

/* ---- PWM ---- */

static inline int32_t mdi_pwm_SetDuty(mdi_pwm_t *ptDev, uint32_t wDuty)
{
    if (NULL == ptDev || NULL == ptDev->fnSetDuty) { return -1; }
    return ptDev->fnSetDuty(ptDev->pPriv, wDuty);
}

static inline int32_t mdi_pwm_Enable(mdi_pwm_t *ptDev, bool bEnable)
{
    if (NULL == ptDev || NULL == ptDev->fnEnable) { return -1; }
    return ptDev->fnEnable(ptDev->pPriv, bEnable);
}

/* ---- Stream ---- */

static inline int32_t mdi_stream_Write(mdi_stream_t *ptDev,
                                       const uint8_t *pchData,
                                       uint32_t wLen)
{
    if (NULL == ptDev || NULL == ptDev->fnWrite) { return -1; }
    return ptDev->fnWrite(ptDev->pPriv, pchData, wLen);
}

static inline int32_t mdi_stream_Read(mdi_stream_t *ptDev,
                                      uint8_t *pchBuf,
                                      uint32_t wLen)
{
    if (NULL == ptDev || NULL == ptDev->fnRead) { return -1; }
    return ptDev->fnRead(ptDev->pPriv, pchBuf, wLen);
}

static inline int32_t mdi_stream_IsBusy(mdi_stream_t *ptDev)
{
    if (NULL == ptDev || NULL == ptDev->fnIsBusy) { return -1; }
    return ptDev->fnIsBusy(ptDev->pPriv);
}

/* ---- ADC ---- */

static inline int32_t mdi_adc_Read(mdi_adc_t *ptDev)
{
    if (NULL == ptDev || NULL == ptDev->fnRead) { return -1; }
    return ptDev->fnRead(ptDev->pPriv);
}

/* ---- IIC ---- */

static inline int32_t mdi_iic_Write(mdi_iic_t *ptDev,
                                    const uint8_t *pchData,
                                    uint32_t wLen)
{
    if (NULL == ptDev || NULL == ptDev->fnWrite) { return -1; }
    return ptDev->fnWrite(ptDev->pPriv, pchData, wLen);
}

static inline int32_t mdi_iic_Read(mdi_iic_t *ptDev,
                                   uint8_t *pchBuf,
                                   uint32_t wLen)
{
    if (NULL == ptDev || NULL == ptDev->fnRead) { return -1; }
    return ptDev->fnRead(ptDev->pPriv, pchBuf, wLen);
}

static inline int32_t mdi_iic_IsBusy(mdi_iic_t *ptDev)
{
    if (NULL == ptDev || NULL == ptDev->fnIsBusy) { return -1; }
    return ptDev->fnIsBusy(ptDev->pPriv);
}

/* ---- SPI ---- */

static inline int32_t mdi_spi_Write(mdi_spi_t *ptDev,
                                    const uint8_t *pchData,
                                    uint32_t wLen)
{
    if (NULL == ptDev || NULL == ptDev->fnWrite) { return -1; }
    return ptDev->fnWrite(ptDev->pPriv, pchData, wLen);
}

static inline int32_t mdi_spi_Read(mdi_spi_t *ptDev,
                                   uint8_t *pchBuf,
                                   uint32_t wLen)
{
    if (NULL == ptDev || NULL == ptDev->fnRead) { return -1; }
    return ptDev->fnRead(ptDev->pPriv, pchBuf, wLen);
}

static inline int32_t mdi_spi_IsBusy(mdi_spi_t *ptDev)
{
    if (NULL == ptDev || NULL == ptDev->fnIsBusy) { return -1; }
    return ptDev->fnIsBusy(ptDev->pPriv);
}

/* ---- Flash (专用接口，不走 _Generic) ---- */

static inline int32_t mdi_flash_Erase(mdi_flash_t *ptDev,
                                      uint32_t wAddr, uint32_t wSize)
{
    if (NULL == ptDev || NULL == ptDev->fnErase) { return -1; }
    return ptDev->fnErase(ptDev->pPriv, wAddr, wSize);
}

static inline int32_t mdi_flash_Write(mdi_flash_t *ptDev,
                                      uint32_t wAddr,
                                      const uint8_t *pchData,
                                      uint32_t wLen)
{
    if (NULL == ptDev || NULL == ptDev->fnWrite) { return -1; }
    return ptDev->fnWrite(ptDev->pPriv, wAddr, pchData, wLen);
}

static inline int32_t mdi_flash_Read(mdi_flash_t *ptDev,
                                     uint32_t wAddr,
                                     uint8_t *pchBuf,
                                     uint32_t wLen)
{
    if (NULL == ptDev || NULL == ptDev->fnRead) { return -1; }
    return ptDev->fnRead(ptDev->pPriv, wAddr, pchBuf, wLen);
}

static inline int32_t mdi_flash_Unlock(mdi_flash_t *ptDev)
{
    if (NULL == ptDev || NULL == ptDev->fnUnlock) { return -1; }
    return ptDev->fnUnlock(ptDev->pPriv);
}

static inline int32_t mdi_flash_Lock(mdi_flash_t *ptDev)
{
    if (NULL == ptDev || NULL == ptDev->fnLock) { return -1; }
    return ptDev->fnLock(ptDev->pPriv);
}

/*============================================================================
 * 参数计数辅助宏
 *===========================================================================*/

#define MDI_CONCAT_(a, b)   a##b
#define MDI_CONCAT(a, b)    MDI_CONCAT_(a, b)

#define MDI_NARGS_(...)             MDI_NARGS_IMPL_(__VA_ARGS__, 5, 4, 3, 2, 1, 0)
#define MDI_NARGS_IMPL_(_1, _2, _3, _4, _5, N, ...)  N
#define MDI_NARGS(...)              MDI_NARGS_(__VA_ARGS__)

/*============================================================================
 * MDI_Write — 统一写入宏
 *
 *   2参: MDI_Write(gpio_dev, level)      → mdi_gpio_Set
 *        MDI_Write(pwm_dev, duty)        → mdi_pwm_SetDuty
 *   3参: MDI_Write(stream_dev, data, len) → mdi_stream_Write
 *        MDI_Write(iic_dev, data, len)    → mdi_iic_Write
 *        MDI_Write(spi_dev, data, len)    → mdi_spi_Write
 *===========================================================================*/

#define MDI_Write(dev, ...)                                                      \
    MDI_CONCAT(MDI_Write_, MDI_NARGS(__VA_ARGS__))(dev, __VA_ARGS__)

/* 2参版 — GPIO Set / PWM SetDuty */
#define MDI_Write_1(dev, val)  _Generic((dev),                                   \
    mdi_gpio_t*:    mdi_gpio_Set,                                                \
    mdi_pwm_t*:     mdi_pwm_SetDuty                                              \
)(dev, val)

/* 3参版 — Stream / IIC / SPI Write */
#define MDI_Write_2(dev, data, len)  _Generic((dev),                             \
    mdi_stream_t*:  mdi_stream_Write,                                            \
    mdi_iic_t*:     mdi_iic_Write,                                               \
    mdi_spi_t*:     mdi_spi_Write                                                \
)(dev, data, len)

/*============================================================================
 * MDI_Read — 统一读取宏
 *
 *   1参: MDI_Read(gpio_dev)            → mdi_gpio_Get
 *        MDI_Read(adc_dev)             → mdi_adc_Read
 *   3参: MDI_Read(stream_dev, buf, len) → mdi_stream_Read
 *        MDI_Read(iic_dev, buf, len)    → mdi_iic_Read
 *        MDI_Read(spi_dev, buf, len)    → mdi_spi_Read
 *===========================================================================*/

#define MDI_Read(dev, ...)                                                       \
    MDI_CONCAT(MDI_Read_, MDI_NARGS(__VA_ARGS__))(dev, __VA_ARGS__)

/* 仅 dev — GPIO Get / ADC Read（0 个额外参数，但宏计数从 dev 后开始） */
/* 注意：无额外参数时 __VA_ARGS__ 为空，需要特殊处理 */

/*
 * 对于 MDI_Read(dev) 无额外参数的情况：
 * 由于 __VA_ARGS__ 为空时参数计数不可靠，采用独立的无参宏路径。
 * 使用 _Generic 直接分发。
 */
#undef MDI_Read
#define MDI_Read(...)                                                            \
    MDI_CONCAT(MDI_Read_N_, MDI_NARGS(__VA_ARGS__))(__VA_ARGS__)

/* 1参: MDI_Read(dev) — GPIO Get / ADC Read */
#define MDI_Read_N_1(dev)  _Generic((dev),                                       \
    mdi_gpio_t*:    mdi_gpio_Get,                                                \
    mdi_adc_t*:     mdi_adc_Read                                                 \
)(dev)

/* 3参: MDI_Read(dev, buf, len) — Stream / IIC / SPI Read */
#define MDI_Read_N_3(dev, buf, len)  _Generic((dev),                             \
    mdi_stream_t*:  mdi_stream_Read,                                             \
    mdi_iic_t*:     mdi_iic_Read,                                                \
    mdi_spi_t*:     mdi_spi_Read                                                 \
)(dev, buf, len)

/*============================================================================
 * MDI_Toggle — GPIO 翻转 (固定参数)
 *===========================================================================*/

#define MDI_Toggle(dev)  _Generic((dev),                                         \
    mdi_gpio_t*:    mdi_gpio_Toggle                                              \
)(dev)

/*============================================================================
 * MDI_Enable — PWM 使能 (固定参数)
 *===========================================================================*/

#define MDI_Enable(dev, en)  _Generic((dev),                                     \
    mdi_pwm_t*:     mdi_pwm_Enable                                               \
)(dev, en)

/*============================================================================
 * MDI_IsBusy — 总线忙检测 (固定参数)
 *===========================================================================*/

#define MDI_IsBusy(dev)  _Generic((dev),                                         \
    mdi_stream_t*:  mdi_stream_IsBusy,                                           \
    mdi_iic_t*:     mdi_iic_IsBusy,                                              \
    mdi_spi_t*:     mdi_spi_IsBusy                                               \
)(dev)

/*============================================================================
 * 全局硬件资源池
 *
 * mdi_hardware_t 和 HW 由各项目自行定义 (见 mdi_types.h 末尾注释)。
 * 使用 GDI 宏的源文件需要 #include 项目的 mdi_hw.h。
 *
 * 参考模板: src/mdi/template_port.h
 *===========================================================================*/

#endif  /* __MDI_H__ */
