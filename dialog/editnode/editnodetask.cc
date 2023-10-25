#include "editnodetask.h"

#include <QColorDialog>

#include "component/constvalue.h"
#include "component/enumclass.h"
#include "component/signalblocker.h"
#include "ui_editnodetask.h"

EditNodeTask::EditNodeTask(CEditNodeParamsFPTS& params, int amount_decimal, CString& display_format, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::EditNodeTask)
    , node_ { params.node }
    , parent_path_ { params.parent_path }
    , name_list_ { params.name_list }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    IniDialog(params.unit_model, amount_decimal, display_format);
    IniConnect();
    IniData(params.node, params.type_enable, params.unit_enable);
}

EditNodeTask::~EditNodeTask() { delete ui; }

void EditNodeTask::IniDialog(QStandardItemModel* unit_model, int amount_decimal, CString& display_format)
{
    ui->lineEditName->setFocus();
    ui->lineEditName->setValidator(&LineEdit::kInputValidator);

    this->setWindowTitle(parent_path_ + node_->name);
    this->setFixedSize(350, 650);

    ui->comboUnit->setModel(unit_model);

    ui->dSpinBoxUnitCost->setRange(0.0, std::numeric_limits<double>::max());
    ui->dSpinBoxUnitCost->setDecimals(amount_decimal);
    ui->dateTime->setDisplayFormat(display_format);
    ui->dateTime->setCalendarPopup(true);
}

void EditNodeTask::IniConnect() { connect(ui->lineEditName, &QLineEdit::textEdited, this, &EditNodeTask::RNameEdited); }

void EditNodeTask::IniData(Node* node, bool type_enable, bool unit_enable)
{
    int item_index { ui->comboUnit->findData(node->unit) };
    ui->comboUnit->setCurrentIndex(item_index);
    ui->comboUnit->setEnabled(unit_enable);

    ui->rBtnDDCI->setChecked(node->rule == kRuleDDCI);
    ui->rBtnDICD->setChecked(node->rule == kRuleDICD);
    ui->rBtnLeaf->setChecked(true);

    if (node->name.isEmpty())
        return ui->pBtnOk->setEnabled(false);

    ui->lineEditName->setText(node->name);
    ui->lineEditCode->setText(node->code);
    ui->lineEditDescription->setText(node->description);
    ui->plainTextEdit->setPlainText(node->note);
    ui->dSpinBoxUnitCost->setValue(node->first);
    ui->chkBoxFinished->setChecked(node_->finished);
    ui->dateTime->setDateTime(QDateTime::fromString(node_->date_time, kDateTimeFST));

    switch (node->type) {
    case kTypeBranch:
        ui->rBtnBranch->setChecked(true);
        break;
    case kTypeLeaf:
        ui->rBtnLeaf->setChecked(true);
        break;
    case kTypeSupport:
        ui->rBtnSupport->setChecked(true);
        break;
    default:
        break;
    }

    ui->rBtnBranch->setEnabled(type_enable);
    ui->rBtnLeaf->setEnabled(type_enable);
    ui->rBtnSupport->setEnabled(type_enable);
    UpdateColor(QColor(node->color));
}

void EditNodeTask::UpdateColor(QColor color)
{
    if (color.isValid())
        ui->pBtnColor->setStyleSheet(QString(R"(
        background-color: %1;
        border-radius: 2px;
        )")
                .arg(node_->color));
}

void EditNodeTask::RNameEdited(const QString& arg1)
{
    const auto& simplified { arg1.simplified() };
    this->setWindowTitle(parent_path_ + simplified);
    ui->pBtnOk->setEnabled(!simplified.isEmpty() && !name_list_.contains(simplified));
}

void EditNodeTask::on_lineEditName_editingFinished() { node_->name = ui->lineEditName->text(); }

void EditNodeTask::on_lineEditCode_editingFinished() { node_->code = ui->lineEditCode->text(); }

void EditNodeTask::on_lineEditDescription_editingFinished() { node_->description = ui->lineEditDescription->text(); }

void EditNodeTask::on_comboUnit_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    node_->unit = ui->comboUnit->currentData().toInt();
}

void EditNodeTask::on_rBtnDDCI_toggled(bool checked)
{
    if (node_->final_total != 0 && node_->rule != checked) {
        node_->final_total = -node_->final_total;
        node_->initial_total = -node_->initial_total;
    }
    node_->rule = checked;
}

void EditNodeTask::on_plainTextEdit_textChanged() { node_->note = ui->plainTextEdit->toPlainText(); }

void EditNodeTask::on_dSpinBoxUnitCost_editingFinished() { node_->first = ui->dSpinBoxUnitCost->value(); }

void EditNodeTask::on_rBtnLeaf_toggled(bool checked)
{
    if (checked)
        node_->type = kTypeLeaf;
}

void EditNodeTask::on_rBtnBranch_toggled(bool checked)
{
    if (checked)
        node_->type = kTypeBranch;
}

void EditNodeTask::on_rBtnSupport_toggled(bool checked)
{
    if (checked)
        node_->type = kTypeSupport;
}

void EditNodeTask::on_pBtnColor_clicked()
{
    QColor color(node_->color);
    if (!color.isValid())
        color = Qt::white;

    QColor selected_color { QColorDialog::getColor(color, nullptr, tr("Choose Color"), QColorDialog::ShowAlphaChannel) };
    if (selected_color.isValid()) {
        node_->color = selected_color.name(QColor::HexRgb);
        UpdateColor(selected_color);
    }
}

void EditNodeTask::on_chkBoxFinished_checkStateChanged(const Qt::CheckState& arg1) { node_->finished = arg1 == Qt::Checked; }

void EditNodeTask::on_dateTime_editingFinished() { node_->date_time = ui->dateTime->dateTime().toString(kDateTimeFST); }
