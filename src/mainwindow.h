#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QRegularExpressionValidator>
#include <QJsonObject>
#include <QMap>
#include "savechangesdialog.h"
#include "settingsdialog.h"
#include "framedefinitiondialog.h"
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

public slots:
    void close();

private slots:
    void on_actionOpen_triggered();

private slots:

    void onSettingsSaved(QSerialPort* serialPort, const QString& schema);

    void onSchemaFileSelected(const QString &schemaFilePath);

    void onFrameReceived(const QJsonDocument& frameData);

    void onHistoryListViewActivated(const QModelIndex &index);

    void onFrameStructureViewActivated(const QModelIndex &index);

    void onSendingDataItemChanged(QStandardItem* item);

    void onSerialPortReadyRead();

    void onFrameDefinitionChanged(int row, const QJsonObject &frameDefinitionObject);

    void onFrameDefinitionViewSelectionChanged(const QItemSelection &selection, const QItemSelection &deselection);

    void onFrameDefinitionsChanged(const QModelIndex &parent, int first, int last);

    void on_actionNew_triggered();

    void on_initScriptPlainTextEdit_textChanged();

    void on_actionExit_triggered();

    void on_actionSettings_triggered();

    void on_addByteButton_clicked();

    void on_removeByteButton_clicked();

    void on_sendButton_clicked();

    void on_insertByteButton_clicked();

    void on_addFrameDefinitionButton_clicked();

    void on_deleteFrameDefinitionButton_clicked();

    void on_editFrameDefinitionButton_clicked();

    void on_frameDefinitionsView_activated(const QModelIndex &index);

    void on_frameDefinitionsView_doubleClicked(const QModelIndex &index);

    void on_frameDefinitionsView_entered(const QModelIndex &index);

    void on_actionSave_triggered();

signals:
    void frameDefinitionClicked(int row, const QJsonObject &frameDefinitionObject);

    void framesChanged(const QStringList &frameNames);

private:
    void refreshButtonBoxStatus();

    void discardChanges();

    bool saveChanges();

    void loadSchemaFile();

private:
    Ui::MainWindow *ui;

    SettingsDialog *m_settingsDialog;

    FrameDefinitionDialog *m_frameDefinitionDialog;

    UartFrameParserWrapper *m_frameParser;

    QStandardItemModel *m_historyViewModel;

    QStandardItemModel *m_frameStructureViewModel;

    QStandardItemModel *m_attributeViewModel;

    QStandardItemModel *m_sendingDataViewModel;

    QStandardItemModel *m_frameDefinitionsViewModel;

    QRegularExpressionValidator *m_validator;

    SaveChangesDialog *m_saveChangesDialog;

    bool changed = false;
};
#endif // MAINWINDOW_H
