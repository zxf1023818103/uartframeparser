#ifndef UARTFRAMEPARSERWRAPPER_H
#define UARTFRAMEPARSERWRAPPER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QJsonDocument>
#include "uartframeparser.h"

class UartFrameParserWrapper : public QObject
{
    Q_OBJECT

public:
    explicit UartFrameParserWrapper(QObject *parent = nullptr);
    ~UartFrameParserWrapper();

    void loadJsonSchema(const QString& jsonSchema);

public slots:
    void feedData(const QByteArray& data);

signals:
    void errorOccurred(const QString& topic, const QString& filename, int line, const QString& message);

    void frameReceived(const QJsonDocument& frameData);

private:
    struct uart_frame_parser *m_parser;
};

#endif // UARTFRAMEPARSERWRAPPER_H
