#include "uartframeparser.h"
#include <stdio.h>


struct uart_frame_parser_buffer {
	
	uart_frame_parser_error_callback_t on_error;
	
	FILE* fp;

};

struct uart_frame_parser_buffer * buffer_create(uart_frame_parser_error_callback_t on_error) {

}

void buffer_release(struct uart_frame_parser_buffer* buffer) {

}

void buffer_append(struct uart_frame_parser_buffer* buffer, uint8_t* data, uint32_t size) {

}

void buffer_read(struct uart_frame_parser_buffer* buffer, uint8_t* data, uint32_t size) {

}

uint32_t buffer_get_size(struct uart_frame_parser_buffer* buffer) {

}

void buffer_clear(struct uart_frame_parser_buffer* buffer) {

}
