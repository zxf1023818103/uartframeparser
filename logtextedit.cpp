#include "logtextedit.h"
#include <QScrollBar>

LogTextEdit::LogTextEdit(QWidget *parent)
{

}

LogTextEdit::~LogTextEdit()
{

}

void LogTextEdit::appendMessage(const QString &text)
{
    this->appendPlainText(text);
    this->verticalScrollBar()->setValue(this->verticalScrollBar()->maximum());
}
