#ifndef __MODUS_MSHELL_PARSE_H__
#define __MODUS_MSHELL_PARSE_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** @brief Result of parsing one MShell argument. */
typedef enum {
    MSHELL_PARSE_OK = 0,       /**< One token was parsed. */
    MSHELL_PARSE_END,          /**< No token remains after whitespace. */
    MSHELL_PARSE_INVALID,      /**< Token syntax is invalid. */
    MSHELL_PARSE_RANGE,        /**< Value exceeds the destination type. */
    MSHELL_PARSE_BAD_ARGUMENT  /**< A required pointer is NULL. */
} mshell_parse_result_t;

/** @brief Non-owning view of one input token. */
typedef struct {
    const char *data;
    size_t length;
} mshell_token_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Skip leading ASCII spaces and tabs.
 * @param text Input string; NULL is returned unchanged.
 * @return Pointer to the first non-space character.
 */
const char *mshell_SkipSpaces(const char *text);

/**
 * @brief Consume a complete, case-sensitive word token.
 * @param cursor Input cursor, advanced only when the word matches.
 * @param word Non-empty word without spaces or tabs.
 * @return true when the complete token matches; otherwise false.
 */
bool mshell_ConsumeWord(const char **cursor, const char *word);

/**
 * @brief Check whether only spaces and tabs remain.
 * @param cursor Input string.
 * @return true at end of input; NULL returns false.
 */
bool mshell_AtEnd(const char *cursor);

/**
 * @brief Read one non-owning token without modifying the input string.
 * @param cursor Input cursor, advanced on success only.
 * @param token Receives the token view on success.
 * @return Parse status; token data remains valid as long as input does.
 */
mshell_parse_result_t mshell_NextToken(const char **cursor,
                                       mshell_token_t *token);

/**
 * @brief Parse a signed decimal integer in int8 range.
 * @param cursor Input cursor; advances on success only.
 * @param value Receives the parsed value on success.
 * @return Parse status.
 * @note Scalar parsers skip leading spaces/tabs and consume a complete token.
 *       On any result other than MSHELL_PARSE_OK, cursor and value are intact.
 */
mshell_parse_result_t mshell_ParseInt8(const char **cursor, int8_t *value);
/** @copydoc mshell_ParseInt8 */
mshell_parse_result_t mshell_ParseInt16(const char **cursor, int16_t *value);
/** @copydoc mshell_ParseInt8 */
mshell_parse_result_t mshell_ParseInt32(const char **cursor, int32_t *value);
/** @copydoc mshell_ParseInt8 */
mshell_parse_result_t mshell_ParseInt64(const char **cursor, int64_t *value);
/**
 * @brief Parse an unsigned decimal integer in uint8 range.
 * @param cursor Input cursor; advances on success only.
 * @param value Receives the parsed value on success.
 * @return Parse status.
 */
mshell_parse_result_t mshell_ParseUint8(const char **cursor, uint8_t *value);
/** @copydoc mshell_ParseUint8 */
mshell_parse_result_t mshell_ParseUint16(const char **cursor, uint16_t *value);
/** @copydoc mshell_ParseUint8 */
mshell_parse_result_t mshell_ParseUint32(const char **cursor, uint32_t *value);
/** @copydoc mshell_ParseUint8 */
mshell_parse_result_t mshell_ParseUint64(const char **cursor, uint64_t *value);
/**
 * @brief Parse a finite decimal float: optional sign and one decimal point.
 * @param cursor Input cursor; advances on success only.
 * @param value Receives the parsed value on success only.
 * @return Parse status; overflow is RANGE, malformed tokens are INVALID.
 * @note At least one digit is required; 1, -0.05, +1.25, .5 and 1. are valid.
 *       Exponents, hex floats, NaN, Inf and unit suffixes are not supported.
 *       Uses single-precision arithmetic, not correctly rounded strtof.
 *       Underflow may round to signed zero; hardware subnormal mode applies.
 */
mshell_parse_result_t mshell_ParseFloat(const char **cursor, float *value);
/**
 * @brief Parse true/false, on/off, or 1/0 in lowercase.
 * @param cursor Input cursor; advances on success only.
 * @param value Receives the parsed value on success.
 * @return Parse status.
 */
mshell_parse_result_t mshell_ParseBool(const char **cursor, bool *value);

/**
 * @brief Parse hexadecimal with an optional 0x prefix and uint8 range.
 * @param cursor Input cursor; advances on success only.
 * @param value Receives the parsed value on success.
 * @return Parse status.
 */
mshell_parse_result_t mshell_ParseHexUint8(const char **cursor,
                                           uint8_t *value);
/** @copydoc mshell_ParseHexUint8 */
mshell_parse_result_t mshell_ParseHexUint16(const char **cursor,
                                            uint16_t *value);
/** @copydoc mshell_ParseHexUint8 */
mshell_parse_result_t mshell_ParseHexUint32(const char **cursor,
                                            uint32_t *value);
/** @copydoc mshell_ParseHexUint8 */
mshell_parse_result_t mshell_ParseHexUint64(const char **cursor,
                                            uint64_t *value);

#ifdef __cplusplus
}
#endif

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) \
    && !defined(__cplusplus)
#define MSHELL_PARSE_SELECT(value) _Generic((value), \
    int8_t *: mshell_ParseInt8, \
    int16_t *: mshell_ParseInt16, \
    int32_t *: mshell_ParseInt32, \
    int64_t *: mshell_ParseInt64, \
    uint8_t *: mshell_ParseUint8, \
    uint16_t *: mshell_ParseUint16, \
    uint32_t *: mshell_ParseUint32, \
    uint64_t *: mshell_ParseUint64, \
    bool *: mshell_ParseBool, \
    float *: mshell_ParseFloat)

#define MSHELL_PARSE_HEX_SELECT(value) _Generic((value), \
    uint8_t *: mshell_ParseHexUint8, \
    uint16_t *: mshell_ParseHexUint16, \
    uint32_t *: mshell_ParseHexUint32, \
    uint64_t *: mshell_ParseHexUint64)

#define mshell_Parse(cursor, value) \
    MSHELL_PARSE_SELECT(value)((cursor), (value))
#define mshell_ParseHex(cursor, value) \
    MSHELL_PARSE_HEX_SELECT(value)((cursor), (value))
#endif

#endif
