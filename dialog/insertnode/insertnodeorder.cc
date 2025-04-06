#include "insertnodeorder.h"

#include <QTimer>

#include "component/signalblocker.h"
#include "global/resourcepool.h"
#include "mainwindow.h"
#include "ui_insertnodeorder.h"

InsertNodeOrder::InsertNodeOrder(CInsertNodeArgO& arg, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::InsertNodeOrder)
    , node_ { arg.node }
    , sql_ { qobject_cast<SqliteO*>(arg.sql) }
    , stakeholder_node_ { arg.stakeholder_node }
    , order_trans_ { arg.order_trans }
    , party_info_ { arg.section == Section::kSales ? kSales : kPurchase }
    , party_unit_ { arg.section == Section::kSales ? std::to_underlying(UnitS::kCust) : std::to_underlying(UnitS::kVend) }
    , party_text_ { arg.section == Section::kSales ? tr("CUST") : tr("VEND") }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    IniDialog(arg.settings);
    IniText(arg.section);
    IniRuleGroup();
    IniUnitGroup();
    IniUnit(arg.node->unit);
    IniShotcut(parent);
    IniConnect();
}

InsertNodeOrder::~InsertNodeOrder() { delete ui; }

QPointer<TransModel> InsertNodeOrder::Model() { return order_trans_; }

void InsertNodeOrder::RUpdateLeafValue(
    int /*node_id*/, double initial_delta, double final_delta, double first_delta, double second_delta, double discount_delta)
{
    // In OrderRule, RO:1, SO and PO:0

    const double adjusted_final_delta { node_->unit == std::to_underlying(UnitO::kIS) ? final_delta : 0.0 };

    node_->first += first_delta;
    node_->second += second_delta;
    node_->initial_total += initial_delta;
    node_->discount += discount_delta;
    node_->final_total += adjusted_final_delta;

    IniLeafValue();

    if (node_id_ != 0) {
        emit SSyncLeafValue(node_id_, initial_delta, adjusted_final_delta, first_delta, second_delta, discount_delta);
    }
}

void InsertNodeOrder::RSyncBoolNode(int node_id, int column, bool value)
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
    case NodeEnumO::kFinished: {
        IniFinished(value);
        LockWidgets(value, node_->type == kTypeBranch);
        break;
    }
    default:
        return;
    }
}

void InsertNodeOrder::RSyncInt(int node_id, int column, int value)
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

void InsertNodeOrder::RSyncString(int node_id, int column, const QString& value)
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

QPointer<QTableView> InsertNodeOrder::View() { return ui->tableViewO; }

void InsertNodeOrder::IniDialog(CSettings* settings)
{
    pmodel_ = stakeholder_node_->IncludeUnitModel(party_unit_);
    ui->comboParty->setModel(pmodel_);
    ui->comboParty->setCurrentIndex(-1);

    emodel_ = stakeholder_node_->IncludeUnitModel(std::to_underlying(UnitS::kEmp));
    ui->comboEmployee->setModel(emodel_);
    ui->comboEmployee->setCurrentIndex(-1);

    ui->dateTimeEdit->setDisplayFormat(kDateTimeFST);
    ui->dateTimeEdit->setDateTime(QDateTime::currentDateTime());
    node_->date_time = ui->dateTimeEdit->dateTime().toString(kDateTimeFST);
    ui->comboParty->lineEdit()->setValidator(&LineEdit::kInputValidator);

    ui->dSpinDiscount->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    ui->dSpinGrossAmount->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    ui->dSpinSettlement->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    ui->dSpinSecond->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    ui->dSpinFirst->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());

    ui->dSpinDiscount->setDecimals(settings->amount_decimal);
    ui->dSpinGrossAmount->setDecimals(settings->amount_decimal);
    ui->dSpinSettlement->setDecimals(settings->amount_decimal);
    ui->dSpinSecond->setDecimals(settings->common_decimal);
    ui->dSpinFirst->setDecimals(settings->common_decimal);

    ui->dSpinDiscount->setAlignment(Qt::AlignRight | Qt::AlignBottom);
    ui->dSpinSettlement->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
    ui->dSpinFirst->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->dSpinSecond->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    ui->pBtnSaveOrder->setEnabled(false);
    ui->pBtnFinishOrder->setEnabled(false);
    ui->rBtnSP->setChecked(true);

    ui->tableViewO->setModel(order_trans_);

    ui->comboParty->setFocus();
}

void InsertNodeOrder::accept()
{
    if (node_id_ == 0) {
        emit QDialog::accepted();
        node_id_ = node_->id;

        if (node_->type == kTypeLeaf) {
            emit SSyncInt(node_id_, std::to_underlying(NodeEnumO::kID), node_id_);
            emit SSyncLeafValue(node_id_, node_->initial_total, node_->final_total, node_->first, node_->second, node_->discount);
        }
    }

    close();
}

void InsertNodeOrder::IniConnect()
{
    connect(ui->pBtnSaveOrder, &QPushButton::clicked, this, &InsertNodeOrder::accept);
    connect(rule_group_, &QButtonGroup::idClicked, this, &InsertNodeOrder::RRuleGroupClicked);
    connect(unit_group_, &QButtonGroup::idClicked, this, &InsertNodeOrder::RUnitGroupClicked);
}

void InsertNodeOrder::LockWidgets(bool finished, bool branch)
{
    bool basic_enable { !finished };
    bool not_branch_enable { !finished && !branch };

    ui->labParty->setEnabled(basic_enable);
    ui->comboParty->setEnabled(basic_enable);

    ui->pBtnInsert->setEnabled(not_branch_enable);

    ui->labSettlement->setEnabled(not_branch_enable);
    ui->dSpinSettlement->setEnabled(not_branch_enable);

    ui->dSpinGrossAmount->setEnabled(not_branch_enable);

    ui->labDiscount->setEnabled(not_branch_enable);
    ui->dSpinDiscount->setEnabled(not_branch_enable);

    ui->labEmployee->setEnabled(not_branch_enable);
    ui->comboEmployee->setEnabled(not_branch_enable);
    ui->tableViewO->setEnabled(not_branch_enable);

    ui->rBtnIS->setEnabled(basic_enable);
    ui->rBtnMS->setEnabled(basic_enable);
    ui->rBtnPEND->setEnabled(basic_enable);
    ui->dateTimeEdit->setEnabled(not_branch_enable);

    ui->dSpinFirst->setEnabled(not_branch_enable);
    ui->labFirst->setEnabled(not_branch_enable);
    ui->dSpinSecond->setEnabled(not_branch_enable);
    ui->labSecond->setEnabled(not_branch_enable);

    ui->rBtnRefund->setEnabled(not_branch_enable);
    ui->rBtnSP->setEnabled(not_branch_enable);
    ui->lineDescription->setEnabled(basic_enable);

    ui->pBtnPrint->setEnabled(finished && !branch);
}

void InsertNodeOrder::IniUnit(int unit)
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

void InsertNodeOrder::IniRule(bool rule)
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

void InsertNodeOrder::IniDataCombo(int party, int employee)
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

void InsertNodeOrder::IniLeafValue()
{
    ui->dSpinFirst->setValue(node_->first);
    ui->dSpinSecond->setValue(node_->second);
    ui->dSpinGrossAmount->setValue(node_->initial_total);
    ui->dSpinDiscount->setValue(node_->discount);
    ui->dSpinSettlement->setValue(node_->final_total);
}

void InsertNodeOrder::IniRuleGroup()
{
    rule_group_ = new QButtonGroup(this);
    rule_group_->addButton(ui->rBtnSP, 0);
    rule_group_->addButton(ui->rBtnRefund, 1);
}

void InsertNodeOrder::IniShotcut(QWidget* parent)
{
    trans_shortcut_ = new QShortcut(QKeySequence("Ctrl+N"), this);
    trans_shortcut_->setContext(Qt::WindowShortcut);

    connect(trans_shortcut_, &QShortcut::activated, parent, [parent]() {
        auto* main_window { qobject_cast<MainWindow*>(parent) };
        if (main_window) {
            main_window->on_actionAppendTrans_triggered();
        }
    });

    node_shortcut_ = new QShortcut(QKeySequence("Alt+N"), this);
    node_shortcut_->setContext(Qt::WindowShortcut);

    connect(node_shortcut_, &QShortcut::activated, parent, [parent]() {
        auto* main_window { qobject_cast<MainWindow*>(parent) };
        if (main_window) {
            main_window->on_actionInsertNode_triggered();
        }
    });

#ifdef Q_OS_MAC
    remove_trans_shortcut_ = new QShortcut(QKeySequence(Qt::Key_Backspace), this);
#endif

#ifdef Q_OS_WIN
    remove_trans_shortcut_ = new QShortcut(QKeySequence(Qt::Key_Delete), this);
#endif

    remove_trans_shortcut_->setContext(Qt::WindowShortcut);
    connect(remove_trans_shortcut_, &QShortcut::activated, parent, [parent]() {
        auto* main_window { qobject_cast<MainWindow*>(parent) };
        if (main_window) {
            main_window->on_actionRemove_triggered();
        }
    });
}

void InsertNodeOrder::IniText(Section section)
{
    const bool is_sales_section { section == Section::kSales };

    setWindowTitle(is_sales_section ? tr("Sales") : tr("Purchase"));
    ui->rBtnSP->setText(is_sales_section ? tr("SO") : tr("PO"));
    ui->labParty->setText(party_text_);
}

void InsertNodeOrder::IniFinished(bool finished)
{
    ui->pBtnFinishOrder->setChecked(finished);
    ui->pBtnFinishOrder->setText(finished ? tr("Edit") : tr("Finish"));
    trans_shortcut_->setEnabled(!finished);
    remove_trans_shortcut_->setEnabled(!finished);

    if (finished) {
        ui->pBtnPrint->setFocus();
        ui->pBtnPrint->setDefault(true);
        ui->tableViewO->clearSelection();
    }
}

void InsertNodeOrder::IniUnitGroup()
{
    unit_group_ = new QButtonGroup(this);
    unit_group_->addButton(ui->rBtnIS, 0);
    unit_group_->addButton(ui->rBtnMS, 1);
    unit_group_->addButton(ui->rBtnPEND, 2);
}

void InsertNodeOrder::on_comboParty_editTextChanged(const QString& arg1)
{
    if (node_->type != kTypeBranch || arg1.isEmpty())
        return;

    node_->name = arg1;

    if (node_id_ == 0) {
        ui->pBtnSaveOrder->setEnabled(true);
        ui->pBtnFinishOrder->setEnabled(true);
    } else {
        sql_->WriteField(party_info_, kName, arg1, node_id_);
    }
}

void InsertNodeOrder::on_comboParty_currentIndexChanged(int /*index*/)
{
    if (node_->type != kTypeLeaf)
        return;

    static const QString title { windowTitle() };

    int party_id { ui->comboParty->currentData().toInt() };
    if (party_id <= 0)
        return;

    node_->party = party_id;
    emit SSyncInt(node_id_, std::to_underlying(NodeEnumO::kParty), party_id);

    if (node_id_ == 0) {
        ui->pBtnSaveOrder->setEnabled(true);
        ui->pBtnFinishOrder->setEnabled(true);
    } else {
        sql_->WriteField(party_info_, kParty, party_id, node_id_);
    }

    this->setWindowTitle(title + "-" + stakeholder_node_->Name(party_id));

    if (ui->comboEmployee->currentIndex() != -1)
        return;

    int employee_index { ui->comboEmployee->findData(stakeholder_node_->Employee(party_id)) };
    ui->comboEmployee->setCurrentIndex(employee_index);
}

void InsertNodeOrder::on_comboEmployee_currentIndexChanged(int /*index*/)
{
    node_->employee = ui->comboEmployee->currentData().toInt();

    if (node_id_ != 0)
        sql_->WriteField(party_info_, kEmployee, node_->employee, node_id_);
}

void InsertNodeOrder::on_pBtnInsert_clicked()
{
    const auto& name { ui->comboParty->currentText() };
    if (node_->type == kTypeBranch || name.isEmpty() || ui->comboParty->currentIndex() != -1)
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

void InsertNodeOrder::on_dateTimeEdit_dateTimeChanged(const QDateTime& date_time)
{
    node_->date_time = date_time.toString(kDateTimeFST);

    if (node_id_ != 0)
        sql_->WriteField(party_info_, kDateTime, node_->date_time, node_id_);
}

void InsertNodeOrder::on_pBtnFinishOrder_toggled(bool checked)
{
    accept();

    node_->finished = checked;

    sql_->WriteField(party_info_, kFinished, checked, node_id_);
    if (node_->type == kTypeLeaf) {
        emit SSyncBoolNode(node_id_, std::to_underlying(NodeEnumO::kFinished), checked);
        emit SSyncBoolTrans(node_id_, std::to_underlying(NodeEnumO::kFinished), checked);
    }

    IniFinished(checked);
    LockWidgets(checked, node_->type == kTypeBranch);

    if (checked)
        sql_->SyncPrice(node_id_);
}

void InsertNodeOrder::on_chkBoxBranch_checkStateChanged(const Qt::CheckState& arg1)
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

    ui->rBtnRefund->setChecked(false);
    ui->tableViewO->clearSelection();
    ui->labParty->setText(enable ? tr("Branch") : party_text_);
}

void InsertNodeOrder::RRuleGroupClicked(int id)
{
    node_->rule = static_cast<bool>(id);

    node_->first *= -1;
    node_->second *= -1;
    node_->initial_total *= -1;
    node_->discount *= -1;
    node_->final_total *= -1;

    IniLeafValue();

    if (node_id_ != 0) {
        sql_->WriteField(party_info_, kRule, node_->rule, node_id_);
        sql_->UpdateLeafValue(node_);
        sql_->InvertTransValue(node_id_);
    }

    emit SSyncBoolTrans(node_id_, std::to_underlying(NodeEnumO::kRule), node_->rule);
}

void InsertNodeOrder::RUnitGroupClicked(int id)
{
    const UnitO unit { id };
    node_->final_total = 0.0;

    switch (unit) {
    case UnitO::kIS:
        node_->final_total = node_->initial_total - node_->discount;
        [[fallthrough]];
    case UnitO::kMS:
        ui->pBtnFinishOrder->setEnabled(ui->comboParty->currentIndex() != -1);
        break;
    case UnitO::kPEND:
        ui->pBtnFinishOrder->setEnabled(false);
        break;
    default:
        break;
    }

    node_->unit = id;
    ui->dSpinSettlement->setValue(node_->final_total);

    if (node_id_ != 0) {
        sql_->WriteField(party_info_, kUnit, id, node_id_);
        sql_->WriteField(party_info_, kSettlement, node_->final_total, node_id_);
    }
}

void InsertNodeOrder::on_lineDescription_editingFinished()
{
    node_->description = ui->lineDescription->text();

    if (node_id_ != 0)
        sql_->WriteField(party_info_, kDescription, node_->description, node_id_);
}
