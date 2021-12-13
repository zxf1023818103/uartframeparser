#include "uartframeparser.h"
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <string.h>

/// <summary>
/// 自定义用户数据在 Lua 注册表中的下标
/// </summary>
enum lua_register_table_indices {

    /// <summary>
    /// byte() 函数下标起始位置对应的 Lua 注册表下标
    /// </summary>
    OFFSET_INDEX = 0,

    /// <summary>
    /// 错误回调函数指针对应的 Lua 注册表下标
    /// </summary>
    ERROR_CALLBACK_INDEX = 1,

    /// <summary>
    /// 传入错误回调函数的用户数据下标
    /// </summary>
    USER_PTR_INDEX = 2,

    /// <summary>
    /// 帧缓冲指针对应的 Lua 注册表下标
    /// </summary>
    BUFFER_INDEX = 3,
};

/// <summary>
/// 表达式解析引擎
/// </summary>
struct uart_frame_parser_expression_engine {
    /// <summary>
    /// Lua 状态机
    /// </summary>
    lua_State *L;

    /// <summary>
    /// 错误回调函数
    /// </summary>
    uart_frame_parser_error_callback_t on_error;

    /// <summary>
    /// 传入错误回调函数的用户数据
    /// </summary>
    void* user_ptr;

    /// <summary>
    /// 该表达式引擎关联的预编译表达式列表
    /// </summary>
    struct uart_frame_parser_expression *expression_head;

    /// <summary>
    /// 最后一次执行的预编译表达式
    /// </summary>
    struct uart_frame_parser_expression *last_expression;
};

/// <summary>
/// 预编译表达式
/// </summary>
struct uart_frame_parser_expression {
    /// <summary>
    /// 指向下一个预编译表达式
    /// </summary>
    struct uart_frame_parser_expression *next;

    /// <summary>
    /// 关联的表达式引擎
    /// </summary>
    struct uart_frame_parser_expression_engine *expression_engine;

    /// <summary>
    /// 该预编译表达式名称，用于在 lua_load() 函数中作为 chunk 名称
    /// </summary>
    char *name;

    /// <summary>
    /// 指向字节码起始位置
    /// </summary>
    uint8_t *data;

    /// <summary>
    /// 字节码长度
    /// </summary>
    size_t data_size;

    /// <summary>
    /// 表达式类型
    /// </summary>
    enum uart_frame_parser_expression_types type;

    /// <summary>
    /// 该表达式对应的结果
    /// </summary>
    struct uart_frame_parser_expression_result *result;
};

/// <summary>
/// 表达式中 byte() 函数的实现，供表达式访问帧缓冲指定下标的字节。执行前栈顶需压入要访问的表达式下标，执行后下标出栈，压入该下标对应的字节并返回。
/// 若出错，压入错误码并调用错误回调函数，引发异常。
/// </summary>
/// <param name="L">函数被调用时的 Lua 状态机</param>
/// <returns>该 Lua 函数返回的结果个数</returns>
static int l_byte(lua_State *L) {
    lua_pushlightuserdata(L, (void *) ERROR_CALLBACK_INDEX);
    lua_gettable(L, LUA_REGISTRYINDEX);
    uart_frame_parser_error_callback_t on_error = lua_touserdata(L, -1);
    lua_pop(L, 1);

    lua_pushlightuserdata(L, (void*)USER_PTR_INDEX);
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* user_ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (lua_gettop(L) == 1) {
        int is_num;
        lua_Integer index = lua_tointegerx(L, -1, &is_num);
        if (is_num) {
            if (index > 0) {
                lua_pushlightuserdata(L, (void *) OFFSET_INDEX);
                lua_gettable(L, LUA_REGISTRYINDEX);
                lua_Integer offset = lua_tointeger(L, -1);
                lua_pop(L, 1);

                lua_pushlightuserdata(L, (void *) BUFFER_INDEX);
                lua_gettable(L, LUA_REGISTRYINDEX);
                struct uart_frame_parser_buffer *buffer = lua_touserdata(L, -1);
                lua_pop(L, 1);

                int byte = uart_frame_parser_buffer_at(buffer, (uint32_t)(index + offset - 1));
                lua_pushinteger(L, byte);
                if (byte >= 0) {
                    return 1;
                }
            } else {
                on_error(user_ptr, UART_FRAME_PARSER_ERROR_LUA, __FILE__, __LINE__, "%s\r\n",
                         lua_pushfstring(L, "argument 1 %lld of byte() is <= 0", index));
                lua_pop(L, 1);
                lua_pushinteger(L, -1);
            }
        } else {
            on_error(user_ptr, UART_FRAME_PARSER_ERROR_LUA, __FILE__, __LINE__, "%s\r\n",
                     lua_pushfstring(L, "argument 1 %s of byte() is not an integer", lua_tostring(L, 1)));
            lua_pop(L, 1);
            lua_pushinteger(L, -2);
        }
    } else {
        on_error(user_ptr, UART_FRAME_PARSER_ERROR_LUA, __FILE__, __LINE__, "%s\r\n",
                 lua_pushstring(L, "byte() must input 1 argument"));
        lua_pop(L, 1);
        lua_pushinteger(L, -2);
    }

    lua_error(L);
    return 0;
}

/// <summary>
/// 获取 Lua 错误信息并调用错误回调函数，该部分仿照自 lua.c 中的 msghandler() 函数
/// </summary>
/// <param name="L">指向 Lua 状态机</param>
/// <param name="on_error">错误回调函数</param>
static void handle_lua_error(lua_State *L, uart_frame_parser_error_callback_t on_error, void* user_ptr) {
    if (lua_gettop(L)) {
        const char *error_message = lua_tostring(L, -1);
        if (error_message) {
            luaL_traceback(L, L, error_message, 1);
        } else {
            if (luaL_callmeta(L, 1, "__tostring") && lua_type(L, -1) != LUA_TSTRING) {
                luaL_traceback(L, L, lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, -1)), 1);
            }
        }

        on_error(user_ptr, UART_FRAME_PARSER_ERROR_LUA, __FILE__, __LINE__, lua_tostring(L, -1));
        lua_settop(L, 0);
    } else {
        on_error(user_ptr, UART_FRAME_PARSER_ERROR_LUA, __FILE__, __LINE__, "empty error message");
    }
}

/// <summary>
/// 创建 Lua 状态机，执行初始化脚本并注册 byte() 函数
/// </summary>
/// <param name="buffer">供表达式访问的帧缓冲区</param>
/// <param name="init_script">创建完成后调用此脚本初始化表达式运行环境，如在脚本中增加自定义函数等</param>
/// <param name="on_error">错误回调函数</param>
/// <returns>返回创建的 Lua 状态机</returns>
static lua_State *lua_state_create(struct uart_frame_parser_buffer *buffer, const char *init_script,
                                   uart_frame_parser_error_callback_t on_error, void* user_ptr) {
    lua_State *L = luaL_newstate();
    if (L) {
        lua_register(L, "byte", l_byte);

        if (init_script) {
            luaL_loadstring(L, init_script);
            int status = lua_pcall(L, 0, 0, 0);
            if (status != LUA_OK) {
                handle_lua_error(L, on_error, user_ptr);
                lua_close(L);
                return NULL;
            }
        }

        lua_pushlightuserdata(L, (void *) BUFFER_INDEX);
        lua_pushlightuserdata(L, buffer);
        lua_settable(L, LUA_REGISTRYINDEX);

        lua_pushlightuserdata(L, (void *) ERROR_CALLBACK_INDEX);
        lua_pushlightuserdata(L, on_error);
        lua_settable(L, LUA_REGISTRYINDEX);

        lua_pushlightuserdata(L, (void*)USER_PTR_INDEX);
        lua_pushlightuserdata(L, user_ptr);
        lua_settable(L, LUA_REGISTRYINDEX);

        return L;
    } else {
        on_error(user_ptr, UART_FRAME_PARSER_ERROR_MALLOC, __FILE__, __LINE__, "cannot allocate a lua state");
        return NULL;
    }
}

struct uart_frame_parser_expression_engine *
uart_frame_parser_expression_engine_create(struct uart_frame_parser_buffer *buffer, const char *init_script,
                                           uart_frame_parser_error_callback_t on_error, void* user_ptr, void *reserved) {
    (void) reserved;

    lua_State *L = lua_state_create(buffer, init_script, on_error, user_ptr);
    if (L) {
        struct uart_frame_parser_expression_engine *expression_engine = calloc(1,
                                                                               sizeof(struct uart_frame_parser_expression_engine));
        if (expression_engine) {
            expression_engine->L = L;
            expression_engine->on_error = on_error;
            expression_engine->user_ptr = user_ptr;
            return expression_engine;
        }
    }

    return NULL;
}

static void expression_release(struct uart_frame_parser_expression *expression) {
    free(expression->name);
    free(expression->data);
    free(expression->result);
    free(expression);
}

void uart_frame_parser_expression_engine_release(struct uart_frame_parser_expression_engine *engine) {
    while (engine->expression_head) {
        struct uart_frame_parser_expression *next = engine->expression_head->next;
        expression_release(engine->expression_head);
        engine->expression_head = next;
    }

    lua_close(engine->L);
    free(engine);
}

/// <summary>
/// lua_Writer 实现，用于存储 lua_dump() 生成的字节码
/// </summary>
/// <param name="L">Lua 状态机</param>
/// <param name="data">指向写入的字节码</param>
/// <param name="data_size">要写入字节码的大小</param>
/// <param name="ud">实际类型为 struct uart_frame_parser_expression *，指向要生成的预编译表达式结构</param>
/// <returns>若内存分配失败返回 LUA_ERRMEM；若写入成功则返回 0</returns>
static int do_write_bytecode(lua_State *L, const void *data, size_t data_size, void *ud) {
    (void) L;

    struct uart_frame_parser_expression *expression = ud;

    uint8_t* old_data = expression->data;
    expression->data = realloc(old_data, expression->data_size + data_size);
    if (expression->data) {
        memcpy(expression->data + expression->data_size, data, data_size);
        expression->data_size += data_size;
        return 0;
    } else {
        free(old_data);
        return LUA_ERRMEM;
    }
}

static struct uart_frame_parser_expression *
expression_create(struct uart_frame_parser_expression_engine *engine, const char *expression_name,
                  enum uart_frame_parser_expression_types expression_type,
                  const char *expression_string, void *reserved) {
    (void) reserved;

    struct uart_frame_parser_expression *expression = calloc(1, sizeof(struct uart_frame_parser_expression));
    if (expression) {
        expression->expression_engine = engine;
        expression->type = expression_type;

        if (!expression_name) {
            expression_name = expression_string;
        }

        expression->name = calloc(strlen(expression_name) + 1, 1);
        if (expression->name) {
            strcpy(expression->name, expression_name);
        } else {
            free(expression);
            engine->on_error(engine->user_ptr, UART_FRAME_PARSER_ERROR_MALLOC, __FILE__, __LINE__,
                             "cannot allocate expression name space");
            return NULL;
        }

        luaL_loadstring(engine->L, expression_string);
        int status = lua_dump(engine->L, do_write_bytecode, expression, 1);
        lua_settop(engine->L, 0);

        if (!status) {
            return expression;
        } else {
            free(expression->name);
            free(expression);
            engine->on_error(engine->user_ptr, UART_FRAME_PARSER_ERROR_LUA, __FILE__, __LINE__, "invalid expression: %s",
                             expression_string);
        }
    } else {
        engine->on_error(engine->user_ptr, UART_FRAME_PARSER_ERROR_MALLOC, __FILE__, __LINE__, "cannot allocate a expression space");
    }

    return NULL;
}

static struct uart_frame_parser_expression * find_expression(struct uart_frame_parser_expression *expression_head, struct uart_frame_parser_expression *expression, struct uart_frame_parser_expression **ptr_previous_expression) {

    *ptr_previous_expression = NULL;

    while (expression_head) {
        if (expression_head == expression) {
            return expression;
        }

        *ptr_previous_expression = expression_head;
        expression_head = expression_head->next;
    }

    return NULL;
}

struct uart_frame_parser_expression *
uart_frame_parser_expression_create(struct uart_frame_parser_expression_engine *engine, const char *expression_name,
                                    enum uart_frame_parser_expression_types expression_type,
                                    const char *expression_string, void *reserved) {
    struct uart_frame_parser_expression *expression = expression_create(engine, expression_name, expression_type,
                                                                        expression_string, reserved);
    if (expression) {
        expression->next = engine->expression_head;
        engine->expression_head = expression;
    }

    return expression;
}

void uart_frame_parser_expression_release(struct uart_frame_parser_expression_engine *expression_engine,
                                          struct uart_frame_parser_expression *expression) {
    struct uart_frame_parser_expression *previous_expression;
    if (find_expression(expression_engine->expression_head, expression, &previous_expression)) {

        if (previous_expression) {
            previous_expression->next = expression->next;
        }
        else {
            expression_engine->expression_head = expression->next;
        }

        expression_release(expression);
    }
}

/// <summary>
/// lua_Reader 实现，用于加载字节码。
/// </summary>
/// <param name="L">Lua 状态机</param>
/// <param name="ud">实际类型为 struct uart_frame_parser_expression *，指向预编译表达式</param>
/// <param name="ptr_data_size">执行成功后用该指针写入字节码大小</param>
/// <returns>指向字节码起始位置</returns>
static const char *do_read_bytecode(lua_State *L, void *ud, size_t *ptr_data_size) {
    (void) L;

    struct uart_frame_parser_expression *expression = ud;
    *ptr_data_size = expression->data_size;
    return (const char*)expression->data;
}

static int expression_result_create(struct uart_frame_parser_expression_result **ptr_result, size_t byte_array_size,
                                    uart_frame_parser_error_callback_t on_error, void* user_ptr) {
    struct uart_frame_parser_expression_result* old_result = *ptr_result;
    *ptr_result = realloc(old_result,
                          offsetof(struct uart_frame_parser_expression_result, byte_array) + byte_array_size);
    if (*ptr_result) {
        return 1;
    } else {
        free(old_result);
        on_error(user_ptr, UART_FRAME_PARSER_ERROR_MALLOC, __FILE__, __LINE__, "allocate a expression result failed");
        return -6;
    }
}

static int handle_validator_expression_result(lua_State *L, int status, int result_type,
                                              struct uart_frame_parser_expression *expression) {
    if (status == LUA_OK) {
        if (result_type == LUA_TBOOLEAN) {
            int ret = expression_result_create(&expression->result, 0, expression->expression_engine->on_error, expression->expression_engine->user_ptr);
            if (ret > 0) {
                expression->result->type = expression->type;
                expression->result->boolean = lua_toboolean(L, -1);
                return 1;
            } else {
                return ret;
            }
        } else {
            expression->expression_engine->on_error(expression->expression_engine->user_ptr, UART_FRAME_PARSER_ERROR_LUA, __FILE__, __LINE__,
                                                    "expression %s type: boolean, actual data type: %s",
                                                    expression->name, luaL_typename(L, -1));
            return -5;
        }
    } else {
        if (result_type == LUA_TNUMBER) {
            int ret = (int) lua_tointeger(L, -1);
            return ret;
        } else {
            handle_lua_error(L, expression->expression_engine->on_error, expression->expression_engine->user_ptr);
            return -2;
        }
    }
}

static int handle_length_expression_result(lua_State *L, int status, int result_type,
                                           struct uart_frame_parser_expression *expression) {
    if (status == LUA_OK) {
        if (result_type == LUA_TNUMBER) {
            long long result = lua_tointeger(L, -1);
            if (result > 0) {
                int ret = expression_result_create(&expression->result, 0, expression->expression_engine->on_error, expression->expression_engine->user_ptr);
                if (ret > 0) {
                    expression->result->type = expression->type;
                    expression->result->integer = result;
                    return 1;
                } else {
                    return ret;
                }
            }
            else {
                expression->expression_engine->on_error(expression->expression_engine->user_ptr, UART_FRAME_PARSER_ERROR_LUA, __FILE__, __LINE__,
                                                        "length expression %s result <= 0: %lld", expression->name,
                                                        result);
                return -8;
            }
        } else {
            expression->expression_engine->on_error(expression->expression_engine->user_ptr, UART_FRAME_PARSER_ERROR_LUA, __FILE__, __LINE__,
                                                    "expression %s type: integer, actual data type: %s",
                                                    expression->name, luaL_typename(L, -1));
            return -5;
        }
    } else {
        if (result_type == LUA_TNUMBER) {
            return (int) lua_tointeger(L, -1);
        } else {
            handle_lua_error(L, expression->expression_engine->on_error, expression->expression_engine->user_ptr);
            return -2;
        }
    }
}

static int handle_default_expression_result(lua_State *L, int status, int result_type,
                                            struct uart_frame_parser_expression *expression) {
    if (status == LUA_OK) {
        if (result_type == LUA_TSTRING) {
            size_t len;
            const char *data = lua_tolstring(L, -1, &len);
            int ret = expression_result_create(&expression->result, len, expression->expression_engine->on_error, expression->expression_engine->user_ptr);
            if (ret > 0) {
                expression->result->type = expression->type;
                expression->result->byte_array_size = (uint32_t)len;
                memcpy(expression->result->byte_array, data, len);
                return 1;
            } else {
                return ret;
            }
        } else {
            expression->expression_engine->on_error(expression->expression_engine->user_ptr, UART_FRAME_PARSER_ERROR_LUA, __FILE__, __LINE__,
                                                    "expression %s type: string, actual data type: %s",
                                                    expression->name, luaL_typename(L, -1));
            return -5;
        }
    } else {
        if (result_type == LUA_TNUMBER) {
            return (int) lua_tointeger(L, -1);
        } else {
            handle_lua_error(L, expression->expression_engine->on_error, expression->expression_engine->user_ptr);
            return -2;
        }
    }
}

static int handle_tostring_expression_result(lua_State *L, int status, int result_type,
                                             struct uart_frame_parser_expression *expression) {
    if (status == LUA_OK) {
        if (result_type == LUA_TSTRING) {
            size_t len;
            const char *string = lua_tolstring(L, -1, &len);
            int ret = expression_result_create(&expression->result, len + 1, expression->expression_engine->on_error, expression->expression_engine->user_ptr);
            if (ret > 0) {
                expression->result->byte_array_size = (uint32_t)(len + 1);
                memcpy(expression->result->byte_array, string, len + 1);
                expression->result->type = expression->type;
                return 1;
            } else {
                return ret;
            }
        } else {
            expression->expression_engine->on_error(expression->expression_engine->user_ptr, UART_FRAME_PARSER_ERROR_LUA, __FILE__, __LINE__,
                                                    "expression %s type: string, actual data type: %s",
                                                    expression->name, luaL_typename(L, -1));
            return -5;
        }
    } else {
        if (result_type == LUA_TNUMBER) {
            return (int) lua_tointeger(L, -1);
        } else {
            handle_lua_error(L, expression->expression_engine->on_error, expression->expression_engine->user_ptr);
            return -2;
        }
    }
}

struct uart_frame_parser_expression_result *
uart_frame_parser_expression_get_result(struct uart_frame_parser_expression *expression) {
    return expression->result;
}

int uart_frame_parser_expression_eval(struct uart_frame_parser_expression *expression, uint32_t offset) {
    lua_State *L = expression->expression_engine->L;

    lua_pushlightuserdata(L, (void *) OFFSET_INDEX);
    lua_pushinteger(L, offset);
    lua_settable(L, LUA_REGISTRYINDEX);

    if (expression->expression_engine->last_expression != expression) {
        lua_settop(L, 0);
        lua_load(L, do_read_bytecode, expression, expression->name, NULL);
        expression->expression_engine->last_expression = expression;
    }

    int ret = -9;
    int status = lua_pcall(L, 0, 1, 0);
    int result_type = lua_type(L, lua_gettop(L));
    switch (expression->type) {
        case EXPRESSION_VALIDATOR:
            ret = handle_validator_expression_result(L, status, result_type, expression);
            break;
        case EXPRESSION_LENGTH:
            ret = handle_length_expression_result(L, status, result_type, expression);
            break;
        case EXPRESSION_DEFAULT_VALUE:
            ret = handle_default_expression_result(L, status, result_type, expression);
            break;
        case EXPRESSION_TOSTRING:
            ret = handle_tostring_expression_result(L, status, result_type, expression);
            break;
    }

    lua_settop(L, 0);
    return ret;
}
