#include "aboutdialog.h"
#include "ui_aboutdialog.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    ui->versionLabel->setText(QString(tr("Version: %1")).arg(GIT_BRANCH "-" GIT_REV));
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
