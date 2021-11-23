#include <gtest/gtest.h>
#include "uartframeparser.h"
#include <string>
#include <cjson/cJSON.h>

extern "C" {

#define bitset_size(bits)         ((bits) / 8 + ((bits) % 8 ? 1 : 0))
#define bitset_set(bitset, bit)   ((bitset)[(bit) / 8] |=  (1 << ((bit) % 8)))
#define bitset_get(bitset, bit)   ((bitset)[(bit) / 8] &   (1 << ((bit) % 8)))
#define bitset_unset(bitset, bit) ((bitset)[(bit) / 8] &= ~(1 << ((bit) % 8)))

static cJSON *stringify_bitfield_data(const uint8_t *data, struct uart_frame_bitfield_definition *bitfield_definition, uint32_t offset_bits, uint32_t offset) {
    uint32_t bitfield_bytes = bitset_size(bitfield_definition->bits);
    char *hex_data = (char*)calloc(bitfield_bytes * 2 + 1, 1);
    uint8_t *bitfield_data = (uint8_t*)hex_data + bitfield_bytes + 1;

    uint32_t cur_bit = 0;
    for (uint32_t i = offset_bits; i < offset_bits + bitfield_definition->bits; i++) {
        if (bitset_get(data, i)) {
            bitset_set(bitfield_data, cur_bit);
        }
        else {
            bitset_unset(bitfield_data, cur_bit);
        }
        cur_bit++;
    }

    for (uint32_t i = 0; i < bitfield_bytes; i++) {
        sprintf(hex_data + (i * 2), "%02x", bitfield_data[i]);
    }

    cJSON *bitfield_data_node = cJSON_CreateObject();
    cJSON_AddStringToObject(bitfield_data_node, "name", bitfield_definition->name);
    cJSON_AddStringToObject(bitfield_data_node, "description", bitfield_definition->description);
    cJSON_AddNumberToObject(bitfield_data_node, "bits", bitfield_definition->bits);
    cJSON_AddStringToObject(bitfield_data_node, "hex", hex_data);

    free(hex_data);
    return bitfield_data_node;
}

static cJSON *stringify_bitfields_data(void *buffer, struct uart_frame_bitfield_definition *bitfield_definition_head, uint32_t data_size, uint32_t offset) {
    uint32_t bits = 0;
    cJSON *array = cJSON_CreateArray();

    uint8_t *data = (uint8_t*)malloc(data_size);
    uart_frame_parser_buffer_read(buffer, offset, data, data_size);

    while (bitfield_definition_head) {

        if (bits + bitfield_definition_head->bits > data_size * 8) {
            return array;
        }

        cJSON *bitfield_data_node = stringify_bitfield_data(data, bitfield_definition_head, bits, offset);
        cJSON_AddItemToArray(array, bitfield_data_node);
        bits += bitfield_definition_head->bits;
        bitfield_definition_head = bitfield_definition_head->next;
    }

    free(data);

    return array;
}

static cJSON *stringify_binary_data(void *buffer, uint32_t data_size, uint32_t offset) {
    char *hex_data = (char*)malloc(data_size * 2 + 1);
    uint8_t *data = (uint8_t*)hex_data + data_size + 1;
    uart_frame_parser_buffer_read(buffer, offset, data, data_size);

    for (uint32_t i = 0; i < data_size; i++) {
        sprintf(hex_data + (i * 2), "%02x", data[i]);
    }

    cJSON *hex_data_node = cJSON_CreateString(hex_data);
    free(hex_data);
    return hex_data_node;
}

static cJSON *stringify_frame_data(void *buffer, struct uart_frame_definition *frame_definition, struct uart_frame_field_data *field_data_head, uint32_t offset) {
    cJSON *frame_data = cJSON_CreateObject();
    cJSON_AddStringToObject(frame_data, "name", frame_definition->name);
    cJSON_AddStringToObject(frame_data, "description", frame_definition->description);
    cJSON *data = cJSON_CreateArray();

    uint32_t field_offset = offset;
    for (struct uart_frame_field_data *field_data = field_data_head;
         field_data != NULL; field_data = field_data->next) {
        cJSON *field_data_node = cJSON_CreateObject();

        cJSON_AddStringToObject(field_data_node, "name", field_data->field_definition->name);
        cJSON_AddStringToObject(field_data_node, "description", field_data->field_definition->description);
        cJSON_AddNumberToObject(field_data_node, "length", field_data->data_size);

        if (field_data->field_definition->has_bitfields) {
            cJSON *bitfields_data = stringify_bitfields_data(buffer, field_data->field_definition->bitfield_definition_head, field_data->data_size, field_offset);
            cJSON_AddItemToObject(bitfields_data, "bitfields",  bitfields_data);
        }
        else if (field_data->field_definition->has_subframes) {
            cJSON *subframe_data = stringify_frame_data(buffer, field_data->subframe_definition, field_data->subframe_field_data, field_offset);
            cJSON_AddItemToObject(field_data_node, "subframe", subframe_data);
        }
        else {
            cJSON *hex_data = stringify_binary_data(buffer, field_data->data_size, field_offset);
            cJSON_AddItemToObject(field_data_node, "hex", hex_data);
        }

        cJSON_AddItemToArray(data, field_data_node);

        field_offset += field_data->data_size;
    }

    cJSON_AddItemToObject(frame_data, "data", data);

    return frame_data;
}

}

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
                    "default": "return '\\x55\\xaa'",
                    "description": "帧头"
                },
                {
                    "bytes": 1,
                    "name": "length",
                    "description": "长度"
                },
                {
                    "bytes": "return byte(3) - 4",
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
                    "name": "sof",
                    "description": "帧头",
                    "default": "return '\\x01'"
                },
                {
                    "bytes": 1,
                    "name": "working_status",
                    "description": "工作状态"
                }
            ],
            "validator": "return byte(1) == 0x01"
        },
        {
            "name": "subframe2",
            "description": "子帧格式2",
            "fields": [
                {
                    "bytes": 1,
                    "name": "sof",
                    "description": "帧头",
                    "default": "return '\\x02'"
                },
                {
                    "bytes": 1,
                    "name": "fan_status",
                    "description": "风机状态"
                }
            ],
            "validator": "return byte(1) == 0x02"
        }
    ],
    "frames": [
        "status_report"
    ],
    "init": "function sum(from, to) result = 0; for i = from, to, 1 do result = result + byte(i) end; return result; end"
})_";

    void on_error(enum uart_frame_parser_error_types error_type, const char *file, int line, const char *fmt, ...) {

        const char *error_type_name[] = {"", "cJSON", "malloc", "Config Parse", "Lua"};

        const char *log_template = "%s:%d:%s: %s";
        char *format = static_cast<char *>(std::calloc(
                std::snprintf(nullptr, 0, log_template, file, line, error_type_name[error_type], fmt) + 1,
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

    void on_data(void *buffer, struct uart_frame_definition *frame_definition, struct uart_frame_field_data *field_data_head,
                void *user_ptr) {
        *(int *) user_ptr = 1;

        cJSON *data = stringify_frame_data(buffer, frame_definition, field_data_head, 0);

        const char *result = cJSON_Print(data);

        GTEST_LOG_(INFO) << result;

        cJSON_free(data);
    }

    TEST(Example1, ConfigLoadTest) {

        struct uart_frame_parser *parser = uart_frame_parser_create(json, strlen(json), on_error, on_data);

        ASSERT_NE(nullptr, parser);

        uart_frame_parser_release(parser);
    }

    TEST(Example1, FrameParseTest) {

        struct uart_frame_parser *parser = uart_frame_parser_create(json, strlen(json), on_error, on_data);

        ASSERT_NE(nullptr, parser);

        const uint8_t data[] = {0x55, 0xaa, 0x07, 0x01, 0x01, 0x02, 0x0a,
                                0x55, 0xaa, 0x07, 0x02, 0x01, 0x02, 0x0b};

        int success = 0;
        uart_frame_parser_feed_data(parser, const_cast<uint8_t *>(data), sizeof data, &success);

        uart_frame_parser_release(parser);

        ASSERT_EQ(success, 1);
    }
}

int main() {
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
