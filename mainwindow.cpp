#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QApplication>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QRegularExpression>

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

    m_sendingDataViewModel = new QStandardItemModel(ui->sendingDataView);
    m_sendingDataViewModel->setHorizontalHeaderLabels({ tr("Byte") });
    ui->sendingDataView->setModel(m_sendingDataViewModel);

    m_settingsDialog = new SettingsDialog(this);
    m_frameParser = new UartFrameParserWrapper(this);


    m_validator = new QRegularExpressionValidator(QRegularExpression(R"_([0-9a-fA-F]{2})_"), this);

    connect(m_settingsDialog, SIGNAL(settingsSaved(QSerialPort*, QString)), this, SLOT(onSettingsSaved(QSerialPort*, QString)));
    connect(m_frameParser, SIGNAL(errorOccurred(QString, QString, int, QString)), m_settingsDialog, SLOT(appendLog(QString, QString, int, QString)));
    connect(m_frameParser, SIGNAL(frameReceived(QJsonDocument)), this, SLOT(onFrameReceived(QJsonDocument)));
    connect(m_sendingDataViewModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(onSendingDataItemChanged(QStandardItem*)));
    connect(ui->historyView, SIGNAL(activated(QModelIndex)), this, SLOT(onHistoryListViewActivated(QModelIndex)));
    connect(ui->frameStructureView, SIGNAL(activated(QModelIndex)), this, SLOT(onFrameStructureViewActivated(QModelIndex)));
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
    QStandardItem* directionItem = new QStandardItem(tr("Receive"));
    QStandardItem* hexDataItem = new QStandardItem(frameObject.value("hex").toString());

    dateItem->setData(frameData);

    m_historyViewModel->appendRow({ dateItem, directionItem , hexDataItem });
}

namespace {

QList<QStandardItem*> buildFrameStructureItems(const QJsonObject& frameJsonObject);

QList<QStandardItem*> buildBitfieldStructureItems(const QJsonObject& bitfieldJsonObject) {
    QStandardItem* nameItem = new QStandardItem(QString::fromUtf8(bitfieldJsonObject["name"].toString().toUtf8()));
    QStandardItem* hexDataItem = new QStandardItem(QString::fromUtf8(bitfieldJsonObject["hex"].toString().toUtf8()));
    QStandardItem* decodedStringItem = new QStandardItem(QString::fromUtf8(bitfieldJsonObject["text"].toString().toUtf8()));

    nameItem->setData(bitfieldJsonObject);

    return { nameItem, hexDataItem, decodedStringItem };
}

QList<QStandardItem*> buildFieldStructureItems(const QJsonObject& fieldJsonObejct) {
    QStandardItem* nameItem = new QStandardItem(QString::fromUtf8(fieldJsonObejct["name"].toString().toUtf8()));
    QStandardItem* hexDataItem = new QStandardItem(QString::fromUtf8(fieldJsonObejct["hex"].toString().toUtf8()));
    QStandardItem* decodedStringItem = new QStandardItem(QString::fromUtf8(fieldJsonObejct["text"].toString().toUtf8()));

    nameItem->setData(fieldJsonObejct);

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
    QStandardItem* nameItem = new QStandardItem(QString::fromUtf8(frameJsonObject["name"].toString().toUtf8()));
    QStandardItem* hexDataItem = new QStandardItem(QString::fromUtf8(frameJsonObject["hex"].toString().toUtf8()));
    QStandardItem* decodedStringItem = new QStandardItem(QString::fromUtf8(frameJsonObject["text"].toString().toUtf8()));

    nameItem->setData(frameJsonObject);

    const QJsonArray& fieldsJsonArray = frameJsonObject["data"].toArray();
    for (qsizetype i = 0; i < fieldsJsonArray.size(); i++) {
        const QList<QStandardItem*>& fieldJsonObject = buildFieldStructureItems(fieldsJsonArray[i].toObject());
        for (qsizetype j = 0; j < fieldJsonObject.size(); j++) {
            nameItem->setChild(i, j, fieldJsonObject[j]);
        }
    }

    return { nameItem, hexDataItem, decodedStringItem };
}

QList<QList<QStandardItem*>> buildAttributeItems(const QJsonObject& jsonObject) {
    QList<QList<QStandardItem*>> result;
    for (QJsonObject::const_iterator i = jsonObject.constBegin(); i != jsonObject.constEnd(); i++) {
        QJsonValue::Type type = i->type();
        if (type != QJsonValue::Type::Object && type != QJsonValue::Type::Array) {
            QList<QStandardItem*> item = { new QStandardItem(i.key()), new QStandardItem(QString::fromUtf8(i->toVariant().toString().toUtf8())) };
            result.append(item);
        }
    }
    return result;
}

} /// anonymous namespace

void MainWindow::onHistoryListViewActivated(const QModelIndex &index)
{
    QJsonObject object = index.siblingAtColumn(0).data(Qt::UserRole + 1).toJsonObject();
    if (object.size() != 0) {
        m_frameStructureViewModel->removeRows(0, m_frameStructureViewModel->rowCount());
        m_attributeViewModel->removeRows(0, m_attributeViewModel->rowCount());

        const QList<QStandardItem*>& frameData = buildFrameStructureItems(object);
        for (qsizetype i = 0; i < frameData.size(); i++) {
            m_frameStructureViewModel->setItem(0, i, frameData[i]);
        }
        ui->tabWidget->setCurrentWidget(ui->receivingTab);
    }
    else {
        QString text = m_historyViewModel->itemFromIndex(index.siblingAtColumn(2))->text();
        m_sendingDataViewModel->removeRows(0, m_sendingDataViewModel->rowCount());

        for (qsizetype i = 0; i < text.size(); i += 2) {
            m_sendingDataViewModel->appendRow(new QStandardItem(text.mid(i, 2)));
        }
        ui->tabWidget->setCurrentWidget(ui->sendingTab);
    }
}

void MainWindow::onFrameStructureViewActivated(const QModelIndex &index)
{
    m_attributeViewModel->removeRows(0, m_attributeViewModel->rowCount());

    const QList<QList<QStandardItem*>>& attributeData = buildAttributeItems(index.siblingAtColumn(0).data(Qt::UserRole + 1).toJsonObject());
    for (qsizetype i = 0; i < attributeData.size(); i++) {
        for (qsizetype j = 0; j < attributeData[i].size(); j++)
        m_attributeViewModel->setItem(i, j, attributeData[i][j]);
    }
}

void MainWindow::onSendingDataItemChanged(QStandardItem *item)
{
    int pos = 0;
    QString text = item->text();
    if (m_validator->validate(text, pos) != QValidator::State::Acceptable) {
        item->setText("00");
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

void MainWindow::on_addByteButton_clicked()
{
    QStandardItem* item = new QStandardItem("00");
    m_sendingDataViewModel->appendRow(item);
}

void MainWindow::on_removeByteButton_clicked()
{
    const QModelIndex index = ui->sendingDataView->selectionModel()->currentIndex();
    m_sendingDataViewModel->removeRow(index.row(), index.parent());
}

void MainWindow::on_sendButton_clicked()
{
    QByteArray sendData;
    for (int i = 0; i < m_sendingDataViewModel->rowCount(); i++) {
        QString data = m_sendingDataViewModel->item(i)->text();
        sendData.append((char)data.toUShort(nullptr, 16));
    }
    if (sendData.size() != 0) {
        QSerialPort* serialPort = m_settingsDialog->serialPort();
        if (serialPort) {
            if (serialPort->write(sendData) != -1) {
                QStandardItem* dateItem = new QStandardItem(QDateTime::currentDateTime().toString());
                QStandardItem* directionItem = new QStandardItem(tr("Send"));
                QStandardItem* hexDataItem = new QStandardItem(sendData.toHex());
                m_historyViewModel->appendRow({ dateItem, directionItem , hexDataItem });
            }
        }
    }
}

void MainWindow::on_insertByteButton_clicked()
{
    const QModelIndex index = ui->sendingDataView->selectionModel()->currentIndex();
    if (index.isValid()) {
        m_sendingDataViewModel->insertRow(index.row(), new QStandardItem("00"));
    }
    else {
        on_addByteButton_clicked();
    }
}
