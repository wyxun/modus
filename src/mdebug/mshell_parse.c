/**
 * @file mshell_parse.c
 * @brief Stateless token and scalar parsing for MShell commands.
 * @author MODUS contributors
 * @date 2026-10-12
 */
#include "mshell_parse.h"

#include <limits.h>
#include <string.h>

/**
 * @brief Convert one ASCII decimal or hexadecimal digit.
 * @param value Character to convert.
 * @return Digit value, or -1 when the character is not a digit.
 */
static int digit_value(char value)
{
    if (value >= '0' && value <= '9') {
        return (int)(value - '0');
    }
    if (value >= 'a' && value <= 'f') {
        return (int)(value - 'a') + 10;
    }
    if (value >= 'A' && value <= 'F') {
        return (int)(value - 'A') + 10;
    }
    return -1;
}

const char *mshell_SkipSpaces(const char *text)
{
    if (text == NULL) {
        return NULL;
    }
    while (*text == ' ' || *text == '\t') {
        text++;
    }
    return text;
}

bool mshell_AtEnd(const char *cursor)
{
    const char *text = mshell_SkipSpaces(cursor);
    return text != NULL && *text == '\0';
}

mshell_parse_result_t mshell_NextToken(const char **cursor,
                                       mshell_token_t *token)
{
    if (cursor == NULL || token == NULL || *cursor == NULL) {
        return MSHELL_PARSE_BAD_ARGUMENT;
    }

    const char *start = mshell_SkipSpaces(*cursor);
    if (*start == '\0') {
        return MSHELL_PARSE_END;
    }

    const char *end = start;
    while (*end != '\0' && *end != ' ' && *end != '\t') {
        end++;
    }

    mshell_token_t parsed = { start, (size_t)(end - start) };
    *token = parsed;
    *cursor = end;
    return MSHELL_PARSE_OK;
}

bool mshell_ConsumeWord(const char **cursor, const char *word)
{
    if (cursor == NULL || *cursor == NULL || word == NULL || *word == '\0') {
        return false;
    }

    for (const char *p = word; *p != '\0'; p++) {
        if (*p == ' ' || *p == '\t') {
            return false;
        }
    }

    const char *next = *cursor;
    mshell_token_t token = { NULL, 0U };
    if (mshell_NextToken(&next, &token) != MSHELL_PARSE_OK) {
        return false;
    }
    if (strlen(word) != token.length ||
        memcmp(token.data, word, token.length) != 0) {
        return false;
    }

    *cursor = next;
    return true;
}

/**
 * @brief Parse and range-check one unsigned token magnitude.
 * @param cursor Input cursor; advances on success only.
 * @param base Numeric base, either 10 or 16.
 * @param allow_sign Whether a leading sign is accepted.
 * @param allow_prefix Whether a hexadecimal prefix is accepted.
 * @param limit Largest accepted magnitude.
 * @param magnitude Receives the parsed magnitude.
 * @param negative Receives the parsed sign.
 * @return Parse status.
 */
static mshell_parse_result_t parse_magnitude(const char **cursor,
                                             uint8_t base,
                                             bool allow_sign,
                                             bool allow_prefix,
                                             uint64_t limit,
                                             uint64_t *magnitude,
                                             bool *negative)
{
    if (cursor == NULL || magnitude == NULL || negative == NULL ||
        *cursor == NULL) {
        return MSHELL_PARSE_BAD_ARGUMENT;
    }

    const char *start = mshell_SkipSpaces(*cursor);
    const char *p = start;
    bool is_negative = false;
    bool invalid = false;
    bool overflow = false;
    bool has_digit = false;
    uint64_t result = 0U;

    if (*start == '\0') {
        return MSHELL_PARSE_END;
    }

    if (allow_sign && (*p == '+' || *p == '-')) {
        is_negative = (*p == '-');
        p++;
    } else if (*p == '+' || *p == '-') {
        return MSHELL_PARSE_INVALID;
    }

    if (allow_prefix && p[0] == '0' &&
        (p[1] == 'x' || p[1] == 'X')) {
        p += 2;
    }

    const char *digits = p;
    while (*p != '\0' && *p != ' ' && *p != '\t') {
        int digit = digit_value(*p);
        if (digit < 0 || digit >= base) {
            invalid = true;
        } else {
            has_digit = true;
            if (result > (limit - (uint64_t)digit) / base) {
                overflow = true;
            } else if (!overflow) {
                result = result * base + (uint64_t)digit;
            }
        }
        p++;
    }

    if (!has_digit || p == digits || invalid) {
        return MSHELL_PARSE_INVALID;
    }
    if (overflow) {
        return MSHELL_PARSE_RANGE;
    }

    *magnitude = result;
    *negative = is_negative;
    *cursor = p;
    return MSHELL_PARSE_OK;
}

/**
 * @brief Parse one unsigned integer without changing outputs on failure.
 * @param cursor Input cursor.
 * @param limit Largest accepted value.
 * @param base Numeric base.
 * @param allow_prefix Accept an optional hexadecimal prefix.
 * @param value Receives the parsed value.
 * @return Parse status.
 */
static mshell_parse_result_t parse_unsigned(const char **cursor,
                                            uint64_t limit,
                                            uint8_t base,
                                            bool allow_prefix,
                                            uint64_t *value)
{
    if (value == NULL) {
        return MSHELL_PARSE_BAD_ARGUMENT;
    }

    const char *next = (cursor != NULL) ? *cursor : NULL;
    uint64_t magnitude = 0U;
    bool negative = false;
    mshell_parse_result_t result = parse_magnitude(
        &next, base, false, allow_prefix, limit, &magnitude, &negative);
    if (result == MSHELL_PARSE_OK) {
        *value = magnitude;
        *cursor = next;
    }
    return result;
}

/**
 * @brief Parse one signed decimal integer without changing outputs on failure.
 * @param cursor Input cursor.
 * @param positive_limit Largest positive magnitude.
 * @param negative_limit Largest negative magnitude.
 * @param negative_minimum Representable minimum value.
 * @param value Receives the parsed value.
 * @return Parse status.
 */
static mshell_parse_result_t parse_signed(const char **cursor,
                                         uint64_t positive_limit,
                                         uint64_t negative_limit,
                                         int64_t negative_minimum,
                                         int64_t *value)
{
    if (value == NULL) {
        return MSHELL_PARSE_BAD_ARGUMENT;
    }

    const char *next = (cursor != NULL) ? *cursor : NULL;
    uint64_t magnitude = 0U;
    bool negative = false;
    uint64_t limit = positive_limit;
    mshell_parse_result_t result;

    /* Parse sign first to select the signed range without changing the input. */
    const char *start = mshell_SkipSpaces(next);
    if (start == NULL) {
        return MSHELL_PARSE_BAD_ARGUMENT;
    }
    if (*start == '-') {
        limit = negative_limit;
    }

    result = parse_magnitude(&next, 10U, true, false, limit,
                             &magnitude, &negative);
    if (result != MSHELL_PARSE_OK) {
        return result;
    }

    int64_t parsed = 0;
    if (negative) {
        parsed = (magnitude == negative_limit)
               ? negative_minimum : -(int64_t)magnitude;
    } else {
        parsed = (int64_t)magnitude;
    }
    *value = parsed;
    *cursor = next;
    return MSHELL_PARSE_OK;
}

mshell_parse_result_t mshell_ParseInt8(const char **cursor, int8_t *value)
{
    if (value == NULL) {
        return MSHELL_PARSE_BAD_ARGUMENT;
    }
    int64_t parsed = 0;
    mshell_parse_result_t result = parse_signed(
        cursor, INT8_MAX, (uint64_t)INT8_MAX + 1U, INT8_MIN, &parsed);
    if (result == MSHELL_PARSE_OK) {
        *value = (int8_t)parsed;
    }
    return result;
}

mshell_parse_result_t mshell_ParseInt16(const char **cursor, int16_t *value)
{
    if (value == NULL) {
        return MSHELL_PARSE_BAD_ARGUMENT;
    }
    int64_t parsed = 0;
    mshell_parse_result_t result = parse_signed(
        cursor, INT16_MAX, (uint64_t)INT16_MAX + 1U, INT16_MIN, &parsed);
    if (result == MSHELL_PARSE_OK) {
        *value = (int16_t)parsed;
    }
    return result;
}

mshell_parse_result_t mshell_ParseInt32(const char **cursor, int32_t *value)
{
    if (value == NULL) {
        return MSHELL_PARSE_BAD_ARGUMENT;
    }
    int64_t parsed = 0;
    mshell_parse_result_t result = parse_signed(
        cursor, INT32_MAX, (uint64_t)INT32_MAX + 1U, INT32_MIN, &parsed);
    if (result == MSHELL_PARSE_OK) {
        *value = (int32_t)parsed;
    }
    return result;
}

mshell_parse_result_t mshell_ParseInt64(const char **cursor, int64_t *value)
{
    return parse_signed(cursor, (uint64_t)INT64_MAX,
                        (uint64_t)INT64_MAX + 1U, INT64_MIN, value);
}

mshell_parse_result_t mshell_ParseUint8(const char **cursor, uint8_t *value)
{
    if (value == NULL) {
        return MSHELL_PARSE_BAD_ARGUMENT;
    }
    uint64_t parsed = 0U;
    mshell_parse_result_t result = parse_unsigned(
        cursor, UINT8_MAX, 10U, false, &parsed);
    if (result == MSHELL_PARSE_OK) {
        *value = (uint8_t)parsed;
    }
    return result;
}

mshell_parse_result_t mshell_ParseUint16(const char **cursor, uint16_t *value)
{
    if (value == NULL) {
        return MSHELL_PARSE_BAD_ARGUMENT;
    }
    uint64_t parsed = 0U;
    mshell_parse_result_t result = parse_unsigned(
        cursor, UINT16_MAX, 10U, false, &parsed);
    if (result == MSHELL_PARSE_OK) {
        *value = (uint16_t)parsed;
    }
    return result;
}

mshell_parse_result_t mshell_ParseUint32(const char **cursor, uint32_t *value)
{
    if (value == NULL) {
        return MSHELL_PARSE_BAD_ARGUMENT;
    }
    uint64_t parsed = 0U;
    mshell_parse_result_t result = parse_unsigned(
        cursor, UINT32_MAX, 10U, false, &parsed);
    if (result == MSHELL_PARSE_OK) {
        *value = (uint32_t)parsed;
    }
    return result;
}

mshell_parse_result_t mshell_ParseUint64(const char **cursor, uint64_t *value)
{
    return parse_unsigned(cursor, UINT64_MAX, 10U, false, value);
}

/**
 * @brief Parse one supported lowercase boolean token.
 * @param cursor Input cursor; advances on success only.
 * @param value Receives the parsed value.
 * @return Parse status.
 */
static mshell_parse_result_t parse_bool(const char **cursor, bool *value)
{
    if (cursor == NULL || value == NULL || *cursor == NULL) {
        return MSHELL_PARSE_BAD_ARGUMENT;
    }

    const char *next = *cursor;
    bool parsed = false;
    if (mshell_ConsumeWord(&next, "true") ||
        mshell_ConsumeWord(&next, "on") ||
        mshell_ConsumeWord(&next, "1")) {
        parsed = true;
    } else if (mshell_ConsumeWord(&next, "false") ||
               mshell_ConsumeWord(&next, "off") ||
               mshell_ConsumeWord(&next, "0")) {
        parsed = false;
    } else {
        mshell_token_t token = { NULL, 0U };
        return (mshell_NextToken(&next, &token) == MSHELL_PARSE_END)
             ? MSHELL_PARSE_END : MSHELL_PARSE_INVALID;
    }

    *cursor = next;
    *value = parsed;
    return MSHELL_PARSE_OK;
}

mshell_parse_result_t mshell_ParseBool(const char **cursor, bool *value)
{
    return parse_bool(cursor, value);
}

mshell_parse_result_t mshell_ParseHexUint8(const char **cursor,
                                           uint8_t *value)
{
    if (value == NULL) {
        return MSHELL_PARSE_BAD_ARGUMENT;
    }
    uint64_t parsed = 0U;
    mshell_parse_result_t result = parse_unsigned(
        cursor, UINT8_MAX, 16U, true, &parsed);
    if (result == MSHELL_PARSE_OK) {
        *value = (uint8_t)parsed;
    }
    return result;
}

mshell_parse_result_t mshell_ParseHexUint16(const char **cursor,
                                            uint16_t *value)
{
    if (value == NULL) {
        return MSHELL_PARSE_BAD_ARGUMENT;
    }
    uint64_t parsed = 0U;
    mshell_parse_result_t result = parse_unsigned(
        cursor, UINT16_MAX, 16U, true, &parsed);
    if (result == MSHELL_PARSE_OK) {
        *value = (uint16_t)parsed;
    }
    return result;
}

mshell_parse_result_t mshell_ParseHexUint32(const char **cursor,
                                            uint32_t *value)
{
    if (value == NULL) {
        return MSHELL_PARSE_BAD_ARGUMENT;
    }
    uint64_t parsed = 0U;
    mshell_parse_result_t result = parse_unsigned(
        cursor, UINT32_MAX, 16U, true, &parsed);
    if (result == MSHELL_PARSE_OK) {
        *value = (uint32_t)parsed;
    }
    return result;
}

mshell_parse_result_t mshell_ParseHexUint64(const char **cursor,
                                            uint64_t *value)
{
    return parse_unsigned(cursor, UINT64_MAX, 16U, true, value);
}
