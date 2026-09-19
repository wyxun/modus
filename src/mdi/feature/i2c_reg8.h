/**
 * @file i2c_reg8.h
 * @brief Optional 8-bit register-device helpers over an MDI I2C transfer.
 * @author Codex
 * @date 2026-09-18
 * @note This keeps device command conventions out of the core I2C contract.
 * It replaces the common legacy WriteAddress/ReadAddress pattern without a
 * runtime IIC object or a global selected bus.
 */
#ifndef MDI_FEATURE_I2C_REG8_H
#define MDI_FEATURE_I2C_REG8_H
#include "mdi/core/contract.h"

#define MDI_I2C_Reg8_Read(R, A, P, L)                                            \
    MDI_OP(R, _i2c_reg8_Read)((A), (P), (L))
#define MDI_I2C_Reg8_WriteByte(R, A, V)                                          \
    MDI_OP(R, _i2c_reg8_WriteByte)((A), (V))

/** @brief Bind a device using one-byte register/command addresses.
 * @param NAME Device resource token.
 * @param BUS I2C transfer resource token.
 * @param DEV7 Fixed 7-bit device address.
 * @return Generates register read and single-byte write helpers.
 * @note Multi-byte writes should build their register-prefixed frame in the
 * caller's buffer and use MDI_I2C_Transfer directly to avoid hidden VLA stack
 * allocation.
 */
#define MDI_I2C_REG8_BIND(NAME, BUS, DEV7)                                       \
    _Static_assert((DEV7) <= 0x7FU, "I2C device address must be 7-bit");         \
    MDI_INLINE mdi_status_t MDI_OP(NAME, _i2c_reg8_Read)(                        \
        uint8_t chRegister, uint8_t *pchData, uint32_t wLength)                  \
    {                                                                            \
        const mdi_i2c_transfer_t tTransfer = {                                   \
            .pchTx = &chRegister, .wTxLength = 1U, .pchRx = pchData,             \
            .wRxLength = wLength, .wTimeoutUs = 1U, .hwAddress7 = (DEV7)         \
        };                                                                       \
        return MDI_I2C_Transfer(BUS, &tTransfer);                                \
    }                                                                            \
    MDI_INLINE mdi_status_t MDI_OP(NAME, _i2c_reg8_WriteByte)(                   \
        uint8_t chRegister, uint8_t chValue)                                     \
    {                                                                            \
        const uint8_t chFrame[2] = {chRegister, chValue};                        \
        const mdi_i2c_transfer_t tTransfer = {                                   \
            .pchTx = chFrame, .wTxLength = 2U, .pchRx = NULL, .wRxLength = 0U,   \
            .wTimeoutUs = 1U, .hwAddress7 = (DEV7)                               \
        };                                                                       \
        return MDI_I2C_Transfer(BUS, &tTransfer);                                \
    }
#endif
