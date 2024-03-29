#include "fielddefinitiondialog.h"
#include "ui_fielddefinitiondialog.h"

FieldDefinitionDialog::FieldDefinitionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FieldDefinitionDialog)
{
    ui->setupUi(this);

    m_subframesViewModel = new QStandardItemModel(ui->subframesView);
    m_subframesViewModel->setHorizontalHeaderLabels({ tr("Name") });
    ui->subframesView->setModel(m_subframesViewModel);
    connect(ui->subframesView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(onSubframesViewSelectionChanged(QItemSelection,QItemSelection)));
    connect(m_subframesViewModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(onSubframesChanged(QModelIndex,int,int)));
    connect(m_subframesViewModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(onSubframesChanged(QModelIndex,int,int)));

    m_bitfieldDefinitionsViewModel = new QStandardItemModel(ui->bitfieldDefinitionsView);
    m_bitfieldDefinitionsViewModel->setHorizontalHeaderLabels({ tr("Name"), tr("Description"), tr("Bits") });
    ui->bitfieldDefinitionsView->setModel(m_bitfieldDefinitionsViewModel);
    connect(ui->bitfieldDefinitionsView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(onBitfieldDefinitionViewSelectionChanged(QItemSelection,QItemSelection)));
    connect(m_bitfieldDefinitionsViewModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(onBitfieldsChanged(QModelIndex,int,int)));
    connect(m_bitfieldDefinitionsViewModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(onBitfieldsChanged(QModelIndex,int,int)));

    m_bitfieldDefinitionDialog = new BitfieldDefinitionDialog(this);
    connect(this, SIGNAL(bitfieldDefinitionClicked(int,QJsonObject)), m_bitfieldDefinitionDialog, SLOT(onBitfieldDefinitionClicked(int,QJsonObject)));
    connect(m_bitfieldDefinitionDialog, SIGNAL(bitfieldDefinitionChanged(int,QJsonObject)), this, SLOT(onBitfieldDefinitionChanged(int,QJsonObject)));

    m_subframeDialog = new SubframeDialog(this);
    connect(this, SIGNAL(subframeClicked(int,QString)), m_subframeDialog, SLOT(onSubframeClicked(int,QString)));
    connect(m_subframeDialog, SIGNAL(subframeChanged(int,QString)), this, SLOT(onSubframeChanged(int,QString)));
    connect(parent->parent(), SIGNAL(framesChanged(QStringList)), m_subframeDialog, SLOT(onFramesChanged(QStringList)));

    refreshButtonBoxStatus();
}

FieldDefinitionDialog::~FieldDefinitionDialog()
{
    delete ui;
}

void FieldDefinitionDialog::onFieldDefinitionClicked(int row, const QJsonObject &fieldDefinitionObject)
{
    m_row = row;

    if (row < 0) {
        setWindowTitle(tr("New Field Definition"));
        ui->nameLineEdit->clear();
        ui->descriptionLineEdit->clear();
        ui->bytesLineEdit->clear();
        m_subframesViewModel->removeRows(0, m_subframesViewModel->rowCount());
        m_bitfieldDefinitionsViewModel->removeRows(0, m_bitfieldDefinitionsViewModel->rowCount());
        ui->commonFieldTypeRadioButton->click();
        ui->decoderLineEdit->clear();
    }
    else {
        setWindowTitle(tr("Field Definition - %1").arg(fieldDefinitionObject["name"].toString()));
        ui->nameLineEdit->setText(fieldDefinitionObject["name"].toString());
        ui->descriptionLineEdit->setText(fieldDefinitionObject["description"].toString());
        const QJsonValue& bytesValue = fieldDefinitionObject["bytes"];
        ui->bytesLineEdit->setText(bytesValue.isDouble() ? QString("%1").arg(bytesValue.toInt()) : bytesValue.toString());
        if (fieldDefinitionObject.contains("bitfields")) {
            ui->bitfieldsFieldTypeRadioButton->click();
            displayBitfieldDefinitions(fieldDefinitionObject["bitfields"].toArray());
        }
        else if (fieldDefinitionObject.contains("frames")) {
            ui->subframesFieldTypeRadioButton->click();
            displaySubframes(fieldDefinitionObject["subframes"].toArray());
        }
        else {
            ui->commonFieldTypeRadioButton->click();
            ui->decoderLineEdit->setText(fieldDefinitionObject["tostring"].toString());
        }
    }

    refreshToolButtonStatus();
    refreshButtonBoxStatus();

    exec();
}

void FieldDefinitionDialog::onBitfieldDefinitionViewSelectionChanged(const QItemSelection &selection, const QItemSelection &deselection)
{
    refreshToolButtonStatus();
}

void FieldDefinitionDialog::onSubframesViewSelectionChanged(const QItemSelection &selection, const QItemSelection &deselection)
{
    refreshButtonBoxStatus();
    refreshToolButtonStatus();
}

void FieldDefinitionDialog::onBitfieldDefinitionChanged(int row, const QJsonObject &bitfieldDefinitionObject)
{
    if (row < 0) {
        QStandardItem *nameItem = new QStandardItem(bitfieldDefinitionObject["name"].toString());
        nameItem->setData(bitfieldDefinitionObject);
        QStandardItem *descriptionItem = new QStandardItem(bitfieldDefinitionObject["description"].toString());
        const QJsonValue &bitsValue = bitfieldDefinitionObject["bits"];
        QStandardItem *bitsItem = new QStandardItem(bitsValue.isString() ? bitsValue.toString() : QString("%1").arg(bitsValue.toInt()));
        m_bitfieldDefinitionsViewModel->appendRow({ nameItem, descriptionItem, bitsItem });
    }
    else {
        QStandardItem *nameItem = m_bitfieldDefinitionsViewModel->item(row, 0);
        nameItem->setData(bitfieldDefinitionObject);
        nameItem->setText(bitfieldDefinitionObject["name"].toString());
        m_bitfieldDefinitionsViewModel->item(row, 1)->setText(bitfieldDefinitionObject["description"].toString());
        const QJsonValue &bitsValue = bitfieldDefinitionObject["bits"];
        m_bitfieldDefinitionsViewModel->item(row, 2)->setText(bitsValue.isString() ? bitsValue.toString() : QString("%1").arg(bitsValue.toInt()));
    }
}

void FieldDefinitionDialog::onSubframeChanged(int row, const QString &subframe)
{
    if (row < 0) {
        m_subframesViewModel->appendRow(new QStandardItem(subframe));
    }
    else {
        m_subframesViewModel->item(row)->setText(subframe);
    }
}

void FieldDefinitionDialog::onSubframesChanged(const QModelIndex &parent, int first, int last)
{
    refreshButtonBoxStatus();
    refreshToolButtonStatus();
}

void FieldDefinitionDialog::onBitfieldsChanged(const QModelIndex &parent, int first, int last)
{
    refreshButtonBoxStatus();
    refreshToolButtonStatus();
}

void FieldDefinitionDialog::on_commonFieldTypeRadioButton_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->commonFieldTypePage);
    ui->bytesLabel->setText(tr("Byte*"));
    refreshButtonBoxStatus();
}


void FieldDefinitionDialog::on_bitfieldsFieldTypeRadioButton_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->bitfieldFieldTypePage);
    ui->bytesLabel->setText(tr("Byte"));
    refreshButtonBoxStatus();
}


void FieldDefinitionDialog::on_subframesFieldTypeRadioButton_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->subframeFieldTypePage);
    ui->bytesLabel->setText(tr("Byte"));
    refreshButtonBoxStatus();
}


void FieldDefinitionDialog::displayBitfieldDefinitions(const QJsonArray &bitfieldDefinitionsArray)
{
    m_bitfieldDefinitionsViewModel->removeRows(0, m_bitfieldDefinitionsViewModel->rowCount());
    for (auto i = bitfieldDefinitionsArray.constBegin(); i != bitfieldDefinitionsArray.constEnd(); i++) {
        QStandardItem *nameItem = new QStandardItem(i->toObject()["name"].toString());
        nameItem->setData(i->toObject());
        QStandardItem *descriptionItem = new QStandardItem(i->toObject()["description"].toString());
        const QJsonValue &bitsValue = i->toObject()["bits"];
        QStandardItem *bitsItem = new QStandardItem(bitsValue.isString() ? bitsValue.toString() : QString("%1").arg(bitsValue.toInt()));
        m_bitfieldDefinitionsViewModel->appendRow({ nameItem, descriptionItem, bitsItem });
    }
}


void FieldDefinitionDialog::displaySubframes(const QJsonArray &subframesArray)
{
    m_subframesViewModel->removeRows(0, m_subframesViewModel->rowCount());
    for (auto i = subframesArray.constBegin(); i != subframesArray.constEnd(); i++) {
        QStandardItem *nameItem = new QStandardItem(i->toObject()["name"].toString());
        nameItem->setData(i->toObject());
        QStandardItem *descriptionItem = new QStandardItem(i->toObject()["description"].toString());
        const QJsonValue &bitsValue = i->toObject()["bits"];
        QStandardItem *bitsItem = new QStandardItem(bitsValue.isString() ? bitsValue.toString() : QString("%1").arg(bitsValue.toInt()));
        m_bitfieldDefinitionsViewModel->appendRow({ nameItem, descriptionItem, bitsItem });
    }
}

void FieldDefinitionDialog::refreshButtonBoxStatus()
{
    bool empty;
    if (ui->bitfieldsFieldTypeRadioButton->isChecked()) {
        empty = m_bitfieldDefinitionsViewModel->rowCount() == 0;
    }
    else if (ui->subframesFieldTypeRadioButton->isChecked()) {
        empty = m_subframesViewModel->rowCount() == 0;
    }
    else {
        empty = ui->bytesLineEdit->text().trimmed().isEmpty();
    }
    ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(empty || ui->nameLineEdit->text().trimmed().isEmpty());
}

void FieldDefinitionDialog::refreshToolButtonStatus()
{
    if (ui->bitfieldsFieldTypeRadioButton->isChecked()) {
        const QItemSelectionModel *model = ui->bitfieldDefinitionsView->selectionModel();
        const QModelIndex &index = model->currentIndex();

        if (index.isValid()) {
            ui->deleteBitfieldButton->setEnabled(true);
            ui->editBitfieldButton->setEnabled(true);

            if (m_bitfieldDefinitionsViewModel->rowCount() == 0) {
                ui->moveUpBitfieldButton->setEnabled(false);
                ui->moveDownBitfieldButton->setEnabled(false);
            }
            else if (index.row() == m_bitfieldDefinitionsViewModel->rowCount() - 1) {
                ui->moveUpBitfieldButton->setDisabled(false);
                ui->moveDownBitfieldButton->setDisabled(true);
            }
            else if (index.row() == 0) {
                ui->moveUpBitfieldButton->setDisabled(true);
                ui->moveDownBitfieldButton->setDisabled(false);
            }
            else {
                ui->moveUpBitfieldButton->setDisabled(false);
                ui->moveDownBitfieldButton->setDisabled(false);
            }
        }
        else {
            ui->deleteBitfieldButton->setEnabled(false);
            ui->editBitfieldButton->setEnabled(false);
            ui->moveUpBitfieldButton->setEnabled(false);
            ui->moveDownBitfieldButton->setEnabled(false);
        }
    }
    else if (ui->subframesFieldTypeRadioButton->isChecked()) {
        const QItemSelectionModel *model = ui->subframesView->selectionModel();
        const QModelIndex &index = model->currentIndex();

        if (index.isValid()) {
            ui->deleteSubframeButton->setEnabled(true);
            ui->editSubframeButton->setEnabled(true);

            if (m_subframesViewModel->rowCount() == 1) {
                ui->moveUpSubframeButton->setEnabled(false);
                ui->moveDownSubframeButton->setEnabled(false);
            }
            else if (index.row() == m_subframesViewModel->rowCount() - 1) {
                ui->moveUpSubframeButton->setDisabled(false);
                ui->moveDownSubframeButton->setDisabled(true);
            }
            else if (index.row() == 0) {
                ui->moveUpSubframeButton->setDisabled(true);
                ui->moveDownSubframeButton->setDisabled(false);
            }
            else {
                ui->moveUpSubframeButton->setDisabled(false);
                ui->moveDownSubframeButton->setDisabled(false);
            }
        }
        else {
            ui->deleteSubframeButton->setEnabled(false);
            ui->editSubframeButton->setEnabled(false);
            ui->moveUpSubframeButton->setEnabled(false);
            ui->moveDownSubframeButton->setEnabled(false);
        }
    }
}

void FieldDefinitionDialog::on_addBitfieldButton_clicked()
{
    emit bitfieldDefinitionClicked(-1, QJsonObject());
}

void FieldDefinitionDialog::on_buttonBox_accepted()
{
    QJsonObject fieldDefinitionObject;
    fieldDefinitionObject["name"] = ui->nameLineEdit->text().trimmed();
    fieldDefinitionObject["description"] = ui->descriptionLineEdit->text().trimmed();
    bool isInt;
    const QString &bytesString = ui->bytesLineEdit->text().trimmed();
    int bytes = bytesString.toInt(&isInt);
    if (isInt) {
        fieldDefinitionObject["bytes"] = bytes;
    }
    else {
        fieldDefinitionObject["bytes"] = bytesString;
    }

    if (ui->bitfieldsFieldTypeRadioButton->isChecked()) {
        QJsonArray bitfieldDefinitionsArray;
        for (int i = 0; i < m_bitfieldDefinitionsViewModel->rowCount(); i++) {
            bitfieldDefinitionsArray.append(m_bitfieldDefinitionsViewModel->item(i)->data().toJsonObject());
        }
        fieldDefinitionObject["bitfields"] = bitfieldDefinitionsArray;
    }
    else if (ui->subframesFieldTypeRadioButton->isChecked()) {
        QJsonArray subframesArray;
        for (int i = 0; i < m_subframesViewModel->rowCount(); i++) {
            subframesArray.append(m_subframesViewModel->item(i)->data().toJsonObject());
        }
        fieldDefinitionObject["frames"] = subframesArray;
    }
    else {
        fieldDefinitionObject["tostring"] = ui->decoderLineEdit->text().trimmed();
    }
    emit fieldDefinitionChanged(m_row, fieldDefinitionObject);
    close();
}


void FieldDefinitionDialog::on_buttonBox_rejected()
{
    close();
}


void FieldDefinitionDialog::on_subframesView_doubleClicked(const QModelIndex &index)
{
    emit subframeClicked(m_row, m_subframesViewModel->itemFromIndex(index)->text());
}

void FieldDefinitionDialog::on_addSubframeButton_clicked()
{
    emit subframeClicked(-1, QString());
}

void FieldDefinitionDialog::on_nameLineEdit_textChanged(const QString &arg1)
{
    refreshButtonBoxStatus();
}


void FieldDefinitionDialog::on_bytesLineEdit_textChanged(const QString &arg1)
{
    refreshButtonBoxStatus();
}


void FieldDefinitionDialog::on_deleteBitfieldButton_clicked()
{
    const QItemSelectionModel *model = ui->bitfieldDefinitionsView->selectionModel();
    const QModelIndex &index = model->currentIndex();
    m_bitfieldDefinitionsViewModel->removeRow(index.row());
    ui->bitfieldDefinitionsView->setFocus();
}


void FieldDefinitionDialog::on_editBitfieldButton_clicked()
{
    QItemSelectionModel *model = ui->bitfieldDefinitionsView->selectionModel();
    const QModelIndex &index = model->currentIndex().siblingAtColumn(0);
    ui->bitfieldDefinitionsView->setFocus();
    emit bitfieldDefinitionClicked(index.row(), index.data(Qt::UserRole + 1).toJsonObject());
}


void FieldDefinitionDialog::on_moveUpBitfieldButton_clicked()
{
    const QItemSelectionModel *model = ui->bitfieldDefinitionsView->selectionModel();
    const QModelIndex &index = model->currentIndex();
    int row = index.row();
    QList<QStandardItem*> rowItems = m_bitfieldDefinitionsViewModel->takeRow(row);
    m_bitfieldDefinitionsViewModel->insertRow(row - 1, rowItems);
    ui->bitfieldDefinitionsView->setCurrentIndex(m_bitfieldDefinitionsViewModel->index(row - 1, 0));
    ui->bitfieldDefinitionsView->setFocus();
}


void FieldDefinitionDialog::on_moveDownBitfieldButton_clicked()
{
    const QItemSelectionModel *model = ui->bitfieldDefinitionsView->selectionModel();
    const QModelIndex &index = model->currentIndex();
    int row = index.row();
    QList<QStandardItem*> rowItems = m_bitfieldDefinitionsViewModel->takeRow(row);
    m_bitfieldDefinitionsViewModel->insertRow(row + 1, rowItems);
    ui->bitfieldDefinitionsView->setCurrentIndex(m_bitfieldDefinitionsViewModel->index(row + 1, 0));
    ui->bitfieldDefinitionsView->setFocus();
}


void FieldDefinitionDialog::on_deleteSubframeButton_clicked()
{
    const QItemSelectionModel *model = ui->subframesView->selectionModel();
    const QModelIndex &index = model->currentIndex();
    m_subframesViewModel->removeRow(index.row());
    ui->subframesView->setFocus();
}


void FieldDefinitionDialog::on_editSubframeButton_clicked()
{
    QItemSelectionModel *model = ui->subframesView->selectionModel();
    const QModelIndex &index = model->currentIndex().siblingAtColumn(0);
    ui->subframesView->setFocus();
    emit subframeClicked(index.row(), index.data(Qt::UserRole + 1).toString());
}


void FieldDefinitionDialog::on_moveUpSubframeButton_clicked()
{
    const QItemSelectionModel *model = ui->subframesView->selectionModel();
    const QModelIndex &index = model->currentIndex();
    int row = index.row();
    QList<QStandardItem*> rowItems = m_subframesViewModel->takeRow(row);
    m_subframesViewModel->insertRow(row - 1, rowItems);
    ui->subframesView->setCurrentIndex(m_subframesViewModel->index(row - 1, 0));
    ui->subframesView->setFocus();
}


void FieldDefinitionDialog::on_moveDownSubframeButton_clicked()
{
    const QItemSelectionModel *model = ui->subframesView->selectionModel();
    const QModelIndex &index = model->currentIndex();
    int row = index.row();
    QList<QStandardItem*> rowItems = m_subframesViewModel->takeRow(row);
    m_subframesViewModel->insertRow(row + 1, rowItems);
    ui->subframesView->setCurrentIndex(m_subframesViewModel->index(row + 1, 0));
    ui->subframesView->setFocus();
}


void FieldDefinitionDialog::on_bitfieldDefinitionsView_doubleClicked(const QModelIndex &index)
{
    emit bitfieldDefinitionClicked(index.row(), index.siblingAtColumn(0).data(Qt::UserRole + 1).toJsonObject());
}
