#include "insertnodefinance.h"

#include "component/signalblocker.h"
#include "ui_insertnodefinance.h"

InsertNodeFinance::InsertNodeFinance(CInsertNodeParamsFPTS& params, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::InsertNodeFinance)
    , node_ { params.node }
    , parent_path_ { params.parent_path }
    , name_list_ { params.name_list }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    IniDialog(params.unit_model);
    IniData(params.node);
    IniRuleGroup();
    IniTypeGroup();
    IniConnect();
}

InsertNodeFinance::~InsertNodeFinance() { delete ui; }

void InsertNodeFinance::IniDialog(QStandardItemModel* unit_model)
{
    ui->lineName->setFocus();
    ui->lineName->setValidator(&LineEdit::kInputValidator);

    this->setWindowTitle(parent_path_ + node_->name);
    this->setFixedSize(300, 500);

    ui->comboUnit->setModel(unit_model);
}

void InsertNodeFinance::IniData(Node* node)
{
    int item_index { ui->comboUnit->findData(node->unit) };
    ui->comboUnit->setCurrentIndex(item_index);

    ui->rBtnDDCI->setChecked(node->rule == kRuleDDCI);
    ui->rBtnDICD->setChecked(node->rule == kRuleDICD);

    ui->rBtnLeaf->setChecked(true);
    ui->pBtnOk->setEnabled(false);
}

void InsertNodeFinance::IniConnect()
{
    connect(ui->lineName, &QLineEdit::textEdited, this, &InsertNodeFinance::RNameEdited);
    connect(rule_group_, &QButtonGroup::idClicked, this, &InsertNodeFinance::RRuleGroupClicked);
    connect(type_group_, &QButtonGroup::idClicked, this, &InsertNodeFinance::RTypeGroupClicked);
}

void InsertNodeFinance::IniTypeGroup()
{
    type_group_ = new QButtonGroup(this);
    type_group_->addButton(ui->rBtnLeaf, 0);
    type_group_->addButton(ui->rBtnBranch, 1);
    type_group_->addButton(ui->rBtnSupport, 2);
}

void InsertNodeFinance::IniRuleGroup()
{
    rule_group_ = new QButtonGroup(this);
    rule_group_->addButton(ui->rBtnDICD, 0);
    rule_group_->addButton(ui->rBtnDDCI, 1);
}

void InsertNodeFinance::RNameEdited(const QString& arg1)
{
    const auto& simplified { arg1.simplified() };
    this->setWindowTitle(parent_path_ + simplified);
    ui->pBtnOk->setEnabled(!simplified.isEmpty() && !name_list_.contains(simplified));
}

void InsertNodeFinance::on_lineName_editingFinished() { node_->name = ui->lineName->text(); }

void InsertNodeFinance::on_lineCode_editingFinished() { node_->code = ui->lineCode->text(); }

void InsertNodeFinance::on_lineDescription_editingFinished() { node_->description = ui->lineDescription->text(); }

void InsertNodeFinance::on_comboUnit_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    node_->unit = ui->comboUnit->currentData().toInt();
}

void InsertNodeFinance::RRuleGroupClicked(int id)
{
    const bool kRule { static_cast<bool>(id) };
    node_->rule = kRule;
}

void InsertNodeFinance::RTypeGroupClicked(int id) { node_->type = id; }

void InsertNodeFinance::on_plainNote_textChanged() { node_->note = ui->plainNote->toPlainText(); }
