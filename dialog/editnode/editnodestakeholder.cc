#include "editnodestakeholder.h"

#include "component/constvalue.h"
#include "component/enumclass.h"
#include "component/signalblocker.h"
#include "ui_editnodestakeholder.h"

EditNodeStakeholder::EditNodeStakeholder(CEditNodeParamsFPTS& params, QStandardItemModel* employee_model, int amount_decimal, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::EditNodeStakeholder)
    , node_ { params.node }
    , parent_path_ { params.parent_path }
    , name_list_ { params.name_list }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    IniDialog(params.unit_model, employee_model, amount_decimal);
    IniConnect();
    IniData(params.node, params.type_enable, params.unit_enable);
}

EditNodeStakeholder::~EditNodeStakeholder() { delete ui; }

void EditNodeStakeholder::IniDialog(QStandardItemModel* unit_model, QStandardItemModel* employee_model, int amount_decimal)
{
    ui->lineEditName->setFocus();
    ui->lineEditName->setValidator(&LineEdit::kInputValidator);

    this->setWindowTitle(parent_path_ + node_->name);
    this->setFixedSize(350, 650);

    ui->comboUnit->setModel(unit_model);
    ui->comboEmployee->setModel(employee_model);
    ui->comboEmployee->setCurrentIndex(0);

    ui->dSpinPaymentPeriod->setRange(0, std::numeric_limits<int>::max());
    ui->dSpinTaxRate->setRange(0.0, std::numeric_limits<double>::max());
    ui->dSpinTaxRate->setDecimals(amount_decimal);

    ui->deadline->setDateTime(QDateTime::currentDateTime());
    ui->deadline->setDisplayFormat(kDD);
    ui->deadline->setCalendarPopup(true);
}

void EditNodeStakeholder::IniConnect() { connect(ui->lineEditName, &QLineEdit::textEdited, this, &EditNodeStakeholder::RNameEdited); }

void EditNodeStakeholder::IniData(Node* node, bool type_enable, bool unit_enable)
{
    int unit_index { ui->comboUnit->findData(node_->unit) };
    ui->comboUnit->setCurrentIndex(unit_index);
    ui->comboUnit->setEnabled(unit_enable);

    ui->rBtnMonthly->setChecked(node->rule == kRuleMS);
    ui->rBtnImmediate->setChecked(node->rule == kRuleIS);
    ui->rBtnLeaf->setChecked(true);

    if (node->name.isEmpty()) {
        ui->pBtnOk->setEnabled(false);
        return;
    }

    int employee_index { ui->comboEmployee->findData(node->employee) };
    ui->comboEmployee->setCurrentIndex(employee_index);

    ui->lineEditName->setText(node->name);
    ui->lineEditCode->setText(node->code);
    ui->lineEditDescription->setText(node->description);
    ui->plainTextEdit->setPlainText(node->note);
    ui->dSpinPaymentPeriod->setValue(node->first);
    ui->dSpinTaxRate->setValue(node->second * kHundred);
    ui->deadline->setDateTime(QDateTime::fromString(node->date_time, kDateTimeFST));

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
}

void EditNodeStakeholder::RNameEdited(const QString& arg1)
{
    const auto& simplified { arg1.simplified() };
    this->setWindowTitle(parent_path_ + simplified);
    ui->pBtnOk->setEnabled(!simplified.isEmpty() && !name_list_.contains(simplified));
}

void EditNodeStakeholder::on_lineEditName_editingFinished() { node_->name = ui->lineEditName->text(); }

void EditNodeStakeholder::on_lineEditCode_editingFinished() { node_->code = ui->lineEditCode->text(); }

void EditNodeStakeholder::on_lineEditDescription_editingFinished() { node_->description = ui->lineEditDescription->text(); }

void EditNodeStakeholder::on_plainTextEdit_textChanged() { node_->note = ui->plainTextEdit->toPlainText(); }

void EditNodeStakeholder::on_comboUnit_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    node_->unit = ui->comboUnit->currentData().toInt();
}

void EditNodeStakeholder::on_dSpinPaymentPeriod_editingFinished() { node_->first = ui->dSpinPaymentPeriod->value(); }

void EditNodeStakeholder::on_dSpinTaxRate_editingFinished() { node_->second = ui->dSpinTaxRate->value() / kHundred; }

void EditNodeStakeholder::on_comboEmployee_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    node_->employee = ui->comboEmployee->currentData().toInt();
}

void EditNodeStakeholder::on_rBtnMonthly_toggled(bool checked) { node_->rule = checked; }

void EditNodeStakeholder::on_deadline_editingFinished() { node_->date_time = ui->deadline->dateTime().toString(kDateTimeFST); }

void EditNodeStakeholder::on_rBtnLeaf_toggled(bool checked)
{
    if (checked)
        node_->type = kTypeLeaf;
}

void EditNodeStakeholder::on_rBtnBranch_toggled(bool checked)
{
    if (checked)
        node_->type = kTypeBranch;
}

void EditNodeStakeholder::on_rBtnSupport_toggled(bool checked)
{
    if (checked)
        node_->type = kTypeSupport;
}
