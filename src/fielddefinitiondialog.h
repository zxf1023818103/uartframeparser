#ifndef FIELDDEFINITIONDIALOG_H
#define FIELDDEFINITIONDIALOG_H

#include <QDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QModelIndex>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QItemSelection>
#include "bitfielddefinitiondialog.h"
#include "subframedialog.h"

namespace Ui {
class FieldDefinitionDialog;
}

class FieldDefinitionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FieldDefinitionDialog(QWidget *parent = nullptr);
    ~FieldDefinitionDialog();

signals:
    void bitfieldDefinitionClicked(int row, const QJsonObject& bitfieldDefinitionObject);

    void fieldDefinitionChanged(int row, const QJsonObject &fieldDefinitionObject);

    void subframeClicked(int row, const QString &subframe);

public slots:
    void onFieldDefinitionClicked(int row, const QJsonObject& fieldDefinitionObject);

private slots:
    void onBitfieldDefinitionViewSelectionChanged(const QItemSelection &selection, const QItemSelection &deselection);

    void onSubframesViewSelectionChanged(const QItemSelection &selection, const QItemSelection &deselection);

    void onBitfieldDefinitionChanged(int row, const QJsonObject& bitfieldDefinitionObject);

    void onSubframeChanged(int row, const QString &subframe);

    void onSubframesChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles = QList<int>());

    void onBitfieldsChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles = QList<int>());

    void on_commonFieldTypeRadioButton_clicked();

    void on_bitfieldsFieldTypeRadioButton_clicked();

    void on_subframesFieldTypeRadioButton_clicked();

    void on_addBitfieldButton_clicked();

    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

    void on_subframesView_doubleClicked(const QModelIndex &index);

    void on_subframesView_entered(const QModelIndex &index);

    void on_addSubframeButton_clicked();

    void on_nameLineEdit_textChanged(const QString &arg1);

    void on_bytesLineEdit_textChanged(const QString &arg1);

private:
    void displayBitfieldDefinitions(const QJsonArray &bitfieldDefinitionsArray);

    void displaySubframes(const QJsonArray &subframesArray);

    void refreshButtonBoxStatus();

    void refreshToolButtonStatus();

private:
    Ui::FieldDefinitionDialog *ui;

    QStandardItemModel *m_subframesViewModel;

    QStandardItemModel *m_bitfieldDefinitionsViewModel;

    BitfieldDefinitionDialog *m_bitfieldDefinitionDialog;

    SubframeDialog *m_subframeDialog;

    int m_row;
};

#endif // FIELDDEFINITIONDIALOG_H
