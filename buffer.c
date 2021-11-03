#include "uartframeparser.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

struct uart_frame_parser_buffer {
	
	uart_frame_parser_error_callback_t on_error;
	
	FILE* fp;

	long long origin;
};

struct uart_frame_parser_buffer * uart_frame_parser_buffer_create(uart_frame_parser_error_callback_t on_error) {

	FILE* fp = tmpfile();
	assert(fp);

	struct uart_frame_parser_buffer* buffer = calloc(1, sizeof(struct uart_frame_parser_buffer));
	if (buffer)
	{
		buffer->fp = fp;
		buffer->on_error = on_error;
		return buffer;
	}
	else
	{
		fclose(fp);
		on_error(UART_FRAME_PARSER_ERROR_MALLOC, "cannot allocate a buffer");
	}
	
	return NULL;
}

void uart_frame_parser_buffer_release(struct uart_frame_parser_buffer* buffer) {
	fclose(buffer->fp);
	free(buffer);
}

void uart_frame_parser_buffer_append(struct uart_frame_parser_buffer* buffer, uint8_t* data, uint32_t size) {
	fseek(buffer->fp, 0, SEEK_END);
	assert(fwrite(data, 1, size, buffer->fp) == size);
}

int uart_frame_parser_buffer_read(struct uart_frame_parser_buffer* buffer, uint32_t offset, uint8_t* data, uint32_t size) {
	fseek(buffer->fp, 0, SEEK_END);
	long size = ftell(buffer->fp);
	if (size >= buffer->origin + offset)
	{
		fseek(buffer->fp, buffer->origin + offset, SEEK_SET);
		assert(fread(data, 1, size, buffer->fp) == size);
		return 1;
	}
	return 0;
}

uint32_t uart_frame_parser_buffer_get_size(struct uart_frame_parser_buffer* buffer) {
	fseek(buffer->fp, 0, SEEK_END);
	long size = ftell(buffer->fp);
	assert(size >= 0);
	uint32_t result = size;
	result -= buffer->origin;
	return result;
}

void uart_frame_parser_buffer_increase_origin(struct uart_frame_parser_buffer* buffer, uint32_t increasement) {
	buffer->origin += increasement;
}

int uart_frame_parser_buffer_at(struct uart_frame_parser_buffer* buffer, uint32_t offset)
{
	uint8_t result;
	if (buffer_read(buffer, offset, &result, 1))
	{
		return result;
	}
	return -1;
}
