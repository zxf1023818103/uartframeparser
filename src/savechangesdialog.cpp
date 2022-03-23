#include "savechangesdialog.h"
#include "ui_savechangesdialog.h"

SaveChangesDialog::SaveChangesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SaveChangesDialog)
{
    ui->setupUi(this);
}

SaveChangesDialog::~SaveChangesDialog()
{
    delete ui;
}

bool SaveChangesDialog::askForSaving()
{
    exec();
    return m_save;
}

void SaveChangesDialog::on_buttonBox_accepted()
{
    m_save = true;
    emit saved();
    close();
}


void SaveChangesDialog::on_buttonBox_rejected()
{
    m_save = false;
    emit discarded();
    close();
}

