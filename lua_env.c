#include <lauxlib.h>
#include <lualib.h>
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

static void register_validator_expressions(lua_State* L, struct uart_frame_definition* frame_definition_head)
{
	lua_newtable(L);
	while (frame_definition_head)
	{
		lua_pushstring(L, frame_definition_head->validator_expression);
		lua_setfield(L, -2, frame_definition_head->name);
		frame_definition_head = frame_definition_head->next;
	}
	lua_setfield(L, LUA_REGISTRYINDEX, "validator_expressions");
}

lua_State *lua_state_create(struct uart_frame_definition* frame_definition_head, uart_frame_parser_error_callback_t on_error)
{
	lua_State *L = luaL_newstate();
	if (L)
	{
		register_functions(L);
		
		register_validator_expressions(L, frame_definition_head);

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

/// <summary>
/// 尝试解析指定帧格式
/// </summary>
/// <param name="frame_name">帧格式名称</param>
/// <param name="L">要执行验证帧格式合法性的表达式的 Lua 状态机</param>
/// <param name="on_error">错误回调函数</param>
/// <returns>-2：Lua语句执行错误；-1：需要更多数据；0：表达式校验未通过；其他非负值：解析成功，返回帧长度</returns>
static int try_to_parse_frame(const char* frame_name, lua_State* L, uart_frame_parser_error_callback_t on_error)
{
	int result = eval_validator_expression(L, frame_name);
	
}

void uart_frame_parser_feed_data(struct uart_frame_parser* parser, uint8_t* data, uint32_t size)
{
	uart_frame_parser_buffer_append(parser->buffer, data, size);
	while (1)
	{
		uint32_t success_numbers = 0;
		
		struct uart_frame_detected_frame* detected_frame = parser->detected_frame_head;
		while (detected_frame)
		{
			int frame_bytes = try_to_parse_frame(detected_frame->name, parser->L, parser->on_error);
			if (frame_bytes > 0)
			{
				success_numbers++;
				uart_frame_parser_buffer_increase_origin(parser->buffer, frame_bytes);
			}
			else if (frame_bytes < 0)
			{
				return;
			}
			
			detected_frame = detected_frame->next;
		}

		if (success_numbers == 0)
		{
			uart_frame_parser_buffer_increase_origin(parser->buffer, 1);
		}
	}
}
