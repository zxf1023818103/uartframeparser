#include <stdlib.h>
#include <string.h>
#include "uartframeparser.h"

static int next_frame(struct uart_frame_definition *frame_definition_head,
                      struct uart_frame_detected_frame *detected_frame_head,
                      struct uart_frame_field_info **ptr_field_info,
                      struct uart_frame_definition **ptr_frame_definition,
                      struct uart_frame_parser_buffer *buffer,
                      uint32_t offset,
                      uint32_t max_size,
                      uart_frame_parser_error_callback_t on_error);

static void uart_frame_field_info_release(struct uart_frame_field_info *field_info_head) {
    while (field_info_head) {
        struct uart_frame_field_info *next = field_info_head->next;
        free(field_info_head);
        field_info_head = next;
    }
}

static int
parse_field(struct uart_frame_field_definition *field_definition, struct uart_frame_definition *frame_definition_head,
            struct uart_frame_field_info **ptr_field_info, struct uart_frame_parser_buffer *buffer, uint32_t offset,
            uint32_t field_offset, uint32_t max_size, uart_frame_parser_error_callback_t on_error) {
    int length;
    if (field_definition->has_length_expression) {
        int result = uart_frame_parser_expression_eval(field_definition->length.expression, offset);
        if (result > 0) {
            length = (int) uart_frame_parser_expression_get_result(field_definition->length.expression)->integer;
        } else {
            return result;
        }
    } else {
        length = (int) field_definition->length.value;
    }

    if (max_size == 0 || length + field_offset <= max_size) {
        struct uart_frame_field_info *field_info = calloc(1, sizeof(struct uart_frame_field_info));
        if (field_info) {
            struct uart_frame_field_info *subframe_field_info = NULL;
            struct uart_frame_definition *subframe_definition = NULL;
            if (field_definition->has_subframes) {
                int result = next_frame(frame_definition_head, field_definition->detected_subframe_head,
                                        &subframe_field_info, &subframe_definition, buffer, field_offset, length, on_error);
                if (result <= 0) {
                    free(field_info);
                    return result;
                }
            }

            field_info->field_definition = field_definition;
            field_info->parent_offset = offset;
            field_info->offset = field_offset + offset;
            field_info->data_size = length;
            field_info->subframe_field_info = subframe_field_info;
            field_info->subframe_definition = subframe_definition;
            *ptr_field_info = field_info;

            return length;
        } else {
            on_error(UART_FRAME_PARSER_ERROR_MALLOC, __FILE__, __LINE__, "cannot allocate a field info");
            return -6;
        }
    }
    else {
        return -7;
    }
}

/// <summary>
/// 不验证帧格式，直接解析帧数据
/// </summary>
/// <param name="frame_definition">要解析的帧格式定义</param>
/// <param name="buffer">帧缓冲区</param>
/// <param name="offset">从帧缓冲区指定偏移量开始解析帧数据</param>
/// <param name="on_error">错误回调函数</param>
/// <param name="ptr_field_info">指向解析出的帧数据指针</param>
/// <returns>-8：长度表达式小于或等于0；-7：帧格式超过max_size指定大小；-6：malloc失败；-5：表达式返回类型不对；-4：帧格式名称错误，-3：帧校验表达式不存在；-2：Lua语句执行错误；-1：需要更多数据；0：表达式校验未通过；其他非负值：解析成功，返回帧长度</returns>
static int
parse_frame(struct uart_frame_definition *frame_definition, struct uart_frame_definition *frame_definition_head,
            struct uart_frame_field_info **ptr_field_info_head, struct uart_frame_parser_buffer *buffer,
            uint32_t offset, uint32_t max_size, uart_frame_parser_error_callback_t on_error) {
    int field_offset = 0;

    struct uart_frame_field_info *field_info_cur = NULL;
    struct uart_frame_field_info *field_info_head = NULL;

    struct uart_frame_field_definition *field_definition = frame_definition->field_definition_head;
    while (field_definition) {
        struct uart_frame_field_info *field_info;
        int field_size = parse_field(field_definition, frame_definition_head, &field_info, buffer,
                                     offset, field_offset, max_size, on_error);
        if (field_size > 0) {
            if (field_info_cur) {
                field_info_cur = field_info_cur->next = field_info;
            } else {
                field_info_cur = field_info_head = field_info;
            }
            field_offset += field_size;
        } else {
            if (field_size != -1) {
                uart_frame_field_info_release(field_info_head);
            }
            return field_size;
        }

        field_definition = field_definition->next;
    }

    *ptr_field_info_head = field_info_head;
    return field_offset;
}

/// <summary>
/// 根据帧定义名称查找帧定义
/// </summary>
/// <param name="frame_definition_head">帧定义列表头</param>
/// <param name="name">帧定义名称</param>
/// <returns>返回查找到的帧定义，否则返回 NULL</returns>
static struct uart_frame_definition *
find_frame_definition_by_name(struct uart_frame_definition *frame_definition_head, const char *name) {
    while (frame_definition_head) {
        if (strcmp(frame_definition_head->name, name) == 0) {
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
/// <param name="ptr_field_info">指向解析出的帧数据指针</param>
/// <returns>-7：帧格式超过max_size指定大小；-6：malloc失败；-5：表达式返回类型不对；-4：帧格式名称错误，-3：帧校验表达式不存在；-2：Lua语句执行错误；-1：需要更多数据；0：表达式校验未通过；其他非负值：解析成功，返回帧长度</returns>
static int try_to_parse_the_frame(const char *frame_name, struct uart_frame_definition *frame_definition_head,
                                  struct uart_frame_field_info **ptr_field_info_head,
                                  struct uart_frame_definition **ptr_frame_definition,
                                  struct uart_frame_parser_buffer *buffer, uint32_t offset, uint32_t max_size,
                                  uart_frame_parser_error_callback_t on_error) {
    struct uart_frame_definition *frame_definition = find_frame_definition_by_name(frame_definition_head, frame_name);
    if (frame_definition) {
        if (frame_definition->validator_expression) {
            int ret = uart_frame_parser_expression_eval(frame_definition->validator_expression, offset);
            if (ret > 0) {
                if (uart_frame_parser_expression_get_result(frame_definition->validator_expression)->boolean) {
                    *ptr_frame_definition = frame_definition;
                    return parse_frame(frame_definition, frame_definition_head,
                                       ptr_field_info_head, buffer, offset, max_size, on_error);
                } else {
                    return 0;
                }
            } else {
                return ret;
            }
        } else {
            return parse_frame(frame_definition, frame_definition_head, ptr_field_info_head,
                               buffer, offset, max_size, on_error);
        }
    } else {
        on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__, "frame name %s not found", frame_name);
        return -4;
    }
}

static int try_to_parse_a_frame(struct uart_frame_definition *frame_definition_head,
                                struct uart_frame_detected_frame *detected_frame_head,
                                struct uart_frame_field_info **ptr_field_info,
                                struct uart_frame_definition **ptr_frame_definition,
                                struct uart_frame_parser_buffer *buffer,
                                uint32_t offset,
                                uint32_t max_size,
                                uart_frame_parser_error_callback_t on_error) {
    struct uart_frame_detected_frame *detected_frame = detected_frame_head;
    while (detected_frame) {
        int frame_bytes = try_to_parse_the_frame(detected_frame->name, frame_definition_head,
                                                 ptr_field_info, ptr_frame_definition, buffer, offset, max_size,
                                                 on_error);
        if (frame_bytes) {
            return frame_bytes;
        }

        detected_frame = detected_frame->next;
    }

    return 0;
}

static int next_frame(struct uart_frame_definition *frame_definition_head,
                      struct uart_frame_detected_frame *detected_frame_head,
                      struct uart_frame_field_info **ptr_field_info,
                      struct uart_frame_definition **ptr_frame_definition,
                      struct uart_frame_parser_buffer *buffer,
                      uint32_t offset,
                      uint32_t max_size,
                      uart_frame_parser_error_callback_t on_error) {
    for (;;) {
        int frame_bytes = try_to_parse_a_frame(frame_definition_head, detected_frame_head, ptr_field_info,
                                               ptr_frame_definition, buffer, offset, max_size, on_error);
        if (frame_bytes) {
            return frame_bytes;
        } else {
            uart_frame_parser_buffer_increase_origin(buffer, 1);
        }
    }
}

int uart_frame_parser_feed_data(struct uart_frame_parser *parser, uint8_t *data, uint32_t size, void *user_ptr) {
    uart_frame_parser_buffer_append(parser->buffer, data, size);

    if (parser->last_field_info_head && parser->last_frame_definition) {
        if (parser->frame_bytes <= uart_frame_parser_buffer_get_size(parser->buffer)) {
            parser->on_data(parser->buffer, parser->last_frame_definition, parser->last_field_info_head, user_ptr);
            parser->frame_bytes = 0;
            parser->last_field_info_head = NULL;
            parser->last_frame_definition = NULL;
        } else {
            return -1;
        }
    }

    for (;;) {
        struct uart_frame_field_info *field_info_head;
        struct uart_frame_definition *frame_definition;
        int frame_bytes = next_frame(parser->frame_definition_head, parser->detected_frame_head, &field_info_head,
                                     &frame_definition, parser->buffer, 0, 0, parser->on_error);
        if (frame_bytes > 0) {
            if ((uint32_t)frame_bytes <= uart_frame_parser_buffer_get_size(parser->buffer)) {
                parser->on_data(parser->buffer, frame_definition, field_info_head, user_ptr);
                uart_frame_parser_buffer_increase_origin(parser->buffer, frame_bytes);
            } else {
                parser->frame_bytes = frame_bytes;
                parser->last_frame_definition = frame_definition;
                parser->last_field_info_head = field_info_head;
                return -1;
            }
        } else {
            return frame_bytes;
        }
    }
}
