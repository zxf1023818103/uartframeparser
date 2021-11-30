#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include "uartframeparser.h"

static void uart_frame_detected_frame_release(struct uart_frame_detected_frame *detected_frame_head) {
    struct uart_frame_detected_frame *next;
    while (detected_frame_head) {
        next = detected_frame_head->next;
        free(detected_frame_head);
        detected_frame_head = next;
    }
}

static void uart_frame_bitfield_definition_release(struct uart_frame_bitfield_definition *bitfield_definition_head) {
    struct uart_frame_bitfield_definition *next;
    while (bitfield_definition_head) {
        next = bitfield_definition_head->next;
        free(bitfield_definition_head);
        bitfield_definition_head = next;
    }
}

static void uart_frame_field_definition_release(struct uart_frame_field_definition *field_definition_head) {
    struct uart_frame_field_definition *next;
    while (field_definition_head) {
        if (field_definition_head->has_subframes) {
            uart_frame_detected_frame_release(field_definition_head->detected_subframe_head);
        } else if (field_definition_head->has_bitfields) {
            uart_frame_bitfield_definition_release(field_definition_head->bitfield_definition_head);
        }

        next = field_definition_head->next;
        free(field_definition_head);
        field_definition_head = next;
    }
}

static void uart_frame_definition_release(struct uart_frame_definition *frame_definition_head) {
    struct uart_frame_definition *next;
    while (frame_definition_head) {
        uart_frame_field_definition_release(frame_definition_head->field_head);
        next = frame_definition_head->next;
        free(frame_definition_head);
        frame_definition_head = next;
    }
}

static struct uart_frame_detected_frame *
parse_detected_frame_node(cJSON *detected_frame_node, uart_frame_parser_error_callback_t on_error) {
    if (cJSON_IsString(detected_frame_node)) {
        struct uart_frame_detected_frame *detected_frame = calloc(1, sizeof(struct uart_frame_detected_frame));
        if (detected_frame) {
            detected_frame->name = detected_frame_node->valuestring;
            return detected_frame;
        } else {
            on_error(UART_FRAME_PARSER_ERROR_MALLOC, __FILE__, __LINE__, "cannot allocate a detected frame: %s",
                     cJSON_Print(detected_frame_node));
        }
    } else {
        on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__, "detected frames is not a string: %s",
                 cJSON_Print(detected_frame_node));
    }

    return NULL;
}

static struct uart_frame_detected_frame *
parse_detected_frames_node(cJSON *detected_frames_node, uart_frame_parser_error_callback_t on_error) {
    if (detected_frames_node) {
        if (cJSON_IsArray(detected_frames_node)) {
            cJSON *detected_frame_node = detected_frames_node->child;
            if (detected_frame_node) {
                struct uart_frame_detected_frame *cur_detected_frame = NULL, *detected_frame_head = NULL;
                while (detected_frame_node) {
                    struct uart_frame_detected_frame *detected_frame = parse_detected_frame_node(detected_frame_node,
                                                                                                 on_error);
                    if (detected_frame) {
                        if (cur_detected_frame) {
                            cur_detected_frame->next = detected_frame;
                            cur_detected_frame = cur_detected_frame->next;
                        } else {
                            detected_frame_head = cur_detected_frame = detected_frame;
                        }
                    } else {
                        uart_frame_detected_frame_release(detected_frame_head);
                        return NULL;
                    }

                    detected_frame_node = detected_frame_node->next;
                }

                return detected_frame_head;
            } else {
                on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__, "frames is empty: %s",
                         cJSON_Print(detected_frames_node));
            }
        } else {
            on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__, "frames is not array: %s",
                     cJSON_Print(detected_frames_node));
        }
    } else {
        on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__, "frames is null");
    }

    return NULL;
}

static struct uart_frame_bitfield_definition *
parse_frame_bitfield_node(cJSON *bitfield_node, uint32_t offset_bits,
                          struct uart_frame_parser_expression_engine *expression_engine, uart_frame_parser_error_callback_t on_error) {
    if (cJSON_IsObject(bitfield_node)) {
        cJSON *bitfield_attribute_node = bitfield_node->child;
        char *name = NULL;
        char *description = NULL;
        struct uart_frame_parser_expression *default_expression = NULL;
        uint32_t bits = 0;
        while (bitfield_attribute_node) {
            if (strcmp(bitfield_attribute_node->string, "name") == 0) {
                if (cJSON_IsString(bitfield_attribute_node)) {
                    name = bitfield_attribute_node->valuestring;
                } else {
                    on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__,
                             "bitfield name is not a string: %s", cJSON_Print(bitfield_attribute_node));
                }
            } else if (strcmp(bitfield_attribute_node->string, "description") == 0) {
                if (cJSON_IsString(bitfield_attribute_node)) {
                    description = bitfield_attribute_node->valuestring;
                } else {
                    on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__,
                             "bitfield description is not a string: %s", cJSON_Print(bitfield_attribute_node));
                }
            } else if (strcmp(bitfield_attribute_node->string, "default") == 0) {
                if (cJSON_IsString(bitfield_attribute_node)) {
                    default_expression = uart_frame_parser_expression_create(expression_engine, NULL,
                                                                             EXPRESSION_DEFAULT_VALUE,
                                                                             bitfield_attribute_node->valuestring,
                                                                             NULL);
                    if (!default_expression) {
                        on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__,
                                 "bitfield default expression is invalid: %s", bitfield_attribute_node->valuestring);
                        return NULL;
                    }
                } else {
                    on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__,
                             "bitfield default is not a string: %s", cJSON_Print(bitfield_attribute_node));
                }
            } else if (strcmp(bitfield_attribute_node->string, "bits") == 0) {
                if (cJSON_IsNumber(bitfield_attribute_node)) {
                    bits = bitfield_attribute_node->valueint;
                } else {
                    on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__,
                             "bitfield bits is not an integer: %s", cJSON_Print(bitfield_attribute_node));
                }
            }

            bitfield_attribute_node = bitfield_attribute_node->next;
        }

        if (bits) {
            struct uart_frame_bitfield_definition* bitfield_definition = calloc(1,
                sizeof(struct uart_frame_bitfield_definition));
            if (bitfield_definition) {
                bitfield_definition->name = name;
                bitfield_definition->description = description;
                bitfield_definition->offset_bits = offset_bits;
                bitfield_definition->bits = bits;

                return bitfield_definition;
            }
            else {
                on_error(UART_FRAME_PARSER_ERROR_MALLOC, __FILE__, __LINE__, "cannot allocate a bitfield definition");
            }
        }
        else {
            on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__, "bitfield length is 0");
        }
    } else {
        on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__, "bitfield is not an object: %s",
                 cJSON_Print(bitfield_node));
    }

    return NULL;
}

static struct uart_frame_bitfield_definition *
parse_frame_bitfields_node(cJSON *bitfields_node, struct uart_frame_parser_expression_engine *expression_engine,
                           uart_frame_parser_error_callback_t on_error) {
    if (cJSON_IsArray(bitfields_node)) {
        cJSON *bitfield_node = bitfields_node->child;
        if (bitfield_node) {
            struct uart_frame_bitfield_definition *bitfield_definition_head = NULL;
            struct uart_frame_bitfield_definition *bitfield_definition_cur = NULL;
            uint32_t offset_bits = 0;
            while (bitfield_node) {
                struct uart_frame_bitfield_definition *bitfield_definition = parse_frame_bitfield_node(bitfield_node, offset_bits,
                                                                                                       expression_engine, on_error);
                if (bitfield_definition) {
                    if (bitfield_definition_cur) {
                        bitfield_definition_cur->next = bitfield_definition;
                        bitfield_definition_cur = bitfield_definition_cur->next;
                    } else {
                        bitfield_definition_head = bitfield_definition_cur = bitfield_definition;
                    }

                    offset_bits += bitfield_definition->bits;
                } else {
                    uart_frame_bitfield_definition_release(bitfield_definition_head);
                }
                bitfield_node = bitfield_node->next;
            }

            return bitfield_definition_head;
        } else {
            on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__, "bitfields is empty: %s",
                     cJSON_Print(bitfields_node));
        }
    } else {
        on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__, "bitfields is not an array: %s",
                 cJSON_Print(bitfields_node));
    }

    return NULL;
}

static struct uart_frame_field_definition *
parse_frame_field_node(cJSON *field_node, struct uart_frame_parser_expression_engine *expression_engine,
                       uart_frame_parser_error_callback_t on_error) {
    if (cJSON_IsObject(field_node)) {
        cJSON *bitfields_node = NULL;
        cJSON *subframes_node = NULL;
        struct uart_frame_parser_expression *tostring_expression = NULL;
        char *name = NULL;
        char *description = NULL;
        struct uart_frame_parser_expression *length_expression = NULL;
        uint32_t length_value = 0;
        struct uart_frame_parser_expression *default_value_expression = NULL;

        cJSON *field_attribute_node = field_node->child;
        while (field_attribute_node) {
            if (strcmp("name", field_attribute_node->string) == 0) {
                if (cJSON_IsString(field_attribute_node)) {
                    name = field_attribute_node->valuestring;
                } else {
                    on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__, "field name is not a string: %s",
                             cJSON_Print(field_attribute_node));
                    goto err;
                }
            } else if (strcmp("description", field_attribute_node->string) == 0) {
                if (cJSON_IsString(field_attribute_node)) {
                    description = field_attribute_node->valuestring;
                } else {
                    on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__,
                             "field description is not a string: %s", cJSON_Print(field_attribute_node));
                    goto err;
                }
            } else if (strcmp("bytes", field_attribute_node->string) == 0) {
                if (cJSON_IsString(field_attribute_node)) {
                    if (length_expression == NULL) {
                        length_expression = uart_frame_parser_expression_create(expression_engine, NULL,
                                                                                EXPRESSION_LENGTH,
                                                                                field_attribute_node->valuestring,
                                                                                NULL);
                        if (!length_expression) {
                            on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__,
                                     "field length expression is invalid: %s",
                                     field_attribute_node->valuestring);
                            goto err;
                        }
                    } else {
                        on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__,
                                 "duplicated field byte node: %s",
                                 cJSON_Print(field_attribute_node));
                    }
                } else if (cJSON_IsNumber(field_attribute_node)) {
                    length_value = field_attribute_node->valueint;
                } else {
                    on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__,
                             "field description is not a string or number: %s", cJSON_Print(field_attribute_node));
                    goto err;
                }
            } else if (strcmp("default", field_attribute_node->string) == 0) {
                if (cJSON_IsString(field_attribute_node)) {
                    default_value_expression = uart_frame_parser_expression_create(expression_engine,
                                                                                   NULL, EXPRESSION_DEFAULT_VALUE,
                                                                                   field_attribute_node->valuestring,
                                                                                   NULL);
                    if (!default_value_expression) {
                        on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__,
                                 "field default expression is invalid: %s", field_attribute_node->valuestring);
                        goto err;
                    }
                } else {
                    on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__,
                             "field default is not a string: %s", cJSON_Print(field_attribute_node));
                    goto err;
                }
            } else if (strcmp("frames", field_attribute_node->string) == 0) {
                subframes_node = field_attribute_node;
            } else if (strcmp("bitfields", field_attribute_node->string) == 0) {
                bitfields_node = field_attribute_node;
            } else if (strcmp("tostring", field_attribute_node->string) == 0) {
                if (cJSON_IsString(field_attribute_node)) {
                    if (tostring_expression == NULL) {
                        tostring_expression = uart_frame_parser_expression_create(expression_engine,
                                                                                  NULL,
                                                                                  EXPRESSION_TOSTRING,
                                                                                  field_attribute_node->valuestring,
                                                                                  NULL);
                        if (!tostring_expression) {
                            on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__,
                                     "field tostring expression is invalid: %s",
                                     field_attribute_node->valuestring);
                            goto err;
                        }
                    } else {
                        on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__,
                                 "duplicated field tostring node: %s",
                                 cJSON_Print(field_attribute_node));
                    }
                } else {
                    on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__,
                             "field tostring is not a string: %s", cJSON_Print(field_attribute_node));
                    goto err;
                }
            } else {
                on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__,
                         "unknown field definition attribute: %s", cJSON_Print(field_attribute_node));
                goto err;
            }

            field_attribute_node = field_attribute_node->next;
        }

        if (!length_expression && !length_value) {
            on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__, "field attribute bytes not found: %s",
                     cJSON_Print(field_node));
            goto err;
        }

        if ((bitfields_node || subframes_node) && (tostring_expression || default_value_expression)) {
            on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__,
                     "exclusive field attributes tostring/default and bitfields/subframes: %s",
                     cJSON_Print(field_node));
            goto err;
        }

        if (bitfields_node && subframes_node) {
            on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__,
                     "exclusive field attribute bitfields and frames: %s", cJSON_Print(field_node));
            goto err;
        }

        struct uart_frame_field_definition field_definition = {
                .name = name,
                .description = description,
        };

        if (length_expression) {
            field_definition.has_length_expression = 1;
            field_definition.length.expression = length_expression;
        } else {
            field_definition.length.value = length_value;
        }

        if (bitfields_node) {
            struct uart_frame_bitfield_definition *bitfield_definition_head = parse_frame_bitfields_node(bitfields_node,
                                                                                                         expression_engine,
                                                                                                         on_error);
            if (bitfield_definition_head) {
                field_definition.has_bitfields = 1;
                field_definition.bitfield_definition_head = bitfield_definition_head;
            } else {
                goto err;
            }
        } else if (subframes_node) {
            struct uart_frame_detected_frame *detected_subframe_head = parse_detected_frames_node(subframes_node,
                                                                                                  on_error);
            if (detected_subframe_head) {
                field_definition.has_subframes = 1;
                field_definition.detected_subframe_head = detected_subframe_head;
            } else {
                goto err;
            }
        } else {
            field_definition.default_value_expression = default_value_expression;
            field_definition.tostring_expression = tostring_expression;
        }

        struct uart_frame_field_definition *ptr_field_definition = malloc(sizeof field_definition);
        *ptr_field_definition = field_definition;
        return ptr_field_definition;

        err:
        uart_frame_parser_expression_release(expression_engine, tostring_expression);
        uart_frame_parser_expression_release(expression_engine, default_value_expression);
        uart_frame_parser_expression_release(expression_engine, length_expression);
    } else {
        on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__, "field is not an object: %s",
                 cJSON_Print(field_node));
    }

    return NULL;
}

static struct uart_frame_field_definition *
parse_frame_fields_node(cJSON *fields_node, struct uart_frame_parser_expression_engine *expression_engine,
                        uart_frame_parser_error_callback_t on_error) {
    if (cJSON_IsArray(fields_node)) {
        cJSON *field_node = fields_node->child;
        if (field_node) {
            struct uart_frame_field_definition *cur_field_definition = NULL, *field_definition_head = NULL;
            while (field_node) {
                struct uart_frame_field_definition *field_definition = parse_frame_field_node(field_node,
                                                                                              expression_engine,
                                                                                              on_error);
                if (field_definition) {
                    if (cur_field_definition) {
                        cur_field_definition->next = field_definition;
                        cur_field_definition = cur_field_definition->next;
                    } else {
                        field_definition_head = cur_field_definition = field_definition;
                    }
                } else {
                    uart_frame_field_definition_release(field_definition_head);
                    return NULL;
                }
                field_node = field_node->next;
            }

            return field_definition_head;
        } else {
            on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__, "frame fields is empty: %s",
                     cJSON_Print(fields_node));
        }
    } else {
        on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__, "frame fields is not an array: %s",
                 cJSON_Print(fields_node));
    }

    return NULL;
}

static struct uart_frame_definition *
parse_definition_node(cJSON *definition_node, struct uart_frame_parser_expression_engine *expression_engine,
                      uart_frame_parser_error_callback_t on_error) {
    if (cJSON_IsObject(definition_node)) {
        cJSON *definition_attribute_node = definition_node->child;
        if (definition_attribute_node) {
            char *name = NULL;
            char *description = NULL;
            struct uart_frame_parser_expression *validator_expression = NULL;
            cJSON *fields_node = NULL;

            while (definition_attribute_node) {
                if (strcmp("name", definition_attribute_node->string) == 0) {
                    if (cJSON_IsString(definition_attribute_node)) {
                        name = definition_attribute_node->valuestring;
                    } else if (!cJSON_IsNull(definition_attribute_node)) {
                        on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__,
                                 "frame definition name is not a string: %s", cJSON_Print(definition_attribute_node));
                    }
                } else if (strcmp("description", definition_attribute_node->string) == 0) {
                    if (cJSON_IsString(definition_attribute_node)) {
                        description = definition_attribute_node->valuestring;
                    } else if (!cJSON_IsNull(definition_attribute_node)) {
                        on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__,
                                 "frame description is not a string: %s", cJSON_Print(definition_attribute_node));
                    }
                } else if (strcmp("validator", definition_attribute_node->string) == 0) {
                    if (cJSON_IsString(definition_attribute_node)) {
                        validator_expression = uart_frame_parser_expression_create(expression_engine,
                                                                                   NULL,
                                                                                   EXPRESSION_VALIDATOR,
                                                                                   definition_attribute_node->valuestring,
                                                                                   NULL);
                    } else if (!cJSON_IsNull(definition_attribute_node)) {
                        on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__,
                                 "frame validator is not a string: %s", cJSON_Print(definition_attribute_node));
                    }
                } else if (strcmp("fields", definition_attribute_node->string) == 0) {
                    fields_node = definition_attribute_node;
                } else {
                    on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__,
                             "unknown field definition attributes: %s", cJSON_Print(definition_attribute_node));
                }

                definition_attribute_node = definition_attribute_node->next;
            }

            if (name) {
                struct uart_frame_field_definition *field_head = parse_frame_fields_node(fields_node, expression_engine,
                                                                                         on_error);
                if (field_head) {
                    struct uart_frame_definition *frame_definition = calloc(1, sizeof(struct uart_frame_definition));
                    if (frame_definition) {
                        frame_definition->name = name;
                        frame_definition->description = description;
                        frame_definition->validator_expression = validator_expression;
                        frame_definition->field_head = field_head;
                        return frame_definition;
                    } else {
                        on_error(UART_FRAME_PARSER_ERROR_MALLOC, __FILE__, __LINE__,
                                 "cannot allocate a frame definition");
                    }
                }
            } else {
                on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__, "frame definition name is null: %s",
                         cJSON_Print(definition_node));
            }
        } else {
            on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__, "frame definition is empty: %s",
                     cJSON_Print(definition_node));
        }
    } else {
        on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__, "definition is not an object: %s",
                 cJSON_Print(definition_node));
    }

    return NULL;
}

static struct uart_frame_definition *
parse_definitions_node(cJSON *definitions_node, struct uart_frame_parser_expression_engine *expression_engine,
                       uart_frame_parser_error_callback_t on_error) {
    if (definitions_node) {
        if (cJSON_IsArray(definitions_node)) {
            cJSON *definition_node = definitions_node->child;
            if (definition_node) {
                struct uart_frame_definition *cur_frame_definition = NULL, *frame_definition_head = NULL;
                while (definition_node) {
                    struct uart_frame_definition *frame_definition = parse_definition_node(definition_node,
                                                                                           expression_engine, on_error);
                    if (frame_definition) {
                        if (cur_frame_definition) {
                            cur_frame_definition->next = frame_definition;
                            cur_frame_definition = cur_frame_definition->next;
                        } else {
                            frame_definition_head = cur_frame_definition = frame_definition;
                        }
                    } else {
                        uart_frame_definition_release(frame_definition_head);
                        return NULL;
                    }

                    definition_node = definition_node->next;
                }

                return frame_definition_head;
            } else {
                on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__, "definitions is empty: %s",
                         cJSON_Print(definitions_node));
            }
        } else {
            on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__, "definitions is not an array: %s",
                     cJSON_Print(definitions_node));
        }
    } else {
        on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__, "definitions is null: %s",
                 cJSON_Print(definitions_node));
    }

    return NULL;
}

static struct uart_frame_parser *parse_json_config(cJSON *config, uart_frame_parser_error_callback_t on_error,
                                                   uart_frame_parser_data_callback_t on_data) {
    if (cJSON_IsObject(config)) {
        config = config->child;

        cJSON *definitions_node = NULL;
        cJSON *detected_frames_node = NULL;
        cJSON *init_node = NULL;
        while (config) {
            if (strcmp("definitions", config->string) == 0) {
                definitions_node = config;
            } else if (strcmp("frames", config->string) == 0) {
                detected_frames_node = config;
            } else if (strcmp("init", config->string) == 0) {
                init_node = config;
            }

            config = config->next;
        }

        if (init_node && !cJSON_IsString(init_node)) {
            on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__, "init node is not a string: %s",
                     cJSON_Print(init_node));
            return NULL;
        }

        struct uart_frame_detected_frame *detected_frame_head = parse_detected_frames_node(detected_frames_node,
                                                                                           on_error);
        if (detected_frame_head) {
            struct uart_frame_parser_buffer *buffer = uart_frame_parser_buffer_create(on_error, NULL);
            if (buffer) {
                struct uart_frame_parser_expression_engine *expression_engine = uart_frame_parser_expression_engine_create(
                        buffer, init_node ? init_node->valuestring : NULL, on_error, NULL);
                if (expression_engine) {
                    struct uart_frame_definition *frame_definition_head = parse_definitions_node(definitions_node,
                                                                                                 expression_engine,
                                                                                                 on_error);
                    if (frame_definition_head) {
                        struct uart_frame_parser *parser = calloc(1, sizeof(struct uart_frame_parser));
                        if (parser) {
                            parser->detected_frame_head = detected_frame_head;
                            parser->frame_definition_head = frame_definition_head;
                            parser->on_error = on_error;
                            parser->on_data = on_data;
                            parser->buffer = buffer;
                            parser->expression_engine = expression_engine;

                            return parser;
                        } else {
                            on_error(UART_FRAME_PARSER_ERROR_MALLOC, __FILE__, __LINE__, "cannot allocate a parser");
                            uart_frame_definition_release(frame_definition_head);
                            err3:
                            uart_frame_parser_expression_engine_release(expression_engine);
                            err2:
                            uart_frame_parser_buffer_release(buffer);
                            err1:
                            uart_frame_detected_frame_release(detected_frame_head);
                        }
                    } else {
                        goto err3;
                    }
                } else {
                    goto err2;
                }
            } else {
                goto err1;
            }
        }
    } else {
        on_error(UART_FRAME_PARSER_ERROR_PARSE_CONFIG, __FILE__, __LINE__, "config is not an object: %s",
                 cJSON_Print(config));
    }

    return NULL;
}

struct uart_frame_parser *uart_frame_parser_create(const char *json_config, uint32_t json_config_size,
                                                   uart_frame_parser_error_callback_t on_error,
                                                   uart_frame_parser_data_callback_t on_data) {
    const char *json_config_end = json_config + json_config_size;
    cJSON *cjson = cJSON_ParseWithOpts(json_config, &json_config_end, 0);
    if (cjson) {
        return parse_json_config(cjson, on_error, on_data);
    } else {
        on_error(UART_FRAME_PARSER_ERROR_CJSON, __FILE__, __LINE__, cJSON_GetErrorPtr());
        return NULL;
    }
}

void uart_frame_parser_release(struct uart_frame_parser *parser) {
    uart_frame_parser_buffer_release(parser->buffer);
    uart_frame_parser_expression_engine_release(parser->expression_engine);
    uart_frame_detected_frame_release(parser->detected_frame_head);
    uart_frame_definition_release(parser->frame_definition_head);
    free(parser);
}
