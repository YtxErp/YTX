#include "mainwindow.h"

#include <QtConcurrent/qtconcurrentrun.h>

#include <QCloseEvent>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QNetworkReply>
#include <QQueue>
#include <QScrollBar>
#include <QShortcut>
#include <QUrl>

#include "component/constant.h"
#include "component/signalblocker.h"
#include "dialog/about.h"
#include "dialog/preferences.h"
#include "global/tablesstation.h"
#include "ui_mainwindow.h"
#include "utils/mainwindowutils.h"
#include "utils/templateutils.h"
#include "websocket/websocket.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , network_manager_(new QNetworkAccessManager(this))
{
    ReadLocalConfig();

    ui->setupUi(this);
    SignalBlocker blocker(this);

    IniSectionGroup();
    IniMarkGroup();
    InitSystemTray();
    InitStatusLabel();

    SetTabWidget(ui->tabWidgetF);
    SetTabWidget(ui->tabWidgetI);
    SetTabWidget(ui->tabWidgetT);
    SetTabWidget(ui->tabWidgetP);
    SetTabWidget(ui->tabWidgetSale);
    SetTabWidget(ui->tabWidgetPurchase);

    SetIcon();
    SetUniqueConnection();
    SetAction(false);
    SetLoginAction(false);

    utils::SetConnectionStatus(connection_label_, ConnectionState::Connecting);
    utils::SetLoginStatus(login_label_, LoginState::LoggedOut);
    utils::ReadConfig(ui->splitter, &QSplitter::restoreState, app_settings_, kSplitter, kState);
    utils::ReadConfig(this, &QMainWindow::restoreState, app_settings_, kMainwindow, kState, 0);
    utils::ReadConfig(this, &QMainWindow::restoreGeometry, app_settings_, kMainwindow, kGeometry);

    QTimer::singleShot(0, this, &::MainWindow::InitilizeContext);

    ui->actionDelete->setShortcut(QKeySequence::Delete);
}

QSet<QString> MainWindow::ChildrenName(const Node* node) const
{
    QSet<QString> set {};

    if (!node || node->kind != NodeKind::kBranch || node->children.isEmpty())
        return set;

    set.reserve(node->children.size());

    for (const auto* child : std::as_const(node->children)) {
        if (child)
            set.insert(child->name);
    }

    return set;
}

QSet<QUuid> MainWindow::LeafChildrenId(const Node* node) const
{
    QSet<QUuid> set {};

    if (!node || node->kind != NodeKind::kBranch || node->children.isEmpty())
        return set;

    QQueue<const Node*> queue {};
    queue.enqueue(node);

    while (!queue.isEmpty()) {
        auto* queue_node = queue.dequeue();
        const NodeKind kind { queue_node->kind };

        switch (kind) {
        case NodeKind::kBranch:
            for (const auto* child : queue_node->children)
                queue.enqueue(child);
            break;
        case NodeKind::kLeaf:
            set.insert(queue_node->id);
            break;
        default:
            break;
        }
    }

    return set;
}

MainWindow::~MainWindow()
{
    WriteConfig();
    delete ui;
}

// FocusTabWidget - Activate and focus the leaf widget tab
// -------------------------------
// Used for:
//  • Node double-click → navigate to node's tab
//  • Entry jump → switch between different node tabs
//
// Note: Clears view selection after switching
void MainWindow::FocusTabWidget(const QUuid& node_id) const
{
    auto widget { qobject_cast<TableWidget*>(sc_->widget_hash.value(node_id).widget) };
    Q_ASSERT_X(widget, Q_FUNC_INFO, "Table widget not found for node_id");

    sc_->tab_widget->setCurrentWidget(widget);
    widget->activateWindow();

    widget->View()->setCurrentIndex(QModelIndex());
    widget->View()->clearSelection();
}

void MainWindow::InsertNodeFunction(const QModelIndex& parent_index)
{
    switch (start_) {
    case Section::kSale:
    case Section::kPurchase:
        InsertNodeO(parent_index);
        return;
    case Section::kFinance:
    case Section::kTask:
    case Section::kPartner:
    case Section::kInventory:
        InsertNodeFIPT(parent_index);
        return;
    }

    Q_UNREACHABLE();
}

void MainWindow::on_actionDelete_triggered()
{
    qInfo() << Q_FUNC_INFO;

    if (QApplication::activeWindow() != this)
        return;

    auto* widget { sc_->tab_widget->currentWidget() };

    if (qobject_cast<TreeWidget*>(widget)) {
        DeleteNode();
        return;
    }

    if (auto* d_widget { qobject_cast<TableWidget*>(widget) }) {
        DeleteEntry(d_widget);
        return;
    }
}

void MainWindow::ResetMainwindow()
{
    section_settings_.clear();

    ui->actionWorkspaceManager->setVisible(false);
    ui->actionAuditLog->setVisible(false);

    utils::CloseWidgets(widget_hash_);

    {
        utils::ResetSectionContext(sc_f_);
        utils::ResetSectionContext(sc_i_);
        utils::ResetSectionContext(sc_t_);
        utils::ResetSectionContext(sc_p_);
        utils::ResetSectionContext(sc_sale_);
        utils::ResetSectionContext(sc_purchase_);
    }

    {
        ui->actionName->setText(tr("Name", "Person"));
        ui->actionWorkspace->setText(tr("Workspace"));
        ui->actionExpireDate->setText(tr("Expire Date"));
    }

    {
        ui->actionReconnect->setEnabled(true);
        ui->actionSignIn->setEnabled(false);
        ui->actionSignOut->setEnabled(false);
    }
}

void MainWindow::IniSectionGroup()
{
    section_group_ = new QButtonGroup(this);

    section_group_->addButton(ui->rBtnFinance, std::to_underlying(Section::kFinance));
    section_group_->addButton(ui->rBtnTask, std::to_underlying(Section::kTask));
    section_group_->addButton(ui->rBtnInventory, std::to_underlying(Section::kInventory));
    section_group_->addButton(ui->rBtnPartner, std::to_underlying(Section::kPartner));
    section_group_->addButton(ui->rBtnSale, std::to_underlying(Section::kSale));
    section_group_->addButton(ui->rBtnPurchase, std::to_underlying(Section::kPurchase));

    const QList<QPair<QKeySequence, int>> shortcuts = {
        { Qt::ALT | Qt::Key_1, std::to_underlying(Section::kFinance) },
        { Qt::ALT | Qt::Key_2, std::to_underlying(Section::kTask) },
        { Qt::ALT | Qt::Key_3, std::to_underlying(Section::kInventory) },
        { Qt::ALT | Qt::Key_4, std::to_underlying(Section::kPartner) },
        { Qt::ALT | Qt::Key_5, std::to_underlying(Section::kSale) },
        { Qt::ALT | Qt::Key_6, std::to_underlying(Section::kPurchase) },
    };

    for (auto& [key, id] : shortcuts) {
        auto* shortcut { new QShortcut(key, this) };
        connect(shortcut, &QShortcut::activated, this, [this, id]() {
            if (auto* btn { section_group_->button(id) })
                btn->click();
        });
    }
}

void MainWindow::IniMarkGroup()
{
    mark_group_ = new QActionGroup(this);

    ui->actionMarkAll->setData(std::to_underlying(MarkOperation::kSelect));
    ui->actionMarkNone->setData(std::to_underlying(MarkOperation::kClear));
    ui->actionMarkToggle->setData(std::to_underlying(MarkOperation::kToggle));

    mark_group_->addAction(ui->actionMarkAll);
    mark_group_->addAction(ui->actionMarkNone);
    mark_group_->addAction(ui->actionMarkToggle);

    connect(mark_group_, &QActionGroup::triggered, this, [this](QAction* action) {
        const MarkOperation operation { action->data().toInt() };

        auto* widget { sc_->tab_widget->currentWidget() };

        if (IsTableWidgetFIPT(widget)) {
            RMarkEntries(operation);
            return;
        }

        if (IsStatementTertiaryWidget(widget)) {
            RStatementMarkEntries(operation);
        }
    });
}

void MainWindow::RFreeWidget(Section section, const QUuid& node_id)
{
    auto* sc { GetSectionContex(section) };

    utils::CloseWidget(node_id, sc->widget_hash);
    TableSStation::Instance()->DeregisterModel(node_id);
}

void MainWindow::RFlushCaches()
{
    auto* widget { sc_->tab_widget->currentWidget() };

    if (qobject_cast<TreeWidget*>(widget)) {
        sc_->tree_model->FlushTimers();
    }

    if (auto* leaf_widget { qobject_cast<TableWidget*>(widget) }) {
        leaf_widget->Model()->FlushTimers();
    }

    FlushCaches(sc_f_);
    FlushCaches(sc_i_);
    FlushCaches(sc_p_);
    FlushCaches(sc_t_);

    QCoreApplication::processEvents(QEventLoop::AllEvents, 500);
}

void MainWindow::FlushCaches(SectionContext& sc)
{
    // First, flush the caches of the tree model
    if (sc.tree_model)
        sc.tree_model->FlushTimers();

    // Iterate through all views in the view_hash
    for (auto it = sc.widget_hash.begin(); it != sc.widget_hash.end(); ++it) {
        const auto& wc = it.value();

        // Skip if the widget pointer is null
        if (!wc.widget)
            continue;

        // Only process specific NodeTab types
        if (wc.role != WidgetRole::kNodeTabFIT && wc.role != WidgetRole::kNodeTabO)
            continue;

        // Safely cast the widget to TableWidget
        if (auto* table_widget = qobject_cast<TableWidget*>(wc.widget)) {
            // Flush caches of the model if it exists
            if (auto* model = table_widget->Model())
                model->FlushTimers();
        }
    }
}

void MainWindow::RegisterWidget(QWidget* widget, const QUuid& widget_id, WidgetRole role)
{
    Q_ASSERT(widget);

    WidgetContext wc { widget, widget_id, role };

    sc_->widget_hash.insert(widget_id, wc);

    sc_->tab_widget->setCurrentWidget(widget);
}

void MainWindow::WriteConfig()
{
    if (app_settings_) {
        utils::WriteConfig(ui->splitter, &QSplitter::saveState, app_settings_, kSplitter, kState);
        utils::WriteConfig(this, &QMainWindow::saveState, app_settings_, kMainwindow, kState, 0);
        utils::WriteConfig(this, &QMainWindow::saveGeometry, app_settings_, kMainwindow, kGeometry);
        utils::WriteConfig(app_settings_, std::to_underlying(start_), kStart, kSection);
    }
}

SectionContext* MainWindow::GetSectionContex(Section section)
{
    switch (section) {
    case Section::kFinance:
        return &sc_f_;
    case Section::kPartner:
        return &sc_p_;
    case Section::kInventory:
        return &sc_i_;
    case Section::kTask:
        return &sc_t_;
    case Section::kSale:
        return &sc_sale_;
    case Section::kPurchase:
        return &sc_purchase_;
    }

    Q_UNREACHABLE();
}

void MainWindow::InitSystemTray()
{
    tray_icon_ = new QSystemTrayIcon(this);

#if defined(Q_OS_WIN)
    tray_icon_->setIcon(QIcon(":/logo/logo/logo.ico"));
#elif defined(Q_OS_MACOS)
    tray_icon_->setIcon(QIcon(":/logo/logo/logo.icns"));

    connect(qApp, &QApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
        if (state == Qt::ApplicationActive && !this->isVisible()) {
            this->showNormal();
            this->raise();
            this->activateWindow();
        }
    });
#else
    tray_icon_->setIcon(QIcon(":/logo/logo/logo.png"));
#endif

    tray_icon_->setToolTip("YTX");

    tray_menu_ = new QMenu(this);
    tray_menu_->addAction(tr("Show Window"), this, [this] {
        this->showNormal();
        this->raise();
        this->activateWindow();
    });

    tray_menu_->addSeparator();
    tray_menu_->addAction(tr("Quit"), qApp, &QCoreApplication::quit);

    tray_icon_->setContextMenu(tray_menu_);
    tray_icon_->show();

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    connect(tray_icon_, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            this->showNormal();
            this->raise();
            this->activateWindow();
        }
    });
#endif
}

void MainWindow::InitStatusLabel()
{
    login_label_ = new QLabel(this);
    ui->statusbar->addPermanentWidget(login_label_);

    connection_label_ = new QLabel(this);
    ui->statusbar->addPermanentWidget(connection_label_);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (tray_icon_ && tray_icon_->isVisible()) {
        hide();
        event->ignore();
        return;
    }

    QMainWindow::closeEvent(event);
}

void MainWindow::SetUniqueConnection() const
{
    connect(qApp, &QCoreApplication::aboutToQuit, this, &MainWindow::RFlushCaches);
    connect(ui->actionQuit, &QAction::triggered, qApp, &QCoreApplication::quit);

    connect(section_group_, &QButtonGroup::idClicked, this, &MainWindow::RSectionGroup);

    connect(WebSocket::Instance(), &WebSocket::SLeafDeleteDeny, this, &MainWindow::RDenyLeafDelete);
    connect(WebSocket::Instance(), &WebSocket::SSharedConfigApply, this, &MainWindow::RApplySharedConfig);
    connect(WebSocket::Instance(), &WebSocket::STagApply, this, &MainWindow::RApplyTag);
    connect(WebSocket::Instance(), &WebSocket::STagInsert, this, &MainWindow::RInsertTag);
    connect(WebSocket::Instance(), &WebSocket::STagUpdate, this, &MainWindow::RUpdateTag);
    connect(WebSocket::Instance(), &WebSocket::STagDelete, this, &MainWindow::RDeleteTag);
    connect(WebSocket::Instance(), &WebSocket::SDefaultUnitUpdate, this, &MainWindow::RUpdateDefaultUnit);
    connect(WebSocket::Instance(), &WebSocket::SDefaultUnitDeny, this, &MainWindow::RDenyDefaultUnit);
    connect(WebSocket::Instance(), &WebSocket::SDocumentDirUpdate, this, &MainWindow::RUpdateDocumentDir);
    connect(WebSocket::Instance(), &WebSocket::SConnectionAllow, this, &MainWindow::RAllowConnection);
    connect(WebSocket::Instance(), &WebSocket::SConnectionDeny, this, &MainWindow::RDenyConnection);
    connect(WebSocket::Instance(), &WebSocket::SLoginAllow, this, &MainWindow::RAllowLogin);
    connect(WebSocket::Instance(), &WebSocket::SLoginDeny, this, &MainWindow::RDenyLogin);
    connect(WebSocket::Instance(), &WebSocket::SRemoteHostClosed, this, &MainWindow::RRemoteHostClosed);
    connect(WebSocket::Instance(), &WebSocket::SEntrySelect, this, &MainWindow::RSelectEntry);
    connect(WebSocket::Instance(), &WebSocket::SOrderHistory, this, &MainWindow::ROrderHistory);
    connect(WebSocket::Instance(), &WebSocket::SStatementPrimary, this, &MainWindow::RStatementPrimary);
    connect(WebSocket::Instance(), &WebSocket::SStatementSecondary, this, &MainWindow::RStatemetSecondary);
    connect(WebSocket::Instance(), &WebSocket::SStatementTertiary, this, &MainWindow::RStatementTertiary);
    connect(WebSocket::Instance(), &WebSocket::SSettlementPrimary, this, &MainWindow::RSettlementPrimary);
    connect(WebSocket::Instance(), &WebSocket::SSettlementSecondary, this, &MainWindow::RSettlementSecondary);
    connect(WebSocket::Instance(), &WebSocket::SSettlementInsert, this, &MainWindow::RInsertSettlement);
    connect(WebSocket::Instance(), &WebSocket::SSettlementRecall, this, &MainWindow::RRecallSettlement);
    connect(WebSocket::Instance(), &WebSocket::SSettlementUpdate, this, &MainWindow::RUpdateSettlement);
    connect(WebSocket::Instance(), &WebSocket::SSettlementView, this, &MainWindow::RSettlementView);
    connect(WebSocket::Instance(), &WebSocket::SOrderRelease, this, &MainWindow::RReleaseOrder);
    connect(WebSocket::Instance(), &WebSocket::SOrderRecall, this, &MainWindow::RRecallOrder);
    connect(WebSocket::Instance(), &WebSocket::SOrderSave, this, &MainWindow::RSaveOrder);
    connect(WebSocket::Instance(), &WebSocket::SOperationDeny, this, &MainWindow::RDenyOperation);
    connect(WebSocket::Instance(), &WebSocket::SNodeSelect, this, &MainWindow::RSelectNode);
    connect(WebSocket::Instance(), &WebSocket::SNodeLocate, this, &MainWindow::RLocateNode);
    connect(WebSocket::Instance(), &WebSocket::STreeSyncFinish, this, &MainWindow::RFinishTreeSync);
    connect(WebSocket::Instance(), &WebSocket::SAccountName, this, &MainWindow::RAccountName);
    connect(WebSocket::Instance(), &WebSocket::SAccountUsername, this, &MainWindow::RAccountUsername);
    connect(WebSocket::Instance(), &WebSocket::SWorkspaceMemberAck, this, &MainWindow::RWorkspaceMemberAck);
    connect(WebSocket::Instance(), &WebSocket::SAccountRoleUpdate, this, &MainWindow::RAccountRoleUpdate);
    connect(WebSocket::Instance(), &WebSocket::SAuditLogAck, this, &MainWindow::RAuditLogAck);
    connect(WebSocket::Instance(), &WebSocket::SInventoryHeatAck, this, &MainWindow::RInventoryHeatAck);
    connect(WebSocket::Instance(), &WebSocket::SPartnerHeatAck, this, &MainWindow::RPartnerHeatAck);
    connect(WebSocket::Instance(), &WebSocket::SBalanceSheetAck, this, &MainWindow::RBalanceSheetAck);
    connect(WebSocket::Instance(), &WebSocket::SIncomeStatementAck, this, &MainWindow::RIncomeStatementAck);
    connect(WebSocket::Instance(), &WebSocket::SCashFlowStatementAck, this, &MainWindow::RCashFlowStatementAck);
}

void MainWindow::SetIcon() const
{
    ui->actionInsertNode->setIcon(QIcon(":/solarized_dark/solarized_dark/insert.png"));
    ui->actionRename->setIcon(QIcon(":/solarized_dark/solarized_dark/edit.png"));
    ui->actionDelete->setIcon(QIcon(":/solarized_dark/solarized_dark/remove.png"));
    ui->actionAbout->setIcon(QIcon(":/solarized_dark/solarized_dark/about.png"));
    ui->actionCheckUpdates->setIcon(QIcon(":/solarized_dark/solarized_dark/update.png"));
    ui->actionAppendNode->setIcon(QIcon(":/solarized_dark/solarized_dark/append.png"));
    ui->actionJumpEntry->setIcon(QIcon(":/solarized_dark/solarized_dark/jump.png"));
    ui->actionPreferences->setIcon(QIcon(":/solarized_dark/solarized_dark/settings.png"));
    ui->actionSearch->setIcon(QIcon(":/solarized_dark/solarized_dark/search.png"));
    ui->actionMarkAll->setIcon(QIcon(":/solarized_dark/solarized_dark/mark-all.png"));
    ui->actionMarkNone->setIcon(QIcon(":/solarized_dark/solarized_dark/mark-none.png"));
    ui->actionMarkToggle->setIcon(QIcon(":/solarized_dark/solarized_dark/mark-toggle.png"));
    ui->actionAppendEntry->setIcon(QIcon(":/solarized_dark/solarized_dark/append_trans.png"));
    ui->actionStatement->setIcon(QIcon(":/solarized_dark/solarized_dark/statement.png"));
    ui->actionSettlement->setIcon(QIcon(":/solarized_dark/solarized_dark/settle.png"));
    ui->actionClearColor->setIcon(QIcon(":/solarized_dark/solarized_dark/reset_color.png"));
    ui->actionNewBranch->setIcon(QIcon(":/solarized_dark/solarized_dark/new-group.png"));
    ui->actionQuit->setIcon(QIcon(":/solarized_dark/solarized_dark/quit.png"));
    ui->actionTags->setIcon(QIcon(":/solarized_dark/solarized_dark/tag.png"));
}

void MainWindow::on_actionInsertNode_triggered()
{
    qInfo() << Q_FUNC_INFO;

    if (QApplication::activeWindow() != this)
        return;

    auto* widget { sc_->tab_widget->currentWidget() };
    if (!IsTreeWidget(widget) && !IsTableWidgetO(widget)) {
        return;
    }

    auto current_index { sc_->tree_view->currentIndex() };
    auto parent_index { current_index.parent() };

    InsertNodeFunction(parent_index);
}

void MainWindow::on_actionAppendNode_triggered()
{
    qInfo() << Q_FUNC_INFO;

    if (QApplication::activeWindow() != this)
        return;

    auto* widget { sc_->tab_widget->currentWidget() };
    if (!IsTreeWidget(widget)) {
        return;
    }

    auto index { sc_->tree_view->currentIndex() };
    if (!index.isValid())
        return;

    auto* node { static_cast<Node*>(index.internalPointer()) };
    if (node->kind != NodeKind::kBranch) {
        return;
    }

    InsertNodeFunction(index);
}

void MainWindow::on_actionRename_triggered()
{
    qInfo() << Q_FUNC_INFO;

    if (QApplication::activeWindow() != this)
        return;

    auto* widget { sc_->tab_widget->currentWidget() };
    if (!IsTreeWidget(widget)) {
        return;
    }

    switch (start_) {
    case Section::kSale:
    case Section::kPurchase:
        EditNameO();
        return;
    case Section::kFinance:
    case Section::kTask:
    case Section::kPartner:
    case Section::kInventory:
        EditNameFIPT();
        return;
    }

    Q_UNREACHABLE();
}

void MainWindow::RUpdateConfig(const AppConfig& app, const SharedConfig& shared, const SectionConfig& section)
{
    UpdateAppConfig(app);
    UpdateSectionConfig(section);
    UpdateSharedConfig(shared);
}

void MainWindow::ResizeColumn(QHeaderView* header, int stretch_column) const
{
    if (stretch_column < 0)
        return;

    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    header->setSectionResizeMode(stretch_column, QHeaderView::Stretch);
}

void MainWindow::on_actionPreferences_triggered()
{
    qInfo() << Q_FUNC_INFO;

    auto model { sc_->tree_model };

    auto* dialog { new Preferences(model, sc_->info, app_config_, sc_->shared_config, sc_->section_config, this) };

    utils::ManageDialog(sc_->widget_hash, dialog);
    dialog->setWindowModality(Qt::WindowModal);

    connect(dialog, &Preferences::SUpdateConfig, this, &MainWindow::RUpdateConfig);
    dialog->show();
}

void MainWindow::on_actionAbout_triggered()
{
    qInfo() << Q_FUNC_INFO;

    static QPointer<About> dialog {};

    if (!dialog) {
        dialog = new About(this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
    }

    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

void MainWindow::RSectionGroup(int id)
{
    qInfo() << Q_FUNC_INFO << kSectionString.value(Section(id));

    const Section section { id };
    start_ = section;

    utils::SwitchDialog(sc_, false);

    switch (section) {
    case Section::kFinance:
        sc_ = &sc_f_;
        break;
    case Section::kInventory:
        sc_ = &sc_i_;
        break;
    case Section::kTask:
        sc_ = &sc_t_;
        break;
    case Section::kPartner:
        sc_ = &sc_p_;
        break;
    case Section::kSale:
        sc_ = &sc_sale_;
        break;
    case Section::kPurchase:
        sc_ = &sc_purchase_;
        break;
    default:
        break;
    }

    ui->stackedWidget->setCurrentIndex(id);
    tabWidget_currentChanged();

    utils::SwitchDialog(sc_, true);
}

void MainWindow::on_actionCheckUpdates_triggered()
{
    qInfo() << Q_FUNC_INFO;

    const QString url { "https://api.github.com/repos/ytxerp/ytx-client/releases/latest" };

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "YTX-Updater");

    QNetworkReply* reply = network_manager_->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            utils::ShowMessage(QMessageBox::Warning, tr("Update Check"), tr("Failed to check updates."), time_const::kAutoCloseMs);
            return;
        }

        const QByteArray data { reply->readAll() };
        QJsonDocument doc { QJsonDocument::fromJson(data) };
        if (!doc.isObject()) {
            utils::ShowMessage(QMessageBox::Warning, tr("Update Check"), tr("Invalid update information received."), time_const::kAutoCloseMs);
            return;
        }

        const QJsonObject obj { doc.object() };
        const QString latest_tag { obj.value("tag_name").toString() };

        const bool is_chinese { app_config_.language.startsWith("zh", Qt::CaseInsensitive) };
        const QString download_url
            = is_chinese ? "https://gitee.com/ytxerp/ytx-client/releases/latest" : "https://github.com/ytxerp/ytx-client/releases/latest";

        if (utils::CompareVersion(latest_tag, QCoreApplication::applicationVersion()) > 0) {
            auto* dlg = utils::CreateMessage(QMessageBox::Information, tr("Update Available"),
                tr("A new version %1 is available.\n\nWould you like to download it now?").arg(latest_tag), true, QMessageBox::Yes | QMessageBox::No, this);

            dlg->setDefaultButton(QMessageBox::Yes);

            connect(dlg, &QMessageBox::finished, this, [download_url](int ret) {
                if (ret == QMessageBox::Yes) {
                    QDesktopServices::openUrl(QUrl(download_url));
                }
            });

            dlg->show();
        } else {
            utils::ShowMessage(QMessageBox::Information, tr("No Update"), tr("You are using the latest version."), time_const::kAutoCloseMs);
        }
    });
}
