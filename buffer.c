#include "uartframeparser.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

struct uart_frame_parser_buffer {

    uart_frame_parser_error_callback_t on_error;

    FILE *fp;

    uint32_t origin;
};

void *
uart_frame_parser_buffer_create(uart_frame_parser_error_callback_t on_error, void *reserved) {

    FILE *fp;
    if (reserved) {
        fp = fopen(reserved, "rw");
    } else {
        fp = tmpfile();
    }
    assert(fp);

    struct uart_frame_parser_buffer *buffer = calloc(1, sizeof(struct uart_frame_parser_buffer));
    if (buffer) {
        buffer->fp = fp;
        buffer->on_error = on_error;
        return buffer;
    } else {
        fclose(fp);
        on_error(UART_FRAME_PARSER_ERROR_MALLOC, __FILE__, __LINE__, "cannot allocate a buffer");
    }

    return NULL;
}

void uart_frame_parser_buffer_release(void *buffer) {
    fclose(((struct uart_frame_parser_buffer*)buffer)->fp);
    free(buffer);
}

void uart_frame_parser_buffer_append(void *buffer, uint8_t *data, uint32_t size) {
    fseek(((struct uart_frame_parser_buffer*)buffer)->fp, 0, SEEK_END);
    assert(fwrite(data, 1, size, ((struct uart_frame_parser_buffer*)buffer)->fp) == size);
}

int
uart_frame_parser_buffer_read(void *buffer, uint32_t offset, uint8_t *data, uint32_t size) {
    fseek(((struct uart_frame_parser_buffer*)buffer)->fp, 0, SEEK_END);
    long filesize = ftell(((struct uart_frame_parser_buffer*)buffer)->fp);
    if (filesize >= (long)(((struct uart_frame_parser_buffer*)buffer)->origin + offset + size)) {
        fseek(((struct uart_frame_parser_buffer*)buffer)->fp, ((long)((struct uart_frame_parser_buffer*)buffer)->origin) + offset, SEEK_SET);
        assert(fread(data, 1, size, ((struct uart_frame_parser_buffer*)buffer)->fp) == size);
        return 1;
    }
    return 0;
}

uint32_t uart_frame_parser_buffer_get_size(void *buffer) {
    fseek(((struct uart_frame_parser_buffer*)buffer)->fp, 0, SEEK_END);
    long size = ftell(((struct uart_frame_parser_buffer*)buffer)->fp);
    assert(size >= 0);
    uint32_t result = size;
    result -= ((struct uart_frame_parser_buffer*)buffer)->origin;
    return result;
}

void uart_frame_parser_buffer_increase_origin(void *buffer, uint32_t increment) {
    ((struct uart_frame_parser_buffer*)buffer)->origin += increment;
}

int uart_frame_parser_buffer_at(void *buffer, uint32_t offset) {
    uint8_t result;
    if (uart_frame_parser_buffer_read(buffer, offset, &result, 1)) {
        return result;
    }
    return -1;
}
