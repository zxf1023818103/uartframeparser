#include "uartframeparserwrapper.h"
#include <cstdarg>
#include <cstdio>

UartFrameParserWrapper::UartFrameParserWrapper(QObject *parent)
    : QObject{parent}
{
    m_parser = nullptr;
}

UartFrameParserWrapper::~UartFrameParserWrapper()
{
    if (m_parser) {
        uart_frame_parser_release(m_parser);
        m_parser = nullptr;
    }
}

namespace {

void onError(void* user_ptr, enum uart_frame_parser_error_types error_type, const char* file, int line, const char* fmt, ...) {
    UartFrameParserWrapper *parser = static_cast<UartFrameParserWrapper*>(user_ptr);

    QString topic;
    switch(error_type) {
    case UART_FRAME_PARSER_ERROR_CJSON:
        topic = "cJSON";
        break;
    case UART_FRAME_PARSER_ERROR_MALLOC:
        topic = "Memory";
        break;
    case UART_FRAME_PARSER_ERROR_LUA:
        topic = "Lua";
        break;
    case UART_FRAME_PARSER_ERROR_PARSE_CONFIG:
        topic = "Config";
        break;
    default:
        topic = QString("UnknownTopic%1").arg((int)error_type);
        break;
    }

    std::va_list ap1, ap2;
    va_start(ap1, fmt);
    va_copy(ap2, ap1);

    QByteArray message;
    message.fill(0, std::vsnprintf(nullptr, 0, fmt, ap1));
    std::vsprintf(message.data(), fmt, ap2);
    va_end(ap1);
    va_end(ap2);

    emit parser->errorOccurred(topic, file, line, message);
}

void onData(void* buffer, struct uart_frame_definition* frame_definition, uint32_t frame_bytes, struct uart_frame_field_info* field_info_head,
            void* user_ptr) {
    UartFrameParserWrapper *parser = static_cast<UartFrameParserWrapper*>(user_ptr);
    struct uart_frame_field_data* field_data_head = uart_frame_parser_read_concerned_fields(buffer, field_info_head, NULL, onError, user_ptr);
    uart_frame_parser_eval_tostring_expression(field_info_head);
    const char* json = uart_frame_parser_jsonify_frame_data(buffer, frame_definition, frame_bytes, field_data_head);
    QJsonDocument frameData = QJsonDocument::fromJson(json);
    free((void*)json);
    uart_frame_parser_field_data_release(field_data_head);
    emit parser->frameReceived(frameData);
}

}

void UartFrameParserWrapper::loadJsonSchema(const QString &jsonSchema)
{
    if (m_parser) {
        uart_frame_parser_release(m_parser);
        m_parser = nullptr;
    }

    const QByteArray& json = jsonSchema.toLatin1();
    m_parser = uart_frame_parser_create(json.data(), json.size(), onError, onData, this);
}

void UartFrameParserWrapper::feedData(const QByteArray &data)
{
    if (m_parser) {
        uart_frame_parser_feed_data(m_parser, (const uint8_t*)data.data(), data.size());
    }
}
