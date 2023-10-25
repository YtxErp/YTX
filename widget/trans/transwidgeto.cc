#include "transwidgeto.h"

#include <QTimer>

#include "component/signalblocker.h"
#include "global/resourcepool.h"
#include "ui_transwidgeto.h"

TransWidgetO::TransWidgetO(CEditNodeParamsO& params, QWidget* parent)
    : TransWidget(parent)
    , ui(new Ui::TransWidgetO)
    , node_ { params.node }
    , sql_ { params.sql }
    , order_table_ { params.order_table }
    , stakeholder_tree_ { static_cast<NodeModelS*>(params.stakeholder_tree) }
    , settings_ { params.settings }
    , node_id_ { params.node->id }
    , info_node_ { params.section == Section::kSales ? kSales : kPurchase }
    , party_unit_ { params.section == Section::kSales ? std::to_underlying(UnitS::kCust) : std::to_underlying(UnitS::kVend) }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    IniWidget();
    IniText(params.section);
    IniUnit(params.node->unit);
    IniRuleGroup();
    IniUnitGroup();
    IniRule(params.node->rule);
    IniData();
    IniDataCombo(params.node->party, params.node->employee);
    IniConnect();

    const bool finished { params.node->finished };
    IniFinished(finished);
    LockWidgets(finished);
}

TransWidgetO::~TransWidgetO()
{
    delete order_table_;
    delete ui;
}

QPointer<QTableView> TransWidgetO::View() const { return ui->tableViewO; }

void TransWidgetO::RSyncBool(int node_id, int column, bool value)
{
    if (node_id != node_id_)
        return;

    const NodeEnumO kColumn { column };

    if (kColumn == NodeEnumO::kFinished)
        emit SSyncBool(node_id_, 0, value); // just send to TableModelOrder, thereby set column to 0

    SignalBlocker blocker(this);

    switch (kColumn) {
    case NodeEnumO::kRule:
        IniRule(value);
        IniLeafValue();
        break;
    case NodeEnumO::kFinished:
        IniFinished(value);
        LockWidgets(value);
        break;
    default:
        break;
    }
}

void TransWidgetO::RSyncInt(int node_id, int column, int value)
{
    if (node_id != node_id_)
        return;

    const NodeEnumO kColumn { column };

    SignalBlocker blocker(this);

    switch (kColumn) {
    case NodeEnumO::kUnit:
        IniUnit(value);
        break;
    case NodeEnumO::kEmployee: {
        int employee_index { ui->comboEmployee->findData(value) };
        ui->comboEmployee->setCurrentIndex(employee_index);
        break;
    }
    default:
        break;
    }
}

void TransWidgetO::RSyncString(int node_id, int column, const QString& value)
{
    if (node_id != node_id_)
        return;

    const NodeEnumO kColumn { column };

    SignalBlocker blocker(this);

    switch (kColumn) {
    case NodeEnumO::kDescription:
        ui->lineDescription->setText(value);
        break;
    case NodeEnumO::kDateTime:
        ui->dateTimeEdit->setDateTime(QDateTime::fromString(value, kDateTimeFST));
        break;
    default:
        break;
    }
}

void TransWidgetO::RUpdateLeafValue(int node_id, double initial_delta, double final_delta, double first_delta, double second_delta, double discount_delta)
{
    if (node_id_ != node_id)
        return;

    // In OrderRule, RO:1, SO and PO:0
    const int coefficient { node_->rule ? -1 : 1 };

    const double adjusted_initial_delta { initial_delta * coefficient };
    const double adjusted_final_delta { (node_->unit == std::to_underlying(UnitO::kIS) ? final_delta : 0.0) * coefficient };
    const double adjusted_first_delta { first_delta * coefficient };
    const double adjusted_second_delta { second_delta * coefficient };
    const double adjusted_discount_delta { discount_delta * coefficient };

    node_->first += adjusted_first_delta;
    node_->second += adjusted_second_delta;
    node_->initial_total += adjusted_initial_delta;
    node_->discount += adjusted_discount_delta;
    node_->final_total += adjusted_final_delta;

    IniLeafValue();

    emit SUpdateLeafValue(node_id_, adjusted_initial_delta, adjusted_final_delta, adjusted_first_delta, adjusted_second_delta, adjusted_discount_delta);
}

void TransWidgetO::IniWidget()
{
    pmodel_ = stakeholder_tree_->UnitModelPS(party_unit_);
    ui->comboParty->setModel(pmodel_);

    emodel_ = stakeholder_tree_->UnitModelPS(std::to_underlying(UnitS::kEmp));
    ui->comboEmployee->setModel(emodel_);

    ui->dateTimeEdit->setDisplayFormat(kDateTimeFST);

    ui->dSpinDiscount->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    ui->dSpinGrossAmount->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    ui->dSpinSettlement->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    ui->dSpinSecond->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    ui->dSpinFirst->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());

    ui->dSpinDiscount->setDecimals(settings_->amount_decimal);
    ui->dSpinGrossAmount->setDecimals(settings_->amount_decimal);
    ui->dSpinSettlement->setDecimals(settings_->amount_decimal);
    ui->dSpinSecond->setDecimals(settings_->common_decimal);
    ui->dSpinFirst->setDecimals(settings_->common_decimal);

    ui->tableViewO->setFocus();

    ui->chkBoxBranch->setEnabled(false);
}

void TransWidgetO::IniData()
{
    IniLeafValue();

    ui->rBtnRefund->setChecked(node_->rule);
    ui->chkBoxBranch->setChecked(false);
    ui->lineDescription->setText(node_->description);
    ui->dateTimeEdit->setDateTime(QDateTime::fromString(node_->date_time, kDateTimeFST));
    ui->pBtnFinishOrder->setChecked(node_->finished);

    ui->tableViewO->setModel(order_table_);
}

void TransWidgetO::IniConnect()
{
    connect(rule_group_, &QButtonGroup::idClicked, this, &TransWidgetO::RRuleGroupClicked);
    connect(unit_group_, &QButtonGroup::idClicked, this, &TransWidgetO::RUnitGroupClicked);
}

void TransWidgetO::IniDataCombo(int party, int employee)
{
    int party_index { ui->comboParty->findData(party) };
    ui->comboParty->setCurrentIndex(party_index);

    int employee_index { ui->comboEmployee->findData(employee) };
    ui->comboEmployee->setCurrentIndex(employee_index);
}

void TransWidgetO::LockWidgets(bool finished)
{
    const bool enable { !finished };

    ui->labParty->setEnabled(enable);
    ui->comboParty->setEnabled(enable);

    ui->pBtnInsert->setEnabled(enable);

    ui->labelSettlement->setEnabled(enable);
    ui->dSpinSettlement->setEnabled(enable);

    ui->dSpinGrossAmount->setEnabled(enable);

    ui->labelDiscount->setEnabled(enable);
    ui->dSpinDiscount->setEnabled(enable);

    ui->labelEmployee->setEnabled(enable);
    ui->comboEmployee->setEnabled(enable);
    ui->tableViewO->setEnabled(enable);

    ui->rBtnIS->setEnabled(enable);
    ui->rBtnMS->setEnabled(enable);
    ui->rBtnPEND->setEnabled(enable);
    ui->dateTimeEdit->setEnabled(enable);

    ui->dSpinFirst->setEnabled(enable);
    ui->labelFirst->setEnabled(enable);
    ui->dSpinSecond->setEnabled(enable);
    ui->labelSecond->setEnabled(enable);

    ui->rBtnRefund->setEnabled(enable);
    ui->rBtnSP->setEnabled(enable);
    ui->lineDescription->setEnabled(enable);

    ui->pBtnPrint->setEnabled(finished);
}

void TransWidgetO::IniUnit(int unit)
{
    const UnitO kUnit { unit };

    switch (kUnit) {
    case UnitO::kIS:
        ui->rBtnIS->setChecked(true);
        break;
    case UnitO::kMS:
        ui->rBtnMS->setChecked(true);
        break;
    case UnitO::kPEND:
        ui->rBtnPEND->setChecked(true);
        break;
    default:
        break;
    }
}

void TransWidgetO::IniLeafValue()
{
    ui->dSpinSettlement->setValue(node_->final_total);
    ui->dSpinDiscount->setValue(node_->discount);
    ui->dSpinFirst->setValue(node_->first);
    ui->dSpinSecond->setValue(node_->second);
    ui->dSpinGrossAmount->setValue(node_->initial_total);
}

void TransWidgetO::IniText(Section section)
{
    const bool is_sales_section { section == Section::kSales };

    setWindowTitle(is_sales_section ? tr("Sales") : tr("Purchase"));
    ui->rBtnSP->setText(is_sales_section ? tr("SO") : tr("PO"));
    ui->labParty->setText(is_sales_section ? tr("CUST") : tr("VEND"));
}

void TransWidgetO::IniRule(bool rule)
{
    const int kRule { static_cast<int>(rule) };

    switch (kRule) {
    case 0:
        ui->rBtnSP->setChecked(true);
        break;
    case 1:
        ui->rBtnRefund->setChecked(true);
        break;
    default:
        break;
    }
}

void TransWidgetO::IniFinished(bool finished)
{
    ui->pBtnFinishOrder->setChecked(finished);
    ui->pBtnFinishOrder->setText(finished ? tr("Edit") : tr("Finish"));
    ui->pBtnFinishOrder->setEnabled(node_->unit != std::to_underlying(UnitO::kPEND));

    if (finished) {
        ui->pBtnPrint->setFocus();
        ui->pBtnPrint->setDefault(true);
        ui->tableViewO->clearSelection();
    }
}

void TransWidgetO::IniRuleGroup()
{
    rule_group_ = new QButtonGroup(this);
    rule_group_->addButton(ui->rBtnSP, 0);
    rule_group_->addButton(ui->rBtnRefund, 1);
}

void TransWidgetO::IniUnitGroup()
{
    unit_group_ = new QButtonGroup(this);
    unit_group_->addButton(ui->rBtnIS, 0);
    unit_group_->addButton(ui->rBtnMS, 1);
    unit_group_->addButton(ui->rBtnPEND, 2);
}

void TransWidgetO::on_comboParty_currentIndexChanged(int /*index*/)
{
    int party_id { ui->comboParty->currentData().toInt() };
    if (party_id <= 0)
        return;

    node_->party = party_id;
    sql_->WriteField(info_node_, kParty, party_id, node_id_);
    emit SSyncInt(node_id_, std::to_underlying(NodeEnumO::kParty), party_id);

    if (ui->comboEmployee->currentIndex() != -1)
        return;

    int employee_index { ui->comboEmployee->findData(stakeholder_tree_->Employee(party_id)) };
    ui->comboEmployee->setCurrentIndex(employee_index);

    ui->rBtnIS->setChecked(stakeholder_tree_->Rule(party_id) == kRuleIS);
    ui->rBtnMS->setChecked(stakeholder_tree_->Rule(party_id) == kRuleMS);
}

void TransWidgetO::on_comboEmployee_currentIndexChanged(int /*index*/)
{
    node_->employee = ui->comboEmployee->currentData().toInt();
    sql_->WriteField(info_node_, kEmployee, node_->employee, node_id_);
}

void TransWidgetO::on_pBtnInsert_clicked()
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

void TransWidgetO::on_dateTimeEdit_dateTimeChanged(const QDateTime& date_time)
{
    node_->date_time = date_time.toString(kDateTimeFST);
    sql_->WriteField(info_node_, kDateTime, node_->date_time, node_id_);
}

void TransWidgetO::on_lineDescription_editingFinished()
{
    node_->description = ui->lineDescription->text();
    sql_->WriteField(info_node_, kDescription, node_->description, node_id_);
}

void TransWidgetO::RRuleGroupClicked(int id)
{
    node_->rule = static_cast<bool>(id);

    node_->first *= -1;
    node_->second *= -1;
    node_->initial_total *= -1;
    node_->discount *= -1;
    node_->final_total *= -1;

    IniLeafValue();

    sql_->WriteField(info_node_, kRule, node_->rule, node_id_);
    sql_->WriteLeafValue(node_);
}

void TransWidgetO::RUnitGroupClicked(int id)
{
    const UnitO unit { id };
    node_->final_total = 0.0;

    switch (unit) {
    case UnitO::kIS:
        node_->final_total = node_->initial_total - node_->discount;
        [[fallthrough]];
    case UnitO::kMS:
        ui->pBtnFinishOrder->setEnabled(true);
        break;
    case UnitO::kPEND:
        ui->pBtnFinishOrder->setEnabled(false);
        break;
    default:
        break;
    }

    node_->unit = id;
    ui->dSpinSettlement->setValue(node_->final_total);

    sql_->WriteField(info_node_, kUnit, id, node_id_);
    sql_->WriteField(info_node_, kSettlement, node_->final_total, node_id_);
}

void TransWidgetO::on_pBtnFinishOrder_toggled(bool checked)
{
    node_->finished = checked;
    sql_->WriteField(info_node_, kFinished, checked, node_id_);
    emit SSyncBool(node_id_, std::to_underlying(NodeEnumO::kFinished), checked);

    IniFinished(checked);
    LockWidgets(checked);
}
