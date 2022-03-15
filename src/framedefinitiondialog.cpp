#include "framedefinitiondialog.h"
#include "ui_framedefinitiondialog.h"

FrameDefinitionDialog::FrameDefinitionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FrameDefinitionDialog)
{
    ui->setupUi(this);

    m_fieldDefinitionsViewModel = new QStandardItemModel(ui->fieldDefinitionsView);
    m_fieldDefinitionsViewModel->setHorizontalHeaderLabels({ tr("Name"), tr("Description"), tr("Which contains") });
    ui->fieldDefinitionsView->setModel(m_fieldDefinitionsViewModel);

    m_fieldDefinitionDialog = new FieldDefinitionDialog(this);
    connect(this, SIGNAL(fieldDefinitionClicked(int,QJsonObject)), m_fieldDefinitionDialog, SLOT(onFieldDefinitionClicked(int,QJsonObject)));
    connect(m_fieldDefinitionDialog, SIGNAL(fieldDefinitionChanged(int,QJsonObject)), this, SLOT(onFieldDefinitionChanged(int,QJsonObject)));

    connect(ui->fieldDefinitionsView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(onFieldDefinitionViewSelectionChanged(QItemSelection,QItemSelection)));
    connect(m_fieldDefinitionsViewModel, SIGNAL(dataChanged(QModelIndex,QModelIndex,QList<int>)), this, SLOT(onFieldDefinitionsChanged(QModelIndex,QModelIndex,QList<int>)));
}

FrameDefinitionDialog::~FrameDefinitionDialog()
{
    delete ui;
}

void FrameDefinitionDialog::onFrameDefinitionClicked(int row, const QJsonObject &frameDefinitionObject)
{
    m_row = row;
    if (row < 0) {
        setWindowTitle(tr("New Frame Definition"));
        ui->nameLineEdit->clear();
        ui->descriptionLineEdit->clear();
        ui->validatorLineEdit->clear();
        m_fieldDefinitionsViewModel->removeRows(0, m_fieldDefinitionsViewModel->rowCount());
    }
    else {
        setWindowTitle(tr("Frame Definition - %1").arg(frameDefinitionObject["name"].toString()));
        ui->nameLineEdit->setText(frameDefinitionObject["name"].toString());
        ui->descriptionLineEdit->setText(frameDefinitionObject["description"].toString());
        ui->validatorLineEdit->setText(frameDefinitionObject["validator"].toString());
        displayFieldDefinitions(frameDefinitionObject["fields"].toArray());
    }

    refreshToolButtonStatus();
    refreshButtonBoxStatus();

    exec();
}

void FrameDefinitionDialog::onFieldDefinitionViewSelectionChanged(const QItemSelection &selection, const QItemSelection &deselection)
{
    refreshToolButtonStatus();
}

void FrameDefinitionDialog::onFieldDefinitionsChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles)
{
    refreshButtonBoxStatus();
}

void FrameDefinitionDialog::onFieldDefinitionChanged(int row, const QJsonObject &fieldDefinitionObject)
{
    const QString &name = fieldDefinitionObject["name"].toString();
    const QString &description = fieldDefinitionObject["description"].toString();
    const QString &whichContains = tr(fieldDefinitionObject.contains("frames") ? "Subframes" : (fieldDefinitionObject.contains("bitfields") ? "Bitfields" : "None"));

    if (row < 0) {
        QStandardItem *nameItem = new QStandardItem(name);
        nameItem->setData(fieldDefinitionObject);
        QStandardItem *descriptionItem = new QStandardItem(description);
        QStandardItem *whichContainsItem = new QStandardItem(whichContains);
        m_fieldDefinitionsViewModel->appendRow({ nameItem, descriptionItem, whichContainsItem });
    }
    else {
        QStandardItem *nameItem = m_fieldDefinitionsViewModel->item(row, 0);
        nameItem->setText(name);
        nameItem->setData(fieldDefinitionObject);
        m_fieldDefinitionsViewModel->item(row, 1)->setText(description);
        m_fieldDefinitionsViewModel->item(row, 2)->setText(whichContains);
    }
}

void FrameDefinitionDialog::on_addFieldButton_clicked()
{
    emit fieldDefinitionClicked(-1, QJsonObject());
}


void FrameDefinitionDialog::on_deleteFieldButton_clicked()
{
    const QItemSelection& selection = ui->fieldDefinitionsView->selectionModel()->selection();
    for (auto i = selection.begin(); i != selection.end(); i++) {
        m_fieldDefinitionsViewModel->removeRows(i->top(), i->height(), i->parent());
    }
}


void FrameDefinitionDialog::on_editFieldButton_clicked()
{
    QItemSelectionModel *model = ui->fieldDefinitionsView->selectionModel();
    const QItemSelection &selection = model->selection();
    if (model->hasSelection()) {
        const QModelIndex &index = selection[0].indexes()[0].siblingAtColumn(0);
        emit fieldDefinitionClicked(index.row(), index.data(Qt::UserRole + 1).toJsonObject());
    }
}


void FrameDefinitionDialog::on_moveUpFieldButton_clicked()
{
    const QItemSelectionModel *model = ui->fieldDefinitionsView->selectionModel();
    const QItemSelection &selection = model->selection();
    if (ui->fieldDefinitionsView->selectionModel()->hasSelection()) {
        const QModelIndex &index = selection[0].indexes()[0];
        if (index.row() != 0) {
            m_fieldDefinitionsViewModel->moveRow(index.parent(), index.row(), index.parent(), index.row() - 1);
        }
    }
}


void FrameDefinitionDialog::on_moveDownFieldButton_clicked()
{
    const QItemSelectionModel *model = ui->fieldDefinitionsView->selectionModel();
    const QItemSelection &selection = model->selection();
    if (ui->fieldDefinitionsView->selectionModel()->hasSelection()) {
        const QModelIndex &index = selection[0].indexes()[0];
        if (index.row() != m_fieldDefinitionsViewModel->rowCount() - 1) {
            m_fieldDefinitionsViewModel->moveRow(index.parent(), index.row(), index.parent(), index.row() + 1);
        }
    }
}


void FrameDefinitionDialog::on_buttonBox_accepted()
{
    QJsonArray fieldDefinitions;
    for (int i = 0; i < m_fieldDefinitionsViewModel->rowCount(); i++) {
        fieldDefinitions.append(m_fieldDefinitionsViewModel->index(i, 0).data(Qt::UserRole + 1).toJsonObject());
    }
    QJsonObject frameDefinitionObject;
    frameDefinitionObject["name"] = ui->nameLineEdit->text();
    frameDefinitionObject["description"] = ui->descriptionLineEdit->text();
    frameDefinitionObject["validator"] = ui->validatorLineEdit->text();
    frameDefinitionObject["fields"] = fieldDefinitions;
    emit frameDefinitionChanged(m_row, frameDefinitionObject);
}


void FrameDefinitionDialog::on_buttonBox_rejected()
{
    close();
}

void FrameDefinitionDialog::displayFieldDefinitions(const QJsonArray &fieldDefinitionsArray)
{
    m_fieldDefinitionsViewModel->removeRows(0, m_fieldDefinitionsViewModel->rowCount());
    for (auto i = fieldDefinitionsArray.constBegin(); i != fieldDefinitionsArray.constEnd(); i++) {
        onFieldDefinitionChanged(-1, i->toObject());
    }
}

void FrameDefinitionDialog::refreshButtonBoxStatus()
{
    bool emptyFieldDefinitions = m_fieldDefinitionsViewModel->rowCount() == 0;
    ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(emptyFieldDefinitions || ui->nameLineEdit->text().trimmed().isEmpty());
}

void FrameDefinitionDialog::refreshToolButtonStatus()
{
    const QItemSelectionModel *model = ui->fieldDefinitionsView->selectionModel();
    const QItemSelection &selection = model->selection();

    if (model->hasSelection()) {
        ui->deleteFieldButton->setEnabled(true);
        ui->editFieldButton->setEnabled(true);

        const QModelIndex &index = selection[0].indexes()[0];
        if (index.row() == m_fieldDefinitionsViewModel->rowCount() - 1) {
            ui->moveUpFieldButton->setDisabled(false);
            ui->moveDownFieldButton->setDisabled(true);
        }
        else if (index.row() == 0) {
            ui->moveUpFieldButton->setDisabled(true);
            ui->moveDownFieldButton->setDisabled(false);
        }
        else {
            ui->moveUpFieldButton->setDisabled(false);
            ui->moveDownFieldButton->setDisabled(false);
        }
    }
    else {
        ui->deleteFieldButton->setEnabled(false);
        ui->editFieldButton->setEnabled(false);
        ui->moveUpFieldButton->setEnabled(false);
        ui->moveDownFieldButton->setEnabled(false);
    }
}

void FrameDefinitionDialog::on_nameLineEdit_textChanged(const QString &arg1)
{
    refreshButtonBoxStatus();
}

