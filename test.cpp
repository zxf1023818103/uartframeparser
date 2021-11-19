#include <gtest/gtest.h>
#include "uartframeparser.h"
#include <string>

namespace {

    const char *json = R"_({
    "$schema": "./frame.json",
    "definitions": [
        {
            "name": "status_report",
            "fields": [
                {
                    "bytes": 2,
                    "name": "sof",
                    "default": "return 0x55aa",
                    "description": "帧头"
                },
                {
                    "bytes": 1,
                    "name": "length",
                    "description": "长度"
                },
                {
                    "bytes": "return byte(1) - 3",
                    "name": "subframe",
                    "frames": ["subframe1", "subframe2"]
                },
                {
                    "bytes": 1,
                    "name": "checksum"
                }
            ],
            "validator": "return byte(1) == 0x55 and byte(2) == 0xaa and byte(byte(3)) == (sum(1, byte(3) - 1) & 0xff)"
        },
        {
            "name": "subframe1",
            "description": "子帧格式1",
            "fields": [
                {
                    "bytes": 1,
                    "name": "working_status",
                    "description": "工作状态"
                }
            ],
            "validator": "return byte(0) == 0x01"
        },
        {
            "name": "subframe2",
            "description": "子帧格式2",
            "fields": [
                {
                    "bytes": 1,
                    "name": "fan_status",
                    "description": "风机状态"
                }
            ],
            "validator": "return byte(0) == 0x02"
        }
    ],
    "frames": [
        "status_report"
    ],
    "init": "function sum(from, to) result = 0; for i = from, to, 1 do result = result + byte(i) end; return result; end"
})_";


    void on_error(enum uart_frame_parser_error_types error_type, const char *file, int line, const char *fmt, ...) {

        const char *log_template = "%s:%d: %s";
        char *format = static_cast<char *>(std::calloc(std::snprintf(nullptr, 0, log_template, file, line, fmt) + 1,
                                                       1));
        std::sprintf(format, log_template, file, line, fmt);

        va_list ap1, ap2;
        va_start(ap1, fmt);
        va_copy(ap2, ap1);

        char *message = static_cast<char *>(std::calloc(std::vsnprintf(nullptr, 0, format, ap1) + 1, 1));
        std::vsprintf(message, format, ap2);
        GTEST_NONFATAL_FAILURE_(message);
        std::free(message);
        std::free(format);
    }

    TEST(ConfigLoadTest, Example1) {

        struct uart_frame_parser *parser = uart_frame_parser_create(json, strlen(json), on_error,
                                                                    [](struct uart_frame_definition *frame_definition,
                                                                       struct uart_frame_field_data *field_data_head,
                                                                       void *user_ptr) -> int {

                                                                        return 0;
                                                                    });

        ASSERT_NE(nullptr, parser);

        uart_frame_parser_release(parser);
    }

    TEST(FrameParseTest, Example1) {
        struct uart_frame_parser *parser = uart_frame_parser_create(json, strlen(json), on_error,
                                                                    [](struct uart_frame_definition *frame_definition,
                                                                       struct uart_frame_field_data *field_data_head,
                                                                       void *user_ptr) -> int {

                                                                        return 0;
                                                                    });

        ASSERT_NE(nullptr, parser);

        const uint8_t data[] = {0x55, 0xaa,};

        uart_frame_parser_feed_data(parser, const_cast<uint8_t *>(data), sizeof data, nullptr);

        uart_frame_parser_release(parser);
    }
}

int main() {
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
