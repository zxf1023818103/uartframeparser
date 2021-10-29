#ifndef UART_FRAME_PARSER_H
#define UART_FRAME_PARSER_H

#include <stdint.h>
#include <lua.h>

enum uart_frame_parser_errors {
	UART_FRAME_PARSER_ERROR_CJSON = 1,
	UART_FRAME_PARSER_ERROR_MALLOC = 2,
	UART_FRAME_PARSER_ERROR_CONFIG = 3,
};

/// <summary>
/// 帧解析器错误回调函数定义
/// </summary>
typedef void(*uart_frame_parser_error_callback_t)(uint8_t error_type, ...);

/// <summary>
/// 帧类型信息
/// </summary>
struct uart_frame_config {

	/// <summary>
	/// 帧类型名称
	/// </summary>
	char* name;

	/// <summary>
	/// 帧类型描述
	/// </summary>
	char* description;

	/// <summary>
	/// 帧类型校验表达式
	/// </summary>
	char* validator_expression;

	/// <summary>
	/// 帧字段列表
	/// </summary>
	struct uart_frame_field_config *field_head;
};

/// <summary>
/// 帧字段信息
/// </summary>
struct uart_frame_field_config {

	/// <summary>
	/// 帧字段名称
	/// </summary>
	char* name;

	/// <summary>
	/// 帧字段描述
	/// </summary>
	char* description;

	struct {
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
	union {

		/// <summary>
		/// 帧字段长度表达式
		/// </summary>
		char* expression;

		/// <summary>
		/// 帧字段长度数值
		/// </summary>
		uint32_t value;
	} length;

	union {

		/// <summary>
		/// 帧字段默认值表达式
		/// </summary>
		char* default_value_expression;

		struct {
			/// <summary>
			/// 要检测的子帧类型个数
			/// </summary>
			uint32_t subframe_numbers;

			/// <summary>
			/// 要检测的子帧类型列表
			/// </summary>
			struct uart_frame_config* subframes[1];
		};

		struct {
			/// <summary>
			/// 该字段包含的位字段个数
			/// </summary>
			uint32_t bitfield_numbers;

			/// <summary>
			/// 该字段包含的位字段列表
			/// </summary>
			struct {
				
				/// <summary>
				/// 位字段名称
				/// </summary>
				char* name;

				/// <summary>
				/// 位字段描述
				/// </summary>
				char* description;

				/// <summary>
				/// 位字段长度
				/// </summary>
				uint8_t bits;
			} bitfields [1];
		};
	};
};

/// <summary>
/// 帧解析器信息
/// </summary>
struct uart_frame_parser {

	/// <summary>
	/// 用于执行表达式的 Lua 状态机
	/// </summary>
	lua_State* L;

	/// <summary>
	/// 帧解析器的错误回调函数
	/// </summary>
	uart_frame_parser_error_callback_t on_error;

	/// <summary>
	/// 帧类型数量
	/// </summary>
	uint32_t frame_numbers;

	/// <summary>
	/// 帧类型列表
	/// </summary>
	struct uart_frame_config* frames;

	/// <summary>
	/// 要检测的帧类型数量
	/// </summary>
	uint32_t detected_frame_numbers;

	/// <summary>
	/// 要检测的帧类型列表
	/// </summary>
	struct uart_frame_config* detected_frames[1];
};

/// <summary>
/// 初始化帧解析器
/// </summary>
/// <param name="ptr_parser">指向传出的帧解析器指针</param>
/// <param name="json_config">帧解析器的 JSON 配置文件</param>
/// <param name="json_config_size">帧解析器的 JSON 配置文件长度</param>
/// <param name="on_error">帧解析器的错误回调函数</param>
void uart_frame_parser_init(struct uart_frame_parser** ptr_parser, const char* json_config, uint32_t json_config_size, uart_frame_parser_error_callback_t on_error);

/// <summary>
/// 释放帧解析器
/// </summary>
/// <param name="parser">要释放的帧解析器</param>
void uart_frame_parser_release(struct uart_frame_parser* parser);

/// <summary>
/// 向帧解析器传入要解析的原始数据
/// </summary>
/// <param name="parser">要传入数据的帧解析器</param>
/// <param name="data">要传入的数据</param>
/// <param name="size">传入数据的长度（字节）</param>
void uart_frame_parser_feed_data(struct uart_frame_parser* parser, uint8_t* data, uint32_t size);

#endif // UART_FRAME_PARSER_H