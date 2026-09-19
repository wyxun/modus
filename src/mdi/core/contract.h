/**
 * @file contract.h
 * @brief Experimental C11 static capability API, independent of chip and role.
 * @author Codex
 * @date 2026-09-18
 * @note Include a board binding before using operations. A resource is a
 * preprocessing token, never a runtime device index. See README.md.
 */
#ifndef MDI_CORE_CONTRACT_H
#define MDI_CORE_CONTRACT_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__GNUC__) || defined(__clang__)
#define MDI_INLINE static inline __attribute__((always_inline))
#else
#define MDI_INLINE static inline
#endif

typedef enum {
    MDI_OK = 0,
    MDI_INVALID = -1,
    MDI_RANGE = -2,
    MDI_BUSY = -3,
    MDI_IO_ERROR = -4,
    MDI_TIMEOUT = -5
} mdi_status_t;

/* Compile-time IO wiring capabilities. Mode registers and ownership remain
 * board-binding responsibilities; these bits document and constrain how a
 * consumer may use the bound resource. */
enum {
    MDI_IO_CAP_INPUT = 1U << 0,
    MDI_IO_CAP_OUTPUT = 1U << 1,
    MDI_IO_CAP_OPEN_DRAIN = 1U << 2
};

#define MDI_JOIN_(A, B) A##B
#define MDI_JOIN(A, B) MDI_JOIN_(A, B)
#define MDI_OP(R, S) MDI_JOIN(R, S)

/* IO is a right-aligned logical bit vector, including width-one pins.
 * Only the bound pins are affected; extra high value bits are discarded.
 * Reads sample the pad, not its output latch. For non-inverted open drain,
 * writing one releases the line and writing zero pulls it low.
 */
#define MDI_IO_Write(R, V) MDI_OP(R, _io_Write)((V))
#define MDI_IO_WriteMasked(R, M, V)                                              \
    MDI_OP(R, _io_WriteMasked)((M), (V))
#define MDI_IO_Read(R) MDI_OP(R, _io_Read)()
#define MDI_IO_Width(R) MDI_OP(R, _io_width)
#define MDI_IO_Capabilities(R) MDI_OP(R, _io_caps)

/* Sample layout/units belong to the resource; this API never starts or waits
 * for conversion. Caller must own a complete stable acquisition frame.
 */
#define MDI_Sample_Frame(R) MDI_OP(R, _sample_frame_t)
#define MDI_Sample_Read(R, P) MDI_OP(R, _sample_Read)((P))
#define MDI_Sample_ReadStable(R, P) MDI_OP(R, _sample_ReadStable)((P))
#define MDI_Sample_IsReady(R) MDI_OP(R, _sample_IsReady)()
#define MDI_Sample_ReadCompleted(R, P) MDI_OP(R, _sample_ReadCompleted)((P))

/* Compare values are timer ticks. Stage validates the whole frame before
 * writes. StageFast has the same semantics with nonnull/in-range preconditions.
 * Preload configuration and update deadline are the provider's contract.
 */
#define MDI_PWM_Frame(R) MDI_OP(R, _pwm_frame_t)
#define MDI_PWM_DutyFrame(R) MDI_OP(R, _pwm_duty_frame_t)
#define MDI_PWM_Cycle(R) MDI_OP(R, _pwm_cycle_t)
#define MDI_PWM_Stage(R, P) MDI_OP(R, _pwm_Stage)((P))
#define MDI_PWM_StageFast(R, P) MDI_OP(R, _pwm_StageFast)((P))

/* Optional capabilities. A provider must actually implement each operation;
 * there is no success stub or fallback. Frequency belongs to a timer group.
 * Duty uses unsigned Q16 fractions: 0..65536 means 0..100 percent.
 */
#define MDI_PWM_SetFrequency(R, HZ) MDI_OP(R, _pwm_SetFrequency)((HZ))
#define MDI_PWM_SetDuty(R, P) MDI_OP(R, _pwm_SetDuty)((P))
/* StageCycle prepares a complete period frame; Commit requests the provider's
 * hardware update event after the caller has met its safe update-window rule.
 */
#define MDI_PWM_StageCycle(R, P) MDI_OP(R, _pwm_StageCycle)((P))
#define MDI_PWM_Commit(R) MDI_OP(R, _pwm_Commit)()
#define MDI_PWM_Enable(R, B) MDI_OP(R, _pwm_Enable)((B))
#define MDI_PWM_SafeStop(R) MDI_OP(R, _pwm_SafeStop)()
#define MDI_PWM_IsEnabled(R) MDI_OP(R, _pwm_IsEnabled)()
#define MDI_PWM_FaultActive(R) MDI_OP(R, _pwm_FaultActive)()
#define MDI_PWM_ClearFault(R) MDI_OP(R, _pwm_ClearFault)()

typedef struct {
    uint32_t wDivider;
    uint32_t wPeriodTicks;
} mdi_pwm_timing_t;

/* Transactions are not byte streams. All pointers are borrowed for the
 * synchronous call only. A separate submit/completion capability is required
 * for DMA or asynchronous providers; they must not silently retain pointers.
 */
typedef struct {
    const uint8_t *pchTx;
    uint32_t wTxLength;
    uint8_t *pchRx;
    uint32_t wRxLength;
    uint32_t wTimeoutUs;
    uint16_t hwAddress7;
} mdi_i2c_transfer_t;

/* Full-duplex SPI, one CS assertion over the entire transfer. A null TX uses
 * chFill; a null RX discards received bytes. Word length is eight bits here.
 */
typedef struct {
    const uint8_t *pchTx;
    uint8_t *pchRx;
    uint32_t wLength;
    uint32_t wTimeoutUs;
    uint8_t chFill;
} mdi_spi_transfer_t;

/* I2C TX+RX requires repeated START, with STOP only at transaction end.
 * Richer message lists/10-bit addressing are separate optional capabilities.
 */
#define MDI_I2C_Transfer(R, P) MDI_OP(R, _i2c_Transfer)((P))
#define MDI_SPI_Transfer(R, P) MDI_OP(R, _spi_Transfer)((P))

/* Software-I2C line capability. The bound resource owns the pin policy;
 * these operations only expose the protocol engine's required edge actions.
 */
#define MDI_SoftI2C_SdaLow(R) MDI_OP(R, _SdaLow)()
#define MDI_SoftI2C_SdaRelease(R) MDI_OP(R, _SdaRelease)()
#define MDI_SoftI2C_SclLow(R) MDI_OP(R, _SclLow)()
#define MDI_SoftI2C_SclRelease(R) MDI_OP(R, _SclRelease)()
#define MDI_SoftI2C_ReadSda(R) MDI_OP(R, _ReadSda)()
#define MDI_SoftI2C_ReadScl(R) MDI_OP(R, _ReadScl)()
#define MDI_SoftI2C_PrepareBit(R, B) MDI_OP(R, _PrepareBit)((B))
#define MDI_SoftI2C_ReleaseClock(R) MDI_OP(R, _ReleaseClock)()
#define MDI_SoftI2C_ClockHigh(R) MDI_OP(R, _ClockHigh)()
#define MDI_SoftI2C_DataHigh(R) MDI_OP(R, _DataHigh)()
#endif
