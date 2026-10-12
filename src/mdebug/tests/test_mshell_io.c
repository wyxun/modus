#include "mdebug/mshell.h"
#include "mbase.h"
#include "mdebug/util_debug.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static char s_input[512];
static size_t s_input_length;
static size_t s_input_offset;
static char s_output[4096];
static size_t s_output_length;
static unsigned s_handler_calls;
static char s_last_args[MSHELL_LINE_SIZE];

uint8_t g_chGLogMask = 0U;
const struct {
    uint8_t chPurpose;
    uint8_t chInterface;
    uint8_t chMajor;
    uint8_t chMinor;
} MODUSVersion = { 0U, 6U, 1U, 3U };

mlist_t *mbase_GetBaseList(void)
{
    return NULL;
}

int mbase_EventPost(uint32_t id, uint32_t event)
{
    (void)id;
    (void)event;
    return MODUS_SUCCESS;
}

void util_debug_Printf(const char *format, ...)
{
    (void)format;
}

static unsigned fake_read(char *buffer, unsigned capacity)
{
    size_t remaining = s_input_length - s_input_offset;
    size_t count = remaining < capacity ? remaining : capacity;
    memcpy(buffer, &s_input[s_input_offset], count);
    s_input_offset += count;
    return (unsigned)count;
}

static void fake_write(const char *buffer, unsigned length)
{
    size_t available = sizeof(s_output) - s_output_length - 1U;
    size_t count = length < available ? length : available;
    memcpy(&s_output[s_output_length], buffer, count);
    s_output_length += count;
    s_output[s_output_length] = '\0';
}

static void capture_command(const char *args)
{
    s_handler_calls++;
    (void)snprintf(s_last_args, sizeof(s_last_args), "%s", args);
}

static const mshell_cmd_t s_capture_command = {
    "capture", capture_command, "Capture arguments"
};

static void feed_and_poll(const char *text, size_t length)
{
    if (s_input_offset == s_input_length) {
        s_input_offset = 0U;
        s_input_length = 0U;
    }
    assert(length <= sizeof(s_input) - s_input_length);
    memcpy(&s_input[s_input_length], text, length);
    s_input_length += length;
    while (s_input_offset < s_input_length) {
        mshell_Poll();
    }
}

int main(void)
{
    const mshell_io_t io = { fake_read, fake_write };
    char full_line[MSHELL_LINE_SIZE + 2U];
    size_t command_length = strlen("capture ");

    assert(mshell_RegisterCmd((uintptr_t)&s_capture_command, 0U)
           == MODUS_SUCCESS);
    mshell_SetIO(&io);
    feed_and_poll("capture\tone two\r\n", strlen("capture\tone two\r\n"));
    assert(s_handler_calls == 1U);
    assert(strcmp(s_last_args, "one two") == 0);

    memcpy(full_line, "capture ", command_length);
    memset(&full_line[command_length], 'x', MSHELL_LINE_SIZE - 1U
           - command_length);
    full_line[MSHELL_LINE_SIZE - 1U] = '\n';
    feed_and_poll(full_line, MSHELL_LINE_SIZE);
    assert(s_handler_calls == 2U);
    assert(strlen(s_last_args) == MSHELL_LINE_SIZE - 1U - command_length);

    memset(full_line, 'x', MSHELL_LINE_SIZE + 1U);
    memcpy(full_line, "capture ", command_length);
    full_line[MSHELL_LINE_SIZE + 1U] = '\n';
    feed_and_poll(full_line, MSHELL_LINE_SIZE + 2U);
    assert(s_handler_calls == 2U);
    assert(strstr(s_output, "Input line too long.") != NULL);

    feed_and_poll("capture\trecovered\n", strlen("capture\trecovered\n"));
    assert(s_handler_calls == 3U);
    assert(strcmp(s_last_args, "recovered") == 0);

    feed_and_poll("capture abc\bX\n", strlen("capture abc\bX\n"));
    assert(s_handler_calls == 4U);
    assert(strcmp(s_last_args, "abX") == 0);

    mshell_Write("direct reply\r\n");
    assert(strstr(s_output, "direct reply\r\n") != NULL);
    return 0;
}
