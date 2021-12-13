#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
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

    void onSerialPortReadyRead();

    void on_actionExit_triggered();

    void on_actionSettings_triggered();

private:
    Ui::MainWindow *ui;

    SettingsDialog *m_settingsDialog;

    UartFrameParserWrapper *m_frameParser;

    QStandardItemModel *m_historyViewModel;

    QStandardItemModel *m_frameStructureViewModel;

    QStandardItemModel *m_attributeViewModel;
};
#endif // MAINWINDOW_H
