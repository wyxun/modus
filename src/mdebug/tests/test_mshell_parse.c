#include "mshell_parse.h"

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

static void test_words_and_tokens(void)
{
    const char *cursor = "  start\tvalue rest";
    mshell_token_t token = { NULL, 0U };

    assert(mshell_ConsumeWord(&cursor, "start"));
    assert(!mshell_ConsumeWord(&cursor, "starter"));
    assert(mshell_NextToken(&cursor, &token) == MSHELL_PARSE_OK);
    assert(token.length == 5U);
    assert(memcmp(token.data, "value", token.length) == 0);
    assert(!mshell_AtEnd(cursor));
    assert(mshell_NextToken(&cursor, &token) == MSHELL_PARSE_OK);
    assert(token.length == 4U);
    assert(memcmp(token.data, "rest", token.length) == 0);
    assert(mshell_AtEnd(cursor));
    assert(mshell_ConsumeWord(&cursor, "") == false);
    assert(mshell_SkipSpaces(NULL) == NULL);
    assert(!mshell_AtEnd(NULL));
}

static void test_unsigned_boundaries(void)
{
    const char *cursor = "255 x";
    uint8_t u8 = 7U;
    assert(mshell_Parse(&cursor, &u8) == MSHELL_PARSE_OK);
    assert(u8 == UINT8_MAX);
    assert(*cursor == ' ');

    cursor = "256";
    u8 = 9U;
    assert(mshell_Parse(&cursor, &u8) == MSHELL_PARSE_RANGE);
    assert(cursor[0] == '2' && u8 == 9U);

    cursor = "65535";
    uint16_t u16 = 0U;
    assert(mshell_Parse(&cursor, &u16) == MSHELL_PARSE_OK);
    assert(u16 == UINT16_MAX);
    cursor = "65536";
    u16 = 3U;
    assert(mshell_Parse(&cursor, &u16) == MSHELL_PARSE_RANGE);
    assert(cursor[0] == '6' && u16 == 3U);

    cursor = "4294967295";
    uint32_t u32 = 0U;
    assert(mshell_Parse(&cursor, &u32) == MSHELL_PARSE_OK);
    assert(u32 == UINT32_MAX);
    cursor = "4294967296";
    u32 = 5U;
    assert(mshell_Parse(&cursor, &u32) == MSHELL_PARSE_RANGE);
    assert(cursor[0] == '4' && u32 == 5U);

    cursor = "18446744073709551615";
    uint64_t u64 = 0U;
    assert(mshell_Parse(&cursor, &u64) == MSHELL_PARSE_OK);
    assert(u64 == UINT64_MAX);

    cursor = "18446744073709551616";
    u64 = 11U;
    assert(mshell_Parse(&cursor, &u64) == MSHELL_PARSE_RANGE);
    assert(cursor[0] == '1' && u64 == 11U);
}

static void test_signed_boundaries(void)
{
    const char *cursor = "-128";
    int8_t s8 = 0;
    assert(mshell_Parse(&cursor, &s8) == MSHELL_PARSE_OK);
    assert(s8 == INT8_MIN);

    cursor = "+127";
    assert(mshell_Parse(&cursor, &s8) == MSHELL_PARSE_OK);
    assert(s8 == INT8_MAX);

    cursor = "128";
    s8 = 1;
    assert(mshell_Parse(&cursor, &s8) == MSHELL_PARSE_RANGE);
    assert(cursor[0] == '1' && s8 == 1);

    cursor = "-129";
    s8 = 6;
    assert(mshell_Parse(&cursor, &s8) == MSHELL_PARSE_RANGE);
    assert(cursor[0] == '-' && s8 == 6);

    cursor = "-32768";
    int16_t s16 = 0;
    assert(mshell_Parse(&cursor, &s16) == MSHELL_PARSE_OK);
    assert(s16 == INT16_MIN);
    cursor = "32767";
    assert(mshell_Parse(&cursor, &s16) == MSHELL_PARSE_OK);
    assert(s16 == INT16_MAX);
    cursor = "-32769";
    s16 = 7;
    assert(mshell_Parse(&cursor, &s16) == MSHELL_PARSE_RANGE);
    assert(cursor[0] == '-' && s16 == 7);

    cursor = "-2147483648";
    int32_t s32 = 0;
    assert(mshell_Parse(&cursor, &s32) == MSHELL_PARSE_OK);
    assert(s32 == INT32_MIN);

    cursor = "2147483647";
    assert(mshell_Parse(&cursor, &s32) == MSHELL_PARSE_OK);
    assert(s32 == INT32_MAX);
    cursor = "-2147483649";
    s32 = 8;
    assert(mshell_Parse(&cursor, &s32) == MSHELL_PARSE_RANGE);
    assert(cursor[0] == '-' && s32 == 8);

    cursor = "-9223372036854775808";
    int64_t s64 = 0;
    assert(mshell_Parse(&cursor, &s64) == MSHELL_PARSE_OK);
    assert(s64 == INT64_MIN);

    cursor = "9223372036854775807";
    assert(mshell_Parse(&cursor, &s64) == MSHELL_PARSE_OK);
    assert(s64 == INT64_MAX);
    cursor = "9223372036854775808";
    s64 = 13;
    assert(mshell_Parse(&cursor, &s64) == MSHELL_PARSE_RANGE);
    assert(cursor[0] == '9' && s64 == 13);
}

static void test_lexical_errors_and_bool(void)
{
    const char *cursor = "12abc";
    uint32_t value = 41U;
    assert(mshell_Parse(&cursor, &value) == MSHELL_PARSE_INVALID);
    assert(cursor[0] == '1' && value == 41U);

    cursor = "999999999999999999999x";
    assert(mshell_Parse(&cursor, &value) == MSHELL_PARSE_INVALID);

    char long_number[302];
    memset(long_number, '9', sizeof(long_number) - 1U);
    long_number[sizeof(long_number) - 1U] = '\0';
    cursor = long_number;
    assert(mshell_Parse(&cursor, &value) == MSHELL_PARSE_RANGE);
    assert(cursor == long_number && value == 41U);

    cursor = "-1";
    assert(mshell_Parse(&cursor, &value) == MSHELL_PARSE_INVALID);
    cursor = "+1";
    assert(mshell_Parse(&cursor, &value) == MSHELL_PARSE_INVALID);

    cursor = "010";
    assert(mshell_Parse(&cursor, &value) == MSHELL_PARSE_OK);
    assert(value == 10U);

    cursor = "true";
    bool enabled = false;
    assert(mshell_Parse(&cursor, &enabled) == MSHELL_PARSE_OK);
    assert(enabled);
    cursor = "off";
    assert(mshell_Parse(&cursor, &enabled) == MSHELL_PARSE_OK);
    assert(!enabled);
    cursor = "1";
    assert(mshell_Parse(&cursor, &enabled) == MSHELL_PARSE_OK);
    assert(enabled);
    cursor = "0";
    assert(mshell_Parse(&cursor, &enabled) == MSHELL_PARSE_OK);
    assert(!enabled);
    cursor = "false";
    assert(mshell_Parse(&cursor, &enabled) == MSHELL_PARSE_OK);
    assert(!enabled);
    cursor = "on";
    assert(mshell_Parse(&cursor, &enabled) == MSHELL_PARSE_OK);
    assert(enabled);
    cursor = "off";
    assert(mshell_Parse(&cursor, &enabled) == MSHELL_PARSE_OK);
    assert(!enabled);
    cursor = "True";
    enabled = true;
    assert(mshell_Parse(&cursor, &enabled) == MSHELL_PARSE_INVALID);
    assert(cursor[0] == 'T' && enabled);
}

static void test_hex_and_empty_input(void)
{
    const char *cursor = "0XfF rest";
    uint32_t value = 0U;
    bool enabled = true;
    mshell_token_t token = { NULL, 0U };
    assert(mshell_ParseHex(&cursor, &value) == MSHELL_PARSE_OK);
    assert(value == 255U && *cursor == ' ');

    cursor = "ff";
    assert(mshell_ParseHex(&cursor, &value) == MSHELL_PARSE_OK);
    assert(value == 255U);

    cursor = "ff";
    uint8_t hex8 = 0U;
    assert(mshell_ParseHex(&cursor, &hex8) == MSHELL_PARSE_OK);
    assert(hex8 == UINT8_MAX);
    cursor = "ffff";
    uint16_t hex16 = 0U;
    assert(mshell_ParseHex(&cursor, &hex16) == MSHELL_PARSE_OK);
    assert(hex16 == UINT16_MAX);
    cursor = "ffffffffffffffff";
    uint64_t hex64 = 0U;
    assert(mshell_ParseHex(&cursor, &hex64) == MSHELL_PARSE_OK);
    assert(hex64 == UINT64_MAX);
    cursor = "10000000000000000";
    hex64 = 3U;
    assert(mshell_ParseHex(&cursor, &hex64) == MSHELL_PARSE_RANGE);
    assert(cursor[0] == '1' && hex64 == 3U);
    cursor = "100";
    hex8 = 2U;
    assert(mshell_ParseHex(&cursor, &hex8) == MSHELL_PARSE_RANGE);
    assert(cursor[0] == '1' && hex8 == 2U);

    cursor = "0x";
    value = 17U;
    assert(mshell_ParseHex(&cursor, &value) == MSHELL_PARSE_INVALID);
    assert(cursor[0] == '0' && value == 17U);

    cursor = "  \t";
    assert(mshell_Parse(&cursor, &value) == MSHELL_PARSE_END);
    assert(cursor[0] == ' ');
    assert(value == 17U);
    assert(mshell_NextToken(&cursor, &token) == MSHELL_PARSE_END);
    assert(cursor[0] == ' ' && token.data == NULL && token.length == 0U);
    assert(mshell_NextToken(&cursor, NULL) == MSHELL_PARSE_BAD_ARGUMENT);
    assert(mshell_ParseUint32(NULL, &value) == MSHELL_PARSE_BAD_ARGUMENT);
    assert(mshell_ParseUint32(&cursor, NULL) == MSHELL_PARSE_BAD_ARGUMENT);
    const char *null_cursor = NULL;
    assert(mshell_ParseUint32(&null_cursor, &value)
           == MSHELL_PARSE_BAD_ARGUMENT);
    assert(null_cursor == NULL);
    cursor = "  \t";
    enabled = true;
    assert(mshell_ParseBool(&cursor, &enabled) == MSHELL_PARSE_END);
    assert(cursor[0] == ' ' && enabled);
}

static void test_generic_argument_is_evaluated_once(void)
{
    const char *cursor = "4";
    uint32_t values[2] = { 0U, 0U };
    unsigned index = 0U;

    assert(mshell_Parse(&cursor, &values[index++]) == MSHELL_PARSE_OK);
    assert(index == 1U && values[0] == 4U);
}

int main(void)
{
    test_words_and_tokens();
    test_unsigned_boundaries();
    test_signed_boundaries();
    test_lexical_errors_and_bool();
    test_hex_and_empty_input();
    test_generic_argument_is_evaluated_once();
    return 0;
}
