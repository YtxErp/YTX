#include "editnodeorder.h"

#include <QShortcut>
#include <QTimer>

#include "component/signalblocker.h"
#include "global/resourcepool.h"
#include "mainwindow.h"
#include "ui_editnodeorder.h"

EditNodeOrder::EditNodeOrder(CEditNodeParamsOrder& params, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::EditNodeOrder)
    , node_shadow_ { params.node_shadow }
    , sql_ { params.sql }
    , stakeholder_tree_ { static_cast<TreeModelStakeholder*>(params.stakeholder_tree) }
    , order_table_ { params.order_table }
    , info_node_ { params.section == Section::kSales ? kSales : kPurchase }
    , party_unit_ { params.section == Section::kSales ? kUnitCust : kUnitVend }
    , node_id_ { *params.node_shadow->id }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    IniDialog(params.settings);
    IniConnect();

    ui->tableViewOrder->setModel(order_table_);
    ui->pBtnSaveOrder->setEnabled(false);
    ui->pBtnFinishOrder->setEnabled(false);

    ui->labParty->setText(tr("Party"));
    ui->comboParty->setFocus();

    IniUnit(*params.node_shadow->unit);
    setWindowTitle(params.section == Section::kSales ? tr("Sales") : tr("Purchase"));

    QShortcut* trans_shortcut { new QShortcut(QKeySequence("Ctrl+N"), this) };
    trans_shortcut->setContext(Qt::WindowShortcut);

    connect(trans_shortcut, &QShortcut::activated, parent, [parent]() {
        auto* main_window { qobject_cast<MainWindow*>(parent) };
        if (main_window) {
            main_window->on_actionAppendTrans_triggered();
        }
    });

    QShortcut* node_shortcut { new QShortcut(QKeySequence("Alt+N"), this) };
    node_shortcut->setContext(Qt::WindowShortcut);

    connect(node_shortcut, &QShortcut::activated, parent, [parent]() {
        auto* main_window { qobject_cast<MainWindow*>(parent) };
        if (main_window) {
            main_window->on_actionInsertNode_triggered();
        }
    });
}

EditNodeOrder::~EditNodeOrder() { delete ui; }

void EditNodeOrder::RUpdateData(int node_id, TreeEnumOrder column, const QVariant& value)
{
    if (node_id != node_id_)
        return;

    SignalBlocker blocker(this);

    switch (column) {
    case TreeEnumOrder::kDescription:
        ui->lineDescription->setText(value.toString());
        break;
    case TreeEnumOrder::kRule:
        ui->chkBoxRefund->setChecked(value.toBool());
        break;
    case TreeEnumOrder::kUnit:
        IniUnit(value.toInt());
        break;
    case TreeEnumOrder::kParty: {
        int party_index { ui->comboParty->findData(value.toInt()) };
        ui->comboParty->setCurrentIndex(party_index);
        break;
    }
    case TreeEnumOrder::kEmployee: {
        int employee_index { ui->comboEmployee->findData(value.toInt()) };
        ui->comboEmployee->setCurrentIndex(employee_index);
        break;
    }
    case TreeEnumOrder::kDateTime:
        ui->dateTimeEdit->setDateTime(QDateTime::fromString(value.toString(), kDateTimeFST));
        break;
    case TreeEnumOrder::kFinished: {
        bool finished { value.toBool() };

        ui->pBtnFinishOrder->setChecked(finished);
        ui->pBtnFinishOrder->setText(finished ? tr("Edit") : tr("Finish"));
        LockWidgets(finished, *node_shadow_->type == kTypeBranch);

        if (finished) {
            ui->pBtnPrint->setFocus();
            ui->pBtnPrint->setDefault(true);
            ui->tableViewOrder->clearSelection();
        }
        break;
    }
    default:
        return;
    }
}

QPointer<TableModel> EditNodeOrder::Model() { return order_table_; }

void EditNodeOrder::RUpdateLeafValueOne(int /*node_id*/, double diff) { ui->dSpinFirst->setValue(ui->dSpinFirst->value() + diff); }

void EditNodeOrder::RUpdateLeafValue(int /*node_id*/, double first_diff, double second_diff, double amount_diff, double discount_diff, double settled_diff)
{
    if (*node_shadow_->type != kTypeLeaf)
        return;

    ui->dSpinFirst->setValue(ui->dSpinFirst->value() + first_diff);
    ui->dSpinSecond->setValue(ui->dSpinSecond->value() + second_diff);
    ui->dSpinAmount->setValue(ui->dSpinAmount->value() + amount_diff);
    ui->dSpinDiscount->setValue(ui->dSpinDiscount->value() + discount_diff);
    ui->dSpinSettled->setValue(ui->dSpinSettled->value() + (*node_shadow_->unit == kUnitIM ? settled_diff : 0.0));
}

QPointer<QTableView> EditNodeOrder::View() { return ui->tableViewOrder; }

void EditNodeOrder::IniDialog(CSettings* settings)
{
    combo_model_party_ = stakeholder_tree_->UnitModelPS(party_unit_);
    ui->comboParty->setModel(combo_model_party_);
    ui->comboParty->setCurrentIndex(-1);

    combo_model_employee_ = stakeholder_tree_->UnitModelPS(kUnitEmp);
    ui->comboEmployee->setModel(combo_model_employee_);
    ui->comboEmployee->setCurrentIndex(-1);

    ui->dateTimeEdit->setDisplayFormat(kDateTimeFST);
    ui->dateTimeEdit->setDateTime(QDateTime::currentDateTime());
    *node_shadow_->date_time = ui->dateTimeEdit->dateTime().toString(kDateTimeFST);
    ui->comboParty->lineEdit()->setValidator(&LineEdit::kInputValidator);

    ui->dSpinDiscount->setRange(std::numeric_limits<double>::min(), std::numeric_limits<double>::max());
    ui->dSpinAmount->setRange(std::numeric_limits<double>::min(), std::numeric_limits<double>::max());
    ui->dSpinSettled->setRange(std::numeric_limits<double>::min(), std::numeric_limits<double>::max());
    ui->dSpinSecond->setRange(std::numeric_limits<double>::min(), std::numeric_limits<double>::max());
    ui->dSpinFirst->setRange(std::numeric_limits<double>::min(), std::numeric_limits<double>::max());

    ui->dSpinDiscount->setDecimals(settings->amount_decimal);
    ui->dSpinAmount->setDecimals(settings->amount_decimal);
    ui->dSpinSettled->setDecimals(settings->amount_decimal);
    ui->dSpinSecond->setDecimals(settings->common_decimal);
    ui->dSpinFirst->setDecimals(settings->common_decimal);

    ui->comboParty->setFocus();
}

void EditNodeOrder::accept()
{
    if (auto* focus_widget { this->focusWidget() })
        focus_widget->clearFocus();

    if (node_id_ == 0) {
        emit QDialog::accepted();
        node_id_ = *node_shadow_->id;

        if (*node_shadow_->type == kTypeLeaf)
            emit SUpdateNodeID(node_id_);

        ui->chkBoxBranch->setEnabled(false);
        ui->pBtnSaveOrder->setEnabled(false);
        ui->tableViewOrder->clearSelection();
    }
}

void EditNodeOrder::IniConnect() { connect(ui->pBtnSaveOrder, &QPushButton::clicked, this, &EditNodeOrder::accept); }

void EditNodeOrder::LockWidgets(bool finished, bool branch)
{
    bool basic_enable { !finished };
    bool not_branch_enable { !finished && !branch };

    ui->labParty->setEnabled(basic_enable);
    ui->comboParty->setEnabled(basic_enable);

    ui->pBtnInsertParty->setEnabled(not_branch_enable);

    ui->labSettled->setEnabled(not_branch_enable);
    ui->dSpinSettled->setEnabled(not_branch_enable);

    ui->dSpinAmount->setEnabled(not_branch_enable);

    ui->labDiscount->setEnabled(not_branch_enable);
    ui->dSpinDiscount->setEnabled(not_branch_enable);

    ui->labEmployee->setEnabled(not_branch_enable);
    ui->comboEmployee->setEnabled(not_branch_enable);
    ui->tableViewOrder->setEnabled(not_branch_enable);

    ui->rBtnCash->setEnabled(basic_enable);
    ui->rBtnMonthly->setEnabled(basic_enable);
    ui->rBtnPending->setEnabled(basic_enable);
    ui->dateTimeEdit->setEnabled(not_branch_enable);

    ui->dSpinFirst->setEnabled(not_branch_enable);
    ui->labFirst->setEnabled(not_branch_enable);
    ui->dSpinSecond->setEnabled(not_branch_enable);
    ui->labSecond->setEnabled(not_branch_enable);

    ui->chkBoxRefund->setEnabled(not_branch_enable);
    ui->lineDescription->setEnabled(basic_enable);

    ui->pBtnPrint->setEnabled(finished && !branch);
}

void EditNodeOrder::IniUnit(int unit)
{
    switch (unit) {
    case kUnitIM:
        ui->rBtnCash->setChecked(true);
        break;
    case kUnitMS:
        ui->rBtnMonthly->setChecked(true);
        break;
    case kUnitPEND:
        ui->rBtnPending->setChecked(true);
        break;
    default:
        break;
    }
}

void EditNodeOrder::IniDataCombo(int party, int employee)
{
    ui->comboEmployee->blockSignals(true);
    ui->comboParty->blockSignals(true);

    int party_index { ui->comboParty->findData(party) };
    ui->comboParty->setCurrentIndex(party_index);

    int employee_index { ui->comboEmployee->findData(employee) };
    ui->comboEmployee->setCurrentIndex(employee_index);

    ui->comboEmployee->blockSignals(false);
    ui->comboParty->blockSignals(false);
}

void EditNodeOrder::on_comboParty_editTextChanged(const QString& arg1)
{
    if (*node_shadow_->type != kTypeBranch || arg1.isEmpty())
        return;

    *node_shadow_->name = arg1;

    if (node_id_ == 0) {
        ui->pBtnSaveOrder->setEnabled(true);
        ui->pBtnFinishOrder->setEnabled(true);
    }

    if (node_id_ != 0)
        sql_->UpdateField(info_node_, arg1, kName, node_id_);
}

void EditNodeOrder::on_comboParty_currentIndexChanged(int /*index*/)
{
    if (*node_shadow_->type != kTypeLeaf)
        return;

    int party_id { ui->comboParty->currentData().toInt() };
    if (party_id <= 0)
        return;

    *node_shadow_->party = party_id;
    emit SUpdateParty(node_id_, party_id);

    if (node_id_ == 0) {
        ui->pBtnSaveOrder->setEnabled(true);
        ui->pBtnFinishOrder->setEnabled(true);
    }

    if (node_id_ != 0)
        sql_->UpdateField(info_node_, party_id, kParty, node_id_);

    if (ui->comboEmployee->currentIndex() != -1)
        return;

    int employee_index { ui->comboEmployee->findData(stakeholder_tree_->Employee(party_id)) };
    ui->comboEmployee->setCurrentIndex(employee_index);

    ui->rBtnCash->setChecked(stakeholder_tree_->Rule(party_id) == kRuleIS);
    ui->rBtnMonthly->setChecked(stakeholder_tree_->Rule(party_id) == kRuleMS);
}

void EditNodeOrder::on_chkBoxRefund_toggled(bool checked)
{
    *node_shadow_->rule = checked;

    if (node_id_ != 0)
        sql_->UpdateField(info_node_, checked, kRule, node_id_);
}

void EditNodeOrder::on_comboEmployee_currentIndexChanged(int /*index*/)
{
    *node_shadow_->employee = ui->comboEmployee->currentData().toInt();

    if (node_id_ != 0)
        sql_->UpdateField(info_node_, *node_shadow_->employee, kEmployee, node_id_);
}

void EditNodeOrder::on_rBtnCash_toggled(bool checked)
{
    if (!checked)
        return;

    *node_shadow_->unit = kUnitIM;

    *node_shadow_->final_total = *node_shadow_->initial_total - *node_shadow_->discount;
    ui->dSpinSettled->setValue(*node_shadow_->final_total);

    if (node_id_ != 0) {
        sql_->UpdateField(info_node_, kUnitIM, kUnit, node_id_);
        sql_->UpdateField(info_node_, *node_shadow_->final_total, kSettled, node_id_);
    }
}

void EditNodeOrder::on_rBtnMonthly_toggled(bool checked)
{
    if (!checked)
        return;

    *node_shadow_->unit = kUnitMS;

    *node_shadow_->final_total = 0.0;
    ui->dSpinSettled->setValue(0.0);

    if (node_id_ != 0) {
        sql_->UpdateField(info_node_, kUnitMS, kUnit, node_id_);
        sql_->UpdateField(info_node_, 0.0, kSettled, node_id_);
    }
}

void EditNodeOrder::on_rBtnPending_toggled(bool checked)
{
    if (!checked)
        return;

    *node_shadow_->unit = kUnitPEND;

    *node_shadow_->final_total = 0.0;
    ui->dSpinSettled->setValue(0.0);

    if (node_id_ != 0) {
        sql_->UpdateField(info_node_, kUnitPEND, kUnit, node_id_);
        sql_->UpdateField(info_node_, 0.0, kSettled, node_id_);
    }
}

void EditNodeOrder::on_pBtnInsertParty_clicked()
{
    const auto& name { ui->comboParty->currentText() };
    if (*node_shadow_->type == kTypeBranch || name.isEmpty() || ui->comboParty->currentIndex() != -1)
        return;

    auto* node { ResourcePool<Node>::Instance().Allocate() };
    node->rule = stakeholder_tree_->Rule(-1);
    stakeholder_tree_->SetParent(node, -1);
    node->name = name;

    node->unit = party_unit_;

    stakeholder_tree_->InsertNode(0, QModelIndex(), node);

    int party_index { ui->comboParty->findData(node->id) };
    ui->comboParty->setCurrentIndex(party_index);
}

void EditNodeOrder::on_dateTimeEdit_dateTimeChanged(const QDateTime& date_time)
{
    *node_shadow_->date_time = date_time.toString(kDateTimeFST);

    if (node_id_ != 0)
        sql_->UpdateField(info_node_, *node_shadow_->date_time, kDateTime, node_id_);
}

void EditNodeOrder::on_pBtnFinishOrder_toggled(bool checked)
{
    accept();

    *node_shadow_->finished = checked;

    sql_->UpdateField(info_node_, checked, kFinished, node_id_);
    if (*node_shadow_->type == kTypeLeaf)
        emit SUpdateFinished(node_id_, checked);

    ui->pBtnFinishOrder->setText(checked ? tr("Edit") : tr("Finish"));

    LockWidgets(checked, *node_shadow_->type == kTypeBranch);

    if (checked) {
        ui->tableViewOrder->clearSelection();
        ui->pBtnPrint->setFocus();
        ui->pBtnPrint->setDefault(true);
    }
}

void EditNodeOrder::on_chkBoxBranch_checkStateChanged(const Qt::CheckState& arg1)
{
    bool enable { arg1 == Qt::Checked };
    *node_shadow_->type = enable;
    LockWidgets(false, enable);

    ui->comboEmployee->setCurrentIndex(-1);
    ui->comboParty->setCurrentIndex(-1);

    ui->pBtnSaveOrder->setEnabled(false);
    ui->pBtnFinishOrder->setEnabled(false);

    *node_shadow_->party = 0;
    *node_shadow_->employee = 0;
    if (enable)
        node_shadow_->date_time->clear();
    else
        *node_shadow_->date_time = ui->dateTimeEdit->dateTime().toString(kDateTimeFST);

    ui->chkBoxRefund->setChecked(false);
    ui->tableViewOrder->clearSelection();
    ui->labParty->setText(enable ? tr("Branch") : tr("Party"));
}

void EditNodeOrder::on_lineDescription_editingFinished()
{
    *node_shadow_->description = ui->lineDescription->text();

    if (node_id_ != 0)
        sql_->UpdateField(info_node_, *node_shadow_->description, kDescription, node_id_);
}
