#include "subframedialog.h"
#include "ui_subframedialog.h"
#include <QPushButton>

SubframeDialog::SubframeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SubframeDialog)
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
}

SubframeDialog::~SubframeDialog()
{
    delete ui;
}

void SubframeDialog::onSubframeClicked(int row, const QString &subframe)
{
    m_row = row;
    if (row >= 0) {
        ui->subframeComboBox->setCurrentText(subframe);
    }

    exec();
}

void SubframeDialog::onFramesChanged(const QStringList &frameNames)
{
    ui->subframeComboBox->clear();
    ui->subframeComboBox->addItems(frameNames);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(frameNames.empty());
}

void SubframeDialog::on_buttonBox_accepted()
{
    emit subframeChanged(m_row, ui->subframeComboBox->currentText());

    close();
}

void SubframeDialog::on_buttonBox_rejected()
{
    close();
}

