#ifndef UART_FRAME_PARSER_H
#define UART_FRAME_PARSER_H

#include <stdint.h>
#include <lua.h>

/// <summary>
/// 帧解析器错误类型
/// </summary>
enum uart_frame_parser_error_types
{

	/// <summary>
	/// cJSON 库返回的错误
	/// </summary>
	UART_FRAME_PARSER_ERROR_CJSON = 1,

	/// <summary>
	/// 内存分配失败
	/// </summary>
	UART_FRAME_PARSER_ERROR_MALLOC = 2,

	/// <summary>
	/// 配置项错误
	/// </summary>
	UART_FRAME_PARSER_ERROR_PARSE_CONFIG = 3,

	/// <summary>
	/// Lua 返回的错误
	/// </summary>
	UART_FRAME_PARSER_ERROR_LUA = 4,
};

/// <summary>
/// 帧解析器错误回调函数定义
/// </summary>
typedef void (*uart_frame_parser_error_callback_t)(uint8_t error_type, ...);

/// <summary>
/// 用于帧解析器阻塞读取数据的回调函数的定义
/// </summary>
typedef void (*uart_frame_parser_blocking_read_data_fn_t)(uint8_t* data, uint32_t size);

/// <summary>
/// 帧类型信息
/// </summary>
struct uart_frame_definition
{

	/// <summary>
	/// 指向下一个帧类型
	/// </summary>
	struct uart_frame_definition *next;

	/// <summary>
	/// 帧类型名称
	/// </summary>
	char *name;

	/// <summary>
	/// 帧类型描述
	/// </summary>
	char *description;

	/// <summary>
	/// 帧类型校验表达式
	/// </summary>
	char *validator_expression;

	/// <summary>
	/// 帧字段列表
	/// </summary>
	struct uart_frame_field_definition *field_head;
};

/// <summary>
/// 帧字段信息
/// </summary>
struct uart_frame_field_definition
{

	/// <summary>
	/// 指向下一个帧字段
	/// </summary>
	struct uart_frame_field_definition *next;

	/// <summary>
	/// 帧字段名称
	/// </summary>
	char *name;

	/// <summary>
	/// 帧字段描述
	/// </summary>
	char *description;

	struct
	{
		/// <summary>
		/// 帧字段信息含有长度表达式
		/// </summary>
		uint8_t has_length_expression : 1;

		/// <summary>
		/// 帧字段含有位字段
		/// </summary>
		uint8_t has_bitfields : 1;

		/// <summary>
		/// 帧字段含有子帧
		/// </summary>
		uint8_t has_subframes : 1;
	};

	/// <summary>
	/// 帧字段长度（字节）
	/// </summary>
	union
	{
		/// <summary>
		/// 帧字段长度表达式
		/// </summary>
		char *expression;

		/// <summary>
		/// 帧字段长度数值
		/// </summary>
		uint32_t value;
	} length;

	union
	{
		/// <summary>
		/// 帧字段默认值表达式
		/// </summary>
		char *default_value_expression;

		/// <summary>
		/// 要探测的子帧类型列表
		/// </summary>
		struct uart_frame_detected_frame *detected_subframe_head;

		/// <summary>
		/// 包含的位字段列表
		/// </summary>
		struct uart_frame_bitfield_definition *bitfield_head;
	};
};

/// <summary>
/// 帧解析器信息
/// </summary>
struct uart_frame_parser
{
	/// <summary>
	/// 数据缓冲区
	/// </summary>
	struct uart_frame_parser_buffer* buffer;

	/// <summary>
	/// 用于执行表达式的 Lua 状态机
	/// </summary>
	lua_State *L;

	/// <summary>
	/// 帧解析器的错误回调函数
	/// </summary>
	uart_frame_parser_error_callback_t on_error;

	/// <summary>
	/// 帧类型列表
	/// </summary>
	struct uart_frame_definition *frame_definition_head;

	/// <summary>
	/// 要检测的帧类型列表
	/// </summary>
	struct uart_frame_detected_frame *detected_frame_head;
};

/// <summary>
/// 要探测的帧类型信息
/// </summary>
struct uart_frame_detected_frame
{
	/// <summary>
	/// 指向下一个要探测的帧类型
	/// </summary>
	struct uart_frame_detected_frame *next;

	/// <summary>
	/// 要探测的帧类型名称
	/// </summary>
	char *name;
};

/// <summary>
/// 帧内位字段定义
/// </summary>
struct uart_frame_bitfield_definition
{
	/// <summary>
	/// 指向下一个位字段
	/// </summary>
	struct uart_frame_bitfield_definition* next;

	/// <summary>
	/// 位字段名称
	/// </summary>
	char *name;

	/// <summary>
	/// 位字段描述
	/// </summary>
	char *description;

	/// <summary>
	/// 位字段长度
	/// </summary>
	uint8_t bits;
};

/// <summary>
/// 创建帧解析器
/// </summary>
/// <param name="ptr_parser">指向传出的帧解析器指针</param>
/// <param name="json_config">帧解析器的 JSON 配置文件</param>
/// <param name="json_config_size">帧解析器的 JSON 配置文件长度</param>
/// <param name="on_error">帧解析器的错误回调函数</param>
/// <return>创建的帧解析器实例</return>
struct uart_frame_parser *uart_frame_parser_create(const char *json_config, uint32_t json_config_size, uart_frame_parser_error_callback_t on_error);

/// <summary>
/// 释放帧解析器实例
/// </summary>
/// <param name="parser">要释放的帧解析器实例</param>
void uart_frame_parser_release(struct uart_frame_parser *parser);

/// <summary>
/// 向帧解析器填充数据进行解析
/// </summary>
/// <param name="parser">帧解析器</param>
/// <param name="data">要填充的数据的起始位置</param>
/// <param name="size">要填充的数据大小（字节）</param>
void uart_frame_parser_feed_data(struct uart_frame_parser* parser, uint8_t* data, uint32_t size);

/// <summary>
/// 创建帧解析器缓冲区
/// </summary>
/// <param name="on_error">错误回调函数</param>
struct uart_frame_parser_buffer* uart_frame_parser_buffer_create(uart_frame_parser_error_callback_t on_error);

/// <summary>
/// 释放帧缓冲区
/// </summary>
/// <param name="buffer">要释放的帧缓冲区</param>
void uart_frame_parser_buffer_release(struct uart_frame_parser_buffer* buffer);

/// <summary>
/// 向帧缓冲区追加数据
/// </summary>
/// <param name="buffer">要写入数据的帧缓冲区</param>
/// <param name="data">指向数据起始位置</param>
/// <param name="size">写入数据大小（字节）</param>
void uart_frame_parser_buffer_append(struct uart_frame_parser_buffer* buffer, uint8_t* data, uint32_t size);

/// <summary>
/// 读取帧缓冲区指定位置的数据
/// </summary>
/// <param name="buffer">要读取数据的缓冲区</param>
/// <param name="offset">数据相对于缓冲区起始位置的偏移量</param>
/// <param name="data">指向数据存放起始位置</param>
/// <param name="size">要读取数据的大小（字节）</param>
/// <returns>
/// 0：读取的数据超出范围；1：成功
/// </returns>
int uart_frame_parser_buffer_read(struct uart_frame_parser_buffer* buffer, uint32_t offset, uint8_t* data, uint32_t size);

/// <summary>
/// 获取帧缓冲区从当前起始位置开始的数据大小
/// </summary>
/// <param name="buffer">要获取大小的帧缓冲区</param>
/// <returns>帧缓冲区相对于起始位置的数据大小</returns>
uint32_t uart_frame_parser_buffer_get_size(struct uart_frame_parser_buffer* buffer);

/// <summary>
/// 向后移动帧缓冲区的起始位置，例如起始向后移动 1 字节，原来第 2 个字节的数据变为第 1 个字节的数据，以此类推
/// </summary>
/// <param name="buffer">帧缓冲区</param>
/// <param name="increasement">起始位置向后移动多少字节</param>
void uart_frame_parser_buffer_increase_origin(struct uart_frame_parser_buffer* buffer, uint32_t increasement);

/// <summary>
/// 访问帧缓冲区指定偏移量的字节
/// </summary>
/// <param name="buffer">帧缓冲区</param>
/// <param name="offset">偏移量</param>
/// <returns>-1：下标越界；非负对应指定位置的字节</returns>
int uart_frame_parser_buffer_at(struct uart_frame_parser_buffer* buffer, uint32_t offset);

#endif // UART_FRAME_PARSER_H