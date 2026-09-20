/**
 * @file mdi_hw.h
 * @brief BLM 项目的 GDI 硬件资源池定义
 *
 * 定义 blm 项目使用的外设清单。
 * 其他项目复制此文件后按需修改。
 */
#ifndef __MDI_HW_H__
#define __MDI_HW_H__

#include "mdi/legacy/mdi.h"

typedef struct {
    mdi_gpio_t   *ptLedStatus;      /**< 状态 LED    */
    mdi_stream_t *ptSerialDebug;    /**< 调试/通信串口 */
    mdi_flash_t  *ptAppFlash;       /**< 应用程序 Flash */
} mdi_hardware_t;

extern const mdi_hardware_t HW;

#endif  /* __MDI_HW_H__ */
