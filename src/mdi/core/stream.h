/**
 * @file stream.h
 * @brief Static byte-stream capability contract.
 *
 * A stream is a byte-oriented transport resource. The provider defines its
 * buffering, scheduling and flow-control policy; consumers only see the byte
 * operations. No runtime stream object or function table is created.
 */
#ifndef MDI_CORE_STREAM_H
#define MDI_CORE_STREAM_H

#include "mdi/core/contract.h"

#define MDI_STREAM_Write(R, P, L) MDI_OP(R, _stream_Write)((P), (L))
#define MDI_STREAM_Read(R, P, L) MDI_OP(R, _stream_Read)((P), (L))
#define MDI_STREAM_Available(R) MDI_OP(R, _stream_Available)()
#define MDI_STREAM_IsBusy(R) MDI_OP(R, _stream_IsBusy)()

/** Bind a provider with no runtime context argument. */
#define MDI_STREAM_BIND(NAME, WRITE, READ, AVAILABLE, BUSY)                    \
    MDI_INLINE int32_t MDI_OP(NAME, _stream_Write)(                             \
        const uint8_t *pchData, uint32_t wLength)                               \
    {                                                                            \
        return WRITE(pchData, wLength);                                          \
    }                                                                            \
    MDI_INLINE int32_t MDI_OP(NAME, _stream_Read)(                              \
        uint8_t *pchData, uint32_t wLength)                                     \
    {                                                                            \
        return READ(pchData, wLength);                                           \
    }                                                                            \
    MDI_INLINE uint32_t MDI_OP(NAME, _stream_Available)(void)                   \
    {                                                                            \
        return AVAILABLE();                                                     \
    }                                                                            \
    MDI_INLINE bool MDI_OP(NAME, _stream_IsBusy)(void)                          \
    {                                                                            \
        return BUSY();                                                          \
    }

/** Bind provider callbacks that use a fixed board-local context pointer. */
#define MDI_STREAM_BIND_PRIV(NAME, PRIV, WRITE, READ, AVAILABLE, BUSY)          \
    MDI_INLINE int32_t MDI_OP(NAME, _stream_Write)(                             \
        const uint8_t *pchData, uint32_t wLength)                               \
    {                                                                            \
        return WRITE((PRIV), pchData, wLength);                                  \
    }                                                                            \
    MDI_INLINE int32_t MDI_OP(NAME, _stream_Read)(                              \
        uint8_t *pchData, uint32_t wLength)                                     \
    {                                                                            \
        return READ((PRIV), pchData, wLength);                                  \
    }                                                                            \
    MDI_INLINE uint32_t MDI_OP(NAME, _stream_Available)(void)                   \
    {                                                                            \
        return AVAILABLE((PRIV));                                               \
    }                                                                            \
    MDI_INLINE bool MDI_OP(NAME, _stream_IsBusy)(void)                          \
    {                                                                            \
        return BUSY((PRIV)) != 0;                                                \
    }

#endif /* MDI_CORE_STREAM_H */
