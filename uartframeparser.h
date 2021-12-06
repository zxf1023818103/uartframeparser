#ifndef UART_FRAME_PARSER_H
#define UART_FRAME_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/// <summary>
/// 帧解析器错误类型
/// </summary>
enum uart_frame_parser_error_types {

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
/// 帧解析器表达式类型
/// </summary>
enum uart_frame_parser_expression_types {
    /// <summary>
    /// 帧验证表达式，返回一个布尔值
    /// </summary>
    EXPRESSION_VALIDATOR = 1,

    /// <summary>
    /// 长度表达式，返回一个无符号整数
    /// </summary>
    EXPRESSION_LENGTH = 2,

    /// <summary>
    /// 帧字段默认值表达式，返回一个字节数组
    /// </summary>
    EXPRESSION_DEFAULT_VALUE = 3,

    /// <summary>
    /// 帧字段解码表达式，把字段翻译成自然语言，返回一个字符串
    /// </summary>
    EXPRESSION_TOSTRING = 4,
};

struct uart_frame_parser_expression_result {

    enum uart_frame_parser_expression_types type;

    union {

        int boolean;

        long long integer;

        uint32_t byte_array_size;
    };

    uint8_t byte_array[1];
};

/// <summary>
/// 帧类型信息
/// </summary>
struct uart_frame_definition {
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
    struct uart_frame_parser_expression *validator_expression;

    /// <summary>
    /// 帧字段列表
    /// </summary>
    struct uart_frame_field_definition *field_definition_head;
};

/// <summary>
/// 帧字段信息
/// </summary>
struct uart_frame_field_definition {

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

    struct {
        /// <summary>
        /// 帧字段信息含有长度表达式
        /// </summary>
        uint8_t has_length_expression: 1;

        /// <summary>
        /// 帧字段含有位字段
        /// </summary>
        uint8_t has_bitfields: 1;

        /// <summary>
        /// 帧字段含有子帧
        /// </summary>
        uint8_t has_subframes: 1;
    };

    /// <summary>
    /// 帧字段长度（字节）
    /// </summary>
    union {
        /// <summary>
        /// 帧字段长度表达式
        /// </summary>
        struct uart_frame_parser_expression *expression;

        /// <summary>
        /// 帧字段长度数值
        /// </summary>
        uint32_t value;
    } length;

    union {
        struct {
            /// <summary>
            /// 帧字段默认值表达式
            /// </summary>
            struct uart_frame_parser_expression *default_value_expression;

            /// <summary>
            /// to string 自定义实现
            /// </summary>
            struct uart_frame_parser_expression *tostring_expression;
        };

        /// <summary>
        /// 要探测的子帧类型列表
        /// </summary>
        struct uart_frame_detected_frame *detected_subframe_head;

        /// <summary>
        /// 包含的位字段列表
        /// </summary>
        struct uart_frame_bitfield_definition *bitfield_definition_head;
    };
};

/// <summary>
/// 要探测的帧类型信息
/// </summary>
struct uart_frame_detected_frame {
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
struct uart_frame_bitfield_definition {
    /// <summary>
    /// 指向下一个位字段
    /// </summary>
    struct uart_frame_bitfield_definition *next;

    /// <summary>
    /// 位字段名称
    /// </summary>
    char *name;

    /// <summary>
    /// 位字段描述
    /// </summary>
    char *description;

    /// <summary>
    /// 位字段偏移量
    /// </summary>
    uint32_t offset_bits;

    /// <summary>
    /// 位字段长度
    /// </summary>
    uint32_t bits;
};

/// <summary>
/// 帧字段数据
/// </summary>
struct uart_frame_field_info {
    /// <summary>
    /// 指向下一个帧字段数据起始位置
    /// </summary>
    struct uart_frame_field_info *next;

    /// <summary>
    /// 对应的帧字段信息
    /// </summary>
    struct uart_frame_field_definition *field_definition;

    /// <summary>
    /// 父帧字段偏移量
    /// </summary>
    uint32_t parent_offset;

    /// <summary>
    /// 帧字段偏移量
    /// </summary>
    uint32_t offset;

    /// <summary>
    /// 帧字段长度
    /// </summary>
    uint32_t data_size;

    /// <summary>
    /// 包含的子帧数据
    /// </summary>
    struct uart_frame_field_info *subframe_field_info;

    /// <summary>
    /// 包含的子帧定义
    /// </summary>
    struct uart_frame_definition *subframe_definition;
};

/// <summary>
/// 帧字段数据
/// </summary>
struct uart_frame_field_data {

    /// <summary>
    /// 下一个帧字段数据
    /// </summary>
    struct uart_frame_field_data* next;

    /// <summary>
    /// 对应的帧字段信息
    /// </summary>
    struct uart_frame_field_info* field_info;

    union {

        /// <summary>
        /// 包含的子帧字段数据
        /// </summary>
        struct uart_frame_field_data* subframe_field_data_head;

        /// <summary>
        /// 包含的位字段数据
        /// </summary>
        struct uart_frame_bitfield_data* bitfield_data_head;

        /// <summary>
        /// 包含的帧字段数据
        /// </summary>
        uint8_t data[1];
    };
};

/// <summary>
/// 位字段数据
/// </summary>
struct uart_frame_bitfield_data {

    /// <summary>
    /// 下一个位字段数据
    /// </summary>
    struct uart_frame_bitfield_data* next;

    /// <summary>
    /// 对应的位字段定义
    /// </summary>
    struct uart_frame_bitfield_definition* bitfield_definition;

    /// <summary>
    /// 包含的位字段数据
    /// </summary>
    uint8_t data[1];
};

/// <summary>
/// 帧解析器错误回调函数定义
/// </summary>
typedef void (*uart_frame_parser_error_callback_t)(enum uart_frame_parser_error_types error_type, const char *file,
                                                   int line, const char *fmt, ...);

/// <summary>
/// 帧解析器数据回调函数定义
/// </summary>
/// <param name="frame_data">指向解析出的帧数据</param>
/// <param name="user_ptr">用户传入的自定义参数</param>
typedef void (*uart_frame_parser_data_callback_t)(void *buffer, struct uart_frame_definition *frame_definition,
                                                 struct uart_frame_field_info *field_info_head, void *user_ptr);

/// <summary>
/// 帧解析器信息
/// </summary>
struct uart_frame_parser {
    /// <summary>
    /// 数据缓冲区
    /// </summary>
    struct uart_frame_parser_buffer *buffer;

    /// <summary>
    /// 表达式计算引擎
    /// </summary>
    struct uart_frame_parser_expression_engine *expression_engine;

    /// <summary>
    /// 帧解析器的错误回调函数
    /// </summary>
    uart_frame_parser_error_callback_t on_error;

    /// <summary>
    /// 帧解析器的数据回调函数
    /// </summary>
    uart_frame_parser_data_callback_t on_data;

    /// <summary>
    /// 帧类型列表
    /// </summary>
    struct uart_frame_definition *frame_definition_head;

    /// <summary>
    /// 要检测的帧类型列表
    /// </summary>
    struct uart_frame_detected_frame *detected_frame_head;

    struct uart_frame_field_info *last_field_info_head;

    struct uart_frame_definition *last_frame_definition;

    uint32_t frame_bytes;
};

/// <summary>
/// 创建帧解析器
/// </summary>
/// <param name="ptr_parser">指向传出的帧解析器指针</param>
/// <param name="json_config">帧解析器的 JSON 配置文件</param>
/// <param name="json_config_size">帧解析器的 JSON 配置文件长度</param>
/// <param name="on_error">帧解析器的错误回调函数</param>
/// <return>创建的帧解析器实例</return>
struct uart_frame_parser *uart_frame_parser_create(const char *json_config, uint32_t json_config_size,
                                                   uart_frame_parser_error_callback_t on_error,
                                                   uart_frame_parser_data_callback_t on_data);

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
/// <param name="user_ptr">传递给 uart_frame_parser_data_callback_t 的 user_ptr</param>
/// <returns></returns>
int uart_frame_parser_feed_data(struct uart_frame_parser *parser, uint8_t *data, uint32_t size, void *user_ptr);

/// <summary>
/// 创建帧解析器缓冲区
/// </summary>
/// <param name="on_error">错误回调函数</param>
/// <param name="reserved">用于向自定义帧缓冲区实现中传递更多参数，默认可填 NULL</param>
void *
uart_frame_parser_buffer_create(uart_frame_parser_error_callback_t on_error, void *reserved);

/// <summary>
/// 释放帧缓冲区
/// </summary>
/// <param name="buffer">要释放的帧缓冲区</param>
void uart_frame_parser_buffer_release(void *buffer);

/// <summary>
/// 向帧缓冲区追加数据
/// </summary>
/// <param name="buffer">要写入数据的帧缓冲区</param>
/// <param name="data">指向数据起始位置</param>
/// <param name="size">写入数据大小（字节）</param>
void uart_frame_parser_buffer_append(void *buffer, uint8_t *data, uint32_t size);

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
int
uart_frame_parser_buffer_read(void *buffer, uint32_t offset, uint8_t *data, uint32_t size);

/// <summary>
/// 获取帧缓冲区从当前起始位置开始的数据大小
/// </summary>
/// <param name="buffer">要获取大小的帧缓冲区</param>
/// <returns>帧缓冲区相对于起始位置的数据大小</returns>
uint32_t uart_frame_parser_buffer_get_size(void *buffer);

/// <summary>
/// 向后移动帧缓冲区的起始位置，例如起始向后移动 1 字节，原来第 2 个字节的数据变为第 1 个字节的数据，以此类推
/// </summary>
/// <param name="buffer">帧缓冲区</param>
/// <param name="increment">起始位置向后移动多少字节</param>
void uart_frame_parser_buffer_increase_origin(void *buffer, uint32_t increment);

/// <summary>
/// 访问帧缓冲区指定偏移量的字节
/// </summary>
/// <param name="buffer">帧缓冲区</param>
/// <param name="offset">偏移量</param>
/// <returns>-1：下标越界；非负对应指定位置的字节</returns>
int uart_frame_parser_buffer_at(void *buffer, uint32_t offset);

/// <summary>
/// 创建表达式计算引擎
/// </summary>
/// <param name="buffer">供表达式访问的帧缓冲区</param>
/// <param name="init_script">创建完成后调用此脚本初始化表达式运行环境，如在脚本中增加自定义函数等</param>
/// <param name="on_error">错误回调函数</param>
/// <param name="reserved"></param>
/// <returns>创建的表达式引擎</returns>
struct uart_frame_parser_expression_engine *
uart_frame_parser_expression_engine_create(struct uart_frame_parser_buffer *buffer, const char *init_script,
                                           uart_frame_parser_error_callback_t on_error, void *reserved);

/// <summary>
/// 释放表达式计算引擎
/// </summary>
/// <param name="engine">要释放的表达式计算引擎</param>
void uart_frame_parser_expression_engine_release(struct uart_frame_parser_expression_engine *engine);

/// <summary>
/// 创建预编译的表达式
/// </summary>
/// <param name="engine">用来执行表达式的引擎</param>
/// <param name="expression_string">表达式字符串</param>
/// <param name="expression_name">表达式名称，为错误信息提供显示</param>
/// <param name="reserved">保留参数，填 NULL</param>
/// <returns>返回预编译的表达式</returns>
struct uart_frame_parser_expression *
uart_frame_parser_expression_create(struct uart_frame_parser_expression_engine *engine, const char *expression_name,
                                    enum uart_frame_parser_expression_types expression_type,
                                    const char *expression_string, void *reserved);

/// <summary>
/// 释放预编译表达式
/// </summary>
void uart_frame_parser_expression_release(struct uart_frame_parser_expression_engine *expression_engine,
                                          struct uart_frame_parser_expression *expression);

/// <summary>
/// 获取表达式值
/// </summary>
/// <param name="expression">要执行的预编译表达式</param>
/// <param name="offset">相对于帧缓冲队列起始位置的偏移量</param>
int uart_frame_parser_expression_eval(struct uart_frame_parser_expression *expression, uint32_t offset);

/// <summary>
/// 计算完成后获取表达式结果
/// </summary>
/// <param name="expression">要获取结果的表达式</param>
/// <returns>返回表达式结果</returns>
struct uart_frame_parser_expression_result *
uart_frame_parser_expression_get_result(struct uart_frame_parser_expression *expression);

/// <summary>
/// 根据帧信息读取指定字段的数据
/// </summary>
/// <param name="buffer">帧缓冲区</param>
/// <param name="field_info_head">帧字段信息列表</param>
/// <param name="concerned_field_definitions">要读取的帧字段的帧定义指针数组，最后一个元素需要为 NULL，传入 NULL 读取所有字段</param>
/// <param name="on_error">错误回调函数</param>
/// <returns>返回帧数据列表</returns>
struct uart_frame_field_data* uart_frame_parser_read_concerned_fields(void* buffer,
    struct uart_frame_field_info* field_info_head,
    struct uart_frame_field_definition** concerned_field_definitions,
    uart_frame_parser_error_callback_t on_error);

/// <summary>
/// 释放帧数据
/// </summary>
/// <param name="field_data_head">要释放的帧数据列表</param>
void uart_frame_parser_field_data_release(struct uart_frame_field_data* field_data_head);

#ifdef __cplusplus
}
#endif

#endif // UART_FRAME_PARSER_H