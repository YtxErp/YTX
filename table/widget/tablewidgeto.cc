#include "tablewidgeto.h"

#include "component/constantwebsocket.h"
#include "component/signalblocker.h"
#include "global/nodepool.h"
#include "global/printhub.h"
#include "ui_tablewidgeto.h"
#include "utils/mainwindowutils.h"
#include "websocket/jsongen.h"
#include "websocket/websocket.h"

TableWidgetO::TableWidgetO(COrderWidgetArg& arg, NodeO* node, QWidget* parent)
    : TableWidget(parent)
    , ui(new Ui::TableWidgetO)
    , tmp_node_ { node }
    , table_model_order_ { qobject_cast<TableModelO*>(arg.table_model) }
    , tree_model_partner_ { arg.tree_model_partner }
    , config_ { arg.section_config }
    , node_id_ { node->id }
    , section_ { arg.section }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    InitWidget();
    InitRuleGroup();
    InitUnitGroup();
    InitData(tmp_node_);
    InitConnect();

    if (node->sync_state == SyncState::kCreating)
        QTimer::singleShot(0, this, [this]() { ui->comboPartner->setFocus(); });
}

TableWidgetO::~TableWidgetO()
{
    NodePool::Instance().Recycle(tmp_node_, section_);
    delete ui;
}

QTableView* TableWidgetO::View() const { return ui->tableViewO; }

void TableWidgetO::ReleaseSucceeded(int version)
{
    qDebug() << "TableWidgetO::ReleaseSucceeded";

    MarkSynced(version);
    LockWidgets(OrderStatus::kReleased);
    tmp_node_->order_status = OrderStatus::kReleased;
}

void TableWidgetO::RecallSucceeded(int version)
{
    qDebug() << "TableWidgetO::RecallSucceeded";

    MarkSynced(version);
    LockWidgets(OrderStatus::kRecalled);
    tmp_node_->order_status = OrderStatus::kRecalled;
}

void TableWidgetO::SaveSucceeded(int version)
{
    qDebug() << "TableWidgetO::SaveSucceeded";
    MarkSynced(version);
}

void TableWidgetO::MarkSynced(int version)
{
    ui->pBtnPrint->setFocus();

    tmp_node_->version = version;
    tmp_node_->sync_state = SyncState::kSynced;
}

void TableWidgetO::MarkUpdating()
{
    tmp_node_->sync_state = SyncState::kUpdating;
    has_pending_update_ = false;
    ui->tableViewO->clearSelection();
}

bool TableWidgetO::HasPendingUpdate() const
{
    const bool node_pending { has_pending_update_ };
    const bool entry_pending { table_model_order_->HasPendingUpdate() };

    qDebug() << "[HasPendingUpdate]"
             << "node_pending =" << node_pending << ", entry_pending =" << entry_pending << ", result =" << (node_pending || entry_pending);

    return node_pending || entry_pending;
}

void TableWidgetO::RSyncDeltaO(const QUuid& node_id, double initial_delta, double count_delta, double measure_delta)
{
    Q_ASSERT(node_id_ == node_id && "RSyncDelta called with mismatched node_id");

    {
        if (tmp_node_->direction_rule == direction_rule::kRO) {
            initial_delta *= -1;
            count_delta *= -1;
            measure_delta *= -1;
        }
    }

    {
        tmp_node_->count_total += count_delta;
        tmp_node_->measure_total += measure_delta;
        tmp_node_->initial_total += initial_delta;

        InitUiValue();
    }
}

void TableWidgetO::InitWidget()
{
    {
        auto* pmodel { tree_model_partner_->IncludeUnit(section_ == Section::kSale ? NodeUnit::PCustomer : NodeUnit::PVendor, this) };
        ui->comboPartner->setModel(pmodel);
        ui->comboPartner->setCurrentIndex(-1);

        auto* emodel { tree_model_partner_->IncludeUnit(NodeUnit::PEmployee, this) };
        ui->comboEmployee->setModel(emodel);
        ui->comboEmployee->setCurrentIndex(-1);
    }

    {
        ui->tableViewO->setModel(table_model_order_);
        table_model_order_->setParent(ui->tableViewO);
    }

    {
        ui->dateTimeEdit->setDisplayFormat(datetime_format::kDateTime);
    }

    {
        ui->dSpinInitialTotal->setRange(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max());
        ui->dSpinMeasureTotal->setRange(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max());
        ui->dSpinCountTotal->setRange(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max());

        ui->dSpinInitialTotal->setDecimals(config_.amount_decimal);
        ui->dSpinMeasureTotal->setDecimals(config_.quantity_decimal);
        ui->dSpinCountTotal->setDecimals(config_.quantity_decimal);

        ui->dSpinCountTotal->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->dSpinMeasureTotal->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }

    {
        utils::SetPushButton(ui->pBtnSave, QKeySequence::Save);
        utils::SetPushButton(ui->pBtnRelease, QKeySequence(Qt::CTRL | Qt::Key_Return));
        utils::SetPushButton(ui->pBtnRecall, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R));
        utils::SetPushButton(ui->pBtnPreview, QKeySequence(Qt::CTRL | Qt::Key_P));
        utils::SetPushButton(ui->pBtnPrint, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P));

        utils::SetRadioButton(ui->rBtnIS, QKeySequence(Qt::CTRL | Qt::Key_1));
        utils::SetRadioButton(ui->rBtnMS, QKeySequence(Qt::CTRL | Qt::Key_2));
        utils::SetRadioButton(ui->rBtnPEND, QKeySequence(Qt::CTRL | Qt::Key_3));
        utils::SetRadioButton(ui->rBtnRO, QKeySequence(Qt::CTRL | Qt::Key_5));
        utils::SetRadioButton(ui->rBtnFO, QKeySequence(Qt::CTRL | Qt::Key_4));
    }

    {
        auto& templates { PrintHub::Instance().TemplateMap() };
        for (auto it = templates.constBegin(); it != templates.constEnd(); ++it) {
            const QString display_name { it.key().section('_', 1) };
            ui->comboTemplate->addItem(display_name, it.value());
        }

        if (ui->comboTemplate->count() > 0) {
            PrintHub::Instance().LoadTemplate(ui->comboTemplate->currentData().toString());
        }
    }
}

void TableWidgetO::InitConnect()
{
    connect(rule_group_, &QButtonGroup::idClicked, this, &TableWidgetO::RRuleGroupClicked);
    connect(unit_group_, &QButtonGroup::idClicked, this, &TableWidgetO::RUnitGroupClicked);
}

void TableWidgetO::InitData(const NodeO* node)
{
    {
        (node->direction_rule ? ui->rBtnRO : ui->rBtnFO)->setChecked(true);
        ui->dateTimeEdit->setDateTime(node->issued_time);
    }

    {
        const NodeUnit kUnit { node->unit };

        switch (kUnit) {
        case NodeUnit::OImmediate:
            ui->rBtnIS->setChecked(true);
            break;
        case NodeUnit::OMonthly:
            ui->rBtnMS->setChecked(true);
            break;
        case NodeUnit::OPending:
            ui->rBtnPEND->setChecked(true);
            break;
        default:
            break;
        }
    }

    {
        LockWidgets(OrderStatus(node->order_status));
    }

    if (tmp_node_->sync_state == SyncState::kCreating)
        return;

    {
        InitUiValue();

        ui->lineDescription->setText(node->description);

        int partner_index { ui->comboPartner->findData(node->partner_id) };
        ui->comboPartner->setCurrentIndex(partner_index);

        int employee_index { ui->comboEmployee->findData(node->employee_id) };
        ui->comboEmployee->setCurrentIndex(employee_index);
    }
}

void TableWidgetO::LockWidgets(OrderStatus value)
{
    const bool is_unreleased { value == OrderStatus::kUnreleased };
    const bool is_released { value == OrderStatus::kReleased };

    ui->comboPartner->setReadOnly(!is_unreleased);
    ui->comboEmployee->setReadOnly(is_released);

    ui->rBtnRO->setEnabled(is_unreleased);
    ui->rBtnFO->setEnabled(is_unreleased);

    ui->rBtnIS->setEnabled(!is_released);
    ui->rBtnMS->setEnabled(!is_released);
    ui->rBtnPEND->setEnabled(!is_released);

    ui->lineDescription->setReadOnly(is_released);
    ui->dateTimeEdit->setReadOnly(is_released);

    ui->pBtnSave->setEnabled(!is_released);
    ui->pBtnRelease->setEnabled(!is_released && tmp_node_->unit != NodeUnit::OPending);
    ui->pBtnRecall->setEnabled(is_released);
    ui->pushButtonDelete->setEnabled(!is_released);
}

void TableWidgetO::InitUiValue()
{
    ui->dSpinCountTotal->setValue(tmp_node_->count_total);
    ui->dSpinMeasureTotal->setValue(tmp_node_->measure_total);
    ui->dSpinInitialTotal->setValue(tmp_node_->initial_total);
}

void TableWidgetO::InitRuleGroup()
{
    rule_group_ = new QButtonGroup(this);
    rule_group_->addButton(ui->rBtnRO, static_cast<int>(direction_rule::kRO));
    rule_group_->addButton(ui->rBtnFO, static_cast<int>(direction_rule::kFO));
}

void TableWidgetO::InitUnitGroup()
{
    unit_group_ = new QButtonGroup(this);
    unit_group_->addButton(ui->rBtnIS, std::to_underlying(NodeUnit::OImmediate));
    unit_group_->addButton(ui->rBtnMS, std::to_underlying(NodeUnit::OMonthly));
    unit_group_->addButton(ui->rBtnPEND, std::to_underlying(NodeUnit::OPending));
}

void TableWidgetO::on_comboPartner_currentIndexChanged(int /*index*/)
{
    const QUuid partner_id { ui->comboPartner->currentData().toUuid() };
    if (tmp_node_->partner_id == partner_id)
        return;

    tmp_node_->partner_id = partner_id;
    emit SSyncPartner(tmp_node_->parent->id, node_id_, partner_id);

    if (tmp_node_->sync_state == SyncState::kSynced) {
        pending_update_.insert(kPartnerId, partner_id.toString(QUuid::WithoutBraces));
    }

    has_pending_update_ = true;
}

void TableWidgetO::on_comboEmployee_currentIndexChanged(int /*index*/)
{
    const QUuid employee_id { ui->comboEmployee->currentData().toUuid() };
    if (tmp_node_->employee_id == employee_id)
        return;

    tmp_node_->employee_id = employee_id;

    if (tmp_node_->sync_state == SyncState::kSynced) {
        pending_update_.insert(kEmployeeId, employee_id.toString(QUuid::WithoutBraces));
    }

    has_pending_update_ = true;
}

void TableWidgetO::on_dateTimeEdit_dateTimeChanged(const QDateTime& date_time)
{
    if (tmp_node_->issued_time == date_time)
        return;

    tmp_node_->issued_time = date_time;

    if (tmp_node_->sync_state == SyncState::kSynced) {
        pending_update_.insert(kIssuedTime, date_time.toUTC().toString(Qt::ISODate));
    }

    has_pending_update_ = true;
}

void TableWidgetO::on_lineDescription_textChanged(const QString& arg1)
{
    if (tmp_node_->description == arg1)
        return;

    tmp_node_->description = arg1;

    if (tmp_node_->sync_state == SyncState::kSynced) {
        pending_update_.insert(kDescription, arg1);
    }

    has_pending_update_ = true;
}

void TableWidgetO::RRuleGroupClicked(int id)
{
    if (tmp_node_->order_status != OrderStatus::kUnreleased)
        return;

    tmp_node_->direction_rule = static_cast<bool>(id);
    has_pending_update_ = true;

    tmp_node_->count_total *= -1;
    tmp_node_->measure_total *= -1;
    tmp_node_->initial_total *= -1;

    InitUiValue();

    if (tmp_node_->sync_state == SyncState::kSynced) {
        pending_update_.insert(kDirectionRule, tmp_node_->direction_rule);
    }
}

void TableWidgetO::RUnitGroupClicked(int id)
{
    const NodeUnit unit { id };

    tmp_node_->unit = unit;

    const bool is_pending { unit == NodeUnit::OPending };
    ui->pBtnRelease->setEnabled(!is_pending);

    if (tmp_node_->sync_state == SyncState::kSynced) {
        pending_update_.insert(kUnit, id);
    }

    has_pending_update_ = true;
}

void TableWidgetO::on_pBtnSave_clicked() { SaveOrder(); }

void TableWidgetO::on_pBtnPrint_clicked()
{
    QEvent leave_event(QEvent::Leave);
    QApplication::sendEvent(ui->pBtnPrint, &leave_event);

    if (!PreparePrint())
        return;

    PrintHub::Instance().Print();
}

void TableWidgetO::on_pBtnPreview_clicked()
{
    QEvent leave_event(QEvent::Leave);
    QApplication::sendEvent(ui->pBtnPreview, &leave_event);

    if (!PreparePrint())
        return;

    PrintHub::Instance().Preview();
}

bool TableWidgetO::PreparePrint()
{
    if (ui->comboTemplate->currentIndex() == -1) {
        utils::ShowMessage(
            QMessageBox::Warning, tr("Required Information Missing"), tr("Please select a printable template before continuing."), time_const::kAutoCloseMs);
        return false;
    }

    PrintHub::Instance().SetSectionConfig(&config_);
    PrintHub::Instance().SetValue(tmp_node_, table_model_order_->GetEntryList());

    return true;
}

void TableWidgetO::BuildNodeInsert(QJsonObject& order_message)
{
    const QJsonObject node_json { tmp_node_->WriteJson() };

    QJsonObject path_json {};
    path_json.insert(kAncestor, tmp_node_->parent->id.toString(QUuid::WithoutBraces));
    path_json.insert(kDescendant, node_id_.toString(QUuid::WithoutBraces));

    order_message.insert(kNode, node_json);
    order_message.insert(kPath, path_json);
}

void TableWidgetO::BuildNodeUpdate(QJsonObject& order_message)
{
    order_message.insert(kVersion, tmp_node_->version);
    order_message.insert(kNodeId, node_id_.toString(QUuid::WithoutBraces));
    order_message.insert(kPartnerId, tmp_node_->partner_id.toString(QUuid::WithoutBraces));
    order_message.insert(WsField::kNodeUpdate, pending_update_);
}

void TableWidgetO::on_pBtnRecall_clicked()
{
    if (!ValidateSyncState())
        return;

    if (!tmp_node_->settlement_id.isNull()) {
        utils::ShowMessage(QMessageBox::Information, tr("Operation Rejected"),
            tr("This order has already been settled and cannot be recalled.\n"
               "Settled orders cannot be modified."),
            time_const::kAutoCloseMs);

        return;
    }

    pending_update_.insert(kStatus, std::to_underlying(OrderStatus::kRecalled));

    qDebug() << Q_FUNC_INFO << tmp_node_->version;

    WebSocket::Instance()->SendMessage(WsKey::kOrderRecall, JsonGen::OrderRecall(section_, node_id_, pending_update_, tmp_node_->version));

    MarkUpdating();
}

bool TableWidgetO::ValidatePartner() const
{
    if (tmp_node_->partner_id.isNull()) {
        utils::ShowMessage(QMessageBox::Information, tr("Operation Rejected"), tr("A partner must be selected before continuing."), time_const::kAutoCloseMs);
        return false;
    }

    qDebug() << Q_FUNC_INFO << tree_model_partner_->Name(tmp_node_->partner_id);
    return true;
}

bool TableWidgetO::ValidateSyncState() const
{
    if (tmp_node_->sync_state == SyncState::kUpdating) {
        utils::ShowMessage(
            QMessageBox::Information, tr("Operation Rejected"), tr("The data is being updated. Please refresh and try again."), time_const::kAutoCloseMs);
        return false;
    }

    qDebug() << Q_FUNC_INFO << static_cast<int>(tmp_node_->sync_state);

    return true;
}

bool TableWidgetO::ValidateUnitPrice() const
{
    if (!table_model_order_->HasZeroUnitPrice())
        return true;

    const auto result { QMessageBox::question(const_cast<TableWidgetO*>(this), tr("Operation Warning"),
        tr("Some entries have a unit price of zero. Do you want to continue saving?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) };

    return result == QMessageBox::Yes;
}

void TableWidgetO::SaveOrder()
{
    if (!ValidatePartner())
        return;

    if (!ValidateSyncState())
        return;

    table_model_order_->Purify();

    if (!HasPendingUpdate())
        return;

    if (!ValidateUnitPrice())
        return;

    Q_ASSERT(tmp_node_->order_status != OrderStatus::kReleased);

    QJsonObject order_message {};
    order_message.insert(kSection, std::to_underlying(section_));

    table_model_order_->Finalize(order_message);

    if (tmp_node_->sync_state == SyncState::kSynced) {
        BuildNodeUpdate(order_message);

        const auto key { tmp_node_->order_status == OrderStatus::kUnreleased ? WsKey::kUnreleasedOrderSave : WsKey::kRecalledOrderSave };
        WebSocket::Instance()->SendMessage(key, order_message);

        pending_update_ = QJsonObject();
    }

    if (tmp_node_->sync_state == SyncState::kCreating) {
        tmp_node_->order_status = OrderStatus::kUnreleased; // Same as default

        BuildNodeInsert(order_message);
        WebSocket::Instance()->SendMessage(WsKey::kOrderInsertSave, order_message);
    }

    qInfo() << Q_FUNC_INFO << tmp_node_->version;

    MarkUpdating();
}

void TableWidgetO::on_pBtnRelease_clicked()
{
    if (!ValidatePartner())
        return;

    if (!ValidateSyncState())
        return;

    table_model_order_->Purify();

    if (!ValidateUnitPrice())
        return;

    Q_ASSERT(tmp_node_->order_status != OrderStatus::kReleased);

    QJsonObject order_message {};
    order_message.insert(kSection, std::to_underlying(section_));
    table_model_order_->Finalize(order_message);

    if (tmp_node_->sync_state == SyncState::kSynced) {
        pending_update_.insert(kStatus, std::to_underlying(OrderStatus::kReleased));

        BuildNodeUpdate(order_message);

        const auto key { tmp_node_->order_status == OrderStatus::kUnreleased ? WsKey::kUnreleasedOrderRelease : WsKey::kRecalledOrderRelease };
        WebSocket::Instance()->SendMessage(key, order_message);

        pending_update_ = QJsonObject();
    }

    if (tmp_node_->sync_state == SyncState::kCreating) {
        tmp_node_->order_status = OrderStatus::kReleased;

        BuildNodeInsert(order_message);
        WebSocket::Instance()->SendMessage(WsKey::kOrderInsertRelease, order_message);

        ui->rBtnRO->setEnabled(false);
        ui->rBtnFO->setEnabled(false);
    }

    qInfo() << Q_FUNC_INFO << tmp_node_->version;

    MarkUpdating();
}

void TableWidgetO::on_comboTemplate_currentIndexChanged(int /*index*/)
{
    if (!PrintHub::Instance().LoadTemplate(ui->comboTemplate->currentData().toString())) {
        utils::ShowMessage(QMessageBox::Warning, tr("Operation Failed"), tr("Failed to load the print template."), time_const::kAutoCloseMs);
    }
}

void TableWidgetO::on_pushButtonDelete_clicked()
{
    if (tmp_node_->order_status == OrderStatus::kReleased) {
        utils::ShowMessage(QMessageBox::Information, tr("Operation Rejected"),
            tr("This order has been released and cannot be deleted.\n"
               "Please recall it before deleting."),
            time_const::kAutoCloseMs);

        return;
    }

    const QJsonObject value { JsonGen::LeafDelete(section_, node_id_, tmp_node_->version) };
    WebSocket::Instance()->SendMessage(WsKey::kOrderLeafDelete, value);
}
