#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QApplication>
#include <QJsonObject>
#include <QJsonArray>

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

    connect(m_settingsDialog, SIGNAL(settingsSaved(QSerialPort*, QString)), this, SLOT(onSettingsSaved(QSerialPort*, QString)));
    connect(m_frameParser, SIGNAL(errorOccurred(QString, QString, int, QString)), m_settingsDialog, SLOT(appendLog(QString, QString, int, QString)));
    connect(m_frameParser, SIGNAL(frameReceived(QJsonDocument)), this, SLOT(onFrameReceived(QJsonDocument)));
    connect(ui->historyView, SIGNAL(activated(QModelIndex)), this, SLOT(onHistoryListViewActivated(QModelIndex)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onSettingsSaved(QSerialPort *serialPort, const QString& schema)
{
    connect(serialPort, SIGNAL(readyRead()), this, SLOT(onSerialPortReadyRead()), Qt::UniqueConnection);
    m_frameParser->loadJsonSchema(schema);
}

void MainWindow::onFrameReceived(const QJsonDocument &frameData)
{
    const QJsonObject& frameObject = frameData.object();
    QStandardItem* dateItem = new QStandardItem(QDateTime::currentDateTime().toString());
    dateItem->setData(frameData);
    QStandardItem* directionItem = new QStandardItem(tr("Receive"));
    QStandardItem* hexDataItem = new QStandardItem(frameObject.value("hex").toString());
    m_historyViewModel->appendRow({ dateItem, directionItem , hexDataItem });
}

namespace {

QList<QStandardItem*> buildFrameStructureItems(const QJsonObject& frameJsonObject);

QList<QStandardItem*> buildBitfieldStructureItems(const QJsonObject& bitfieldJsonObject) {
    QStandardItem* nameItem = new QStandardItem(bitfieldJsonObject["name"].toString());
    QStandardItem* hexDataItem = new QStandardItem(bitfieldJsonObject["hex"].toString());
    QStandardItem* decodedStringItem = new QStandardItem(bitfieldJsonObject["text"].toString());
    return { nameItem, hexDataItem, decodedStringItem };
}

QList<QStandardItem*> buildFieldStructureItems(const QJsonObject& fieldJsonObejct) {
    QStandardItem* nameItem = new QStandardItem(fieldJsonObejct["name"].toString());
    QStandardItem* hexDataItem = new QStandardItem(fieldJsonObejct["hex"].toString());
    QStandardItem* decodedStringItem = new QStandardItem(fieldJsonObejct["text"].toString());

    const QJsonObject::const_iterator& subframeIter = fieldJsonObejct.find("subframe");
    if (subframeIter != fieldJsonObejct.end()) {
        const QList<QStandardItem*>& subframeData = buildFrameStructureItems(subframeIter->toObject());
        for (qsizetype i = 0; i < subframeData.size(); i++) {
            nameItem->setChild(0, i, subframeData[i]);
        }
    }
    else {
        const QJsonObject::const_iterator& bitfieldsIter = fieldJsonObejct.find("bitfields");
        if (bitfieldsIter != fieldJsonObejct.end()) {
            const QJsonArray& bitfieldsJsonArray = bitfieldsIter->toArray();
            for (qsizetype i = 0; i < bitfieldsJsonArray.size(); i++) {
                const QList<QStandardItem*>& bitfieldsData = buildBitfieldStructureItems(bitfieldsJsonArray[i].toObject());
                for (qsizetype j = 0; j < bitfieldsData.size(); j++) {
                    nameItem->setChild(i, j, bitfieldsData[j]);
                }
            }
        }
    }

    return { nameItem, hexDataItem, decodedStringItem };
}

QList<QStandardItem*> buildFrameStructureItems(const QJsonObject& frameJsonObject) {
    QStandardItem* nameItem = new QStandardItem(frameJsonObject["name"].toString());
    QStandardItem* hexDataItem = new QStandardItem(frameJsonObject["hex"].toString());
    QStandardItem* decodedStringItem = new QStandardItem(frameJsonObject["description"].toString());

    const QJsonArray& fieldsJsonArray = frameJsonObject["data"].toArray();
    for (qsizetype i = 0; i < fieldsJsonArray.size(); i++) {
        const QList<QStandardItem*>& fieldJsonObject = buildFieldStructureItems(fieldsJsonArray[i].toObject());
        for (qsizetype j = 0; j < fieldJsonObject.size(); j++) {
            nameItem->setChild(i, j, fieldJsonObject[j]);
        }
    }

    return { nameItem, hexDataItem, decodedStringItem };
}

} /// anonymous namespace

void MainWindow::onHistoryListViewActivated(const QModelIndex &index)
{
    const QJsonObject& frameJsonObejct = index.siblingAtRow(0).data().toJsonDocument().object();
    m_frameStructureViewModel->removeRows(0, m_frameStructureViewModel->rowCount());

    const QList<QStandardItem*>& frameData = buildFrameStructureItems(frameJsonObejct);
    for (qsizetype i = 0; i < frameData.size(); i++) {
        m_frameStructureViewModel->setItem(0, i, frameData[i]);
    }
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
