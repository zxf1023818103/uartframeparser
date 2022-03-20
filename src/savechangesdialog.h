#ifndef SAVECHANGESDIALOG_H
#define SAVECHANGESDIALOG_H

#include <QDialog>

namespace Ui {
class SaveChangesDialog;
}

class SaveChangesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SaveChangesDialog(QWidget *parent = nullptr);
    ~SaveChangesDialog();

    bool askForSaving();

signals:
    void saved();

    void discarded();

private slots:
    void on_buttonBox_rejected();

private slots:
    void on_buttonBox_accepted();

private:
    Ui::SaveChangesDialog *ui;

    bool m_save;
};

#endif // SAVECHANGESDIALOG_H
