#include "uartframeparser.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>

#define bitset_size(bits)         ((bits) / 8 + ((bits) % 8 ? 1 : 0))
#define bitset_set(bitset, bit)   ((bitset)[(bit) / 8] |=  (1 << ((bit) % 8)))
#define bitset_get(bitset, bit)   ((bitset)[(bit) / 8] &   (1 << ((bit) % 8)))
#define bitset_unset(bitset, bit) ((bitset)[(bit) / 8] &= ~(1 << ((bit) % 8)))

static int eval_bitfield_tostring_expression(struct uart_frame_bitfield_definition* bitfield_definition_head, uint32_t offset) {
	while (bitfield_definition_head) {
		if (bitfield_definition_head->tostring_expression) {
			int result = uart_frame_parser_expression_eval(bitfield_definition_head->tostring_expression, offset);
			if (result < 0) {
				return result;
			}
		}
		bitfield_definition_head = bitfield_definition_head->next;
	}
	return 0;
}

static int eval_field_tostring_expression(struct uart_frame_field_info* field_info) {
	int result = 0;
	if (field_info->field_definition->has_bitfields) {
		result = eval_bitfield_tostring_expression(field_info->field_definition->bitfield_definition_head, field_info->parent_offset);
	}
	else if (field_info->field_definition->has_subframes) {
		result = uart_frame_parser_eval_tostring_expression(field_info->subframe_field_info);
	}
	else if (field_info->field_definition->tostring_expression) {
		result = uart_frame_parser_expression_eval(field_info->field_definition->tostring_expression, field_info->parent_offset);
	}
	return result;
}

int uart_frame_parser_eval_tostring_expression(struct uart_frame_field_info* field_info_head) {
	int result = 0;
	while (field_info_head) {
		result = eval_field_tostring_expression(field_info_head);
		if (result < 0) {
			return result;
		}
		field_info_head = field_info_head->next;
	}

	return result;
}

static cJSON* stringify_binary_data2(uint8_t* data, uint32_t data_size) {
    char* hex_data = malloc(data_size * 2 + 1);

    char* hex = hex_data;
    if (hex) {
        for (uint32_t i = 0; i < data_size; i++) {
            sprintf(hex, "%02x", data[i]);
            hex += 2;
        }
    }

    cJSON* hex_data_node = cJSON_CreateString(hex_data);
    free(hex_data);
    return hex_data_node;
}

static cJSON* stringify_bitfield_data2(struct uart_frame_bitfield_data* bitfield_data) {
    cJSON* bitfield_data_node = cJSON_CreateObject();
    cJSON_AddStringToObject(bitfield_data_node, "name", bitfield_data->bitfield_definition->name);
    cJSON_AddStringToObject(bitfield_data_node, "description", bitfield_data->bitfield_definition->description);
    cJSON_AddNumberToObject(bitfield_data_node, "bits", bitfield_data->bitfield_definition->bits);
    cJSON_AddItemToObject(bitfield_data_node, "hex", stringify_binary_data2(bitfield_data->data, bitset_size(bitfield_data->bitfield_definition->bits)));
    if (bitfield_data->bitfield_definition->tostring_expression) {
        struct uart_frame_parser_expression_result* result = uart_frame_parser_expression_get_result(bitfield_data->bitfield_definition->tostring_expression);
        if (result) {
            cJSON_AddStringToObject(bitfield_data_node, "text", (const char*)result->byte_array);
        }
    }
    return bitfield_data_node;
}

static cJSON* stringify_bitfields_data2(struct uart_frame_bitfield_data* bitfield_data_head) {

    cJSON* array = cJSON_CreateArray();

    while (bitfield_data_head) {
        cJSON* bitfield_data_node = stringify_bitfield_data2(bitfield_data_head);
        cJSON_AddItemToArray(array, bitfield_data_node);
        bitfield_data_head = bitfield_data_head->next;
    }

    return array;
}

static cJSON* stringify_frame_data2(struct uart_frame_definition* frame_definition, struct uart_frame_field_data* field_data_head) {
    cJSON* frame_data = cJSON_CreateObject();
    cJSON_AddStringToObject(frame_data, "name", frame_definition->name);
    cJSON_AddStringToObject(frame_data, "description", frame_definition->description);
    cJSON* data = cJSON_CreateArray();

    while (field_data_head) {
        cJSON* field_info_node = cJSON_CreateObject();

        cJSON_AddStringToObject(field_info_node, "name", field_data_head->field_info->field_definition->name);
        cJSON_AddStringToObject(field_info_node, "description", field_data_head->field_info->field_definition->description);
        cJSON_AddNumberToObject(field_info_node, "length", field_data_head->field_info->data_size);

        if (field_data_head->field_info->field_definition->has_bitfields) {
            cJSON* bitfields_data = stringify_bitfields_data2(field_data_head->bitfield_data_head);
            cJSON_AddItemToObject(field_info_node, "bitfields", bitfields_data);
        }
        else if (field_data_head->field_info->field_definition->has_subframes) {
            cJSON* subframe_data = stringify_frame_data2(field_data_head->field_info->subframe_definition, field_data_head->subframe_field_data_head);
            cJSON_AddItemToObject(field_info_node, "subframe", subframe_data);
        }
        else {
            cJSON* hex_data = stringify_binary_data2(field_data_head->data, field_data_head->field_info->data_size);
            cJSON_AddItemToObject(field_info_node, "hex", hex_data);
            if (field_data_head->field_info->field_definition->tostring_expression) {
                struct uart_frame_parser_expression_result* result = uart_frame_parser_expression_get_result(field_data_head->field_info->field_definition->tostring_expression);
                if (result) {
                    cJSON_AddStringToObject(field_info_node, "text", (const char*)result->byte_array);
                }
            }
        }

        cJSON_AddItemToArray(data, field_info_node);

        field_data_head = field_data_head->next;
    }

    cJSON_AddItemToObject(frame_data, "data", data);

    return frame_data;
}

const char* uart_frame_parser_jsonify_frame_data(void* buffer,
                                                 struct uart_frame_definition* frame_definition,
                                                 uint32_t frame_bytes,
                                                 struct uart_frame_field_data* field_data_head) {
    cJSON* data = stringify_frame_data2(frame_definition, field_data_head);

    uint8_t* raw = (uint8_t*)malloc(frame_bytes);
    if (raw) {
        uart_frame_parser_buffer_read(buffer, 0, raw, frame_bytes);
        cJSON_AddItemToObject(data, "hex", stringify_binary_data2(raw, frame_bytes));
        free(raw);
    }

    return cJSON_Print(data);
}
