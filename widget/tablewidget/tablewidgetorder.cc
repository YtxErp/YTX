#include "tablewidgetorder.h"

#include <QTimer>

#include "component/signalblocker.h"
#include "global/resourcepool.h"
#include "ui_tablewidgetorder.h"

TableWidgetOrder::TableWidgetOrder(CEditNodeParamsOrder& params, QWidget* parent)
    : TableWidget(parent)
    , ui(new Ui::TableWidgetOrder)
    , node_ { params.node }
    , sql_ { params.sql }
    , order_table_ { params.order_table }
    , stakeholder_tree_ { static_cast<TreeModelStakeholder*>(params.stakeholder_tree) }
    , settings_ { params.settings }
    , node_id_ { params.node->id }
    , info_node_ { params.section == Section::kSales ? kSales : kPurchase }
    , party_unit_ { params.section == Section::kSales ? std::to_underlying(UnitStakeholder::kCust) : std::to_underlying(UnitStakeholder::kVend) }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    bool finished { params.node->finished };

    IniDialog();
    IniData();
    ui->tableViewOrder->setFocus();
    IniDataCombo(node_->party, node_->employee);

    IniUnit(node_->unit);

    ui->chkBoxBranch->setEnabled(false);

    ui->pBtnFinishOrder->setChecked(finished);
    ui->pBtnFinishOrder->setText(finished ? tr("Edit") : tr("Finish"));
    if (finished)
        ui->pBtnPrint->setFocus();

    LockWidgets(finished);
}

TableWidgetOrder::~TableWidgetOrder() { delete ui; }

QPointer<QTableView> TableWidgetOrder::View() const { return ui->tableViewOrder; }

void TableWidgetOrder::RSyncBool(int node_id, int column, bool value)
{
    if (node_id != node_id_)
        return;

    const TreeEnumOrder kColumn { column };

    if (kColumn == TreeEnumOrder::kFinished)
        emit SSyncBool(node_id_, 0, value); // just send to TableModelOrder, thereby set column to 0

    SignalBlocker blocker(this);

    switch (kColumn) {
    case TreeEnumOrder::kRule:
        ui->chkBoxRefund->setChecked(value);
        IniLeafValue();
        break;
    case TreeEnumOrder::kFinished: {
        ui->pBtnFinishOrder->setChecked(value);
        ui->pBtnFinishOrder->setText(value ? tr("Edit") : tr("Finish"));
        LockWidgets(value);

        if (value) {
            ui->pBtnPrint->setFocus();
            ui->pBtnPrint->setDefault(true);
            ui->tableViewOrder->clearSelection();
        }

        break;
    }
    default:
        break;
    }
}

void TableWidgetOrder::RSyncInt(int node_id, int column, int value)
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

void TableWidgetOrder::RSyncString(int node_id, int column, const QString& value)
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

void TableWidgetOrder::RUpdateLeafValue(int node_id, double initial_delta, double final_delta, double first_delta, double second_delta, double discount_delta)
{
    if (node_id_ != node_id)
        return;

    const double adjusted_final_delta { node_->unit == std::to_underlying(UnitOrder::kIS) ? final_delta : 0.0 };

    node_->first += first_delta;
    node_->second += second_delta;
    node_->initial_total += initial_delta;
    node_->discount += discount_delta;
    node_->final_total += adjusted_final_delta;

    IniLeafValue();

    emit SUpdateLeafValue(node_id_, initial_delta, adjusted_final_delta, first_delta, second_delta, discount_delta);
}

void TableWidgetOrder::IniDialog()
{
    pmodel_ = stakeholder_tree_->UnitModelPS(party_unit_);
    ui->comboParty->setModel(pmodel_);
    ui->comboParty->setCurrentIndex(-1);

    emodel_ = stakeholder_tree_->UnitModelPS(std::to_underlying(UnitStakeholder::kEmp));
    ui->comboEmployee->setModel(emodel_);
    ui->comboEmployee->setCurrentIndex(-1);

    ui->dateTimeEdit->setDisplayFormat(kDateTimeFST);

    ui->dSpinDiscount->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    ui->dSpinGrossAmount->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    ui->dSpinNetAmount->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    ui->dSpinSecond->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    ui->dSpinFirst->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());

    ui->dSpinDiscount->setDecimals(settings_->amount_decimal);
    ui->dSpinGrossAmount->setDecimals(settings_->amount_decimal);
    ui->dSpinNetAmount->setDecimals(settings_->amount_decimal);
    ui->dSpinSecond->setDecimals(settings_->common_decimal);
    ui->dSpinFirst->setDecimals(settings_->common_decimal);
}

void TableWidgetOrder::IniData()
{
    IniLeafValue();

    ui->chkBoxRefund->setChecked(node_->rule);
    ui->chkBoxBranch->setChecked(false);
    ui->lineDescription->setText(node_->description);
    ui->dateTimeEdit->setDateTime(QDateTime::fromString(node_->date_time, kDateTimeFST));
    ui->pBtnFinishOrder->setChecked(node_->finished);

    ui->tableViewOrder->setModel(order_table_);
}

void TableWidgetOrder::IniDataCombo(int party, int employee)
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

void TableWidgetOrder::LockWidgets(bool finished)
{
    const bool enable { !finished };

    ui->labelParty->setEnabled(enable);
    ui->comboParty->setEnabled(enable);

    ui->pBtnInsert->setEnabled(enable);

    ui->labelNetAmount->setEnabled(enable);
    ui->dSpinNetAmount->setEnabled(enable);

    ui->dSpinGrossAmount->setEnabled(enable);

    ui->labelDiscount->setEnabled(enable);
    ui->dSpinDiscount->setEnabled(enable);

    ui->labelEmployee->setEnabled(enable);
    ui->comboEmployee->setEnabled(enable);
    ui->tableViewOrder->setEnabled(enable);

    ui->rBtnCash->setEnabled(enable);
    ui->rBtnMonthly->setEnabled(enable);
    ui->rBtnPending->setEnabled(enable);
    ui->dateTimeEdit->setEnabled(enable);

    ui->dSpinFirst->setEnabled(enable);
    ui->labelFirst->setEnabled(enable);
    ui->dSpinSecond->setEnabled(enable);
    ui->labelSecond->setEnabled(enable);

    ui->chkBoxRefund->setEnabled(enable);
    ui->lineDescription->setEnabled(enable);

    ui->pBtnPrint->setEnabled(finished);
}

void TableWidgetOrder::IniUnit(int unit)
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

void TableWidgetOrder::IniLeafValue()
{
    ui->dSpinNetAmount->setValue(node_->final_total);
    ui->dSpinDiscount->setValue(node_->discount);
    ui->dSpinFirst->setValue(node_->first);
    ui->dSpinSecond->setValue(node_->second);
    ui->dSpinGrossAmount->setValue(node_->initial_total);
}

void TableWidgetOrder::on_comboParty_currentIndexChanged(int /*index*/)
{
    int party_id { ui->comboParty->currentData().toInt() };
    if (party_id <= 0)
        return;

    node_->party = party_id;
    sql_->WriteField(info_node_, kParty, party_id, node_id_);
    emit SSyncInt(node_id_, std::to_underlying(TreeEnumOrder::kParty), party_id);

    if (ui->comboEmployee->currentIndex() != -1)
        return;

    int employee_index { ui->comboEmployee->findData(stakeholder_tree_->Employee(party_id)) };
    ui->comboEmployee->setCurrentIndex(employee_index);

    ui->rBtnCash->setChecked(stakeholder_tree_->Rule(party_id) == kRuleIS);
    ui->rBtnMonthly->setChecked(stakeholder_tree_->Rule(party_id) == kRuleMS);
}

void TableWidgetOrder::on_chkBoxRefund_toggled(bool checked)
{
    node_->rule = checked;

    node_->first *= -1;
    node_->second *= -1;
    node_->initial_total *= -1;
    node_->discount *= -1;
    node_->final_total *= -1;

    IniLeafValue();

    sql_->WriteField(info_node_, kRule, checked, node_id_);
    sql_->WriteLeafValue(node_);
}

void TableWidgetOrder::on_comboEmployee_currentIndexChanged(int /*index*/)
{
    node_->employee = ui->comboEmployee->currentData().toInt();
    sql_->WriteField(info_node_, kEmployee, node_->employee, node_id_);
}

void TableWidgetOrder::on_rBtnCash_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = std::to_underlying(UnitOrder::kIS);
    node_->final_total = node_->initial_total - node_->discount;

    ui->dSpinNetAmount->setValue(node_->final_total);

    sql_->WriteField(info_node_, kUnit, std::to_underlying(UnitOrder::kIS), node_id_);
    sql_->WriteField(info_node_, kNetAmount, node_->final_total, node_id_);
}

void TableWidgetOrder::on_rBtnMonthly_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = std::to_underlying(UnitOrder::kMS);
    node_->final_total = 0.0;

    ui->dSpinNetAmount->setValue(0.0);

    sql_->WriteField(info_node_, kUnit, std::to_underlying(UnitOrder::kMS), node_id_);
    sql_->WriteField(info_node_, kNetAmount, 0.0, node_id_);
}

void TableWidgetOrder::on_rBtnPending_toggled(bool checked)
{
    if (!checked)
        return;

    node_->unit = std::to_underlying(UnitOrder::kPEND);
    node_->final_total = 0.0;

    ui->dSpinNetAmount->setValue(0.0);

    sql_->WriteField(info_node_, kUnit, std::to_underlying(UnitOrder::kPEND), node_id_);
    sql_->WriteField(info_node_, kNetAmount, 0.0, node_id_);
}

void TableWidgetOrder::on_pBtnInsert_clicked()
{
    const auto& name { ui->comboParty->currentText() };
    if (name.isEmpty() || ui->comboParty->currentIndex() != -1)
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

void TableWidgetOrder::on_dateTimeEdit_dateTimeChanged(const QDateTime& date_time)
{
    node_->date_time = date_time.toString(kDateTimeFST);
    sql_->WriteField(info_node_, kDateTime, node_->date_time, node_id_);
}

void TableWidgetOrder::on_lineDescription_editingFinished()
{
    node_->description = ui->lineDescription->text();
    sql_->WriteField(info_node_, kDescription, node_->description, node_id_);
}

void TableWidgetOrder::on_pBtnFinishOrder_toggled(bool checked)
{
    node_->finished = checked;
    sql_->WriteField(info_node_, kFinished, checked, node_id_);
    emit SSyncBool(node_id_, std::to_underlying(TreeEnumOrder::kFinished), checked);

    ui->pBtnFinishOrder->setText(checked ? tr("Edit") : tr("Finish"));

    LockWidgets(checked);

    if (checked) {
        ui->pBtnPrint->setFocus();
        ui->pBtnPrint->setDefault(true);
        ui->tableViewOrder->clearSelection();
    }
}
