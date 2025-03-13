#include "insertnodestakeholder.h"

#include "component/constvalue.h"
#include "component/signalblocker.h"
#include "ui_insertnodestakeholder.h"

InsertNodeStakeholder::InsertNodeStakeholder(CInsertNodeArgFPTS& arg, QStandardItemModel* employee_model, int amount_decimal, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::InsertNodeStakeholder)
    , node_ { arg.node }
    , parent_path_ { arg.parent_path }
    , name_list_ { arg.name_list }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    IniDialog(arg.unit_model, employee_model, amount_decimal);
    IniTypeGroup();
    IniData();
    IniConnect();
}

InsertNodeStakeholder::~InsertNodeStakeholder() { delete ui; }

void InsertNodeStakeholder::IniDialog(QStandardItemModel* unit_model, QStandardItemModel* employee_model, int amount_decimal)
{
    ui->lineEditName->setFocus();
    ui->lineEditName->setValidator(&LineEdit::kInputValidator);

    this->setWindowTitle(parent_path_);
    this->setFixedSize(350, 650);

    ui->comboUnit->setModel(unit_model);
    ui->comboEmployee->setModel(employee_model);
    ui->comboEmployee->setCurrentIndex(0);

    ui->dSpinPaymentPeriod->setRange(0, std::numeric_limits<int>::max());
    ui->dSpinTaxRate->setRange(0.0, std::numeric_limits<double>::max());
    ui->dSpinTaxRate->setDecimals(amount_decimal);

    ui->deadline->setDateTime(QDateTime::fromString(node_->date_time, kDateTimeFST));
    ui->deadline->setDisplayFormat(kDD);
    ui->deadline->setCalendarPopup(true);
}

void InsertNodeStakeholder::IniConnect()
{
    connect(ui->lineEditName, &QLineEdit::textEdited, this, &InsertNodeStakeholder::RNameEdited);
    connect(type_group_, &QButtonGroup::idClicked, this, &InsertNodeStakeholder::RTypeGroupClicked);
}

void InsertNodeStakeholder::IniData()
{
    int unit_index { ui->comboUnit->findData(node_->unit) };
    ui->comboUnit->setCurrentIndex(unit_index);

    ui->rBtnLeaf->setChecked(true);
    ui->pBtnOk->setEnabled(false);
}

void InsertNodeStakeholder::IniTypeGroup()
{
    type_group_ = new QButtonGroup(this);
    type_group_->addButton(ui->rBtnLeaf, 0);
    type_group_->addButton(ui->rBtnBranch, 1);
    type_group_->addButton(ui->rBtnSupport, 2);
}

void InsertNodeStakeholder::RNameEdited(const QString& arg1)
{
    const auto& simplified { arg1.simplified() };
    this->setWindowTitle(parent_path_ + simplified);
    ui->pBtnOk->setEnabled(!simplified.isEmpty() && !name_list_.contains(simplified));
}

void InsertNodeStakeholder::on_lineEditName_editingFinished() { node_->name = ui->lineEditName->text(); }

void InsertNodeStakeholder::on_lineEditCode_editingFinished() { node_->code = ui->lineEditCode->text(); }

void InsertNodeStakeholder::on_lineEditDescription_editingFinished() { node_->description = ui->lineEditDescription->text(); }

void InsertNodeStakeholder::on_plainTextEdit_textChanged() { node_->note = ui->plainTextEdit->toPlainText(); }

void InsertNodeStakeholder::on_comboUnit_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    node_->unit = ui->comboUnit->currentData().toInt();
}

void InsertNodeStakeholder::on_dSpinPaymentPeriod_editingFinished() { node_->first = ui->dSpinPaymentPeriod->value(); }

void InsertNodeStakeholder::on_dSpinTaxRate_editingFinished() { node_->second = ui->dSpinTaxRate->value() / kHundred; }

void InsertNodeStakeholder::on_comboEmployee_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    node_->employee = ui->comboEmployee->currentData().toInt();
}

void InsertNodeStakeholder::on_deadline_editingFinished() { node_->date_time = ui->deadline->dateTime().toString(kDateTimeFST); }

void InsertNodeStakeholder::RRuleGroupClicked(int id) { node_->rule = static_cast<bool>(id); }

void InsertNodeStakeholder::RTypeGroupClicked(int id) { node_->type = id; }
