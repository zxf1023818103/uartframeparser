#ifndef FRAMEDEFINITIONDIALOG_H
#define FRAMEDEFINITIONDIALOG_H

#include <QDialog>
#include <QStandardItemModel>
#include <QJsonObject>
#include <QJsonArray>
#include <QItemSelection>
#include "fielddefinitiondialog.h"

namespace Ui {
class FrameDefinitionDialog;
}

class FrameDefinitionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FrameDefinitionDialog(QWidget *parent = nullptr);
    ~FrameDefinitionDialog();

private slots:
    void onFrameDefinitionClicked(int row, const QJsonObject &frameDefinitionObject);

    void onFieldDefinitionViewSelectionChanged(const QItemSelection &selection, const QItemSelection &deselection);

    void onFieldDefinitionsChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles = QList<int>());

    void onFieldDefinitionChanged(int row, const QJsonObject &fieldDefnintionObject);

    void on_addFieldButton_clicked();

    void on_deleteFieldButton_clicked();

    void on_editFieldButton_clicked();

    void on_moveUpFieldButton_clicked();

    void on_moveDownFieldButton_clicked();

    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

    void on_nameLineEdit_textChanged(const QString &arg1);

signals:
    void frameDefinitionChanged(int row, const QJsonObject &frameDefinitionObject);

    void fieldDefinitionClicked(int row, const QJsonObject &fieldDefinitionObject);

private:
    void displayFieldDefinitions(const QJsonArray &fieldDefinitionsArray);

    void refreshButtonBoxStatus();

    void refreshToolButtonStatus();

private:
    Ui::FrameDefinitionDialog *ui;

    QStandardItemModel *m_fieldDefinitionsViewModel;

    FieldDefinitionDialog *m_fieldDefinitionDialog;

    int m_row = -1;
};

#endif // FRAMEDEFINITIONDIALOG_H
