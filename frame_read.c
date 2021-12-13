#include "uartframeparser.h"
#include <stdlib.h>
#include <stddef.h>

#define bitset_size(bits)         (((uint64_t)(bits) / 8) + ((uint64_t)(bits) & 0xf ? 1ull : 0ull))
#define bitset_set(bitset, bit)   ((bitset)[(bit) / 8] |=  (1 << ((bit) % 8)))
#define bitset_get(bitset, bit)   ((bitset)[(bit) / 8] &   (1 << ((bit) % 8)))
#define bitset_unset(bitset, bit) ((bitset)[(bit) / 8] &= ~(1 << ((bit) % 8)))

static struct uart_frame_field_data* read_fields(void* buffer,
	struct uart_frame_field_info* field_info_head,
	uart_frame_parser_error_callback_t on_error, void* user_ptr);

static void bitfield_data_release(struct uart_frame_bitfield_data* bitfield_data_head) {
	while (bitfield_data_head) {
		struct uart_frame_bitfield_data* next = bitfield_data_head->next;
		free(bitfield_data_head);
		bitfield_data_head = next;
	}
}

void uart_frame_parser_field_data_release(struct uart_frame_field_data* field_data_head) {
	while (field_data_head) {
		if (field_data_head->field_info->field_definition->has_subframes) {
			uart_frame_parser_field_data_release(field_data_head->subframe_field_data_head);
		}
		else if (field_data_head->field_info->field_definition->has_bitfields) {
			bitfield_data_release(field_data_head->bitfield_data_head);
		}
		struct uart_frame_field_data* next = field_data_head->next;
		free(field_data_head);
		field_data_head = next;
	}
}

static struct uart_frame_bitfield_data* read_bitfield(uint8_t* data,
	struct uart_frame_bitfield_definition* bitfield_definition,
	uart_frame_parser_error_callback_t on_error, void* user_ptr) {
	struct uart_frame_bitfield_data* bitfield_data = calloc(1, offsetof(struct uart_frame_bitfield_data, data) + bitset_size(bitfield_definition->bits));
	if (bitfield_data) {
		bitfield_data->bitfield_definition = bitfield_definition;

		uint32_t bit = 0;
		for (uint32_t i = bitfield_definition->offset_bits; i < bitfield_definition->offset_bits + bitfield_definition->bits; i++) {
			if (bitset_get(data, i)) {
				bitset_set(bitfield_data->data, bit);
			}
			else {
				bitset_unset(bitfield_data->data, bit);
			}
			bit++;
		}

		return bitfield_data;
	}
	else {
		on_error(user_ptr, UART_FRAME_PARSER_ERROR_MALLOC, __FILE__, __LINE__, "cannot allocate a bitfield data");
	}
	return NULL;
}

static struct uart_frame_bitfield_data* read_bitfields(uint8_t* data,
	uint32_t data_size,
	struct uart_frame_bitfield_definition* bitfield_definition_head,
	uart_frame_parser_error_callback_t on_error, void* user_ptr) {
	struct uart_frame_bitfield_data* bitfield_data_head = NULL;
	struct uart_frame_bitfield_data* bitfield_data_tail = NULL;
	
	uint32_t bits = 0;
	while (bitfield_definition_head) {
		bits += bitfield_definition_head->bits;
		if (bits <= data_size * 8) {
			struct uart_frame_bitfield_data* bitfield_data = read_bitfield(data, bitfield_definition_head, on_error, user_ptr);
			if (bitfield_data) {
				if (bitfield_data_tail) {
					bitfield_data_tail = bitfield_data_tail->next = bitfield_data;
				}
				else {
					bitfield_data_head = bitfield_data_tail = bitfield_data;
				}
			}
			else {
				goto err;
			}
		}
		else {
		err:
			bitfield_data_release(bitfield_data_head);
			return NULL;
		}

		bitfield_definition_head = bitfield_definition_head->next;
	}

	return bitfield_data_head;
}

static struct uart_frame_field_data* read_subframe_contained_field(void* buffer,
	struct uart_frame_field_info* field_info,
	uart_frame_parser_error_callback_t on_error, void* user_ptr) {

	struct uart_frame_field_data* subframe_field_data_head = read_fields(buffer, field_info->subframe_field_info, on_error, user_ptr);
	if (subframe_field_data_head) {
		struct uart_frame_field_data* field_data = calloc(1, sizeof(struct uart_frame_field_data));
		if (field_data) {
			field_data->field_info = field_info;
			field_data->subframe_field_data_head = subframe_field_data_head;
			return field_data;
		}
		else {
			uart_frame_parser_field_data_release(subframe_field_data_head);
			on_error(user_ptr, UART_FRAME_PARSER_ERROR_MALLOC, __FILE__, __LINE__, "cannot allocate a field data");
		}
	}

	return NULL;
}

static struct uart_frame_field_data* read_bitfield_contained_field(void* buffer,
	struct uart_frame_field_info* field_info,
	uart_frame_parser_error_callback_t on_error, void* user_ptr) {

	uint8_t* data = malloc(field_info->data_size);
	if (data) {

		uart_frame_parser_buffer_read(buffer, field_info->offset, data, field_info->data_size);

		struct uart_frame_field_data* field_data = NULL;
		struct uart_frame_bitfield_data* bitfield_data_head = read_bitfields(data, field_info->data_size, field_info->field_definition->bitfield_definition_head, on_error, user_ptr);
		if (bitfield_data_head) {
			field_data = calloc(1, sizeof(struct uart_frame_field_data));
			if (field_data) {
				field_data->field_info = field_info;
				field_data->bitfield_data_head = bitfield_data_head;
			}
			else {
				bitfield_data_release(bitfield_data_head);
				on_error(user_ptr, UART_FRAME_PARSER_ERROR_MALLOC, __FILE__, __LINE__, "cannot allocate a field data");
			}
		}

		free(data);
		return field_data;
	}
	else {
		on_error(user_ptr, UART_FRAME_PARSER_ERROR_MALLOC, __FILE__, __LINE__, "cannot allocate a field data buffer");
	}

	return NULL;
}

static struct uart_frame_field_data* read_common_field(void* buffer,
	struct uart_frame_field_info* field_info,
	uart_frame_parser_error_callback_t on_error, void* user_ptr) {

	struct uart_frame_field_data* field_data = calloc(1, offsetof(struct uart_frame_field_data, data) + field_info->data_size);
	if (field_data) {
		uart_frame_parser_buffer_read(buffer, field_info->offset, field_data->data, field_info->data_size);
		field_data->field_info = field_info;
		return field_data;
	}
	else {
		on_error(user_ptr, UART_FRAME_PARSER_ERROR_MALLOC, __FILE__, __LINE__, "cannot allocate a field data");
	}

	return NULL;
}

static struct uart_frame_field_data* read_field(void* buffer,

	struct uart_frame_field_info* field_info,
	uart_frame_parser_error_callback_t on_error, void* user_ptr) {

	if (field_info->field_definition->has_subframes) {
		return read_subframe_contained_field(buffer, field_info, on_error, user_ptr);
	}
	else if (field_info->field_definition->has_bitfields) {
		return read_bitfield_contained_field(buffer, field_info, on_error, user_ptr);
	}
	else {
		return read_common_field(buffer, field_info, on_error, user_ptr);
	}
}

static struct uart_frame_field_data* read_fields(void* buffer,
	struct uart_frame_field_info* field_info_head,
	uart_frame_parser_error_callback_t on_error, void* user_ptr) {

	struct uart_frame_field_data* field_data_head = NULL;
	struct uart_frame_field_data* field_data_tail = NULL;
	while (field_info_head) {
		struct uart_frame_field_data* field_data = read_field(buffer, field_info_head, on_error, user_ptr);
		if (field_data) {
			if (field_data_tail) {
				field_data_tail = field_data_tail->next = field_data;
			}
			else {
				field_data_head = field_data_tail = field_data;
			}
		}
		else {
			uart_frame_parser_field_data_release(field_data_head);
			return NULL;
		}

		field_info_head = field_info_head->next;
	}
	return field_data_head;
}

static struct uart_frame_field_definition*
find_field_definition(struct uart_frame_field_definition* field_definiton,
	struct uart_frame_field_definition** field_definitons) {

	while (*field_definitons) {
		if (*field_definitons == field_definiton) {
			return field_definiton;
		}
		field_definitons++;
	}
	return NULL;
}

static struct uart_frame_field_data* read_concerned_fields(void* buffer,
	struct uart_frame_field_info* field_info_head,
	struct uart_frame_field_definition** concerned_field_definitions,
	uart_frame_parser_error_callback_t on_error, void* user_ptr) {

	struct uart_frame_field_data* field_data_head = NULL;
	struct uart_frame_field_data* field_data_tail = NULL;
	while (field_info_head) {
		if (find_field_definition(field_info_head->field_definition, concerned_field_definitions)) {
			struct uart_frame_field_data* field_data = read_field(buffer, field_info_head, on_error, user_ptr);
			if (field_data) {
				if (field_data_tail) {
					field_data_tail = field_data_tail->next = field_data;
				}
				else {
					field_data_head = field_data_tail = field_data;
				}
			}
			else {
				uart_frame_parser_field_data_release(field_data_head);
				return NULL;
			}
		}
		field_info_head = field_info_head->next;
	}
	return field_data_head;
}

struct uart_frame_field_data* uart_frame_parser_read_concerned_fields(void* buffer,
	struct uart_frame_field_info* field_info_head,
	struct uart_frame_field_definition** concerned_field_definitions,
	uart_frame_parser_error_callback_t on_error, void* user_ptr) {

	if (concerned_field_definitions) {
		return read_concerned_fields(buffer, field_info_head, concerned_field_definitions, on_error, user_ptr);
	}
	else {
		return read_fields(buffer, field_info_head, on_error, user_ptr);
	}
}
