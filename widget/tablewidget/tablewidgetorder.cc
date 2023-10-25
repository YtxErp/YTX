#include "tablewidgetorder.h"

#include <QTimer>

#include "component/signalblocker.h"
#include "global/resourcepool.h"
#include "ui_tablewidgetorder.h"

TableWidgetOrder::TableWidgetOrder(CEditNodeParamsOrder& params, QWidget* parent)
    : TableWidget(parent)
    , ui(new Ui::TableWidgetOrder)
    , node_shadow_ { params.node_shadow }
    , sql_ { params.sql }
    , order_table_ { params.order_table }
    , stakeholder_tree_ { static_cast<TreeModelStakeholder*>(params.stakeholder_tree) }
    , settings_ { params.settings }
    , node_id_ { *params.node_shadow->id }
    , info_node_ { params.section == Section::kSales ? kSales : kPurchase }
    , party_unit_ { params.section == Section::kSales ? kUnitCust : kUnitVend }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    bool finished { *params.node_shadow->finished };

    IniDialog();
    IniData();
    ui->tableViewOrder->setFocus();
    IniDataCombo(*node_shadow_->party, *node_shadow_->employee);

    IniUnit(*node_shadow_->unit);

    ui->chkBoxBranch->setEnabled(false);

    ui->pBtnFinishOrder->setChecked(finished);
    ui->pBtnFinishOrder->setText(finished ? tr("Edit") : tr("Finish"));
    if (finished)
        ui->pBtnPrint->setFocus();

    LockWidgets(finished);
}

TableWidgetOrder::~TableWidgetOrder()
{
    ResourcePool<NodeShadow>::Instance().Recycle(node_shadow_);
    delete ui;
}

QPointer<QTableView> TableWidgetOrder::View() const { return ui->tableViewOrder; }

void TableWidgetOrder::RUpdateData(int node_id, TreeEnumOrder column, const QVariant& value)
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
        LockWidgets(finished);

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

void TableWidgetOrder::RUpdateLeafValueOne(int /*node_id*/, double diff) { ui->dSpinFirst->setValue(ui->dSpinFirst->value() + diff); }

void TableWidgetOrder::RUpdateLeafValue(int /*node_id*/, double first_diff, double second_diff, double amount_diff, double discount_diff, double settled_diff)
{
    ui->dSpinFirst->setValue(ui->dSpinFirst->value() + first_diff);
    ui->dSpinSecond->setValue(ui->dSpinSecond->value() + second_diff);
    ui->dSpinAmount->setValue(ui->dSpinAmount->value() + amount_diff);
    ui->dSpinDiscount->setValue(ui->dSpinDiscount->value() + discount_diff);
    ui->dSpinSettled->setValue(ui->dSpinSettled->value() + (*node_shadow_->unit == kUnitIM ? settled_diff : 0.0));
}

void TableWidgetOrder::IniDialog()
{
    pmodel_ = stakeholder_tree_->UnitModelPS(party_unit_);
    ui->comboParty->setModel(pmodel_);
    ui->comboParty->setCurrentIndex(-1);

    emodel_ = stakeholder_tree_->UnitModelPS(kUnitEmp);
    ui->comboEmployee->setModel(emodel_);
    ui->comboEmployee->setCurrentIndex(-1);

    ui->dateTimeEdit->setDisplayFormat(kDateTimeFST);

    ui->dSpinDiscount->setRange(std::numeric_limits<double>::min(), std::numeric_limits<double>::max());
    ui->dSpinAmount->setRange(std::numeric_limits<double>::min(), std::numeric_limits<double>::max());
    ui->dSpinSettled->setRange(std::numeric_limits<double>::min(), std::numeric_limits<double>::max());
    ui->dSpinSecond->setRange(std::numeric_limits<double>::min(), std::numeric_limits<double>::max());
    ui->dSpinFirst->setRange(std::numeric_limits<double>::min(), std::numeric_limits<double>::max());

    ui->dSpinDiscount->setDecimals(settings_->amount_decimal);
    ui->dSpinAmount->setDecimals(settings_->amount_decimal);
    ui->dSpinSettled->setDecimals(settings_->amount_decimal);
    ui->dSpinSecond->setDecimals(settings_->common_decimal);
    ui->dSpinFirst->setDecimals(settings_->common_decimal);
}

void TableWidgetOrder::IniData()
{
    ui->dSpinSettled->setValue(*node_shadow_->final_total);
    ui->dSpinDiscount->setValue(*node_shadow_->discount);
    ui->dSpinFirst->setValue(*node_shadow_->first);
    ui->dSpinSecond->setValue(*node_shadow_->second);
    ui->dSpinAmount->setValue(*node_shadow_->initial_total);

    ui->chkBoxRefund->setChecked(*node_shadow_->rule);
    ui->chkBoxBranch->setChecked(false);
    ui->lineDescription->setText(*node_shadow_->description);
    ui->dateTimeEdit->setDateTime(QDateTime::fromString(*node_shadow_->date_time, kDateTimeFST));
    ui->pBtnFinishOrder->setChecked(*node_shadow_->finished);

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

    ui->pBtnInsertParty->setEnabled(enable);

    ui->labelSettled->setEnabled(enable);
    ui->dSpinSettled->setEnabled(enable);

    ui->dSpinAmount->setEnabled(enable);

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

void TableWidgetOrder::on_comboParty_currentIndexChanged(int /*index*/)
{
    int party_id { ui->comboParty->currentData().toInt() };
    if (party_id <= 0)
        return;

    *node_shadow_->party = party_id;
    sql_->UpdateField(info_node_, party_id, kParty, node_id_);
    emit SUpdateParty(node_id_, party_id);

    if (ui->comboEmployee->currentIndex() != -1)
        return;

    int employee_index { ui->comboEmployee->findData(stakeholder_tree_->Employee(party_id)) };
    ui->comboEmployee->setCurrentIndex(employee_index);

    ui->rBtnCash->setChecked(stakeholder_tree_->Rule(party_id) == kRuleIS);
    ui->rBtnMonthly->setChecked(stakeholder_tree_->Rule(party_id) == kRuleMS);
}

void TableWidgetOrder::on_chkBoxRefund_toggled(bool checked)
{
    *node_shadow_->rule = checked;
    sql_->UpdateField(info_node_, checked, kRule, node_id_);
}

void TableWidgetOrder::on_comboEmployee_currentIndexChanged(int /*index*/)
{
    *node_shadow_->employee = ui->comboEmployee->currentData().toInt();
    sql_->UpdateField(info_node_, *node_shadow_->employee, kEmployee, node_id_);
}

void TableWidgetOrder::on_rBtnCash_toggled(bool checked)
{
    if (!checked)
        return;

    *node_shadow_->unit = kUnitIM;

    *node_shadow_->final_total = *node_shadow_->initial_total - *node_shadow_->discount;
    ui->dSpinSettled->setValue(*node_shadow_->final_total);

    sql_->UpdateField(info_node_, kUnitIM, kUnit, node_id_);
    sql_->UpdateField(info_node_, *node_shadow_->final_total, kSettled, node_id_);
}

void TableWidgetOrder::on_rBtnMonthly_toggled(bool checked)
{
    if (!checked)
        return;

    *node_shadow_->unit = kUnitMS;

    *node_shadow_->final_total = 0.0;
    ui->dSpinSettled->setValue(0.0);

    sql_->UpdateField(info_node_, kUnitMS, kUnit, node_id_);
    sql_->UpdateField(info_node_, 0.0, kSettled, node_id_);
}

void TableWidgetOrder::on_rBtnPending_toggled(bool checked)
{
    if (!checked)
        return;

    *node_shadow_->unit = kUnitPEND;

    *node_shadow_->final_total = 0.0;
    ui->dSpinSettled->setValue(0.0);

    sql_->UpdateField(info_node_, kUnitPEND, kUnit, node_id_);
    sql_->UpdateField(info_node_, 0.0, kSettled, node_id_);
}

void TableWidgetOrder::on_pBtnInsertParty_clicked()
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
    *node_shadow_->date_time = date_time.toString(kDateTimeFST);
    sql_->UpdateField(info_node_, *node_shadow_->date_time, kDateTime, node_id_);
}

void TableWidgetOrder::on_lineDescription_editingFinished()
{
    *node_shadow_->description = ui->lineDescription->text();
    sql_->UpdateField(info_node_, *node_shadow_->description, kDescription, node_id_);
}

void TableWidgetOrder::on_pBtnFinishOrder_toggled(bool checked)
{
    *node_shadow_->finished = checked;
    sql_->UpdateField(info_node_, checked, kFinished, node_id_);
    emit SUpdateFinished(node_id_, checked);

    ui->pBtnFinishOrder->setText(checked ? tr("Edit") : tr("Finish"));

    LockWidgets(checked);

    if (checked) {
        ui->pBtnPrint->setFocus();
        ui->pBtnPrint->setDefault(true);
        ui->tableViewOrder->clearSelection();
    }
}
