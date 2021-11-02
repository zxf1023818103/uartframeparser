#include <lauxlib.h>
#include <lualib.h>
#include "uartframeparser.h"

extern void buffer_append(struct uart_frame_parser_buffer* buffer, uint8_t* data, uint32_t size);

extern void buffer_read(struct uart_frame_parser_buffer* buffer, uint8_t* data, uint32_t size);

extern int buffer_at(struct uart_frame_parser_buffer* buffer, uint32_t index);

extern int buffer_get_size(struct uart_frame_parser_buffer* buffer);

extern void buffer_clear(struct uart_frame_parser_buffer* buffer);

static int l_bytek(lua_State* L, int status, lua_KContext ctx)
{
	int isnum;
	lua_Integer index = lua_tointegerx(L, 1, &isnum);
	if (isnum)
	{
		struct uart_frame_parser* parser = lua_touserdata(L, lua_getglobal(L, "parser"));

		int byte = buffer_at(parser->buffer, index);
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
	lua_setglobal (L, "validator_expressions");
	while (frame_definition_head)
	{
		lua_pushstring(L, frame_definition_head->name);
		lua_pushstring(L, frame_definition_head->validator_expression);
		lua_settable(L, LUA_REGISTRYINDEX);

		frame_definition_head = frame_definition_head->next;
	}
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
	lua_getfield(L, )
}