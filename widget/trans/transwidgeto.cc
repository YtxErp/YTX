#include "transwidgeto.h"

#include <QTimer>

#include "component/signalblocker.h"
#include "global/resourcepool.h"
#include "ui_transwidgeto.h"

TransWidgetO::TransWidgetO(CInsertNodeArgO& arg, QWidget* parent)
    : TransWidget(parent)
    , ui(new Ui::TransWidgetO)
    , node_ { arg.node }
    , sql_ { arg.sql }
    , order_trans_ { arg.order_trans }
    , stakeholder_node_ { arg.stakeholder_node }
    , settings_ { arg.settings }
    , node_id_ { arg.node->id }
    , party_unit_ { arg.section == Section::kSales ? std::to_underlying(UnitS::kCust) : std::to_underlying(UnitS::kVend) }
    , party_info_ { arg.section == Section::kSales ? kSales : kPurchase }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    IniWidget();
    IniText(arg.section);
    IniUnit(arg.node->unit);
    IniRuleGroup();
    IniUnitGroup();
    IniRule(arg.node->rule);
    IniData();
    IniDataCombo(arg.node->party, arg.node->employee);
    IniConnect();

    const bool finished { arg.node->finished };
    IniFinished(finished);
    LockWidgets(finished);
}

TransWidgetO::~TransWidgetO()
{
    delete order_trans_;
    delete ui;
}

QPointer<QTableView> TransWidgetO::View() const { return ui->tableViewO; }

void TransWidgetO::RSyncBoolNode(int node_id, int column, bool value)
{
    if (node_id != node_id_)
        return;

    const NodeEnumO kColumn { column };
    emit SSyncBoolTrans(node_id_, column, value);

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

    const double adjusted_final_delta { node_->unit == std::to_underlying(UnitO::kIS) ? final_delta : 0.0 };

    node_->first += first_delta;
    node_->second += second_delta;
    node_->initial_total += initial_delta;
    node_->discount += discount_delta;
    node_->final_total += adjusted_final_delta;

    IniLeafValue();

    emit SUpdateLeafValue(node_id_, initial_delta, adjusted_final_delta, first_delta, second_delta, discount_delta);
}

void TransWidgetO::IniWidget()
{
    pmodel_ = stakeholder_node_->UnitModelPS(party_unit_);
    ui->comboParty->setModel(pmodel_);

    emodel_ = stakeholder_node_->UnitModelPS(std::to_underlying(UnitS::kEmp));
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

    ui->chkBoxBranch->setChecked(false);
    ui->lineDescription->setText(node_->description);
    ui->dateTimeEdit->setDateTime(QDateTime::fromString(node_->date_time, kDateTimeFST));

    ui->tableViewO->setModel(order_trans_);
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
    emit SEnableAction(finished);

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
    sql_->WriteField(party_info_, kParty, party_id, node_id_);
    emit SSyncInt(node_id_, std::to_underlying(NodeEnumO::kParty), party_id);

    if (ui->comboEmployee->currentIndex() != -1)
        return;

    int employee_index { ui->comboEmployee->findData(stakeholder_node_->Employee(party_id)) };
    ui->comboEmployee->setCurrentIndex(employee_index);
}

void TransWidgetO::on_comboEmployee_currentIndexChanged(int /*index*/)
{
    node_->employee = ui->comboEmployee->currentData().toInt();
    sql_->WriteField(party_info_, kEmployee, node_->employee, node_id_);
}

void TransWidgetO::on_pBtnInsert_clicked()
{
    const auto& name { ui->comboParty->currentText() };
    if (name.isEmpty() || ui->comboParty->currentIndex() != -1)
        return;

    auto* node { ResourcePool<Node>::Instance().Allocate() };
    node->rule = stakeholder_node_->Rule(-1);
    stakeholder_node_->SetParent(node, -1);
    node->name = name;

    node->unit = party_unit_;

    stakeholder_node_->InsertNode(0, QModelIndex(), node);

    int party_index { ui->comboParty->findData(node->id) };
    ui->comboParty->setCurrentIndex(party_index);
}

void TransWidgetO::on_dateTimeEdit_dateTimeChanged(const QDateTime& date_time)
{
    node_->date_time = date_time.toString(kDateTimeFST);
    sql_->WriteField(party_info_, kDateTime, node_->date_time, node_id_);
}

void TransWidgetO::on_lineDescription_editingFinished()
{
    node_->description = ui->lineDescription->text();
    sql_->WriteField(party_info_, kDescription, node_->description, node_id_);
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

    sql_->WriteField(party_info_, kRule, node_->rule, node_id_);
    sql_->SyncLeafValue(node_);
    sql_->InvertTransValue(node_id_);

    emit SSyncBoolTrans(node_id_, std::to_underlying(NodeEnumO::kRule), node_->rule);
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

    sql_->WriteField(party_info_, kUnit, id, node_id_);
    sql_->WriteField(party_info_, kSettlement, node_->final_total, node_id_);
}

void TransWidgetO::on_pBtnFinishOrder_toggled(bool checked)
{
    node_->finished = checked;
    sql_->WriteField(party_info_, kFinished, checked, node_id_);

    emit SSyncBoolNode(node_id_, std::to_underlying(NodeEnumO::kFinished), checked);
    emit SSyncBoolTrans(node_id_, std::to_underlying(NodeEnumO::kFinished), checked);

    IniFinished(checked);
    LockWidgets(checked);
}
