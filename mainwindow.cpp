#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QApplication>
#include <QJsonObject>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_model = new QStandardItemModel(this);
    ui->historyListView->setModel(m_model);
    m_settingsDialog = new SettingsDialog(this);
    m_frameParser = new UartFrameParserWrapper(this);
    connect(m_settingsDialog, &SettingsDialog::settingsSaved, this, &MainWindow::onSettingsSaved);
    connect(m_frameParser, &UartFrameParserWrapper::errorOccurred, m_settingsDialog, &SettingsDialog::appendLog);
    connect(m_frameParser, &UartFrameParserWrapper::frameReceived, this, &MainWindow::onFrameReceived);
    connect(ui->historyListView, &QListView::activated, this, &MainWindow::onHistoryListViewActivated);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onSettingsSaved(QSerialPort *serialPort, const QString& schema)
{
    connect(serialPort, &QSerialPort::readyRead, this, &MainWindow::onSerialPortReadyRead, Qt::UniqueConnection);
    m_frameParser->loadJsonSchema(schema);
}

void MainWindow::onFrameReceived(const QJsonDocument &frameData)
{
    QStandardItem* item = new QStandardItem(frameData.object().value("hex").toString());
    item->setData(frameData);
    m_model->insertRow(0, item);
}

void MainWindow::onHistoryListViewActivated(const QModelIndex &index)
{
    const QJsonObject& frameData = index.data().toJsonDocument().object();

}



void MainWindow::onSerialPortReadyRead()
{
    m_frameParser->feedData(m_settingsDialog->serialPort()->readAll());
}

void MainWindow::on_actionExit_triggered()
{
    QApplication::quit();
}

void MainWindow::on_actionSettings_triggered()
{
    m_settingsDialog->hide();
    m_settingsDialog->show();
}
