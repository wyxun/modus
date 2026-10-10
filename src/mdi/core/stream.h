/**
 * @file stream.h
 * @brief Static byte-stream capability contract.
 * @author Codex
 * @date 2026-10-10
 * @note Read/Write return status and report the actual byte count separately.
 * Legal zero-length calls return OK/0; short transfers also return OK. A null
 * count, or null data with nonzero length, is INVALID. Valid arguments reset
 * the count before any operation. OVERRUN may accompany received bytes: the
 * caller owns those bytes and is notified that other bytes were lost.
 */
#ifndef MDI_CORE_STREAM_H
#define MDI_CORE_STREAM_H

#include "mdi/core/contract.h"

#define MDI_STREAM_Write(R, P, L, N) MDI_OP(R, _stream_Write)((P), (L), (N))
#define MDI_STREAM_Read(R, P, L, N) MDI_OP(R, _stream_Read)((P), (L), (N))
#define MDI_STREAM_Available(R) MDI_OP(R, _stream_Available)()
#define MDI_STREAM_IsBusy(R) MDI_OP(R, _stream_IsBusy)()

/** Bind a provider with no runtime context argument. */
#define MDI_STREAM_BIND(NAME, WRITE, READ, AVAILABLE, BUSY)                     \
    MDI_INLINE mdi_status_t MDI_OP(NAME, _stream_Write)(                        \
        const uint8_t *pchData, uint32_t wLength, uint32_t *pwCount)            \
    {                                                                           \
        return WRITE(pchData, wLength, pwCount);                                \
    }                                                                           \
    MDI_INLINE mdi_status_t MDI_OP(NAME, _stream_Read)(                         \
        uint8_t *pchData, uint32_t wLength, uint32_t *pwCount)                  \
    {                                                                           \
        return READ(pchData, wLength, pwCount);                                 \
    }                                                                           \
    MDI_INLINE uint32_t MDI_OP(NAME, _stream_Available)(void)                   \
    {                                                                           \
        return AVAILABLE();                                                     \
    }                                                                           \
    MDI_INLINE bool MDI_OP(NAME, _stream_IsBusy)(void)                          \
    {                                                                           \
        return BUSY();                                                          \
    }

/** Bind provider callbacks that use a fixed board-local context pointer. */
#define MDI_STREAM_BIND_PRIV(NAME, PRIV, WRITE, READ, AVAILABLE, BUSY)          \
    MDI_INLINE mdi_status_t MDI_OP(NAME, _stream_Write)(                        \
        const uint8_t *pchData, uint32_t wLength, uint32_t *pwCount)            \
    {                                                                           \
        return WRITE((PRIV), pchData, wLength, pwCount);                        \
    }                                                                           \
    MDI_INLINE mdi_status_t MDI_OP(NAME, _stream_Read)(                         \
        uint8_t *pchData, uint32_t wLength, uint32_t *pwCount)                  \
    {                                                                           \
        return READ((PRIV), pchData, wLength, pwCount);                         \
    }                                                                           \
    MDI_INLINE uint32_t MDI_OP(NAME, _stream_Available)(void)                   \
    {                                                                           \
        return AVAILABLE((PRIV));                                               \
    }                                                                           \
    MDI_INLINE bool MDI_OP(NAME, _stream_IsBusy)(void)                          \
    {                                                                           \
        return BUSY((PRIV));                                                    \
    }

#endif /* MDI_CORE_STREAM_H */
