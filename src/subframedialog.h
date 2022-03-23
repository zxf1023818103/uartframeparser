#ifndef SUBFRAMEDIALOG_H
#define SUBFRAMEDIALOG_H

#include <QDialog>

namespace Ui {
class SubframeDialog;
}

class SubframeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SubframeDialog(QWidget *parent = nullptr);
    ~SubframeDialog();

signals:
    void subframeChanged(int row, const QString &subframe);

public slots:
    void onSubframeClicked(int row, const QString &subframe);

    void onFramesChanged(const QStringList &frameNames);

private slots:
    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

private:
    Ui::SubframeDialog *ui;

    int m_row;
};

#endif // SUBFRAMEDIALOG_H
