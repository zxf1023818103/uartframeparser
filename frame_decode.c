#include "uartframeparser.h"

static int eval_bitfield_tostring_expression(struct uart_frame_bitfield_definition* bitfield_definition_head, uint32_t offset) {
	while (bitfield_definition_head) {
		int result = uart_frame_parser_expression_eval(bitfield_definition_head->tostring_expression, offset);
		if (result < 0) {
			return result;
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
