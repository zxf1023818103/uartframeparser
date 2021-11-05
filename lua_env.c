#include <lauxlib.h>
#include <lualib.h>
#include <assert.h>
#include "uartframeparser.h"

static int l_bytek(lua_State* L, int status, lua_KContext ctx)
{
	int isnum;
	lua_Integer index = lua_tointegerx(L, 1, &isnum);
	if (isnum)
	{
		struct uart_frame_parser* parser = lua_touserdata(L, lua_getglobal(L, "parser"));

		int byte = uart_frame_parser_buffer_at(parser->buffer, index);
		if (byte != -1)
		{
			lua_pushinteger(L, byte);
		}
		else
		{
			lua_yieldk(L, 0, NULL, l_byte);
		}
	}
	return 1;
}

static int l_byte(lua_State *L)
{
	return l_bytek(L, 0, NULL);
}

static void register_functions(lua_State *L)
{
	lua_register(L, "byte", l_byte);
	/// TODO: 增加常用校验和函数
}

static void register_frame_definitions(lua_State* L, struct uart_frame_definition* frame_definition_head)
{
	lua_newtable(L);
	while (frame_definition_head)
	{
		lua_newtable(L);

		lua_pushstring(L, frame_definition_head->description);
		lua_setfield(L, -2, "description");
		
		lua_pushstring(L, frame_definition_head->validator_expression);
		lua_setfield(L, -2, "validator_expression");

		lua_newtable(L);

		lua_setfield(L, -2, "fields");

		lua_setfield(L, -1, frame_definition_head->name);
		frame_definition_head = frame_definition_head->next;
	}
}

lua_State *lua_state_create(struct uart_frame_definition* frame_definition_head, uart_frame_parser_error_callback_t on_error)
{
	lua_State *L = luaL_newstate();
	if (L)
	{
		register_functions(L);
		
		register_frame_definitions(L, frame_definition_head);

		return L;
	}
	else
	{
		on_error(UART_FRAME_PARSER_ERROR_MALLOC, "cannot create lua state");
	}

	return NULL;
}

void lua_state_release(lua_State *L)
{
	lua_close(L);
}

static int eval_validator_expression(lua_State *L, const char* frame_name)
{
	lua_getfield(L, LUA_REGISTRYINDEX, "validator_expressions");
	lua_getfield(L, -1, frame_name);
	lua_pcall(L, 0, 1, 0);
	if (lua_status(L) != LUA_YIELD)
	{

	}
}

static int parse_field_data_callback(struct uart_frame_data* frame_data, void* user_ptr)
{
	struct user_frame_data** ptr_frame_data = user_ptr;
	*ptr_frame_data = frame_data;
	return 0;
}

static int parse_field(struct uart_frame_field_definition * field_definition,
					   struct uart_frame_definition* frame_definition_head,
					   struct uart_frame_detected_frame * detected_frame_head,
					   struct uart_frame_field_data ** ptr_field_data,
					   struct uart_frame_parser_buffer * buffer,
					   uint32_t offset,
					   uart_frame_parser_error_callback_t on_error)
{
	uint32_t length = 0;
	if (field_definition->has_length_expression)
	{
		uint32_t return_size;
		int result = uart_frame_parser_expression_eval(field_definition->length.expression, offset, RETURN_TYPE_INTEGER, &length, &return_size);
		if (result <= 0)
		{
			return result;
		}
	}
	else
	{
		length = field_definition->length.value;
	}

	struct uart_frame_field_data* field_data = calloc(1, sizeof(struct uart_frame_field_data));
	if (field_data)
	{
		struct uart_frame_data* subframe_data = NULL;
		if (field_definition->has_subframes)
		{
			int result = next_frame(frame_definition_head, detected_frame_head, buffer, &subframe_data, offset, length, on_error, parse_field_data_callback);
			if (result <= 0)
			{
				free(field_data);
				return result;
			}
		}

		field_data->field_definition = field_definition;
		field_data->data_size = length;
		field_data->subframe_data = subframe_data;
		*ptr_field_data = field_data;

		return length;
	}
	else
	{
		on_error(UART_FRAME_PARSER_ERROR_MALLOC, "cannot allocate a field data");
		return -6;
	}
}

/// <summary>
/// 不验证帧格式，直接解析帧数据
/// </summary>
/// <param name="frame_definition">要解析的帧格式定义</param>
/// <param name="buffer">帧缓冲区</param>
/// <param name="offset">从帧缓冲区指定偏移量开始解析帧数据</param>
/// <param name="on_error">错误回调函数</param>
/// <param name="ptr_field_data">指向解析出的帧数据指针</param>
/// <returns>-7：帧格式超过max_size指定大小；-6：malloc失败；-5：表达式返回类型不对；-4：帧格式名称错误，-3：帧校验表达式不存在；-2：Lua语句执行错误；-1：需要更多数据；0：表达式校验未通过；其他非负值：解析成功，返回帧长度</returns>
static int parse_frame(struct uart_frame_definition* frame_definition,
	                   struct uart_frame_definition* frame_definition_head,
	                   struct uart_frame_detected_frame* detected_frame_head,
					   struct uart_frame_field_data** ptr_field_data_head,
	                   struct uart_frame_parser_buffer* buffer,
	                   uint32_t offset,
	                   uint32_t max_size,
	                   uart_frame_parser_error_callback_t on_error)
{
	int field_offset = 0;

	struct uart_frame_field_data* field_data_cur = NULL;
	struct uart_frame_field_data* field_data_head = NULL;

	struct uart_frame_field_definition* field_definition = frame_definition->field_head;
	while (field_definition)
	{
		struct uart_frame_field_data* field_data;
		int field_size = parse_field(field_definition, frame_definition_head, detected_frame_head, &field_data, buffer, offset + field_offset, on_error);
		if (field_size > 0)
		{
			if (field_data_cur)
			{
				field_data_cur->next = field_data;
			}
			else
			{
				field_data_cur = field_data_head = field_data;
			}
			field_offset += field_size;

			if (max_size < field_offset)
			{
				uart_frame_field_data_release(field_data_head);
				return -7;
			}
		}
		else
		{
			if (field_size != -1)
			{
				uart_frame_field_data_release(field_data_head);
			}
			return field_size;
		}

		field_definition = field_definition->next;
	}

	*ptr_field_data_head = field_data_head;
	return field_offset;
}

/// <summary>
/// 根据帧定义名称查找帧定义
/// </summary>
/// <param name="frame_definition_head">帧定义列表头</param>
/// <param name="name">帧定义名称</param>
/// <returns>返回查找到的帧定义，否则返回 NULL</returns>
static struct uart_frame_definition* find_frame_definition_by_name(struct uart_frame_definition* frame_definition_head, const char* name)
{
	while (frame_definition_head)
	{
		if (strcmp(frame_definition_head->name, name) == 0)
		{
			return frame_definition_head;
		}
		frame_definition_head = frame_definition_head->next;
	}
	return NULL;
}

/// <summary>
/// 验证指定帧格式并解析帧数据
/// </summary>
/// <param name="frame_definition_head">帧格式定义列表</param>
/// <param name="frame_name">帧格式名称</param>
/// <param name="buffer">帧缓冲区</param>
/// <param name="offset">从帧缓冲区指定偏移量开始解析帧数据</param>
/// <param name="on_error">错误回调函数</param>
/// <param name="ptr_field_data">指向解析出的帧数据指针</param>
/// <returns>-7：帧格式超过max_size指定大小；-6：malloc失败；-5：表达式返回类型不对；-4：帧格式名称错误，-3：帧校验表达式不存在；-2：Lua语句执行错误；-1：需要更多数据；0：表达式校验未通过；其他非负值：解析成功，返回帧长度</returns>
static int try_to_parse_the_frame(const char* frame_name,
								  struct uart_frame_definition* frame_definition_head,
								  struct uart_frame_detected_frame* detected_frame_head,
								  struct uart_frame_field_data ** ptr_field_data_head,
								  struct uart_frame_definition** ptr_frame_definition,
								  struct uart_frame_parser_buffer* buffer,
								  uint32_t offset,
								  uint32_t max_size,
							      uart_frame_parser_error_callback_t on_error)
{
	struct uart_frame_definition* frame_definition = find_frame_definition_by_name(frame_definition_head, frame_name);
	if (frame_definition)
	{
		if (frame_definition->validator_expression)
		{
			uint8_t valid;
			uint32_t valid_size;
			int validator_result = uart_frame_parser_expression_eval(frame_definition->validator_expression, offset, RETURN_TYPE_BOOLEAN, &valid, &valid_size);
			if (validator_result > 0)
			{
				return parse_frame(frame_definition, frame_definition_head, detected_frame_head, ptr_field_data_head, buffer, offset, max_size, on_error);
			}
			else
			{
				return validator_result;
			}
		}
		else
		{
			return parse_frame(frame_definition, frame_definition_head, detected_frame_head, ptr_field_data_head, buffer, offset, max_size, on_error);
		}
	}
	else
	{
		on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, "frame name %s not found", frame_name);
		return -4;
	}
}

static int try_to_parse_a_frame(struct uart_frame_definition* frame_definition_head,
								struct uart_frame_detected_frame* detected_frame_head,
								struct uart_frame_field_data** ptr_field_data,
								struct uart_frame_definition** ptr_frame_definition,
								struct uart_frame_parser_buffer* buffer,
								uint32_t offset,
								uint32_t max_size,
								uart_frame_parser_error_callback_t on_error)
{
	struct uart_frame_detected_frame* detected_frame = detected_frame_head;
	while (detected_frame)
	{
		int frame_bytes = try_to_parse_the_frame(detected_frame->name, frame_definition_head, detected_frame_head, ptr_field_data, ptr_frame_definition, buffer, offset, max_size, on_error);
		if (frame_bytes != 0)
		{
			return frame_bytes;
		}

		detected_frame = detected_frame->next;
	}

	return 0;
}

static int next_frame(struct uart_frame_definition* frame_definition_head,
					  struct uart_frame_detected_frame* detected_frame_head,
					  struct uart_frame_field_data **ptr_field_data,
					  struct uart_frame_definition ** ptr_frame_definition,
					  struct uart_frame_parser_buffer* buffer,
					  uint32_t offset,
	                  uint32_t max_size,
	                  uart_frame_parser_error_callback_t on_error)
{
	while (1)
	{
		int frame_bytes = try_to_parse_a_frame(frame_definition_head, detected_frame_head, ptr_field_data, ptr_frame_definition, buffer, offset, max_size, on_error);
		if (frame_bytes != 0)
		{
			return frame_bytes;
		}
		uart_frame_parser_buffer_increase_origin(buffer, 1);
	}
}

void uart_frame_parser_feed_data(struct uart_frame_parser* parser, uint8_t* data, uint32_t size)
{
	uart_frame_parser_buffer_append(parser->buffer, data, size);
	while (1)
	{
		struct uart_frame_field_data* field_data_head;
		struct uart_frame_definition* frame_definition;
		int frame_bytes = next_frame(parser->frame_definition_head, parser->detected_frame_head, &field_data_head, &frame_definition, parser->buffer, 0, 0, parser->on_error);
		if (frame_bytes > 0)
		{
			parser->on_data(frame_definition, field_data_head, )
		}
	}
}
