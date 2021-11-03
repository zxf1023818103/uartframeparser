#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include "uartframeparser.h"

extern lua_State* lua_state_create(struct uart_frame_definition* frame_definition_head, uart_frame_parser_error_callback_t on_error);

extern void lua_state_release(lua_State *L);

static void uart_frame_detected_frame_release(struct uart_frame_detected_frame *detected_frame_head)
{
	struct uart_frame_detected_frame *next;
	while (detected_frame_head)
	{
		next = detected_frame_head->next;
		free(detected_frame_head);
		detected_frame_head = next;
	}
}

static void uart_frame_bitfield_definition_release(struct uart_frame_bitfield_definition *bitfield_head)
{
	struct uart_frame_bitfield *next;
	while (bitfield_head)
	{
		next = bitfield_head->next;
		free(bitfield_head);
		bitfield_head = next;
	}
}

static void uart_frame_field_definition_release(struct uart_frame_field_definition *field_definition_head)
{
	struct uart_frame_field_definition *next;
	while (field_definition_head)
	{
		if (field_definition_head->has_subframes)
		{
			uart_frame_detected_frame_release(field_definition_head->detected_subframe_head);
		}
		else if (field_definition_head->has_bitfields)
		{
			uart_frame_bitfield_definition_release(field_definition_head->bitfield_head);
		}

		next = field_definition_head->next;
		free(field_definition_head);
		field_definition_head = next;
	}
}

static void uart_frame_definition_release(struct uart_frame_definition *frame_definition_head)
{
	struct uart_frame_definition *next;
	while (frame_definition_head)
	{
		uart_frame_field_definition_release(frame_definition_head->field_head);
		next = frame_definition_head->next;
		free(frame_definition_head);
		frame_definition_head = next;
	}
}

static struct uart_frame_field_definition *parse_frame_field_node(cJSON *field_node, uart_frame_parser_error_callback_t on_error)
{
	if (cJSON_IsObject(field_node))
	{
		cJSON *bitfields_node = NULL;
		cJSON *subframes_node = NULL;
		char *name = NULL;
		char *description = NULL;
		char *length_expression = NULL;
		uint32_t length_value = 0;
		char *default_value_expression = NULL;

		cJSON *field_attribute_node = field_node->child;
		while (field_attribute_node)
		{
			if (strcmp("name", field_attribute_node->string) == 0)
			{
				if (cJSON_IsString(field_attribute_node))
				{
					name = field_attribute_node->valuestring;
				}
				else
				{
					on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, "field name is not a string");
				}
			}
			else if (strcmp("description", field_attribute_node->string) == 0)
			{
				if (cJSON_IsString(field_attribute_node))
				{
					description = field_attribute_node->valuestring;
				}
				else
				{
					on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, "field description is not a string");
				}
			}
			else if (strcmp("bytes", field_attribute_node->string) == 0)
			{
				if (cJSON_IsString(field_attribute_node))
				{
					length_expression = field_attribute_node->valuestring;
				}
				else if (cJSON_IsNumber(field_attribute_node))
				{
					length_value = field_attribute_node->valueint;
				}
				else
				{
					on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, "field description is not a string or number");
				}
			}
			else if (strcmp("default", field_attribute_node->string) == 0)
			{
				if (cJSON_IsString(field_attribute_node))
				{
					default_value_expression = field_attribute_node->valuestring;
				}
				else
				{
					on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, "field default is not a string");
				}
			}
			else if (strcmp("frames", field_attribute_node->string) == 0)
			{
				subframes_node = field_attribute_node;
			}
			else if (strcmp("bitfields", field_attribute_node->string) == 0)
			{
				bitfields_node = field_attribute_node;
			}

			field_attribute_node = field_attribute_node->next;
		}

		if (length_expression || length_value)
		{
			if (bitfields_node || subframes_node)
			{
				if (!(bitfields_node && subframes_node))
				{
					if (!(subframes_node && default_value_expression))
					{
						struct uart_frame_field_definition *field_definition = calloc(1, sizeof(struct uart_frame_field_definition));

						field_definition->name = name;
						field_definition->description = description;
						if (length_expression)
						{
							field_definition->has_length_expression = 1;
							field_definition->length.expression = length_expression;
						}
						else
						{
							field_definition->length.value = length_value;
						}

						if (bitfields_node)
						{
							struct uart_frame_bitfield_definition *bitfield_head = parse_frame_bitfields_node(bitfields_node, on_error);
							if (bitfield_head)
							{
								field_definition->has_bitfields = 1;
								field_definition->bitfield_head = bitfield_head;
							}
							else
							{
								uart_frame_bitfield_definition_release(bitfield_head);
								free(field_definition);
								return NULL;
							}
						}
						else if (subframes_node)
						{
							field_definition->has_subframes = 1;
							field_definition->detected_subframe_head = subframes_node;
						}
						else
						{
							field_definition->default_value_expression = default_value_expression;
						}

						return field_definition;
					}
					else
					{
						on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, "exclusive field attribute default and frames");
					}
				}
				else
				{
					on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, "exclusive field attribute bitfields and frames");
				}
			}
		}
		else
		{
			on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, "field attribute bytes not found");
		}
	}
	else
	{
		on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, "field is not an object");
	}

	return NULL;
}

static struct uart_frame_field_definition *parse_frame_fields_node(cJSON *fields_node, uart_frame_parser_error_callback_t on_error)
{
	if (cJSON_IsArray(fields_node))
	{
		cJSON *field_node = fields_node->child;
		if (field_node)
		{
			struct uart_frame_field_definition *cur_field_definition = NULL, *field_definition_head = NULL;
			while (field_node)
			{
				struct uart_frame_field_definition *field_definition = parse_frame_field_node(field_node, on_error);
				if (field_definition)
				{
					if (cur_field_definition)
					{
						cur_field_definition->next = field_definition;
						cur_field_definition = cur_field_definition->next;
					}
					else
					{
						field_definition_head = cur_field_definition = field_definition;
					}
				}
				else
				{
					uart_frame_field_definition_release(field_definition_head);
					return NULL;
				}
				field_node = field_node->next;
			}
			return field_definition_head;
		}
		else
		{
			on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, "frame fields is empty");
		}
	}
	else
	{
		on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, "frame fields is not an array");
	}

	return NULL;
}

static struct uart_frame_definition *parse_definition_node(cJSON *definition_node, uart_frame_parser_error_callback_t on_error)
{
	if (cJSON_IsObject(definition_node))
	{
		cJSON *definition_attribute_node = definition_node->child;
		if (definition_attribute_node)
		{
			char *name = NULL;
			char *description = NULL;
			char *validator_expression = NULL;
			cJSON *fields_node = NULL;

			while (definition_attribute_node)
			{
				if (strcmp("name", definition_attribute_node->string) == 0)
				{
					if (cJSON_IsString(definition_attribute_node))
					{
						name = definition_attribute_node->valuestring;
					}
					else if (!cJSON_IsNull(definition_attribute_node))
					{
						on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, "frame definition name is not a string");
					}
				}
				else if (strcmp("description", definition_attribute_node->string) == 0)
				{
					if (cJSON_IsString(definition_attribute_node))
					{
						description = definition_attribute_node->valuestring;
					}
					else if (!cJSON_IsNull(definition_attribute_node))
					{
						on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, "frame description is not a string");
					}
				}
				else if (strcmp("validator", definition_attribute_node->string) == 0)
				{
					if (cJSON_IsString(definition_attribute_node))
					{
						validator_expression = definition_attribute_node->valuestring;
					}
					else if (!cJSON_IsNull(definition_attribute_node))
					{
						on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, "frame validator is not a string");
					}
				}
				else if (strcmp("fields", definition_attribute_node->string) == 0)
				{
					fields_node = definition_attribute_node;
				}

				definition_attribute_node = definition_attribute_node->next;
			}

			if (name)
			{
				struct uart_frame_field_definition *field_head = parse_frame_fields_node(fields_node, on_error);
				if (field_head)
				{
					struct uart_frame_definition *frame_definition = calloc(1, sizeof(struct uart_frame_definition));
					if (frame_definition)
					{
						frame_definition->name = name;
						frame_definition->description = description;
						frame_definition->validator_expression = validator_expression;
						frame_definition->field_head = field_head;
						return frame_definition;
					}
					else
					{
						on_error(UART_FRAME_PARSER_ERROR_MALLOC, "cannot malloc a frame definition");
					}
				}
			}
			else
			{
				on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, "frame definition name is null");
			}
		}
		else
		{
			on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, "frame definition is empty");
		}
	}
	else
	{
		on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, "definition is not an object");
	}

	return NULL;
}

static struct uart_frame_detected_frame *parse_detected_frame_node(cJSON *detected_frame_node, uart_frame_parser_error_callback_t on_error)
{
	if (cJSON_IsString(detected_frame_node))
	{
		struct uart_frame_detected_frame *detected_frame = calloc(1, sizeof(struct uart_frame_detected_frame));
		if (detected_frame)
		{
			detected_frame->name = detected_frame_node->valuestring;
			return detected_frame;
		}
		else
		{
			on_error(UART_FRAME_PARSER_ERROR_MALLOC, "cannot malloc a detected frame");
		}
	}
	else
	{
		on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, "detected frames is not a string");
	}

	return NULL;
}

static struct uart_frame_definition *parse_definitions_node(cJSON *definitions_node, uart_frame_parser_error_callback_t on_error)
{
	if (definitions_node)
	{
		if (cJSON_IsArray(definitions_node))
		{
			cJSON *definition_node = definitions_node->child;
			if (definition_node)
			{
				struct uart_frame_definition *cur_frame_definition = NULL, *frame_definition_head = NULL;
				while (definition_node)
				{
					struct uart_frame_definition *frame_definition = parse_definition_node(definition_node, on_error);
					if (frame_definition)
					{
						if (cur_frame_definition)
						{
							cur_frame_definition->next = frame_definition;
							cur_frame_definition = cur_frame_definition->next;
						}
						else
						{
							frame_definition_head = cur_frame_definition = frame_definition;
						}
					}
					else
					{
						uart_frame_definition_release(frame_definition_head);
						return NULL;
					}

					definition_node = definition_node->next;
				}

				return frame_definition_head;
			}
			else
			{
				on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, "definitions is empty");
			}
		}
		else
		{
			on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, "definitions is not an array");
		}
	}
	else
	{
		on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, "definitions is null");
	}

	return NULL;
}

static struct uart_frame_detected_frame *parse_detected_frames_node(cJSON *detected_frames_node, uart_frame_parser_error_callback_t on_error)
{
	if (detected_frames_node)
	{
		if (cJSON_IsArray(detected_frames_node))
		{
			cJSON *detected_frame_node = detected_frames_node->child;
			if (detected_frame_node)
			{
				struct uart_frame_detected_frame *cur_detected_frame = NULL, *detected_frame_head = NULL;
				while (detected_frame_node)
				{
					struct uart_frame_detected_frame *detected_frame = parse_detected_frame_node(detected_frame_node, on_error);
					if (detected_frame)
					{
						if (cur_detected_frame)
						{
							cur_detected_frame->next = detected_frame;
							cur_detected_frame = cur_detected_frame->next;
						}
						else
						{
							detected_frame_head = cur_detected_frame = detected_frame;
						}
					}
					else
					{
						uart_frame_detected_frame_release(detected_frame_head);
						return NULL;
					}

					detected_frame_node = detected_frame_node->next;
				}

				return detected_frame_head;
			}
			else
			{
				on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, "frames is empty");
			}
		}
		else
		{
			on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, "frames is not array");
		}
	}
	else
	{
		on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, "frames is null");
	}

	return NULL;
}

static struct uart_frame_parser *parse_json_config(cJSON *config, uart_frame_parser_error_callback_t on_error)
{
	if (cJSON_IsObject(config))
	{
		config = config->child;

		cJSON *definitions_node = NULL;
		cJSON *detected_frames_node = NULL;
		while (config)
		{
			if (strcmp("definitions", config->string) == 0)
			{
				definitions_node = config;
			}
			else if (strcmp("frames", config->string) == 0)
			{
				detected_frames_node = config;
			}

			config = config->next;
		}

		struct uart_frame_definition *frame_definition_head = parse_definitions_node(definitions_node, on_error);

		if (frame_definition_head)
		{
			struct uart_frame_detected_frame *detected_frame_head = parse_detected_frames_node(detected_frames_node, on_error);

			if (detected_frame_head)
			{
				struct buffer* buffer = uart_frame_parser_buffer_create(on_error);
				if (buffer)
				{
					lua_State* L = lua_state_create(frame_definition_head, on_error);
					if (L)
					{
						struct uart_frame_parser* parser = calloc(1, sizeof(struct uart_frame_parser));
						if (parser)
						{
							parser->detected_frame_head = detected_frame_head;
							parser->frame_definition_head = frame_definition_head;
							parser->on_error = on_error;
							parser->buffer = buffer;
							parser->L = L;

							lua_pushlightuserdata(L, parser);
							lua_setfield(L, LUA_REGISTRYINDEX, "parser");

							return parser;
						}
						else
						{
							on_error(UART_FRAME_PARSER_ERROR_MALLOC, "cannot malloc a parser");
						err4:
							lua_state_release(L);
						err3:
							uart_frame_parser_buffer_release(buffer);
						err2:
							uart_frame_detected_frame_release(detected_frame_head);
						err1:
							uart_frame_definition_release(frame_definition_head);
						}
					}
					else
					{
						goto err3;
					}
				}
				else
				{
					goto err2;
				}
			}
			else
			{
				goto err1;
			}
		}
	}
	else
	{
		on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, "config is not an object");
	}

	return NULL;
}

struct uart_frame_parser *uart_frame_parser_create(const char *json_config, uint32_t json_config_size, uart_frame_parser_error_callback_t on_error)
{
	char *json_config_end = json_config + json_config_size;
	cJSON *cjson = cJSON_ParseWithOpts(json_config, &json_config_end, 0);
	if (cjson)
	{
		return parse_json_config(cjson, on_error);
	}
	else
	{
		on_error(UART_FRAME_PARSER_ERROR_CJSON, cJSON_GetErrorPtr());
	}

	return NULL;
}
