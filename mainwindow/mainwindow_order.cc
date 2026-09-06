#include "component/constantwebsocket.h"
#include "dialog/editnodenameo.h"
#include "dialog/insertnode/insertnodebranch.h"
#include "global/nodepool.h"
#include "global/tablesstation.h"
#include "mainwindow.h"
#include "utils/mainwindowutils.h"
#include "websocket/jsongen.h"
#include "websocket/websocket.h"

void MainWindow::EditNameO()
{
    const auto index { sc_->tree_view->currentIndex() };
    if (!index.isValid())
        return;

    auto* node { static_cast<Node*>(index.internalPointer()) };
    if (node->kind == NodeKind::kLeaf)
        return;

    auto model { sc_->tree_model };

    auto* dialog { new EditNodeNameO(node->name, this) };

    utils::ManageDialog(sc_->widget_hash, dialog);
    dialog->setWindowModality(Qt::WindowModal);

    connect(dialog, &QDialog::accepted, this, [this, dialog, node]() {
        const auto message { JsonGen::NodeName(start_, node->id, dialog->GetName(), node->version) };
        WebSocket::Instance()->SendMessage(WsKey::kNodeNameUpdate, message);
    });

    dialog->show();
}

void MainWindow::on_actionNewBranch_triggered()
{
    qInfo() << Q_FUNC_INFO;

    if (start_ != Section::kSale && start_ != Section::kPurchase)
        return;

    auto current_index { sc_->tree_view->currentIndex() };
    current_index = current_index.isValid() ? current_index : QModelIndex();

    auto parent_index { current_index.parent() };
    parent_index = parent_index.isValid() ? parent_index : QModelIndex();

    auto tree_model { sc_->tree_model };
    auto unit_model { sc_->info.unit_model };

    auto* parent_node { tree_model->GetNodeByIndex(parent_index) };

    const QUuid parent_id { parent_node->id };

    auto* node { NodePool::Instance().Allocate(start_) };
    auto* d_node { static_cast<NodeO*>(node) };

    node->id = QUuid::createUuidV7();
    node->unit = parent_index.isValid() ? parent_node->unit : NodeUnit(sc_->shared_config.default_unit);
    node->kind = NodeKind::kBranch;
    d_node->order_status = OrderStatus::kUnreleased; // Branch nodes are always kUnreleased
    node->parent = parent_node;

    static_cast<NodeO*>(node)->issued_time = QDateTime::currentDateTime();

    auto parent_path { tree_model->Path(parent_id) };
    if (!parent_path.isEmpty())
        parent_path += app_config_.separator;

    const auto children_name { ChildrenName(parent_node) };

    auto* dialog { new InsertNodeBranch(node, unit_model, parent_path, children_name, this) };

    utils::ManageDialog(sc_->widget_hash, dialog);
    dialog->setWindowModality(Qt::WindowModal);

    connect(dialog, &QDialog::accepted, this, [this, node]() {
        const auto message { JsonGen::NodeInsert(start_, node, node->parent->id) };
        WebSocket::Instance()->SendMessage(WsKey::kNodeInsert, message);
    });

    connect(dialog, &QDialog::destroyed, this, [this, node]() { NodePool::Instance().Recycle(node, start_); });

    dialog->show();
}

void MainWindow::RReleaseOrder(Section section, const QUuid& node_id, int version)
{
    auto* sc { GetSectionContex(section) };

    auto widget { sc->widget_hash.value(node_id).widget };
    if (widget) {
        auto* ptr { widget.data() };

        Q_ASSERT(qobject_cast<TableWidgetO*>(ptr));
        auto* d_widget { static_cast<TableWidgetO*>(ptr) };

        d_widget->ReleaseSucceeded(version);
    }
}

void MainWindow::RRecallOrder(Section section, const QUuid& node_id, int version)
{
    auto* sc { GetSectionContex(section) };

    auto widget { sc->widget_hash.value(node_id).widget };
    if (widget) {
        auto* ptr { widget.data() };

        Q_ASSERT(qobject_cast<TableWidgetO*>(ptr));
        auto* d_widget { static_cast<TableWidgetO*>(ptr) };

        d_widget->RecallSucceeded(version);
    }
}

void MainWindow::RSaveOrder(Section section, const QUuid& node_id, int version)
{
    auto* sc { GetSectionContex(section) };

    auto widget { sc->widget_hash.value(node_id).widget };
    if (widget) {
        auto* ptr { widget.data() };

        Q_ASSERT(qobject_cast<TableWidgetO*>(ptr));
        auto* d_widget { static_cast<TableWidgetO*>(ptr) };

        d_widget->SaveSucceeded(version);
    }
}

void MainWindow::RDenyOperation()
{
    utils::ShowMessage(QMessageBox::Information, tr("Invalid Operation"),
        tr("The operation you attempted is invalid because your local data is outdated. Please refresh and try again."), time_const::kAutoCloseMs);
}

void MainWindow::RPermissionDeny()
{
    utils::ShowMessage(
        QMessageBox::Information, tr("Permission Denied"), tr("You do not have permission to perform this operation."), time_const::kAutoCloseMs);
}

void MainWindow::RSelectNode(Section section, const QUuid& node_id)
{
    auto* sc { GetSectionContex(section) };
    auto tree_model { sc->tree_model };

    const auto index { tree_model->GetIndex(node_id) };
    if (!index.isValid())
        return;

    sc->tree_view->setCurrentIndex(index);
}

void MainWindow::InsertNodeO(const QModelIndex& parent_index)
{
    // Extract frequently used shortcuts
    auto& section_config = sc_->section_config;
    auto tab_widget = sc_->tab_widget;
    auto* tab_bar = tab_widget->tabBar();
    auto* parent_node { sc_->tree_model->GetNodeByIndex(parent_index) };

    NodeO* node { static_cast<NodeO*>(NodePool::Instance().Allocate(start_)) };
    node->id = QUuid::createUuidV7();
    node->direction_rule = parent_node->direction_rule;
    node->unit = parent_index.isValid() ? parent_node->unit : NodeUnit(sc_->shared_config.default_unit);
    node->parent = parent_node;
    node->issued_time = QDateTime::currentDateTime();
    node->code = utils::UuidToShortCode(node->id);

    const QUuid node_id { node->id };

    // Prepare dependencies
    auto tree_model_p { sc_p_.tree_model };
    auto tree_model_i { sc_i_.tree_model };

    // Create model and widget
    TableModelArg table_model_arg { sc_->info, node_id, true };
    auto* table_model { new TableModelO(table_model_arg, this) };
    table_model->SetNode(node);

    OrderWidgetArg order_arg {
        table_model,
        tree_model_p,
        app_config_,
        section_config,
        start_,
    };
    auto* widget { new TableWidgetO(order_arg, node, this) };
    WidgetContext wc { widget, node_id, WidgetRole::kNodeTabO };

    // Setup tab
    const int tab_index { tab_widget->addTab(widget, QString()) };
    tab_bar->setTabData(tab_index, node_id);
    tab_widget->setCurrentIndex(tab_index);

    // Configure view
    auto* view { widget->View() };
    TableConnectO(table_model, widget);
    SetTableView(view, sc_->info.section, std::to_underlying(EntryEnumO::kDescription));
    TableDelegateO(view, sc_->info, section_config);

    view->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(view, &QWidget::customContextMenuRequested, this, &MainWindow::RTableViewCustomContextMenuRequested);

    sc_->widget_hash.insert(node_id, wc);
}

void MainWindow::CreateLeafO(SectionContext* sc, const QUuid& node_id)
{
    // Extract frequently used shortcuts
    auto& section_config = sc->section_config;
    auto tab_widget = sc_->tab_widget;
    auto* tab_bar = tab_widget->tabBar();

    // Validate node
    auto* node { static_cast<NodeO*>(sc_->tree_model->GetNode(node_id)) };
    if (!node) {
        return;
    }

    NodeO* tmp_node { static_cast<NodeO*>(NodePool::Instance().Allocate(start_)) };
    *tmp_node = *node;

    const auto partner_id { tmp_node->partner_id };
    Q_ASSERT(!partner_id.isNull());

    // Prepare dependencies
    auto tree_model_p { sc_p_.tree_model };
    auto tree_model_i { sc_i_.tree_model };

    // Create model and widget
    TableModelArg table_model_arg { sc->info, node_id, tmp_node->direction_rule };
    auto* table_model = new TableModelO(table_model_arg, nullptr);
    table_model->SetNode(tmp_node);

    OrderWidgetArg order_arg {
        table_model,
        tree_model_p,
        app_config_,
        section_config,
        start_,
    };
    auto* widget = new TableWidgetO(order_arg, tmp_node, this);
    WidgetContext wc { widget, node_id, WidgetRole::kNodeTabO };

    // Setup tab
    const QString partner_name { tree_model_p->Name(partner_id) };
    const int tab_index { tab_widget->addTab(widget, partner_name) };

    tab_bar->setTabToolTip(tab_index, sc_->tree_model->Path(node->parent->id) + app_config_.separator + partner_name);
    tab_bar->setTabData(tab_index, node_id);

    // Configure view
    auto* view = widget->View();
    SetTableView(view, sc->info.section, std::to_underlying(EntryEnumO::kDescription));
    TableConnectO(table_model, widget);
    TableDelegateO(view, sc->info, section_config);

    view->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(view, &QWidget::customContextMenuRequested, this, &MainWindow::RTableViewCustomContextMenuRequested);

    sc->widget_hash.insert(node_id, wc);
    TableSStation::Instance()->RegisterModel(node_id, table_model);
}

void MainWindow::RSyncPartner(const QUuid& parent_id, const QUuid& node_id, const QUuid& partner_id)
{
    auto widget { sc_->tab_widget };
    auto* tab_bar { widget->tabBar() };
    const int count { widget->count() };

    for (int index = 0; index != count; ++index) {
        if (tab_bar->tabData(index).toUuid() == node_id) {
            const QString partner_name { sc_p_.tree_model->Name(partner_id) };

            tab_bar->setTabText(index, partner_name);
            tab_bar->setTabToolTip(index, sc_->tree_model->Path(parent_id) + app_config_.separator + partner_name);

            break;
        }
    }
}
