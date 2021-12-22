#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QRegularExpressionValidator>
#include "settingsdialog.h"
#include "uartframeparserwrapper.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSettingsSaved(QSerialPort* serialPort, const QString& schema);

    void onFrameReceived(const QJsonDocument& frameData);

    void onHistoryListViewActivated(const QModelIndex &index);

    void onFrameStructureViewActivated(const QModelIndex &index);

    void onSendingDataItemChanged(QStandardItem* item);

    void onSerialPortReadyRead();

    void on_actionExit_triggered();

    void on_actionSettings_triggered();

    void on_addByteButton_clicked();

    void on_removeByteButton_clicked();

    void on_sendButton_clicked();

    void on_insertByteButton_clicked();

private:
    Ui::MainWindow *ui;

    SettingsDialog *m_settingsDialog;

    UartFrameParserWrapper *m_frameParser;

    QStandardItemModel *m_historyViewModel;

    QStandardItemModel *m_frameStructureViewModel;

    QStandardItemModel *m_attributeViewModel;

    QStandardItemModel *m_sendingDataViewModel;

    QRegularExpressionValidator *m_validator;
};
#endif // MAINWINDOW_H
