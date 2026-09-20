/**
 * @file soft_i2c_edges.h
 * @brief Reusable static pin policy for a software I2C engine.
 * @author Codex
 * @date 2026-09-18
 * @note These bounded edge primitives are NOT a complete I2C driver. The
 * owning engine must enforce setup/hold times, ACK, arbitration, stretching
 * timeout and START/STOP. It owns its state and time source. No busy wait.
 */
#ifndef MDI_FEATURE_SOFT_I2C_EDGES_H
#define MDI_FEATURE_SOFT_I2C_EDGES_H
#include "mdi/core/contract.h"

/** @brief Specialize the same pin policy for any two IO providers.
 * @param NAME Name of the software bus policy.
 * @param SCL Non-inverted width-one open-drain clock resource.
 * @param SDA Non-inverted width-one open-drain data resource.
 * @return Generates bounded PrepareBit/ReleaseClock/ClockHigh/DataHigh.
 * @note Pin mode/polarity/ownership are board preconditions. Call ReleaseClock
 * only after data setup; poll ClockHigh with an engine-owned deadline.
 */
#define MDI_SOFT_I2C_EDGES_BIND(NAME, SCL, SDA)                                  \
    _Static_assert(MDI_IO_Width(SCL) == 1, "SCL must be one pin");               \
    _Static_assert(MDI_IO_Width(SDA) == 1, "SDA must be one pin");               \
    _Static_assert((MDI_IO_Capabilities(SCL) &                                   \
                    (MDI_IO_CAP_INPUT | MDI_IO_CAP_OUTPUT |                      \
                     MDI_IO_CAP_OPEN_DRAIN)) ==                                  \
                   (MDI_IO_CAP_INPUT | MDI_IO_CAP_OUTPUT |                       \
                    MDI_IO_CAP_OPEN_DRAIN),                                      \
                   "SCL must be input/output open-drain");                       \
    _Static_assert((MDI_IO_Capabilities(SDA) &                                   \
                    (MDI_IO_CAP_INPUT | MDI_IO_CAP_OUTPUT |                      \
                     MDI_IO_CAP_OPEN_DRAIN)) ==                                  \
                   (MDI_IO_CAP_INPUT | MDI_IO_CAP_OUTPUT |                       \
                    MDI_IO_CAP_OPEN_DRAIN),                                      \
                   "SDA must be input/output open-drain");                       \
    MDI_INLINE void MDI_OP(NAME, _SdaLow)(void)                                  \
    {                                                                            \
        MDI_IO_Write(SDA, 0U);                                                   \
    }                                                                            \
    MDI_INLINE void MDI_OP(NAME, _SdaRelease)(void)                              \
    {                                                                            \
        MDI_IO_Write(SDA, 1U);                                                   \
    }                                                                            \
    MDI_INLINE void MDI_OP(NAME, _SclLow)(void)                                  \
    {                                                                            \
        MDI_IO_Write(SCL, 0U);                                                   \
    }                                                                            \
    MDI_INLINE void MDI_OP(NAME, _SclRelease)(void)                              \
    {                                                                            \
        MDI_IO_Write(SCL, 1U);                                                   \
    }                                                                            \
    MDI_INLINE bool MDI_OP(NAME, _ReadScl)(void)                                 \
    {                                                                            \
        return MDI_IO_Read(SCL) != 0U;                                           \
    }                                                                            \
    MDI_INLINE bool MDI_OP(NAME, _ReadSda)(void)                                 \
    {                                                                            \
        return MDI_IO_Read(SDA) != 0U;                                           \
    }                                                                            \
    MDI_INLINE void MDI_OP(NAME, _PrepareBit)(bool bOne)                         \
    {                                                                            \
        MDI_OP(NAME, _SclLow)();                                                 \
        if (bOne) { MDI_OP(NAME, _SdaRelease)(); }                               \
        else { MDI_OP(NAME, _SdaLow)(); }                                        \
    }                                                                            \
    MDI_INLINE void MDI_OP(NAME, _ReleaseClock)(void)                            \
    {                                                                            \
        MDI_OP(NAME, _SclRelease)();                                             \
    }                                                                            \
    MDI_INLINE bool MDI_OP(NAME, _ClockHigh)(void)                               \
    {                                                                            \
        return MDI_OP(NAME, _ReadScl)();                                         \
    }                                                                            \
    MDI_INLINE bool MDI_OP(NAME, _DataHigh)(void)                                \
    {                                                                            \
        return MDI_OP(NAME, _ReadSda)();                                         \
    }
#endif
