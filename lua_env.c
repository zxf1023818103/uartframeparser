#include <lauxlib.h>
#include <lualib.h>
#include <setjmp.h>
#include "uartframeparser.h"

static void l_byte(lua_State *L)
{
    struct uart_frame_parser* parser = lua_touserdata(L, lua_getglobal(L, "parser"));
    
}

lua_State *lua_state_create(const char* validator_expression, uart_frame_parser_error_callback_t on_error)
{
	lua_State *L = luaL_newstate();
	if (L)
	{
		lua_register(L, "byte", l_byte);
		/// TODO: 增加常用校验和函数

        
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
