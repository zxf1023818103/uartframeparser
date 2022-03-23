#ifndef LOGTEXTEDIT_H
#define LOGTEXTEDIT_H

#include <QPlainTextEdit>

class LogTextEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit LogTextEdit(QWidget *parent = nullptr);
    ~LogTextEdit();

public slots:
    void appendMessage(const QString& text);
};

#endif // LOGTEXTEDIT_H
