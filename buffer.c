#include "uartframeparser.h"
#include <stdlib.h>
#include <string.h>

struct uart_frame_parser_buffer {

    uart_frame_parser_error_callback_t on_error;

    void* user_ptr;

    uint32_t capacity;

    uint32_t size;

    uint8_t *data;
};

void *
uart_frame_parser_buffer_create(uart_frame_parser_error_callback_t on_error, void* user_ptr, void *reserved) {

    (void)reserved;

    struct uart_frame_parser_buffer *buffer = calloc(1, sizeof(struct uart_frame_parser_buffer));
    if (buffer) {
        buffer->on_error = on_error;
        buffer->user_ptr = user_ptr;
        return buffer;
    } else {
        on_error(user_ptr, UART_FRAME_PARSER_ERROR_MALLOC, __FILE__, __LINE__, "cannot allocate a buffer");
    }

    return NULL;
}

void uart_frame_parser_buffer_release(void *buffer) {
    struct uart_frame_parser_buffer* _buffer = buffer;
    if (_buffer->data) {
        free(_buffer->data);
    }
    free(buffer);
}

void uart_frame_parser_buffer_append(void *buffer, const uint8_t * data, uint32_t size) {
    struct uart_frame_parser_buffer* _buffer = buffer;

    uint32_t remaining_bytes = _buffer->capacity - _buffer->size;
    if (remaining_bytes < size) {
        uint32_t new_capacity = _buffer->size + size;

        void* new_data = realloc(_buffer->data, new_capacity);
        if (new_data) {
            _buffer->data = new_data;
            _buffer->capacity = new_capacity;
        }
        else {
            _buffer->on_error(_buffer->user_ptr, UART_FRAME_PARSER_ERROR_MALLOC, __FILE__, __LINE__, "cannot reallocate buffer: new capacity %u bytes", new_capacity);
            return;
        }
    }
    
    memcpy(_buffer->data + _buffer->size, data, size);
    _buffer->size += size;
}

int
uart_frame_parser_buffer_read(void *buffer, uint32_t offset, uint8_t *data, uint32_t size) {
    struct uart_frame_parser_buffer* _buffer = buffer;
    if (offset + size <= _buffer->size) {
        memcpy(data, _buffer->data + offset, size);
        return 1;
    }
    else {
        return 0;
    }
}

uint32_t uart_frame_parser_buffer_get_size(void *buffer) {
    struct uart_frame_parser_buffer* _buffer = buffer;
    return _buffer->size;
}

void uart_frame_parser_buffer_increase_origin(void *buffer, uint32_t increment) {
    struct uart_frame_parser_buffer* _buffer = buffer;
    uint32_t new_size = _buffer->size > increment ? (_buffer->size - increment) : 0;
    if (new_size) {
        memmove(_buffer->data, _buffer->data + increment, _buffer->size - new_size);
    }
    _buffer->size = new_size;
}

int uart_frame_parser_buffer_at(void *buffer, uint32_t offset) {
    struct uart_frame_parser_buffer* _buffer = buffer;
    return _buffer->size > offset ? _buffer->data[offset] : -1;
}
