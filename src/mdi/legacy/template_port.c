/**
 * @file template_port.c
 * @brief GDI 适配层模板 — 供开发者参考
 *
 * 包含所有外设接口的空实现（桩代码），以及 HW 全局资源池的实例化示例。
 * 开发者复制此文件到具体项目的 port 目录中，填入对应芯片的 HAL 函数。
 *
 * @usage
 *   1. 复制此文件到项目 port 目录，重命名为 port_mdi.c
 *   2. 根据目标芯片，include 对应的 CMSIS/HAL 头文件
 *   3. 在各桩函数中填入实际的硬件操作代码
 *   4. 按需修改 mdi_hardware_t HW 结构体的成员
 */

#include "template_port.h"

/*============================================================================
 * 平台相关头文件（按需取消注释）
 *===========================================================================*/
/* #include "stm32g4xx.h"   // STM32G4 CMSIS */
/* #include "at32f4xx.h"    // AT32F4 */

/*============================================================================
 * GPIO 适配
 *
 * 示例：将 mdi_gpio_Set() 对接到 HAL_GPIO_WritePin()
 *
 *   static int32_t led_Set(void *pPriv, mdi_gpio_level_t eLevel) {
 *       GPIO_TypeDef *port = ((gpio_priv_t *)pPriv)->port;
 *       uint16_t pin       = ((gpio_priv_t *)pPriv)->pin;
 *       HAL_GPIO_WritePin(port, pin, (GPIO_PinState)eLevel);
 *       return 0;
 *   }
 *===========================================================================*/

static int32_t stub_gpio_Set(void *pPriv, mdi_gpio_level_t eLevel)
{
    (void)pPriv;
    (void)eLevel;
    /* TODO: 填入 GPIO 写操作, 例如:
     * GPIOA->BSRR = (eLevel == MDI_GPIO_HIGH) ? GPIO_PIN : (GPIO_PIN << 16);
     */
    return 0;
}

static int32_t stub_gpio_Get(void *pPriv)
{
    (void)pPriv;
    /* TODO: 填入 GPIO 读操作, 例如:
     * return (GPIOA->IDR & GPIO_PIN) ? MDI_GPIO_HIGH : MDI_GPIO_LOW;
     */
    return MDI_GPIO_LOW;
}

static int32_t stub_gpio_Toggle(void *pPriv)
{
    (void)pPriv;
    /* TODO: 填入 GPIO 翻转操作, 例如:
     * GPIOA->ODR ^= GPIO_PIN;
     */
    return 0;
}

/*============================================================================
 * PWM 适配
 *
 * 示例：将 mdi_pwm_SetDuty() 对接到定时器比较值寄存器
 *
 *   static int32_t motor_SetDuty(void *pPriv, uint32_t wDuty) {
 *       TIM3->CCR1 = wDuty;
 *       return 0;
 *   }
 *===========================================================================*/

static int32_t stub_pwm_SetDuty(void *pPriv, uint32_t wDuty)
{
    (void)pPriv;
    (void)wDuty;
    /* TODO: 填入 PWM 占空比设置, 例如:
     * TIM3->CCR1 = wDuty;
     */
    return 0;
}

static int32_t stub_pwm_Enable(void *pPriv, bool bEnable)
{
    (void)pPriv;
    (void)bEnable;
    /* TODO: 填入 PWM 使能/禁用, 例如:
     * if (bEnable) TIM3->CR1 |= TIM_CR1_CEN;
     * else         TIM3->CR1 &= ~TIM_CR1_CEN;
     */
    return 0;
}

/*============================================================================
 * Stream (UART) 适配
 *
 * 示例：将 mdi_stream_Write() 对接到 UART 发送
 *
 *   static int32_t uart_Write(void *pPriv, const uint8_t *pchData, uint32_t wLen) {
 *       USART_TypeDef *uart = (USART_TypeDef *)pPriv;
 *       for (uint32_t i = 0; i < wLen; i++) {
 *           while (!(uart->ISR & USART_ISR_TXE_TXFNF));
 *           uart->TDR = pchData[i];
 *       }
 *       while (!(uart->ISR & USART_ISR_TC));
 *       return (int32_t)wLen;
 *   }
 *===========================================================================*/

static int32_t stub_stream_Write(void *pPriv, const uint8_t *pchData, uint32_t wLen)
{
    (void)pPriv;
    (void)pchData;
    (void)wLen;
    /* TODO: 填入 UART 发送, 例如:
     * for (uint32_t i = 0; i < wLen; i++) {
     *     while (!(USART1->SR & USART_SR_TXE));
     *     USART1->DR = pchData[i];
     * }
     */
    return (int32_t)wLen;
}

static int32_t stub_stream_Read(void *pPriv, uint8_t *pchBuf, uint32_t wLen)
{
    (void)pPriv;
    (void)pchBuf;
    (void)wLen;
    /* TODO: 填入 UART 接收, 例如:
     * for (uint32_t i = 0; i < wLen; i++) {
     *     while (!(USART1->SR & USART_SR_RXNE));
     *     pchBuf[i] = USART1->DR;
     * }
     */
    return (int32_t)wLen;
}

static int32_t stub_stream_IsBusy(void *pPriv)
{
    (void)pPriv;
    /* TODO: 检查 UART 是否忙, 例如:
     * return (USART1->SR & USART_SR_TC) ? 0 : 1;
     */
    return 0;
}

/*============================================================================
 * ADC 适配
 *
 * 示例：将 mdi_adc_Read() 对接到 ADC 单次转换
 *
 *   static int32_t bat_Read(void *pPriv) {
 *       ADC1->CR |= ADC_CR_ADSTART;
 *       while (!(ADC1->ISR & ADC_ISR_EOC));
 *       return (int32_t)ADC1->DR;
 *   }
 *===========================================================================*/

static int32_t stub_adc_Read(void *pPriv)
{
    (void)pPriv;
    /* TODO: 填入 ADC 读取, 例如:
     * ADC1->CR |= ADC_CR_ADSTART;
     * while (!(ADC1->ISR & ADC_ISR_EOC));
     * return (int32_t)ADC1->DR;
     */
    return 0;
}

/*============================================================================
 * IIC 适配
 *===========================================================================*/

static int32_t stub_iic_Write(void *pPriv, const uint8_t *pchData, uint32_t wLen)
{
    (void)pPriv;
    (void)pchData;
    (void)wLen;
    /* TODO: 填入 I2C 写操作 */
    return (int32_t)wLen;
}

static int32_t stub_iic_Read(void *pPriv, uint8_t *pchBuf, uint32_t wLen)
{
    (void)pPriv;
    (void)pchBuf;
    (void)wLen;
    /* TODO: 填入 I2C 读操作 */
    return (int32_t)wLen;
}

static int32_t stub_iic_IsBusy(void *pPriv)
{
    (void)pPriv;
    /* TODO: 填入 I2C 忙检测 */
    return 0;
}

/*============================================================================
 * SPI 适配
 *===========================================================================*/

static int32_t stub_spi_Write(void *pPriv, const uint8_t *pchData, uint32_t wLen)
{
    (void)pPriv;
    (void)pchData;
    (void)wLen;
    /* TODO: 填入 SPI 写操作 */
    return (int32_t)wLen;
}

static int32_t stub_spi_Read(void *pPriv, uint8_t *pchBuf, uint32_t wLen)
{
    (void)pPriv;
    (void)pchBuf;
    (void)wLen;
    /* TODO: 填入 SPI 读操作 */
    return (int32_t)wLen;
}

static int32_t stub_spi_IsBusy(void *pPriv)
{
    (void)pPriv;
    /* TODO: 填入 SPI 忙检测 */
    return 0;
}

/*============================================================================
 * Flash 适配
 *===========================================================================*/

static int32_t stub_flash_Erase(void *pPriv, uint32_t wAddr, uint32_t wSize)
{
    (void)pPriv; (void)wAddr; (void)wSize;
    /* TODO: 填入 Flash 擦除 */
    return 0;
}

static int32_t stub_flash_Write(void *pPriv, uint32_t wAddr, const uint8_t *pchData, uint32_t wLen)
{
    (void)pPriv; (void)wAddr; (void)pchData; (void)wLen;
    /* TODO: 填入 Flash 写入 */
    return wLen;
}

static int32_t stub_flash_Read(void *pPriv, uint32_t wAddr, uint8_t *pchBuf, uint32_t wLen)
{
    (void)pPriv; (void)wAddr; (void)pchBuf; (void)wLen;
    /* TODO: 填入 Flash 读取 */
    return wLen;
}

static int32_t stub_flash_Unlock(void *pPriv)
{
    (void)pPriv;
    /* TODO: 填入 Flash 解锁 */
    return 0;
}

static int32_t stub_flash_Lock(void *pPriv)
{
    (void)pPriv;
    /* TODO: 填入 Flash 上锁 */
    return 0;
}

/*============================================================================
 * 外设实例（静态分配）
 *
 * 在此定义各外设的 mdi_xxx_t 实例，并填入对应的函数指针。
 * pPriv 可指向寄存器基地址、配置结构体等。
 *===========================================================================*/

static mdi_gpio_t s_tLedGpio = {
    .pPriv    = NULL,           /* TODO: 替换为实际的 GPIO 基地址或私有结构 */
    .fnSet    = stub_gpio_Set,
    .fnGet    = stub_gpio_Get,
    .fnToggle = stub_gpio_Toggle,
};

static mdi_gpio_t s_tKeyGpio = {
    .pPriv    = NULL,
    .fnSet    = stub_gpio_Set,
    .fnGet    = stub_gpio_Get,
    .fnToggle = stub_gpio_Toggle,
};

static mdi_stream_t s_tUartDebug = {
    .pPriv    = NULL,           /* TODO: 替换为 USART 基地址, 如 (void *)USART1 */
    .fnWrite  = stub_stream_Write,
    .fnRead   = stub_stream_Read,
    .fnIsBusy = stub_stream_IsBusy,
};

static mdi_stream_t s_tUartComm = {
    .pPriv    = NULL,
    .fnWrite  = stub_stream_Write,
    .fnRead   = stub_stream_Read,
    .fnIsBusy = stub_stream_IsBusy,
};

static mdi_pwm_t s_tMotorPwm = {
    .pPriv     = NULL,
    .fnSetDuty = stub_pwm_SetDuty,
    .fnEnable  = stub_pwm_Enable,
};

static mdi_adc_t s_tBatAdc = {
    .pPriv  = NULL,
    .fnRead = stub_adc_Read,
};

static mdi_flash_t s_tFlash = {
    .pPriv    = NULL,
    .fnErase  = stub_flash_Erase,
    .fnWrite  = stub_flash_Write,
    .fnRead   = stub_flash_Read,
    .fnUnlock = stub_flash_Unlock,
    .fnLock   = stub_flash_Lock,
};

/*============================================================================
 * 全局硬件资源池实例化
 *
 * 这是最终暴露给应用层的 HW 单例。
 * 用户通过 HW.ptLedStatus / HW.ptSerialDebug 等方式访问外设。
 *===========================================================================*/

const mdi_hardware_t HW = {
    .ptLedStatus   = &s_tLedGpio,
    .ptKeyInput    = &s_tKeyGpio,
    .ptSerialDebug = &s_tUartDebug,
    .ptSerialComm  = &s_tUartComm,
    .ptMotorMain   = &s_tMotorPwm,
    .ptBatSensor   = &s_tBatAdc,
};
