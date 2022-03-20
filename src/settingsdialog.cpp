#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QVariant>
#include <QFile>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    m_fileDialog = new QFileDialog(this);
    m_fileDialog->setMimeTypeFilters({"application/json"});
    refreshSerialPortList();
    connect(m_fileDialog, SIGNAL(fileSelected(QString)), this, SLOT(onSchemaFileSelected(QString)));
    connect(ui->serialPortRefreshButton, SIGNAL(clicked()), this, SLOT(refreshSerialPortList()));
    connect(ui->serialPortComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(refreshSerialPortBaudRates(int)));
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

QSerialPort *SettingsDialog::serialPort()
{
    return m_serialPort;
}

const QString &SettingsDialog::schema()
{
    return m_schema;
}

const QString &SettingsDialog::schemaFilePath()
{
    m_schemaFilePath = ui->schemaFilePathLineEdit->text();
    return m_schemaFilePath;
}

void SettingsDialog::selectFile()
{
    m_fileDialog->exec();
}

void SettingsDialog::appendLog(const QString &topic, const QString &filename, int line, const QString &message)
{
    qsizetype index = filename.lastIndexOf('/');
    if (index == -1) {
        index = filename.lastIndexOf('\\');
    }
    if (index != -1) {
        index++;
    }

    ui->logsTextEdit->appendMessage(QString("[%1][%2:%3] %4").arg(tr(topic.toLatin1()), filename.mid(index), QVariant(line).toString(), tr(message.toLatin1())));

    exec();
}

void SettingsDialog::refreshSerialPortList()
{
    ui->serialPortComboBox->clear();
    m_serialPortInfoList = QSerialPortInfo::availablePorts();
    for (qsizetype i = 0; i < m_serialPortInfoList.size(); i++) {
        ui->serialPortComboBox->addItem(m_serialPortInfoList[i].portName(), i);
    }
    refreshSerialPortBaudRates(0);
}

void SettingsDialog::refreshSerialPortBaudRates(int index)
{
    (void)index;

    ui->baudRateComboBox->clear();
    if (m_serialPortInfoList.size()) {
        qlonglong i = ui->baudRateComboBox->currentData().toLongLong();
        for (qint32 baudRate : m_serialPortInfoList[i].standardBaudRates()) {
            ui->baudRateComboBox->addItem(QString("%1bps").arg(baudRate), baudRate);
        }
    }
}

void SettingsDialog::onSerialPortErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError) {
        appendLog("Serial", __FILE__, __LINE__, m_serialPort->errorString());
    }
}

void SettingsDialog::onSchemaFileSelected(const QString &schemaFileName)
{
    ui->schemaFilePathLineEdit->setText(schemaFileName);
    emit schemaFileSelected(schemaFileName);
}

void SettingsDialog::on_schemaFileOpenButton_clicked()
{
    m_fileDialog->exec();
}

void SettingsDialog::on_buttonBox_accepted()
{
    const QVariant& serialPortIndex = ui->serialPortComboBox->currentData();
    const QVariant& baudRateData = ui->baudRateComboBox->currentData();
    if (serialPortIndex.isValid() && baudRateData.isValid()) {
        if (m_serialPort) {
            delete(m_serialPort);
        }
        m_serialPort = new QSerialPort(m_serialPortInfoList[serialPortIndex.toLongLong()], this);
        m_serialPort->setBaudRate(baudRateData.toInt());
        connect(m_serialPort, &QSerialPort::errorOccurred, this, &SettingsDialog::onSerialPortErrorOccurred, Qt::UniqueConnection);
        if (m_serialPort->open(QSerialPort::ReadWrite)) {
            const QString &filename = ui->schemaFilePathLineEdit->text();
            if (filename.size()) {
                QFile file(filename);
                if (file.open(QFile::ReadOnly | QFile::Text)) {
                    const QByteArray& content = file.readAll();
                    if (content.size()) {
                        m_schema = QString::fromUtf8(content);
                        emit settingsSaved(m_serialPort, m_schema);
                    }
                    else {
                        appendLog("Settings", __FILE__, __LINE__, "Schema file is empty");
                    }
                }
                else {
                    appendLog("Settings", __FILE__, __LINE__, file.errorString());
                }
            }
            else {
                appendLog("Settings", __FILE__, __LINE__, "Schema filename is empty");
            }
        }
    }
    else {
        appendLog("Settings", __FILE__, __LINE__, "Serial port is not selected");
    }
}
