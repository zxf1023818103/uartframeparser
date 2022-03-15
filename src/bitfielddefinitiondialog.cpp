#include "bitfielddefinitiondialog.h"
#include "ui_bitfielddefinitiondialog.h"
#include <QPushButton>

BitfieldDefinitionDialog::BitfieldDefinitionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BitfieldDefinitionDialog)
{
    ui->setupUi(this);
}

BitfieldDefinitionDialog::~BitfieldDefinitionDialog()
{
    delete ui;
}

void BitfieldDefinitionDialog::refreshButtonBoxStatus()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(ui->nameLineEdit->text().trimmed().isEmpty() || ui->bitsLineEdit->text().trimmed().isEmpty());
}

void BitfieldDefinitionDialog::onBitfieldDefinitionClicked(int row, const QJsonObject &bitfieldDefinitionObject)
{
    m_row = row;
    if (row < 0) {
        setWindowTitle(tr("New Bitfield Definition"));
        ui->nameLineEdit->clear();
        ui->descriptionLineEdit->clear();
        ui->bitsLineEdit->clear();
        ui->decoderLineEdit->clear();
        ui->defaultsLineEdit->clear();
    }
    else {
        setWindowTitle(tr("Bitfield Definition - %1").arg(bitfieldDefinitionObject["name"].toString()));
        ui->nameLineEdit->setText(bitfieldDefinitionObject["name"].toString());
        ui->descriptionLineEdit->setText(bitfieldDefinitionObject["description"].toString());
        ui->bitsLineEdit->setText(bitfieldDefinitionObject["bits"].toString());
        ui->decoderLineEdit->setText(bitfieldDefinitionObject["tostring"].toString());
        ui->defaultsLineEdit->setText(bitfieldDefinitionObject["defaults"].toString());
    }

    exec();
}

void BitfieldDefinitionDialog::on_nameLineEdit_textChanged(const QString &arg1)
{
    refreshButtonBoxStatus();
}


void BitfieldDefinitionDialog::on_bitsLineEdit_textChanged(const QString &arg1)
{
    refreshButtonBoxStatus();
}


void BitfieldDefinitionDialog::on_buttonBox_accepted()
{
    QJsonObject bitfieldDefinitionObject;
    bitfieldDefinitionObject["name"] = ui->nameLineEdit->text().trimmed();
    bitfieldDefinitionObject["description"] = ui->descriptionLineEdit->text().trimmed();
    bitfieldDefinitionObject["tostring"] = ui->decoderLineEdit->text().trimmed();
    bitfieldDefinitionObject["bits"] = ui->bitsLineEdit->text().trimmed();
    bitfieldDefinitionObject["default"] = ui->defaultsLineEdit->text().trimmed();
    emit bitfieldDefinitionChanged(m_row, bitfieldDefinitionObject);
    close();
}


void BitfieldDefinitionDialog::on_buttonBox_rejected()
{
    close();
}
