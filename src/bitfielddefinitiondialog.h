#ifndef BITFIELDDEFINITIONDIALOG_H
#define BITFIELDDEFINITIONDIALOG_H

#include <QDialog>
#include <QJsonObject>

namespace Ui {
class BitfieldDefinitionDialog;
}

class BitfieldDefinitionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BitfieldDefinitionDialog(QWidget *parent = nullptr);
    ~BitfieldDefinitionDialog();

private:
    void refreshButtonBoxStatus();

public slots:
    void onBitfieldDefinitionClicked(int row, const QJsonObject &bitfieldDefinitionObject);

private slots:
    void on_nameLineEdit_textChanged(const QString &arg1);

    void on_bitsLineEdit_textChanged(const QString &arg1);

    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

signals:
    void bitfieldDefinitionChanged(int, const QJsonObject &bitfieldDefinitionChanged);

private:
    Ui::BitfieldDefinitionDialog *ui;

    int m_row;
};

#endif // BITFIELDDEFINITIONDIALOG_H
