#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include "uartframeparser.h"

static int parse_json_config(struct uart_frame_parser* parser, uint32_t *ptr_parser_size, cJSON *cjson)
{
	
}

void uart_frame_parser_init(struct uart_frame_parser** ptr_parser, const char* json_config, uint32_t json_config_size, uart_frame_parser_error_callback_t on_error)
{
	char* json_config_end = json_config + json_config_size;
	cJSON* cjson = cJSON_ParseWithOpts(json_config, &json_config_end, 0);
	if (cjson) {
		if (cjson->child) {
			cjson = cjson->child;

			cJSON* frames_json_array = NULL;
			cJSON* types_json_array = NULL;
			for (cJSON* cur = cjson; cur != NULL; cur = cur->next) {
				if (strcmp("frames", cur->string) == 0) {
					if (cur->type & cJSON_Array) {
						frames_json_array = cur->child;
					}
					else {
						on_error(UART_FRAME_PARSER_ERROR_CONFIG, "frames is not array");
						return;
					}
				}
				else if (strcmp("types", cur->string) == 0) {
					if (cur->type & cJSON_Array) {
						types_json_array = cur->child;
					}
					else {
						on_error(UART_FRAME_PARSER_ERROR_CONFIG, "types is not array");
						return;
					}
				}
			}

			for (cJSON* cur = types_json_array; cur != NULL; cur = cur->next) {

			}
		}
		else {
			/// 找不到子节点
		}
	}
	else {
		on_error(UART_FRAME_PARSER_ERROR_CJSON, cJSON_GetErrorPtr());
	}
}
