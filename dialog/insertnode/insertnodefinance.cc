#include "insertnodefinance.h"

#include "component/enumclass.h"
#include "component/signalblocker.h"
#include "ui_insertnodefinance.h"

InsertNodeFinance::InsertNodeFinance(CEditNodeParamsFPTS& params, QWidget* parent)
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

void InsertNodeFinance::IniConnect() { connect(ui->lineName, &QLineEdit::textEdited, this, &InsertNodeFinance::RNameEdited); }

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

void InsertNodeFinance::on_rBtnDDCI_toggled(bool checked)
{
    if (node_->final_total != 0 && node_->rule != checked) {
        node_->final_total = -node_->final_total;
        node_->initial_total = -node_->initial_total;
    }
    node_->rule = checked;
}

void InsertNodeFinance::on_plainNote_textChanged() { node_->note = ui->plainNote->toPlainText(); }

void InsertNodeFinance::on_rBtnLeaf_toggled(bool checked)
{
    if (checked)
        node_->type = kTypeLeaf;
}

void InsertNodeFinance::on_rBtnBranch_toggled(bool checked)
{
    if (checked)
        node_->type = kTypeBranch;
}

void InsertNodeFinance::on_rBtnSupport_toggled(bool checked)
{
    if (checked)
        node_->type = kTypeSupport;
}
