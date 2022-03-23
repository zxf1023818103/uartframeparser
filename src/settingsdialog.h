#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QFileDialog>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();    

    QSerialPort* serialPort();

    const QString& schema();

    const QString& schemaFilePath();

    void selectFile();

public slots:
    void appendLog(const QString& topic, const QString& filename, int line, const QString& message);

private slots:
    void refreshSerialPortList();

    void refreshSerialPortBaudRates(int index);

    void onSerialPortErrorOccurred(QSerialPort::SerialPortError error);

    void onSchemaFileSelected(const QString &schemaFileName);

    void on_schemaFileSelectButton_clicked();

    void on_buttonBox_accepted();

    void on_appearanceComboBox_textActivated(const QString &style);

signals:
    void settingsSaved(QSerialPort *serialPort, const QString& schema);

    void schemaFileSelected(const QString &schemaFilePath);

private:
    Ui::SettingsDialog *ui = nullptr;

    QList<QSerialPortInfo> m_serialPortInfoList;

    QFileDialog *m_fileDialog = nullptr;

    QSerialPort *m_serialPort = nullptr;

    QString m_schema;

    QString m_schemaFilePath;
};

#endif // SETTINGSDIALOG_H
