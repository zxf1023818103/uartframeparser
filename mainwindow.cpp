#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QApplication>
#include <QJsonObject>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_historyViewModel = new QStandardItemModel(ui->historyView);
    m_historyViewModel->setHorizontalHeaderLabels({ tr("Time"), tr("Direction"), tr("Hex Data") });
    ui->historyView->setModel(m_historyViewModel);

    m_frameStructureViewModel = new QStandardItemModel(ui->frameStructureView);
    m_frameStructureViewModel->setHorizontalHeaderLabels({ tr("Name"), tr("Hex Data"), tr("Decoded") });
    ui->frameStructureView->setModel(m_frameStructureViewModel);

    m_attributeViewModel = new QStandardItemModel(ui->attributeView);
    m_attributeViewModel->setHorizontalHeaderLabels({ tr("Attribute"), tr("Value") });
    ui->attributeView->setModel(m_attributeViewModel);

    m_settingsDialog = new SettingsDialog(this);
    m_frameParser = new UartFrameParserWrapper(this);

    connect(m_settingsDialog, &SettingsDialog::settingsSaved, this, &MainWindow::onSettingsSaved);
    connect(m_frameParser, &UartFrameParserWrapper::errorOccurred, m_settingsDialog, &SettingsDialog::appendLog);
    connect(m_frameParser, &UartFrameParserWrapper::frameReceived, this, &MainWindow::onFrameReceived);
    connect(ui->historyView, &QTreeView::activated, this, &MainWindow::onHistoryListViewActivated);
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
    m_historyViewModel->insertRow(0, item);
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
