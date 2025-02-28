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
    , node_ { params.node }
    , sql_ { params.sql }
    , stakeholder_tree_ { static_cast<TreeModelStakeholder*>(params.stakeholder_tree) }
    , order_table_ { params.order_table }
    , info_node_ { params.section == Section::kSales ? kSales : kPurchase }
    , party_unit_ { params.section == Section::kSales ? std::to_underlying(UnitStakeholder::kCust) : std::to_underlying(UnitStakeholder::kVend) }
    , node_id_ { params.node->id }
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

    IniUnit(params.node->unit);
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

QPointer<TableModel> EditNodeOrder::Model() { return order_table_; }

void EditNodeOrder::RUpdateLeafValue(int /*node_id*/, double initial_delta, double final_delta, double first_delta, double second_delta, double discount_delta)
{
    const double adjusted_final_delta { node_->unit == std::to_underlying(UnitOrder::kIS) ? final_delta : 0.0 };

    node_->first += first_delta;
    node_->second += second_delta;
    node_->initial_total += initial_delta;
    node_->discount += discount_delta;
    node_->final_total += adjusted_final_delta;

    IniLeafValue();

    if (node_id_ != 0) {
        emit SUpdateLeafValue(node_id_, initial_delta, adjusted_final_delta, first_delta, second_delta, discount_delta);
    }
}

void EditNodeOrder::RSyncBool(int node_id, int column, bool value)
{
    if (node_id != node_id_)
        return;

    const TreeEnumOrder kColumn { column };

    if (kColumn == TreeEnumOrder::kFinished)
        emit SSyncBool(node_id_, 0, value);

    SignalBlocker blocker(this);

    switch (kColumn) {
    case TreeEnumOrder::kRule:
        ui->chkBoxRefund->setChecked(value);
        IniLeafValue();
        break;
    case TreeEnumOrder::kFinished: {
        ui->pBtnFinishOrder->setChecked(value);
        ui->pBtnFinishOrder->setText(value ? tr("Edit") : tr("Finish"));
        LockWidgets(value, node_->type == kTypeBranch);

        if (value) {
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

void EditNodeOrder::RSyncInt(int node_id, int column, int value)
{
    if (node_id != node_id_)
        return;

    const TreeEnumOrder kColumn { column };

    SignalBlocker blocker(this);

    switch (kColumn) {
    case TreeEnumOrder::kUnit:
        IniUnit(value);
        break;
    case TreeEnumOrder::kEmployee: {
        int employee_index { ui->comboEmployee->findData(value) };
        ui->comboEmployee->setCurrentIndex(employee_index);
        break;
    }
    default:
        break;
    }
}

void EditNodeOrder::RSyncString(int node_id, int column, const QString& value)
{
    if (node_id != node_id_)
        return;

    const TreeEnumOrder kColumn { column };

    SignalBlocker blocker(this);

    switch (kColumn) {
    case TreeEnumOrder::kDescription:
        ui->lineDescription->setText(value);
        break;
    case TreeEnumOrder::kDateTime:
        ui->dateTimeEdit->setDateTime(QDateTime::fromString(value, kDateTimeFST));
        break;
    default:
        break;
    }
}

QPointer<QTableView> EditNodeOrder::View() { return ui->tableViewOrder; }

void EditNodeOrder::IniDialog(CSettings* settings)
{
    combo_model_party_ = stakeholder_tree_->UnitModelPS(party_unit_);
    ui->comboParty->setModel(combo_model_party_);
    ui->comboParty->setCurrentIndex(-1);

    combo_model_employee_ = stakeholder_tree_->UnitModelPS(std::to_underlying(UnitStakeholder::kEmp));
    ui->comboEmployee->setModel(combo_model_employee_);
    ui->comboEmployee->setCurrentIndex(-1);

    ui->dateTimeEdit->setDisplayFormat(kDateTimeFST);
    ui->dateTimeEdit->setDateTime(QDateTime::currentDateTime());
    node_->date_time = ui->dateTimeEdit->dateTime().toString(kDateTimeFST);
    ui->comboParty->lineEdit()->setValidator(&LineEdit::kInputValidator);

    ui->dSpinDiscount->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    ui->dSpinGrossAmount->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    ui->dSpinNetAmount->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    ui->dSpinSecond->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    ui->dSpinFirst->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());

    ui->dSpinDiscount->setDecimals(settings->amount_decimal);
    ui->dSpinGrossAmount->setDecimals(settings->amount_decimal);
    ui->dSpinNetAmount->setDecimals(settings->amount_decimal);
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
        node_id_ = node_->id;

        if (node_->type == kTypeLeaf)
            emit SSyncInt(node_id_, std::to_underlying(TreeEnumOrder::kID), node_id_);

        ui->chkBoxBranch->setEnabled(false);
        ui->pBtnSaveOrder->setEnabled(false);
        ui->tableViewOrder->clearSelection();

        emit SUpdateLeafValue(node_id_, node_->initial_total, node_->final_total, node_->first, node_->second, node_->discount);
    }
}

void EditNodeOrder::IniConnect() { connect(ui->pBtnSaveOrder, &QPushButton::clicked, this, &EditNodeOrder::accept); }

void EditNodeOrder::LockWidgets(bool finished, bool branch)
{
    bool basic_enable { !finished };
    bool not_branch_enable { !finished && !branch };

    ui->labParty->setEnabled(basic_enable);
    ui->comboParty->setEnabled(basic_enable);

    ui->pBtnInsert->setEnabled(not_branch_enable);

    ui->labNetAmount->setEnabled(not_branch_enable);
    ui->dSpinNetAmount->setEnabled(not_branch_enable);

    ui->dSpinGrossAmount->setEnabled(not_branch_enable);

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
    const UnitOrder kUnit { unit };

    switch (kUnit) {
    case UnitOrder::kIS:
        ui->rBtnCash->setChecked(true);
        break;
    case UnitOrder::kMS:
        ui->rBtnMonthly->setChecked(true);
        break;
    case UnitOrder::kPEND:
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

void EditNodeOrder::IniLeafValue()
{
    ui->dSpinFirst->setValue(node_->first);
    ui->dSpinSecond->setValue(node_->second);
    ui->dSpinGrossAmount->setValue(node_->initial_total);
    ui->dSpinDiscount->setValue(node_->discount);
    ui->dSpinNetAmount->setValue(node_->final_total);
}

void EditNodeOrder::on_comboParty_editTextChanged(const QString& arg1)
{
    if (node_->type != kTypeBranch || arg1.isEmpty())
        return;

    node_->name = arg1;

    if (node_id_ == 0) {
        ui->pBtnSaveOrder->setEnabled(true);
        ui->pBtnFinishOrder->setEnabled(true);
    } else {
        sql_->WriteField(info_node_, kName, arg1, node_id_);
    }
}

void EditNodeOrder::on_comboParty_currentIndexChanged(int /*index*/)
{
    if (node_->type != kTypeLeaf)
        return;

    int party_id { ui->comboParty->currentData().toInt() };
    if (party_id <= 0)
        return;

    node_->party = party_id;
    emit SSyncInt(node_id_, std::to_underlying(TreeEnumOrder::kParty), party_id);

    if (node_id_ == 0) {
        ui->pBtnSaveOrder->setEnabled(true);
        ui->pBtnFinishOrder->setEnabled(true);
    } else {
        sql_->WriteField(info_node_, kParty, party_id, node_id_);
    }

    if (ui->comboEmployee->currentIndex() != -1)
        return;

    int employee_index { ui->comboEmployee->findData(stakeholder_tree_->Employee(party_id)) };
    ui->comboEmployee->setCurrentIndex(employee_index);

    ui->rBtnCash->setChecked(stakeholder_tree_->Rule(party_id) == kRuleIS);
    ui->rBtnMonthly->setChecked(stakeholder_tree_->Rule(party_id) == kRuleMS);
}

void EditNodeOrder::on_chkBoxRefund_toggled(bool checked)
{
    node_->rule = checked;

    node_->first *= -1;
    node_->second *= -1;
    node_->initial_total *= -1;
    node_->discount *= -1;
    node_->final_total *= -1;

    IniLeafValue();

    if (node_id_ != 0) {
        sql_->WriteField(info_node_, kRule, checked, node_id_);
        sql_->WriteLeafValue(node_);
    }
}

void EditNodeOrder::on_comboEmployee_currentIndexChanged(int /*index*/)
{
    node_->employee = ui->comboEmployee->currentData().toInt();

    if (node_id_ != 0)
        sql_->WriteField(info_node_, kEmployee, node_->employee, node_id_);
}

void EditNodeOrder::on_rBtnCash_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = std::to_underlying(UnitOrder::kIS);
    node_->final_total = node_->initial_total - node_->discount;
    ui->dSpinNetAmount->setValue(node_->final_total);

    if (node_id_ != 0) {
        sql_->WriteField(info_node_, kUnit, std::to_underlying(UnitOrder::kIS), node_id_);
        sql_->WriteField(info_node_, kNetAmount, node_->final_total, node_id_);
    }
}

void EditNodeOrder::on_rBtnMonthly_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = std::to_underlying(UnitOrder::kMS);
    node_->final_total = 0.0;
    ui->dSpinNetAmount->setValue(0.0);

    if (node_id_ != 0) {
        sql_->WriteField(info_node_, kUnit, std::to_underlying(UnitOrder::kMS), node_id_);
        sql_->WriteField(info_node_, kNetAmount, 0.0, node_id_);
    }
}

void EditNodeOrder::on_rBtnPending_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = std::to_underlying(UnitOrder::kPEND);
    node_->final_total = 0.0;
    ui->dSpinNetAmount->setValue(0.0);

    if (node_id_ != 0) {
        sql_->WriteField(info_node_, kUnit, std::to_underlying(UnitOrder::kPEND), node_id_);
        sql_->WriteField(info_node_, kNetAmount, 0.0, node_id_);
    }
}

void EditNodeOrder::on_pBtnInsert_clicked()
{
    const auto& name { ui->comboParty->currentText() };
    if (node_->type == kTypeBranch || name.isEmpty() || ui->comboParty->currentIndex() != -1)
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
    node_->date_time = date_time.toString(kDateTimeFST);

    if (node_id_ != 0)
        sql_->WriteField(info_node_, kDateTime, node_->date_time, node_id_);
}

void EditNodeOrder::on_pBtnFinishOrder_toggled(bool checked)
{
    accept();

    node_->finished = checked;

    sql_->WriteField(info_node_, kFinished, checked, node_id_);
    if (node_->type == kTypeLeaf)
        emit SSyncBool(node_id_, std::to_underlying(TreeEnumOrder::kFinished), checked);

    ui->pBtnFinishOrder->setText(checked ? tr("Edit") : tr("Finish"));

    LockWidgets(checked, node_->type == kTypeBranch);

    if (checked) {
        ui->tableViewOrder->clearSelection();
        ui->pBtnPrint->setFocus();
        ui->pBtnPrint->setDefault(true);
    }
}

void EditNodeOrder::on_chkBoxBranch_checkStateChanged(const Qt::CheckState& arg1)
{
    bool enable { arg1 == Qt::Checked };
    node_->type = enable;
    LockWidgets(false, enable);

    ui->comboEmployee->setCurrentIndex(-1);
    ui->comboParty->setCurrentIndex(-1);

    ui->pBtnSaveOrder->setEnabled(false);
    ui->pBtnFinishOrder->setEnabled(false);

    node_->party = 0;
    node_->employee = 0;
    if (enable)
        node_->date_time.clear();
    else
        node_->date_time = ui->dateTimeEdit->dateTime().toString(kDateTimeFST);

    ui->chkBoxRefund->setChecked(false);
    ui->tableViewOrder->clearSelection();
    ui->labParty->setText(enable ? tr("Branch") : tr("Party"));
}

void EditNodeOrder::on_lineDescription_editingFinished()
{
    node_->description = ui->lineDescription->text();

    if (node_id_ != 0)
        sql_->WriteField(info_node_, kDescription, node_->description, node_id_);
}
