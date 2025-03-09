#include "mainwindow.h"

#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QMessageBox>
#include <QMimeData>
#include <QQueue>
#include <QResource>
#include <QScrollBar>
#include <QtConcurrent>

#include "component/classparams.h"
#include "component/constvalue.h"
#include "component/enumclass.h"
#include "component/signalblocker.h"
#include "component/stringinitializer.h"
#include "database/sqlite/sqlitefinance.h"
#include "database/sqlite/sqliteorder.h"
#include "database/sqlite/sqliteproduct.h"
#include "database/sqlite/sqlitestakeholder.h"
#include "database/sqlite/sqlitetask.h"
#include "delegate/checkbox.h"
#include "delegate/document.h"
#include "delegate/doublespin.h"
#include "delegate/line.h"
#include "delegate/readonly/colorr.h"
#include "delegate/readonly/datetimer.h"
#include "delegate/readonly/doublespinr.h"
#include "delegate/readonly/doublespinrnonezero.h"
#include "delegate/readonly/doublespinunitr.h"
#include "delegate/readonly/doublespinunitrnonezero.h"
#include "delegate/readonly/doublespinunitrps.h"
#include "delegate/readonly/nodenamer.h"
#include "delegate/readonly/nodepathr.h"
#include "delegate/search/searchpathtabler.h"
#include "delegate/specificunit.h"
#include "delegate/spin.h"
#include "delegate/table/supportid.h"
#include "delegate/table/tablecombo.h"
#include "delegate/table/tabledatetime.h"
#include "delegate/tree/color.h"
#include "delegate/tree/finance/financeforeignr.h"
#include "delegate/tree/order/ordernamer.h"
#include "delegate/tree/stakeholder/taxrate.h"
#include "delegate/tree/treecombo.h"
#include "delegate/tree/treedatetime.h"
#include "delegate/tree/treeplaintext.h"
#include "dialog/about.h"
#include "dialog/editdocument.h"
#include "dialog/editnodename.h"
#include "dialog/insertnode/insertnodefinance.h"
#include "dialog/insertnode/insertnodeorder.h"
#include "dialog/insertnode/insertnodeproduct.h"
#include "dialog/insertnode/insertnodestakeholder.h"
#include "dialog/insertnode/insertnodetask.h"
#include "dialog/preferences.h"
#include "dialog/removenode.h"
#include "dialog/search.h"
#include "document.h"
#include "global/leafsstation.h"
#include "global/resourcepool.h"
#include "global/sqlconnection.h"
#include "global/supportsstation.h"
#include "mainwindowutils.h"
#include "table/model/sortfilterproxymodel.h"
#include "table/model/transmodelf.h"
#include "table/model/transmodelp.h"
#include "table/model/transmodels.h"
#include "table/model/transmodelt.h"
#include "table/statementmodel.h"
#include "table/transrefmodel.h"
#include "tree/model/nodemodelf.h"
#include "tree/model/nodemodelo.h"
#include "tree/model/nodemodelp.h"
#include "tree/model/nodemodels.h"
#include "tree/model/nodemodelt.h"
#include "ui_mainwindow.h"
#include "widget/node/nodewidgetf.h"
#include "widget/node/nodewidgeto.h"
#include "widget/node/nodewidgetpt.h"
#include "widget/node/nodewidgets.h"
#include "widget/support/statementwidget.h"
#include "widget/support/supportwidgetfpts.h"
#include "widget/trans/transwidgetfpts.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    QResource::registerResource(MainWindowUtils::ResourceFile());
    AppSettings();

    ui->setupUi(this);
    SignalBlocker blocker(this);

    SetTabWidget();
    IniSectionGroup();
    StringInitializer::SetHeader(finance_data_.info, product_data_.info, stakeholder_data_.info, task_data_.info, sales_data_.info, purchase_data_.info);
    SetAction();
    SetConnect();

    this->setAcceptDrops(true);

    MainWindowUtils::ReadSettings(ui->splitter, &QSplitter::restoreState, app_settings_, kWindow, kSplitterState);
    MainWindowUtils::ReadSettings(this, &QMainWindow::restoreState, app_settings_, kWindow, kMainwindowState, 0);
    MainWindowUtils::ReadSettings(this, &QMainWindow::restoreGeometry, app_settings_, kWindow, kMainwindowGeometry);

    RestoreRecentFile();
    EnableAction(false);

#ifdef Q_OS_WIN
    ui->actionRemove->setShortcut(Qt::Key_Delete);
    qApp->setWindowIcon(QIcon(":/logo/logo/logo.ico"));
#elif defined(Q_OS_MACOS)
    ui->actionRemove->setShortcut(Qt::Key_Backspace);
    qApp->setWindowIcon(QIcon(":/logo/logo/logo.icns"));
#endif
}

MainWindow::~MainWindow()
{
    MainWindowUtils::WriteSettings(ui->splitter, &QSplitter::saveState, app_settings_, kWindow, kSplitterState);
    MainWindowUtils::WriteSettings(this, &QMainWindow::saveState, app_settings_, kWindow, kMainwindowState, 0);
    MainWindowUtils::WriteSettings(this, &QMainWindow::saveGeometry, app_settings_, kWindow, kMainwindowGeometry);
    MainWindowUtils::WriteSettings(app_settings_, std::to_underlying(start_), kStart, kSection);

    if (lock_file_) {
        MainWindowUtils::WriteSettings(file_settings_, MainWindowUtils::SaveTab(finance_trans_wgt_hash_), kFinance, kTabID);
        MainWindowUtils::WriteSettings(finance_tree_->View()->header(), &QHeaderView::saveState, file_settings_, kFinance, kHeaderState);

        MainWindowUtils::WriteSettings(file_settings_, MainWindowUtils::SaveTab(product_trans_wgt_hash_), kProduct, kTabID);
        MainWindowUtils::WriteSettings(product_tree_->View()->header(), &QHeaderView::saveState, file_settings_, kProduct, kHeaderState);

        MainWindowUtils::WriteSettings(file_settings_, MainWindowUtils::SaveTab(stakeholder_trans_wgt_hash_), kStakeholder, kTabID);
        MainWindowUtils::WriteSettings(stakeholder_tree_->View()->header(), &QHeaderView::saveState, file_settings_, kStakeholder, kHeaderState);

        MainWindowUtils::WriteSettings(file_settings_, MainWindowUtils::SaveTab(task_trans_wgt_hash_), kTask, kTabID);
        MainWindowUtils::WriteSettings(task_tree_->View()->header(), &QHeaderView::saveState, file_settings_, kTask, kHeaderState);

        MainWindowUtils::WriteSettings(sales_tree_->View()->header(), &QHeaderView::saveState, file_settings_, kSales, kHeaderState);
        MainWindowUtils::WriteSettings(purchase_tree_->View()->header(), &QHeaderView::saveState, file_settings_, kPurchase, kHeaderState);
    }

    delete ui;
}

bool MainWindow::ROpenFile(CString& file_path)
{
    if (file_path.isEmpty())
        return false;

    const QFileInfo file_info(file_path);

    if (!MainWindowUtils::CheckFileValid(file_path)) {
        MainWindowUtils::Message(
            QMessageBox::Critical, tr("Invalid File"), tr("The file \"%1\" is invalid. Please check the file and try again.").arg(file_path), kThreeThousand);
        return false;
    }

    if (lock_file_) {
        QProcess::startDetached(qApp->applicationFilePath(), QStringList { file_path });
        return false;
    }

    if (!LockFile(file_info))
        return false;

    SqlConnection::Instance().SetDatabaseName(file_path);

    const auto& complete_base_name { file_info.completeBaseName() };

    this->setWindowTitle(complete_base_name);
    file_settings_ = std::make_unique<QSettings>(
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + kSlash + complete_base_name + kDotSuffixINI, QSettings::IniFormat);

    ytx_sql_ = std::make_unique<YtxSqlite>(start_);
    SetFinanceData();
    SetTaskData();
    SetProductData();
    SetStakeholderData();
    SetSalesData();
    SetPurchaseData();

    CreateSection(finance_tree_, finance_trans_wgt_hash_, finance_data_, finance_settings_, tr("Finance"));
    CreateSection(stakeholder_tree_, stakeholder_trans_wgt_hash_, stakeholder_data_, stakeholder_settings_, tr("Stakeholder"));
    CreateSection(product_tree_, product_trans_wgt_hash_, product_data_, product_settings_, tr("Product"));
    CreateSection(task_tree_, task_trans_wgt_hash_, task_data_, task_settings_, tr("Task"));
    CreateSection(sales_tree_, sales_trans_wgt_hash_, sales_data_, sales_settings_, tr("Sales"));
    CreateSection(purchase_tree_, purchase_trans_wgt_hash_, purchase_data_, purchase_settings_, tr("Purchase"));

    RSectionGroup(static_cast<int>(start_));

    AddRecentFile(file_path);
    EnableAction(true);
    on_tabWidget_currentChanged(0);
    return true;
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        const auto suffix { QFileInfo(event->mimeData()->urls().at(0).fileName()).suffix().toLower() };
        if (suffix == ytx)
            return event->acceptProposedAction();
    }

    event->ignore();
}

void MainWindow::dropEvent(QDropEvent* event) { ROpenFile(event->mimeData()->urls().at(0).toLocalFile()); }

void MainWindow::on_actionInsertNode_triggered()
{
    if (!tree_widget_)
        return;

    auto current_index { tree_widget_->View()->currentIndex() };
    current_index = current_index.isValid() ? current_index : QModelIndex();

    auto parent_index { current_index.parent() };
    parent_index = parent_index.isValid() ? parent_index : QModelIndex();

    const int parent_id { parent_index.isValid() ? parent_index.siblingAtColumn(std::to_underlying(NodeEnum::kID)).data().toInt() : -1 };
    InsertNodeFunction(parent_index, parent_id, current_index.row() + 1);
}

void MainWindow::RTreeViewDoubleClicked(const QModelIndex& index)
{
    if (index.column() != 0)
        return;

    const int type { index.siblingAtColumn(std::to_underlying(NodeEnum::kType)).data().toInt() };
    if (type == kTypeBranch)
        return;

    const int node_id { index.siblingAtColumn(std::to_underlying(NodeEnum::kID)).data().toInt() };
    if (node_id <= 0)
        return;

    switch (type) {
    case kTypeLeaf:
        CreateLeafFunction(type, node_id);
        SwitchToLeaf(node_id);
        break;
    case kTypeSupport:
        CreateSupportFunction(type, node_id);
        SwitchToSupport(node_id);
        break;
    default:
        break;
    }
}

void MainWindow::CreateLeafFunction(int type, int node_id)
{
    if (type != kTypeLeaf || trans_wgt_hash_->contains(node_id))
        return;

    if (start_ == Section::kSales || start_ == Section::kPurchase) {
        if (auto it { dialog_hash_->constFind(node_id) }; it != dialog_hash_->constEnd()) {
            auto dialog { *it };
            dialog->show();
            dialog->raise();
            dialog->activateWindow();
            return;
        }

        CreateLeafO(tree_widget_->Model(), trans_wgt_hash_, data_, settings_, node_id);
        return;
    }

    CreateLeafFPTS(tree_widget_->Model(), trans_wgt_hash_, data_, settings_, node_id);
}

void MainWindow::CreateSupportFunction(int type, int node_id)
{
    if (type != kTypeSupport || sup_wgt_hash_->contains(node_id) || start_ == Section::kSales || start_ == Section::kPurchase)
        return;

    CreateSupport(tree_widget_->Model(), sup_wgt_hash_, data_, settings_, node_id);
}

void MainWindow::RSectionGroup(int id)
{
    const Section kSection { id };

    start_ = kSection;

    if (!lock_file_) {
        MainWindowUtils::Message(QMessageBox::Information, tr("File Required"), tr("Please open the file first."), kThreeThousand);
        return;
    }

    MainWindowUtils::SwitchDialog(dialog_list_, false);
    MainWindowUtils::SwitchDialog(dialog_hash_, false);
    UpdateLastTab();

    switch (kSection) {
    case Section::kFinance:
        tree_widget_ = finance_tree_;
        trans_wgt_hash_ = &finance_trans_wgt_hash_;
        dialog_list_ = &finance_dialog_list_;
        dialog_hash_ = &finance_dialog_hash_;
        settings_ = &finance_settings_;
        data_ = &finance_data_;
        sup_wgt_hash_ = &finance_sup_wgt_hash_;
        break;
    case Section::kProduct:
        tree_widget_ = product_tree_;
        trans_wgt_hash_ = &product_trans_wgt_hash_;
        dialog_list_ = &product_dialog_list_;
        dialog_hash_ = &product_dialog_hash_;
        settings_ = &product_settings_;
        data_ = &product_data_;
        sup_wgt_hash_ = &product_sup_wgt_hash_;
        break;
    case Section::kTask:
        tree_widget_ = task_tree_;
        trans_wgt_hash_ = &task_trans_wgt_hash_;
        dialog_list_ = &task_dialog_list_;
        dialog_hash_ = &task_dialog_hash_;
        settings_ = &task_settings_;
        data_ = &task_data_;
        sup_wgt_hash_ = &task_sup_wgt_hash_;
        break;
    case Section::kStakeholder:
        tree_widget_ = stakeholder_tree_;
        trans_wgt_hash_ = &stakeholder_trans_wgt_hash_;
        dialog_list_ = &stakeholder_dialog_list_;
        dialog_hash_ = &stakeholder_dialog_hash_;
        settings_ = &stakeholder_settings_;
        data_ = &stakeholder_data_;
        sup_wgt_hash_ = &stakeholder_sup_wgt_hash_;
        break;
    case Section::kSales:
        tree_widget_ = sales_tree_;
        trans_wgt_hash_ = &sales_trans_wgt_hash_;
        dialog_list_ = &sales_dialog_list_;
        dialog_hash_ = &sales_dialog_hash_;
        settings_ = &sales_settings_;
        data_ = &sales_data_;
        sup_wgt_hash_ = &sales_sup_wgt_hash_;
        break;
    case Section::kPurchase:
        tree_widget_ = purchase_tree_;
        trans_wgt_hash_ = &purchase_trans_wgt_hash_;
        dialog_list_ = &purchase_dialog_list_;
        dialog_hash_ = &purchase_dialog_hash_;
        settings_ = &purchase_settings_;
        data_ = &purchase_data_;
        sup_wgt_hash_ = &purchase_sup_wgt_hash_;
        break;
    default:
        break;
    }

    SwitchSection(data_->tab);
}

void MainWindow::RTransRefDoubleClicked(const QModelIndex& index)
{
    const int kNodeID { index.siblingAtColumn(std::to_underlying(TransRefEnum::kOrderNode)).data().toInt() };
    const int kColumn { std::to_underlying(TransRefEnum::kNetAmount) };

    if (kNodeID <= 0 || index.column() != kColumn)
        return;

    SalesNodeLocation(kNodeID);
}

void MainWindow::RPrimaryStatement(int party_id, QDateTime start, QDateTime end, double pbalance, double cbalance) { }

void MainWindow::RSecondaryStatement(int party_id, QDateTime start, QDateTime end, double pbalance, double cbalance) { }

void MainWindow::SwitchToLeaf(int node_id, int trans_id) const
{
    auto widget { trans_wgt_hash_->value(node_id, nullptr) };
    if (!widget)
        return;

    ui->tabWidget->setCurrentWidget(widget);
    widget->activateWindow();

    if (trans_id == 0)
        return;

    auto view { widget->View() };
    auto index { widget->Model()->GetIndex(trans_id) };

    if (!index.isValid())
        return;

    view->setCurrentIndex(index);
    view->scrollTo(index.siblingAtColumn(std::to_underlying(TransEnum::kDateTime)), QAbstractItemView::PositionAtCenter);
    view->closePersistentEditor(index);
}

void MainWindow::OrderTransLocation(int node_id)
{
    tree_widget_->Model()->RetrieveNode(node_id);

    if (!trans_wgt_hash_->contains(node_id)) {
        CreateLeafO(tree_widget_->Model(), trans_wgt_hash_, data_, settings_, node_id);
    }
}

void MainWindow::CreateLeafFPTS(PTreeModel tree_model, TransWgtHash* trans_wgt_hash, CData* data, CSettings* settings, int node_id)
{
    if (!tree_model || !trans_wgt_hash || !data || !settings || !tree_model->Contains(node_id))
        return;

    CString name { tree_model->Name(node_id) };
    auto* sql { data->sql };
    const Info& info { data->info };
    const Section section { info.section };
    const bool rule { tree_model->Rule(node_id) };

    TransModel* model {};

    switch (section) {
    case Section::kFinance:
        model = new TransModelF(sql, rule, node_id, info, this);
        break;
    case Section::kProduct:
        model = new TransModelP(sql, rule, node_id, info, this);
        break;
    case Section::kTask:
        model = new TransModelT(sql, rule, node_id, info, this);
        break;
    case Section::kStakeholder:
        model = new TransModelS(sql, rule, node_id, info, this);
        break;
    default:
        break;
    }

    TransWidgetFPTS* widget { new TransWidgetFPTS(model, this) };

    const int tab_index { ui->tabWidget->addTab(widget, name) };
    auto* tab_bar { ui->tabWidget->tabBar() };

    tab_bar->setTabData(tab_index, QVariant::fromValue(Tab { section, node_id }));
    tab_bar->setTabToolTip(tab_index, tree_model->GetPath(node_id));

    auto view { widget->View() };
    SetTableView(view, std::to_underlying(TransEnum::kDescription));
    DelegateFPTS(view, tree_model, settings);

    switch (section) {
    case Section::kFinance:
    case Section::kProduct:
    case Section::kTask:
        TableConnectFPT(view, model, tree_model, data);
        DelegateFPT(view, tree_model, settings, node_id);
        break;
    case Section::kStakeholder:
        TableConnectS(view, model, tree_model, data);
        DelegateS(view);
        break;
    default:
        break;
    }

    trans_wgt_hash->insert(node_id, widget);
    LeafSStation::Instance().RegisterModel(section, node_id, model);
}

void MainWindow::CreateSupport(PTreeModel tree_model, SupWgtHash* sup_wgt_hash, CData* data, CSettings* settings, int node_id)
{
    if (!tree_model || !sup_wgt_hash || !data || !settings || !tree_model->Contains(node_id))
        return;

    CString name { tree_model->Name(node_id) };
    auto* sql { data->sql };
    const Info& info { data->info };
    const Section section { info.section };
    const bool rule { tree_model->Rule(node_id) };

    auto* model { new SupportModel(sql, rule, node_id, info, this) };
    auto* widget { new SupportWidgetFPTS(model, this) };

    const int tab_index { ui->tabWidget->addTab(widget, name) };
    auto* tab_bar { ui->tabWidget->tabBar() };

    tab_bar->setTabData(tab_index, QVariant::fromValue(Tab { section, node_id }));
    tab_bar->setTabToolTip(tab_index, tree_model->GetPath(node_id));

    auto view { widget->View() };
    SetTableView(view, std::to_underlying(TransSearchEnum::kDescription));
    DelegateSupport(view, tree_model, settings);

    view->setColumnHidden(std::to_underlying(TransSearchEnum::kSupportID), true);
    view->setColumnHidden(std::to_underlying(TransSearchEnum::kDiscount), true);

    switch (section) {
    case Section::kStakeholder:
        SetSupportViewS(view);
        DelegateSupportS(view, tree_model, product_tree_->Model());
        break;
    default:
        break;
    }

    sup_wgt_hash->insert(node_id, widget);
    SupportSStation::Instance().RegisterModel(section, node_id, model);

    connect(data->sql, &Sqlite::SRemoveMultiSupportTrans, model, &SupportModel::RemoveMultiSupportTrans);
}

void MainWindow::CreateLeafO(PTreeModel tree_model, TransWgtHash* trans_wgt_hash, CData* data, CSettings* settings, int node_id)
{
    const auto& info { data->info };
    const Section section { info.section };

    if (section != Section::kSales && section != Section::kPurchase)
        return;

    Node* node { tree_model->GetNodeO(node_id) };
    const int party_id { node->party };

    if (party_id <= 0)
        return;

    auto* sql { data->sql };

    TransModelO* model { new TransModelO(sql, true, node_id, info, node, product_tree_->Model(), stakeholder_data_.sql, this) };
    auto params { EditNodeParamsO { node, sql, model, stakeholder_tree_->Model(), settings_, section } };

    TransWidgetO* widget { new TransWidgetO(std::move(params), this) };

    const int tab_index { ui->tabWidget->addTab(widget, stakeholder_tree_->Model()->Name(party_id)) };
    auto* tab_bar { ui->tabWidget->tabBar() };

    tab_bar->setTabData(tab_index, QVariant::fromValue(Tab { section, node_id }));
    tab_bar->setTabToolTip(tab_index, stakeholder_tree_->Model()->GetPath(party_id));

    auto view { widget->View() };
    SetTableView(view, std::to_underlying(TransEnumO::kDescription));

    TableConnectO(view, model, tree_model, widget);
    DelegateO(view, settings);

    trans_wgt_hash->insert(node_id, widget);
}

void MainWindow::on_actionStatement_triggered()
{
    if (start_ != Section::kSales && start_ != Section::kPurchase)
        return;

    auto* sql { data_->sql };
    const auto& info { data_->info };

    auto* model { new StatementModel(sql, info, this) };
    auto* widget { new StatementWidget(model, this) };

    const int tab_index { ui->tabWidget->addTab(widget, tr("Statement")) };
    auto* tab_bar { ui->tabWidget->tabBar() };

    tab_bar->setTabData(tab_index, QVariant::fromValue(Tab { start_, statement_id }));
    tab_bar->setTabToolTip(tab_index, tr("Statement"));

    auto view { widget->View() };
    SetStatementView(view);
    DelegateStatement(view, settings_);

    connect(widget, &StatementWidget::SPrimaryStatement, this, &MainWindow::RPrimaryStatement);
    connect(widget, &StatementWidget::SSecondaryStatement, this, &MainWindow::RSecondaryStatement);

    sup_wgt_hash_->insert(statement_id, widget);
    --statement_id;
}

void MainWindow::TableConnectFPT(PTableView table_view, PTableModel table_model, PTreeModel tree_model, const Data* data) const
{
    connect(table_model, &TransModel::SResizeColumnToContents, table_view, &QTableView::resizeColumnToContents);
    connect(table_model, &TransModel::SSearch, tree_model, &NodeModel::RSearch);

    connect(table_model, &TransModel::SUpdateLeafValue, tree_model, &NodeModel::RUpdateLeafValue);
    connect(table_model, &TransModel::SSyncDouble, tree_model, &NodeModel::RSyncDouble);

    connect(table_model, &TransModel::SRemoveOneTrans, &LeafSStation::Instance(), &LeafSStation::RRemoveOneTrans);
    connect(table_model, &TransModel::SAppendOneTrans, &LeafSStation::Instance(), &LeafSStation::RAppendOneTrans);
    connect(table_model, &TransModel::SUpdateBalance, &LeafSStation::Instance(), &LeafSStation::RUpdateBalance);
    connect(table_model, &TransModel::SRemoveSupportTrans, &SupportSStation::Instance(), &SupportSStation::RRemoveSupportTrans);
    connect(table_model, &TransModel::SAppendSupportTrans, &SupportSStation::Instance(), &SupportSStation::RAppendSupportTrans);

    connect(data->sql, &Sqlite::SRemoveMultiTrans, table_model, &TransModel::RRemoveMultiTrans);
    connect(data->sql, &Sqlite::SMoveMultiTrans, table_model, &TransModel::RMoveMultiTrans);
}

void MainWindow::TableConnectO(PTableView table_view, TransModelO* table_model, PTreeModel tree_model, TransWidgetO* widget) const
{
    connect(table_model, &TransModel::SSearch, tree_model, &NodeModel::RSearch);
    connect(table_model, &TransModel::SResizeColumnToContents, table_view, &QTableView::resizeColumnToContents);

    connect(table_model, &TransModel::SUpdateLeafValue, widget, &TransWidgetO::RUpdateLeafValue);
    connect(widget, &TransWidgetO::SUpdateLeafValue, tree_model, &NodeModel::RUpdateLeafValue);

    connect(widget, &TransWidgetO::SSyncInt, table_model, &TransModel::RSyncInt);
    connect(widget, &TransWidgetO::SSyncBool, table_model, &TransModel::RSyncBool);

    connect(widget, &TransWidgetO::SSyncInt, this, &MainWindow::RSyncInt);

    connect(widget, &TransWidgetO::SSyncBool, tree_model, &NodeModel::RSyncBool);
    connect(tree_model, &NodeModel::SSyncBool, widget, &TransWidgetO::RSyncBool);
    connect(tree_model, &NodeModel::SSyncInt, widget, &TransWidgetO::RSyncInt);
    connect(tree_model, &NodeModel::SSyncString, widget, &TransWidgetO::RSyncString);
}

void MainWindow::TableConnectS(PTableView table_view, PTableModel table_model, PTreeModel tree_model, const Data* data) const
{
    connect(table_model, &TransModel::SResizeColumnToContents, table_view, &QTableView::resizeColumnToContents);
    connect(table_model, &TransModel::SSearch, tree_model, &NodeModel::RSearch);

    connect(data->sql, &Sqlite::SMoveMultiTrans, table_model, &TransModel::RMoveMultiTrans);
    connect(data->sql, &Sqlite::SRemoveMultiTrans, table_model, &TransModel::RRemoveMultiTrans);

    connect(table_model, &TransModel::SRemoveSupportTrans, &SupportSStation::Instance(), &SupportSStation::RRemoveSupportTrans);
    connect(table_model, &TransModel::SAppendSupportTrans, &SupportSStation::Instance(), &SupportSStation::RAppendSupportTrans);
}

void MainWindow::DelegateFPTS(PTableView table_view, PTreeModel tree_model, CSettings* settings) const
{
    auto* date_time { new TableDateTime(settings->date_format, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnum::kDateTime), date_time);

    auto* line { new Line(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnum::kDescription), line);
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnum::kCode), line);

    auto* state { new CheckBox(QEvent::MouseButtonRelease, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnum::kState), state);

    auto* document { new Document(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnum::kDocument), document);
    connect(document, &Document::SEditDocument, this, &MainWindow::REditTransDocument);

    auto* lhs_ratio { new DoubleSpin(settings->common_decimal, 0, std::numeric_limits<double>::max(), kCoefficient8, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnum::kLhsRatio), lhs_ratio);

    auto* support_node { new SupportID(tree_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnum::kSupportID), support_node);
}

void MainWindow::DelegateFPT(PTableView table_view, PTreeModel tree_model, CSettings* settings, int node_id) const
{
    auto* value { new DoubleSpin(
        settings->common_decimal, -std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), kCoefficient16, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnumF::kDebit), value);
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnumF::kCredit), value);

    auto* subtotal { new DoubleSpinR(settings->common_decimal, kCoefficient16, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnumF::kSubtotal), subtotal);

    auto* filter_model { new SortFilterProxyModel(node_id, table_view) };
    filter_model->setSourceModel(tree_model->LeafModel());

    auto* node { new TableCombo(tree_model, filter_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnumF::kRhsNode), node);
}

void MainWindow::DelegateS(PTableView table_view) const
{
    auto* product_tree_model { product_tree_->Model().data() };
    auto* inside_product { new SpecificUnit(product_tree_model, product_tree_model->UnitModelPS(), table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnumS::kInsideProduct), inside_product);
}

void MainWindow::DelegateO(PTableView table_view, CSettings* settings) const
{
    auto* product_tree_model { product_tree_->Model().data() };
    auto* inside_product { new SpecificUnit(product_tree_model, product_tree_model->UnitModelPS(), table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnumO::kInsideProduct), inside_product);

    auto stakeholder_tree_model { stakeholder_tree_->Model() };
    auto* support_node { new SupportID(stakeholder_tree_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnumO::kOutsideProduct), support_node);

    auto* color { new ColorR(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnumO::kColor), color);

    auto* line { new Line(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnumO::kDescription), line);
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnumO::kCode), line);

    auto* price { new DoubleSpin(settings->amount_decimal, 0, std::numeric_limits<double>::max(), kCoefficient8, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnumO::kUnitPrice), price);
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnumO::kDiscountPrice), price);

    auto* quantity { new DoubleSpin(
        settings->common_decimal, -std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), kCoefficient8, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnumO::kFirst), quantity);
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnumO::kSecond), quantity);

    auto* amount { new DoubleSpinRNoneZero(settings->amount_decimal, kCoefficient16, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnumO::kGrossAmount), amount);
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnumO::kDiscount), amount);
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnumO::kNetAmount), amount);
}

void MainWindow::CreateSection(NodeWidget* tree_widget, TransWgtHash& trans_wgt_hash, CData& data, CSettings& settings, CString& name)
{
    const auto& info { data.info };
    auto* tab_widget { ui->tabWidget };

    auto view { tree_widget->View() };
    auto model { tree_widget->Model() };

    SetDelegate(view, info, settings);
    TreeConnect(tree_widget, data.sql);

    tab_widget->tabBar()->setTabData(tab_widget->addTab(tree_widget, name), QVariant::fromValue(Tab { info.section, 0 }));

    MainWindowUtils::ReadSettings(view->header(), &QHeaderView::restoreState, file_settings_, info.node, kHeaderState);

    switch (info.section) {
    case Section::kFinance:
    case Section::kTask:
    case Section::kProduct:
    case Section::kStakeholder:
        RestoreTab(model, trans_wgt_hash, MainWindowUtils::ReadSettings(file_settings_, info.node, kTabID), data, settings);
        break;
    default:
        break;
    }

    SetTreeView(view, info);
}

void MainWindow::SetDelegate(PTreeView tree_view, CInfo& info, CSettings& settings) const
{
    DelegateFPTSO(tree_view, info);

    switch (info.section) {
    case Section::kFinance:
        DelegateF(tree_view, info, settings);
        break;
    case Section::kTask:
        DelegateT(tree_view, settings);
        break;
    case Section::kStakeholder:
        DelegateS(tree_view, settings);
        break;
    case Section::kProduct:
        DelegateP(tree_view, settings);
        break;
    case Section::kSales:
    case Section::kPurchase:
        DelegateO(tree_view, info, settings);
        break;
    default:
        break;
    }
}

void MainWindow::DelegateFPTSO(PTreeView tree_view, CInfo& info) const
{
    auto* line { new Line(tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnum::kCode), line);
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnum::kDescription), line);

    auto* plain_text { new TreePlainText(tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnum::kNote), plain_text);

    auto* rule { new TreeCombo(info.rule_map, info.rule_model, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnum::kRule), rule);

    auto* unit { new TreeCombo(info.unit_map, info.unit_model, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnum::kUnit), unit);

    auto* type { new TreeCombo(info.type_map, info.type_model, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnum::kType), type);
}

void MainWindow::DelegateF(PTreeView tree_view, CInfo& info, CSettings& settings) const
{
    auto* final_total { new DoubleSpinUnitR(settings.amount_decimal, settings.default_unit, info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumF::kLocalTotal), final_total);

    auto* initial_total { new FinanceForeignR(settings.amount_decimal, settings.default_unit, info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumF::kForeignTotal), initial_total);
}

void MainWindow::DelegateT(PTreeView tree_view, CSettings& settings) const
{
    auto* quantity { new DoubleSpinR(settings.common_decimal, kCoefficient16, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumT::kQuantity), quantity);

    auto* amount { new DoubleSpinUnitR(settings.amount_decimal, finance_settings_.default_unit, finance_data_.info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumT::kAmount), amount);

    auto* unit_cost { new DoubleSpin(settings.amount_decimal, 0, std::numeric_limits<double>::max(), kCoefficient8, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumT::kUnitCost), unit_cost);

    auto* color { new Color(tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumT::kColor), color);

    auto* date_time { new TreeDateTime(settings.date_format, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumT::kDateTime), date_time);

    auto* finished { new CheckBox(QEvent::MouseButtonDblClick, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumT::kFinished), finished);

    auto* document { new Document(tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumT::kDocument), document);
    connect(document, &Document::SEditDocument, this, &MainWindow::REditNodeDocument);
}

void MainWindow::DelegateP(PTreeView tree_view, CSettings& settings) const
{
    auto* quantity { new DoubleSpinR(settings.common_decimal, kCoefficient16, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kQuantity), quantity);

    auto* amount { new DoubleSpinUnitRPS(settings.amount_decimal, finance_settings_.default_unit, finance_data_.info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kAmount), amount);
    connect(amount, &DoubleSpinUnitRPS::STransRef, this, &MainWindow::RTransRef);

    auto* unit_price { new DoubleSpin(settings.amount_decimal, 0, std::numeric_limits<double>::max(), kCoefficient8, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kUnitPrice), unit_price);
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kCommission), unit_price);

    auto* color { new Color(tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kColor), color);
}

void MainWindow::DelegateS(PTreeView tree_view, CSettings& settings) const
{
    auto* amount { new DoubleSpinUnitRPS(settings.amount_decimal, finance_settings_.default_unit, finance_data_.info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumS::kAmount), amount);
    connect(amount, &DoubleSpinUnitRPS::STransRef, this, &MainWindow::RTransRef);

    auto* payment_term { new Spin(0, std::numeric_limits<int>::max(), tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumS::kPaymentTerm), payment_term);

    auto* tax_rate { new TaxRate(settings.amount_decimal, 0.0, std::numeric_limits<double>::max(), tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumS::kTaxRate), tax_rate);

    auto* deadline { new TreeDateTime(kDD, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumS::kDeadline), deadline);

    auto* employee { new SpecificUnit(stakeholder_tree_->Model(), stakeholder_tree_->Model()->UnitModelPS(std::to_underlying(UnitS::kEmp)), tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumS::kEmployee), employee);
}

void MainWindow::DelegateO(PTreeView tree_view, CInfo& info, CSettings& settings) const
{
    auto* rule { new TreeCombo(info.rule_map, info.rule_model, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kRule), rule);

    auto* amount { new DoubleSpinUnitR(settings.amount_decimal, finance_settings_.default_unit, finance_data_.info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kGrossAmount), amount);
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kNetAmount), amount);

    auto* discount { new DoubleSpinUnitRNoneZero(settings.amount_decimal, finance_settings_.default_unit, finance_data_.info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kDiscount), discount);

    auto* quantity { new DoubleSpinRNoneZero(settings.common_decimal, kCoefficient16, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kSecond), quantity);
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kFirst), quantity);

    auto stakeholder_tree_model { stakeholder_tree_->Model() };

    auto* employee { new SpecificUnit(stakeholder_tree_model, stakeholder_tree_model->UnitModelPS(std::to_underlying(UnitS::kEmp)), tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kEmployee), employee);

    auto* name { new OrderNameR(stakeholder_tree_model, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kName), name);

    auto* date_time { new TreeDateTime(settings.date_format, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kDateTime), date_time);

    auto* finished { new CheckBox(QEvent::MouseButtonDblClick, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kFinished), finished);
}

void MainWindow::TreeConnect(NodeWidget* tree_widget, const Sqlite* sql) const
{
    auto view { tree_widget->View() };
    auto model { tree_widget->Model() };

    connect(view, &QTreeView::doubleClicked, this, &MainWindow::RTreeViewDoubleClicked);
    connect(view, &QTreeView::customContextMenuRequested, this, &MainWindow::RTreeViewCustomContextMenuRequested);

    connect(model, &NodeModel::SUpdateName, this, &MainWindow::RUpdateName);

    connect(model, &NodeModel::SUpdateStatusValue, tree_widget, &NodeWidget::RUpdateStatusValue);

    connect(model, &NodeModel::SResizeColumnToContents, view, &QTreeView::resizeColumnToContents);

    connect(model, &NodeModel::SRule, &LeafSStation::Instance(), &LeafSStation::RRule);

    connect(sql, &Sqlite::SRemoveNode, model, &NodeModel::RRemoveNode);
    connect(sql, &Sqlite::SUpdateMultiLeafTotal, model, &NodeModel::RUpdateMultiLeafTotal);

    connect(sql, &Sqlite::SFreeWidget, this, &MainWindow::RFreeWidget);
}

void MainWindow::InsertNodeFunction(const QModelIndex& parent, int parent_id, int row)
{
    auto model { tree_widget_->Model() };

    auto* node { ResourcePool<Node>::Instance().Allocate() };
    node->rule = model->Rule(parent_id);
    node->unit = model->Unit(parent_id);
    model->SetParent(node, parent_id);

    if (start_ == Section::kSales || start_ == Section::kPurchase)
        InsertNodeO(node, parent, row);

    if (start_ != Section::kSales && start_ != Section::kPurchase)
        InsertNodeFPTS(node, parent, parent_id, row);
}

void MainWindow::on_actionRemove_triggered()
{
    auto* widget { ui->tabWidget->currentWidget() };
    if (!widget || dynamic_cast<SupportWidget*>(widget))
        return;

    if (auto* tree_widget { dynamic_cast<NodeWidget*>(widget) }) {
        RemoveNode(tree_widget);
    }

    if (auto* leaf_widget { dynamic_cast<TransWidget*>(widget) }) {
        RemoveTrans(leaf_widget);
    }
}

void MainWindow::RemoveNode(NodeWidget* tree_widget)
{
    auto view { tree_widget->View() };
    if (!view || !MainWindowUtils::HasSelection(view))
        return;

    const auto index { view->currentIndex() };
    if (!index.isValid())
        return;

    auto model { tree_widget->Model() };
    if (!model)
        return;

    const int node_id { index.siblingAtColumn(std::to_underlying(NodeEnum::kID)).data().toInt() };
    const int node_type { index.siblingAtColumn(std::to_underlying(NodeEnum::kType)).data().toInt() };

    if (node_type == kTypeBranch) {
        RemoveBranch(model, index, node_id);
        return;
    }

    auto* sql { data_->sql };
    bool interal_reference { sql->InternalReference(node_id) };
    bool exteral_reference { sql->ExternalReference(node_id) };
    bool support_reference { sql->SupportReference(node_id) };

    if (!interal_reference && !exteral_reference && !support_reference) {
        RemoveNonBranch(model, index, node_id, node_type);
        return;
    }

    const int unit { index.siblingAtColumn(std::to_underlying(NodeEnum::kUnit)).data().toInt() };

    auto* dialog { new class RemoveNode(model, start_, node_id, node_type, unit, exteral_reference, this) };
    connect(dialog, &RemoveNode::SRemoveNode, sql, &Sqlite::RRemoveNode);
    connect(dialog, &RemoveNode::SReplaceNode, sql, &Sqlite::RReplaceNode);
    dialog->exec();
}

void MainWindow::RemoveTrans(TransWidget* leaf_widget)
{
    auto view { leaf_widget->View() };
    if (!view || !MainWindowUtils::HasSelection(view))
        return;

    const QModelIndex current_index { view->currentIndex() };
    if (!current_index.isValid())
        return;

    auto model { leaf_widget->Model() };
    if (!model)
        return;

    const int current_row { current_index.row() };
    if (!model->removeRows(current_row, 1)) {
        qDebug() << "Failed to remove row:" << current_row;
        return;
    }

    const int new_row_count { model->rowCount() };
    if (new_row_count == 0)
        return;

    QModelIndex new_index {};
    if (current_row <= new_row_count - 1) {
        new_index = model->index(current_row, 0);
    } else {
        new_index = model->index(new_row_count - 1, 0);
    }

    if (new_index.isValid())
        view->setCurrentIndex(new_index);
}

void MainWindow::RemoveNonBranch(PTreeModel tree_model, const QModelIndex& index, int node_id, int node_type)
{
    tree_model->RemoveNode(index.row(), index.parent());
    data_->sql->RemoveNode(node_id, node_type);

    auto widget { trans_wgt_hash_->value(node_id) };
    if (widget) {
        MainWindowUtils::FreeWidget(widget);
        trans_wgt_hash_->remove(node_id);
        LeafSStation::Instance().DeregisterModel(start_, node_id);
    }
}

void MainWindow::RestoreTab(PTreeModel tree_model, TransWgtHash& trans_wgt_hash, CIntSet& set, CData& data, CSettings& settings)
{
    if (!tree_model || set.isEmpty())
        return;

    for (int node_id : set) {
        switch (tree_model->Type(node_id)) {
        case kTypeLeaf:
            CreateLeafFPTS(tree_model, &trans_wgt_hash, &data, &settings, node_id);
            break;
        default:
            break;
        }
    }
}

void MainWindow::EnableAction(bool enable) const
{
    ui->actionAppendNode->setEnabled(enable);
    ui->actionCheckAll->setEnabled(enable);
    ui->actionCheckNone->setEnabled(enable);
    ui->actionCheckReverse->setEnabled(enable);
    ui->actionEditNode->setEnabled(enable);
    ui->actionInsertNode->setEnabled(enable);
    ui->actionJump->setEnabled(enable);
    ui->actionPreferences->setEnabled(enable);
    ui->actionSearch->setEnabled(enable);
    ui->actionSupportJump->setEnabled(enable);
    ui->actionRemove->setEnabled(enable);
    ui->actionAppendTrans->setEnabled(enable);
    ui->actionExportExcel->setEnabled(enable);
    ui->actionExportYTX->setEnabled(enable);
    ui->actionStatement->setEnabled(enable);
}

QStandardItemModel* MainWindow::CreateModelFromList(QStringList& list, QObject* parent)
{
    auto* model { new QStandardItemModel(parent) };
    int index {};

    for (auto&& value : list) {
        auto* item { new QStandardItem(std::move(value)) };
        item->setData(index++, Qt::UserRole);
        model->appendRow(item);
    }

    return model;
}

void MainWindow::IniSectionGroup()
{
    section_group_ = new QButtonGroup(this);
    section_group_->addButton(ui->rBtnFinance, 0);
    section_group_->addButton(ui->rBtnProduct, 1);
    section_group_->addButton(ui->rBtnTask, 2);
    section_group_->addButton(ui->rBtnStakeholder, 3);
    section_group_->addButton(ui->rBtnSales, 4);
    section_group_->addButton(ui->rBtnPurchase, 5);
}

void MainWindow::RestoreRecentFile()
{
    recent_file_ = app_settings_->value(kRecentFile).toStringList();

    auto* recent_menu { ui->menuRecent };
    QStringList valid_recent_file {};

    const long long count { std::min(kMaxRecentFile, recent_file_.size()) };
    const auto mid_file { recent_file_.mid(recent_file_.size() - count, count) };

    for (auto it = mid_file.rbegin(); it != mid_file.rend(); ++it) {
        CString& file_path { *it };

        if (QFile::exists(file_path)) {
            auto* action { recent_menu->addAction(file_path) };
            connect(action, &QAction::triggered, this, [file_path, this]() { ROpenFile(file_path); });
            valid_recent_file.prepend(file_path);
        }
    }

    if (recent_file_ != valid_recent_file) {
        recent_file_ = valid_recent_file;
        MainWindowUtils::WriteSettings(app_settings_, recent_file_, kRecent, kFile);
    }

    SetClearMenuAction();
}

void MainWindow::AddRecentFile(CString& file_path)
{
    CString path { QDir::toNativeSeparators(file_path) };

    if (!recent_file_.contains(path)) {
        auto* menu { ui->menuRecent };
        auto* action { new QAction(path, menu) };

        if (menu->isEmpty()) {
            menu->addAction(action);
            SetClearMenuAction();
        } else
            ui->menuRecent->insertAction(ui->actionSeparator, action);

        recent_file_.emplaceBack(path);
        MainWindowUtils::WriteSettings(app_settings_, recent_file_, kRecent, kFile);
    }
}

bool MainWindow::LockFile(const QFileInfo& file_info)
{
    CString lock_file_path { file_info.dir().filePath(file_info.completeBaseName() + kDotSuffixLOCK) };

    lock_file_ = std::make_unique<QLockFile>(lock_file_path);

    if (!lock_file_->tryLock(100)) {
        MainWindowUtils::Message(QMessageBox::Critical, tr("Lock Failed"),
            tr("Unable to lock the file \"%1\". Please ensure no other instance of the application or process is accessing it and try again.")
                .arg(file_info.absoluteFilePath()),
            kThreeThousand);

        lock_file_.reset();
        return false;
    }

    return true;
}

void MainWindow::RemoveBranch(PTreeModel tree_model, const QModelIndex& index, int node_id)
{
    QMessageBox msg {};
    msg.setIcon(QMessageBox::Question);
    msg.setText(tr("Remove %1").arg(tree_model->GetPath(node_id)));
    msg.setInformativeText(tr("The branch will be removed, and its direct children will be promoted to the same level."));
    msg.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);

    if (msg.exec() == QMessageBox::Ok) {
        tree_model->RemoveNode(index.row(), index.parent());
        data_->sql->RemoveNode(node_id, kTypeBranch);
    }
}

void MainWindow::on_tabWidget_tabCloseRequested(int index)
{
    if (index == 0)
        return;

    const int node_id { ui->tabWidget->tabBar()->tabData(index).value<Tab>().node_id };

    // RefFetcher's widget is SupportWidget, its ID is Leaf Node's
    // Only Product and Stakeholder have RefFetcher
    auto widget = QPointer { dynamic_cast<SupportWidget*>(ui->tabWidget->currentWidget()) };

    if ((start_ == Section::kProduct || start_ == Section::kStakeholder) && widget) {
        sup_wgt_hash_->remove(node_id);
        MainWindowUtils::FreeWidget(widget);
        return;
    }

    RFreeWidget(node_id);
}

void MainWindow::RFreeWidget(int node_id)
{
    QPointer<QWidget> widget {};

    switch (tree_widget_->Model()->Type(node_id)) {
    case kTypeLeaf:
        widget = trans_wgt_hash_->value(node_id);
        trans_wgt_hash_->remove(node_id);
        LeafSStation::Instance().DeregisterModel(start_, node_id);
        break;
    case kTypeSupport:
        widget = sup_wgt_hash_->value(node_id);
        sup_wgt_hash_->remove(node_id);
        SupportSStation::Instance().DeregisterModel(start_, node_id);
        break;
    default:
        break;
    }

    MainWindowUtils::FreeWidget(widget);
}

void MainWindow::SetTabWidget()
{
    auto* tab_widget { ui->tabWidget };
    auto* tab_bar { tab_widget->tabBar() };

    tab_bar->setDocumentMode(true);
    tab_bar->setExpanding(false);
    tab_bar->setTabButton(0, QTabBar::LeftSide, nullptr);

    tab_widget->setMovable(true);
    tab_widget->setTabsClosable(true);
    tab_widget->setElideMode(Qt::ElideNone);

    start_ = Section(app_settings_->value(kStartSection, 0).toInt());

    switch (start_) {
    case Section::kFinance:
        ui->rBtnFinance->setChecked(true);
        break;
    case Section::kStakeholder:
        ui->rBtnStakeholder->setChecked(true);
        break;
    case Section::kProduct:
        ui->rBtnProduct->setChecked(true);
        break;
    case Section::kTask:
        ui->rBtnTask->setChecked(true);
        break;
    case Section::kSales:
        ui->rBtnSales->setChecked(true);
        break;
    case Section::kPurchase:
        ui->rBtnPurchase->setChecked(true);
        break;
    default:
        break;
    }
}

void MainWindow::SetTableView(PTableView view, int stretch_column) const
{
    view->setSortingEnabled(true);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setAlternatingRowColors(true);
    view->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::CurrentChanged);
    view->setColumnHidden(std::to_underlying(TransEnum::kID), false);

    auto* h_header { view->horizontalHeader() };
    ResizeColumn(h_header, stretch_column);

    auto* v_header { view->verticalHeader() };
    v_header->setDefaultSectionSize(kRowHeight);
    v_header->setSectionResizeMode(QHeaderView::Fixed);
    v_header->setHidden(true);

    view->scrollToBottom();
    view->setCurrentIndex(QModelIndex());
    view->sortByColumn(std::to_underlying(TransEnum::kDateTime), Qt::AscendingOrder); // will run function: AccumulateSubtotal while sorting
}

void MainWindow::DelegateSupport(PTableView table_view, PTreeModel tree_model, CSettings* settings) const
{
    auto* date_time { new TableDateTime(settings->date_format, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kDateTime), date_time);

    auto* state { new CheckBox(QEvent::MouseButtonRelease, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kState), state);

    auto* value { new DoubleSpinRNoneZero(settings->amount_decimal, kCoefficient8, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kLhsDebit), value);
    table_view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kRhsDebit), value);
    table_view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kLhsCredit), value);
    table_view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kRhsCredit), value);

    auto* ratio { new DoubleSpinRNoneZero(settings->common_decimal, kCoefficient8, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kLhsRatio), ratio);
    table_view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kRhsRatio), ratio);

    auto* node_name { new SearchPathTableR(tree_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kLhsNode), node_name);
    table_view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kRhsNode), node_name);
}

void MainWindow::CreateTransRef(PTreeModel tree_model, SupWgtHash* sup_wgt_hash, CData* data, int node_id)
{
    if (!tree_model || !tree_model->Contains(node_id))
        return;

    CString name { tree_model->Name(node_id) };
    auto* sql { data->sql };
    const Info& info { data->info };
    const Section section { info.section };

    auto* model { new TransRefModel(node_id, info, sql, this) };
    auto* widget { new SupportWidgetFPTS(model, this) };

    const int tab_index { ui->tabWidget->addTab(widget, name) };
    auto* tab_bar { ui->tabWidget->tabBar() };

    tab_bar->setTabData(tab_index, QVariant::fromValue(Tab { section, node_id }));
    tab_bar->setTabToolTip(tab_index, tree_model->GetPath(node_id));

    auto view { widget->View() };
    SetTableView(view, std::to_underlying(TransRefEnum::kDescription));
    DelegateTransRef(view, &sales_settings_);
    connect(view, &QTableView::doubleClicked, this, &MainWindow::RTransRefDoubleClicked);

    sup_wgt_hash->insert(node_id, widget);
}

void MainWindow::DelegateTransRef(PTableView table_view, CSettings* settings) const
{
    auto* price { new DoubleSpinR(settings->amount_decimal, kCoefficient8, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransRefEnum::kUnitPrice), price);
    table_view->setItemDelegateForColumn(std::to_underlying(TransRefEnum::kDiscountPrice), price);

    auto* quantity { new DoubleSpinR(settings->common_decimal, kCoefficient8, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransRefEnum::kFirst), quantity);
    table_view->setItemDelegateForColumn(std::to_underlying(TransRefEnum::kSecond), quantity);

    auto* amount { new DoubleSpinRNoneZero(settings->amount_decimal, kCoefficient16, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransRefEnum::kGrossAmount), amount);
    table_view->setItemDelegateForColumn(std::to_underlying(TransRefEnum::kDiscount), amount);
    table_view->setItemDelegateForColumn(std::to_underlying(TransRefEnum::kNetAmount), amount);

    auto* date_time { new DateTimeR(sales_settings_.date_format, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransRefEnum::kDateTime), date_time);

    auto stakeholder_tree_model { stakeholder_tree_->Model() };
    auto* outside_product { new NodePathR(stakeholder_tree_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransRefEnum::kOutsideProduct), outside_product);

    if (start_ == Section::kProduct) {
        auto* name { new NodeNameR(stakeholder_tree_model, table_view) };
        table_view->setItemDelegateForColumn(std::to_underlying(TransRefEnum::kPP), name);
    }

    if (start_ == Section::kStakeholder) {
        auto* inside_product { new NodeNameR(product_tree_->Model(), table_view) };
        table_view->setItemDelegateForColumn(std::to_underlying(TransRefEnum::kPP), inside_product);
    }
}

void MainWindow::DelegateSupportS(PTableView table_view, PTreeModel tree_model, PTreeModel product_tree_model) const
{
    auto* lhs_node_name { new SearchPathTableR(tree_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kLhsNode), lhs_node_name);

    auto* rhs_node_name { new SearchPathTableR(product_tree_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kRhsNode), rhs_node_name);
}

void MainWindow::SetSupportViewS(PTableView table_view) const
{
    table_view->setColumnHidden(std::to_underlying(TransSearchEnum::kLhsDebit), true);
    table_view->setColumnHidden(std::to_underlying(TransSearchEnum::kRhsDebit), true);
    table_view->setColumnHidden(std::to_underlying(TransSearchEnum::kLhsCredit), true);
    table_view->setColumnHidden(std::to_underlying(TransSearchEnum::kRhsCredit), true);
}

void MainWindow::SetStatementView(PTableView view) const
{
    view->setSortingEnabled(true);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setAlternatingRowColors(true);

    auto* h_header { view->horizontalHeader() };
    ResizeColumn(h_header, std::to_underlying(StatementEnum::kPlaceholder));

    auto* v_header { view->verticalHeader() };
    v_header->setDefaultSectionSize(kRowHeight);
    v_header->setSectionResizeMode(QHeaderView::Fixed);
    v_header->setHidden(true);
}

void MainWindow::DelegateStatement(PTableView table_view, CSettings* settings) const
{
    auto* quantity { new DoubleSpinR(settings->common_decimal, kCoefficient8, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(StatementEnum::kCFirst), quantity);
    table_view->setItemDelegateForColumn(std::to_underlying(StatementEnum::kCSecond), quantity);

    auto* amount { new DoubleSpinRNoneZero(settings->amount_decimal, kCoefficient16, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(StatementEnum::kCGrossAmount), amount);
    table_view->setItemDelegateForColumn(std::to_underlying(StatementEnum::kCSettlement), amount);
    table_view->setItemDelegateForColumn(std::to_underlying(StatementEnum::kPBalance), amount);
    table_view->setItemDelegateForColumn(std::to_underlying(StatementEnum::kCBalance), amount);

    auto* inside_product { new NodeNameR(product_tree_->Model(), table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(StatementEnum::kParty), inside_product);
}

void MainWindow::SetConnect() const
{
    connect(ui->actionCheckAll, &QAction::triggered, this, &MainWindow::RUpdateState);
    connect(ui->actionCheckNone, &QAction::triggered, this, &MainWindow::RUpdateState);
    connect(ui->actionCheckReverse, &QAction::triggered, this, &MainWindow::RUpdateState);
    connect(section_group_, &QButtonGroup::idClicked, this, &MainWindow::RSectionGroup);
}

void MainWindow::SetFinanceData()
{
    const auto section { Section::kFinance };
    auto& info { finance_data_.info };
    auto& sql { finance_data_.sql };

    info.section = section;
    info.node = kFinance;
    info.path = kFinancePath;
    info.trans = kFinanceTrans;

    QStringList unit_list { "CNY", "HKD", "USD", "GBP", "JPY", "CAD", "AUD", "EUR" };
    QStringList unit_symbol_list { "¥", "$", "$", "£", "¥", "$", "$", "€" };
    QStringList rule_list { "DICD", "DDCI" };
    QStringList type_list { "L", "B", "S" };

    for (int i = 0; i != unit_list.size(); ++i) {
        info.unit_map.insert(i, unit_list.at(i));
        info.unit_symbol_map.insert(i, unit_symbol_list.at(i));
    }

    for (int i = 0; i != rule_list.size(); ++i)
        info.rule_map.insert(i, rule_list.at(i));

    for (int i = 0; i != type_list.size(); ++i)
        info.type_map.insert(i, type_list.at(i));

    info.unit_model = CreateModelFromList(unit_list, this);
    info.rule_model = CreateModelFromList(rule_list, this);
    info.type_model = CreateModelFromList(type_list, this);

    ytx_sql_->QuerySettings(finance_settings_, section);

    sql = new SqliteFinance(info, this);

    auto* model { new NodeModelF(sql, info, finance_settings_.default_unit, finance_trans_wgt_hash_, interface_.separator, this) };
    finance_tree_ = new NodeWidgetF(model, info, finance_settings_, this);

    connect(sql, &Sqlite::SMoveMultiSupportTrans, &SupportSStation::Instance(), &SupportSStation::RMoveMultiSupportTransFPTS);
}

void MainWindow::SetProductData()
{
    const auto section { Section::kProduct };
    auto& info { product_data_.info };
    auto& sql { product_data_.sql };

    info.section = section;
    info.node = kProduct;
    info.path = kProductPath;
    info.trans = kProductTrans;

    // POS: Position, PC: Piece, SF: SquareFeet
    QStringList unit_list { {}, "POS", "SF", "PC", "BOX" };
    QStringList rule_list { "DICD", "DDCI" };
    QStringList type_list { "L", "B", "S" };

    for (int i = 0; i != unit_list.size(); ++i)
        info.unit_map.insert(i, unit_list.at(i));

    for (int i = 0; i != rule_list.size(); ++i)
        info.rule_map.insert(i, rule_list.at(i));

    for (int i = 0; i != type_list.size(); ++i)
        info.type_map.insert(i, type_list.at(i));

    info.unit_model = CreateModelFromList(unit_list, this);
    info.rule_model = CreateModelFromList(rule_list, this);
    info.type_model = CreateModelFromList(type_list, this);

    ytx_sql_->QuerySettings(product_settings_, section);

    sql = new SqliteProduct(info, this);

    auto* model { new NodeModelP(sql, info, product_settings_.default_unit, product_trans_wgt_hash_, interface_.separator, this) };
    product_tree_ = new NodeWidgetPT(model, product_settings_, this);

    connect(sql, &Sqlite::SMoveMultiSupportTrans, &SupportSStation::Instance(), &SupportSStation::RMoveMultiSupportTransFPTS);
}

void MainWindow::SetStakeholderData()
{
    const auto section { Section::kStakeholder };
    auto& info { stakeholder_data_.info };
    auto& sql { stakeholder_data_.sql };

    info.section = section;
    info.node = kStakeholder;
    info.path = kStakeholderPath;
    info.trans = kStakeholderTrans;

    // EMP: EMPLOYEE, CUST: CUSTOMER, VEND: VENDOR, PROD: PRODUCT
    QStringList unit_list { tr("CUST"), tr("EMP"), tr("VEND") };
    // IM：Immediate, MS（Monthly Settlement）
    QStringList rule_list { tr("IS"), tr("MS") };
    QStringList type_list { "L", "B", "S" };

    for (int i = 0; i != unit_list.size(); ++i)
        info.unit_map.insert(i, unit_list.at(i));

    for (int i = 0; i != rule_list.size(); ++i)
        info.rule_map.insert(i, rule_list.at(i));

    for (int i = 0; i != type_list.size(); ++i)
        info.type_map.insert(i, type_list.at(i));

    info.unit_model = CreateModelFromList(unit_list, this);
    info.rule_model = CreateModelFromList(rule_list, this);
    info.type_model = CreateModelFromList(type_list, this);

    ytx_sql_->QuerySettings(stakeholder_settings_, section);

    sql = new SqliteStakeholder(info, this);

    auto* model { new NodeModelS(sql, info, stakeholder_settings_.default_unit, stakeholder_trans_wgt_hash_, interface_.separator, this) };
    stakeholder_tree_ = new NodeWidgetS(model, this);

    connect(product_data_.sql, &Sqlite::SUpdateProduct, sql, &Sqlite::RUpdateProduct);
    connect(sql, &Sqlite::SUpdateStakeholder, model, &NodeModel::RUpdateStakeholder);

    connect(static_cast<SqliteStakeholder*>(sql), &SqliteStakeholder::SAppendPrice, &LeafSStation::Instance(), &LeafSStation::RAppendPrice);
    connect(sql, &Sqlite::SMoveMultiSupportTrans, &SupportSStation::Instance(), &SupportSStation::RMoveMultiSupportTransFPTS);
}

void MainWindow::SetTaskData()
{
    const auto section { Section::kTask };
    auto& info { task_data_.info };
    auto& sql { task_data_.sql };

    info.section = section;
    info.node = kTask;
    info.path = kTaskPath;
    info.trans = kTaskTrans;

    // PROD: PRODUCT, STKH: STAKEHOLDER
    QStringList unit_list { {}, tr("PROD"), tr("CUST"), tr("EMP"), tr("VEND") };
    QStringList rule_list { "DICD", "DDCI" };
    QStringList type_list { "L", "B", "S" };

    for (int i = 0; i != unit_list.size(); ++i)
        info.unit_map.insert(i, unit_list.at(i));

    for (int i = 0; i != rule_list.size(); ++i)
        info.rule_map.insert(i, rule_list.at(i));

    for (int i = 0; i != type_list.size(); ++i)
        info.type_map.insert(i, type_list.at(i));

    info.unit_model = CreateModelFromList(unit_list, this);
    info.rule_model = CreateModelFromList(rule_list, this);
    info.type_model = CreateModelFromList(type_list, this);

    ytx_sql_->QuerySettings(task_settings_, section);

    sql = new SqliteTask(info, this);

    auto* model { new NodeModelT(sql, info, task_settings_.default_unit, task_trans_wgt_hash_, interface_.separator, this) };
    task_tree_ = new NodeWidgetPT(model, task_settings_, this);

    connect(sql, &Sqlite::SMoveMultiSupportTrans, &SupportSStation::Instance(), &SupportSStation::RMoveMultiSupportTransFPTS);
}

void MainWindow::SetSalesData()
{
    const auto section { Section::kSales };
    auto& info { sales_data_.info };
    auto& sql { sales_data_.sql };

    info.section = section;
    info.node = kSales;
    info.path = kSalesPath;
    info.trans = kSalesTrans;

    // IM: IMMEDIATE, MS: MONTHLY SETTLEMENT, PEND: PENDING
    QStringList unit_list { tr("IS"), tr("MS"), tr("PEND") };
    // SO: SALES ORDER, RO: REFUND ORDER
    QStringList rule_list { QObject::tr("SO"), QObject::tr("RO") };
    QStringList type_list { "L", "B" };

    for (int i = 0; i != unit_list.size(); ++i)
        info.unit_map.insert(i, unit_list.at(i));

    for (int i = 0; i != rule_list.size(); ++i)
        info.rule_map.insert(i, rule_list.at(i));

    for (int i = 0; i != type_list.size(); ++i)
        info.type_map.insert(i, type_list.at(i));

    info.unit_model = CreateModelFromList(unit_list, this);
    info.rule_model = CreateModelFromList(rule_list, this);
    info.type_model = CreateModelFromList(type_list, this);

    ytx_sql_->QuerySettings(sales_settings_, section);

    sql = new SqliteOrder(info, this);

    auto* model { new NodeModelO(sql, info, sales_settings_.default_unit, sales_trans_wgt_hash_, interface_.separator, this) };
    sales_tree_ = new NodeWidgetO(model, this);

    connect(stakeholder_data_.sql, &Sqlite::SUpdateStakeholder, model, &NodeModel::RUpdateStakeholder);
    connect(product_data_.sql, &Sqlite::SUpdateProduct, sql, &Sqlite::RUpdateProduct);
    connect(model, &NodeModel::SSyncDouble, stakeholder_tree_->Model(), &NodeModel::RSyncDouble);
}

void MainWindow::SetPurchaseData()
{
    const auto section { Section::kPurchase };
    auto& info { purchase_data_.info };
    auto& sql { purchase_data_.sql };

    info.section = section;
    info.node = kPurchase;
    info.path = kPurchasePath;
    info.trans = kPurchaseTrans;

    // IM: IMMEDIATE, MS: MONTHLY SETTLEMENT, PEND: PENDING
    QStringList unit_list { tr("IS"), tr("MS"), tr("PEND") };
    // SO: SALES ORDER, RO: REFUND ORDER
    QStringList rule_list { QObject::tr("PO"), QObject::tr("RO") };
    QStringList type_list { "L", "B" };

    for (int i = 0; i != unit_list.size(); ++i)
        info.unit_map.insert(i, unit_list.at(i));

    for (int i = 0; i != rule_list.size(); ++i)
        info.rule_map.insert(i, rule_list.at(i));

    for (int i = 0; i != type_list.size(); ++i)
        info.type_map.insert(i, type_list.at(i));

    info.unit_model = CreateModelFromList(unit_list, this);
    info.rule_model = CreateModelFromList(rule_list, this);
    info.type_model = CreateModelFromList(type_list, this);

    ytx_sql_->QuerySettings(purchase_settings_, section);

    sql = new SqliteOrder(info, this);

    auto* model { new NodeModelO(sql, info, purchase_settings_.default_unit, purchase_trans_wgt_hash_, interface_.separator, this) };
    purchase_tree_ = new NodeWidgetO(model, this);

    connect(stakeholder_data_.sql, &Sqlite::SUpdateStakeholder, model, &NodeModel::RUpdateStakeholder);
    connect(product_data_.sql, &Sqlite::SUpdateProduct, sql, &Sqlite::RUpdateProduct);
    connect(model, &NodeModel::SSyncDouble, stakeholder_tree_->Model(), &NodeModel::RSyncDouble);
}

void MainWindow::SetAction() const
{
    ui->actionInsertNode->setIcon(QIcon(":/solarized_dark/solarized_dark/insert.png"));
    ui->actionEditNode->setIcon(QIcon(":/solarized_dark/solarized_dark/edit.png"));
    ui->actionRemove->setIcon(QIcon(":/solarized_dark/solarized_dark/remove2.png"));
    ui->actionAbout->setIcon(QIcon(":/solarized_dark/solarized_dark/about.png"));
    ui->actionAppendNode->setIcon(QIcon(":/solarized_dark/solarized_dark/append.png"));
    ui->actionJump->setIcon(QIcon(":/solarized_dark/solarized_dark/jump.png"));
    ui->actionSupportJump->setIcon(QIcon(":/solarized_dark/solarized_dark/jump.png"));
    ui->actionPreferences->setIcon(QIcon(":/solarized_dark/solarized_dark/settings.png"));
    ui->actionSearch->setIcon(QIcon(":/solarized_dark/solarized_dark/search.png"));
    ui->actionNewFile->setIcon(QIcon(":/solarized_dark/solarized_dark/new.png"));
    ui->actionOpenFile->setIcon(QIcon(":/solarized_dark/solarized_dark/open.png"));
    ui->actionCheckAll->setIcon(QIcon(":/solarized_dark/solarized_dark/check-all.png"));
    ui->actionCheckNone->setIcon(QIcon(":/solarized_dark/solarized_dark/check-none.png"));
    ui->actionCheckReverse->setIcon(QIcon(":/solarized_dark/solarized_dark/check-reverse.png"));
    ui->actionAppendTrans->setIcon(QIcon(":/solarized_dark/solarized_dark/append_trans.png"));

    ui->actionCheckAll->setProperty(kCheck, std::to_underlying(Check::kAll));
    ui->actionCheckNone->setProperty(kCheck, std::to_underlying(Check::kNone));
    ui->actionCheckReverse->setProperty(kCheck, std::to_underlying(Check::kReverse));
}

void MainWindow::SetTreeView(PTreeView tree_view, CInfo& info) const
{
    tree_view->setColumnHidden(std::to_underlying(NodeEnum::kID), false);
    if (info.section == Section::kSales || info.section == Section::kPurchase)
        tree_view->setColumnHidden(std::to_underlying(NodeEnumO::kParty), false);

    tree_view->setSelectionMode(QAbstractItemView::SingleSelection);
    tree_view->setDragDropMode(QAbstractItemView::InternalMove);
    tree_view->setEditTriggers(QAbstractItemView::DoubleClicked);
    tree_view->setDropIndicatorShown(true);
    tree_view->setSortingEnabled(true);
    tree_view->setContextMenuPolicy(Qt::CustomContextMenu);
    tree_view->setExpandsOnDoubleClick(true);

    auto* header { tree_view->header() };
    ResizeColumn(header, std::to_underlying(NodeEnum::kDescription));
    header->setStretchLastSection(false);
    header->setDefaultAlignment(Qt::AlignCenter);
}

void MainWindow::on_actionAppendNode_triggered()
{
    if (!tree_widget_)
        return;

    auto view { tree_widget_->View() };
    if (!MainWindowUtils::HasSelection(view))
        return;

    const auto parent_index { view->currentIndex() };
    if (!parent_index.isValid())
        return;

    const int type { parent_index.siblingAtColumn(std::to_underlying(NodeEnum::kType)).data().toInt() };
    if (type != kTypeBranch)
        return;

    const int parent_id { parent_index.siblingAtColumn(std::to_underlying(NodeEnum::kID)).data().toInt() };
    InsertNodeFunction(parent_index, parent_id, 0);
}

void MainWindow::on_actionJump_triggered()
{
    if (start_ == Section::kSales || start_ == Section::kPurchase)
        return;

    auto* leaf_widget { dynamic_cast<TransWidget*>(ui->tabWidget->currentWidget()) };
    if (!leaf_widget)
        return;

    auto view { leaf_widget->View() };
    if (!MainWindowUtils::HasSelection(view))
        return;

    const auto index { view->currentIndex() };
    if (!index.isValid())
        return;

    int row { index.row() };
    const int rhs_node_id { index.sibling(row, std::to_underlying(TransEnum::kRhsNode)).data().toInt() };
    if (rhs_node_id == 0)
        return;

    if (!trans_wgt_hash_->contains(rhs_node_id))
        CreateLeafFPTS(tree_widget_->Model(), trans_wgt_hash_, data_, settings_, rhs_node_id);

    const int trans_id { index.sibling(row, std::to_underlying(TransEnum::kID)).data().toInt() };
    SwitchToLeaf(rhs_node_id, trans_id);
}

void MainWindow::on_actionSupportJump_triggered()
{
    if (start_ == Section::kSales || start_ == Section::kPurchase)
        return;

    auto* widget { ui->tabWidget->currentWidget() };

    if (auto* support_widget { dynamic_cast<SupportWidget*>(widget) }) {
        SupportToLeaf(support_widget);
    }

    if (auto* leaf_widget { dynamic_cast<TransWidget*>(widget) }) {
        LeafToSupport(leaf_widget);
    }
}

void MainWindow::SwitchToSupport(int node_id, int trans_id) const
{
    auto widget { sup_wgt_hash_->value(node_id, nullptr) };
    if (!widget)
        return;

    auto* model { widget->Model().data() };
    if (!model)
        return;

    ui->tabWidget->setCurrentWidget(widget);
    widget->activateWindow();

    if (trans_id == 0)
        return;

    auto view { widget->View() };
    auto index { dynamic_cast<SupportModel*>(model)->GetIndex(trans_id) };

    if (!index.isValid())
        return;

    view->setCurrentIndex(index);
    view->scrollTo(index.siblingAtColumn(std::to_underlying(TransEnumS::kDateTime)), QAbstractItemView::PositionAtCenter);
    view->closePersistentEditor(index);
}

void MainWindow::LeafToSupport(TransWidget* widget)
{
    if (!widget)
        return;

    auto view { widget->View() };
    if (!MainWindowUtils::HasSelection(view))
        return;

    const auto index { view->currentIndex() };
    if (!index.isValid())
        return;

    auto model { widget->Model() };
    if (!model)
        return;

    const int id { index.siblingAtColumn(std::to_underlying(TransEnum::kSupportID)).data().toInt() };

    if (id <= 0)
        return;

    if (!sup_wgt_hash_->contains(id)) {
        CreateSupport(tree_widget_->Model(), sup_wgt_hash_, data_, settings_, id);
    }

    const int trans_id { index.siblingAtColumn(std::to_underlying(TransEnum::kID)).data().toInt() };
    SwitchToSupport(id, trans_id);
}

void MainWindow::SupportToLeaf(SupportWidget* widget)
{
    if (!widget)
        return;

    auto view { widget->View() };
    if (!MainWindowUtils::HasSelection(view))
        return;

    const auto index { view->currentIndex() };
    if (!index.isValid())
        return;

    auto model { widget->Model() };
    if (!model)
        return;

    const int rhs_node { index.siblingAtColumn(std::to_underlying(TransSearchEnum::kRhsNode)).data().toInt() };
    const int lhs_node { index.siblingAtColumn(std::to_underlying(TransSearchEnum::kLhsNode)).data().toInt() };

    if (rhs_node == 0 || lhs_node == 0)
        return;

    const int trans_id { index.siblingAtColumn(std::to_underlying(TransSearchEnum::kID)).data().toInt() };

    RTransLocation(trans_id, lhs_node, rhs_node);
}

void MainWindow::RTreeViewCustomContextMenuRequested(const QPoint& pos)
{
    Q_UNUSED(pos);

    auto* menu = new QMenu(this);
    menu->addAction(ui->actionInsertNode);
    menu->addAction(ui->actionEditNode);
    menu->addAction(ui->actionAppendNode);
    menu->addAction(ui->actionRemove);

    menu->exec(QCursor::pos());
}

void MainWindow::on_actionEditNode_triggered()
{
    if (start_ == Section::kSales || start_ == Section::kPurchase)
        return;

    if (!tree_widget_)
        return;

    const auto view { tree_widget_->View() };
    if (!MainWindowUtils::HasSelection(view))
        return;

    const auto index { view->currentIndex() };
    if (!index.isValid())
        return;

    const int node_id { index.siblingAtColumn(std::to_underlying(NodeEnum::kID)).data().toInt() };
    EditNodeFPTS(index, node_id);
}

void MainWindow::EditNodeFPTS(const QModelIndex& index, int node_id)
{
    auto model { tree_widget_->Model() };

    const auto& parent { index.parent() };
    const int parent_id { parent.isValid() ? parent.siblingAtColumn(std::to_underlying(NodeEnum::kID)).data().toInt() : -1 };
    auto parent_path { model->GetPath(parent_id) };

    if (!parent_path.isEmpty())
        parent_path += interface_.separator;

    CString name { model->Name(node_id) };
    const auto children_name { model->ChildrenNameFPTS(parent_id) };

    auto* edit_name { new EditNodeName(name, parent_path, children_name, this) };
    connect(edit_name, &QDialog::accepted, this, [=]() { model->UpdateName(node_id, edit_name->GetName()); });
    edit_name->exec();
}

void MainWindow::InsertNodeFPTS(Node* node, const QModelIndex& parent, int parent_id, int row)
{
    auto tree_model { tree_widget_->Model() };
    auto* unit_model { data_->info.unit_model };

    auto parent_path { tree_model->GetPath(parent_id) };
    if (!parent_path.isEmpty())
        parent_path += interface_.separator;

    const auto& name_list { tree_model->ChildrenNameFPTS(parent_id) };

    QDialog* dialog {};
    const auto params { InsertNodeParamsFPTS { node, unit_model, parent_path, name_list } };

    switch (start_) {
    case Section::kFinance:
        dialog = new InsertNodeFinance(std::move(params), this);
        break;
    case Section::kTask:
        node->date_time = QDateTime::currentDateTime().toString(kDateTimeFST);
        dialog = new InsertNodeTask(std::move(params), settings_->amount_decimal, settings_->date_format, this);
        break;
    case Section::kStakeholder:
        node->date_time = QDateTime::currentDateTime().toString(kDateTimeFST);
        dialog = new InsertNodeStakeholder(std::move(params), tree_model->UnitModelPS(std::to_underlying(UnitS::kEmp)), settings_->amount_decimal, this);
        break;
    case Section::kProduct:
        dialog = new InsertNodeProduct(std::move(params), settings_->common_decimal, this);
        break;
    default:
        return ResourcePool<Node>::Instance().Recycle(node);
    }

    connect(dialog, &QDialog::accepted, this, [=, this]() {
        if (tree_model->InsertNode(row, parent, node)) {
            auto index = tree_model->index(row, 0, parent);
            tree_widget_->View()->setCurrentIndex(index);
        }
    });

    connect(dialog, &QDialog::rejected, this, [=]() { ResourcePool<Node>::Instance().Recycle(node); });
    dialog->exec();
}

void MainWindow::InsertNodeO(Node* node, const QModelIndex& parent, int row)
{
    auto tree_model { tree_widget_->Model() };

    auto* sql { data_->sql };

    auto* table_model { new TransModelO(sql, node->rule, 0, data_->info, node, product_tree_->Model(), stakeholder_data_.sql, this) };

    auto params { EditNodeParamsO { node, sql, table_model, stakeholder_tree_->Model(), settings_, start_ } };
    auto* dialog { new InsertNodeOrder(std::move(params), this) };

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowFlags(Qt::Window);

    connect(dialog, &QDialog::accepted, this, [=, this]() {
        if (tree_model->InsertNode(row, parent, node)) {
            auto index = tree_model->index(row, 0, parent);
            tree_widget_->View()->setCurrentIndex(index);
            dialog_hash_->insert(node->id, dialog);
            dialog_list_->removeOne(dialog);
        }
    });
    connect(dialog, &QDialog::rejected, this, [=, this]() {
        if (node->id == 0) {
            ResourcePool<Node>::Instance().Recycle(node);
            dialog_list_->removeOne(dialog);
        } else {
            dialog_hash_->remove(node->id);
        }
    });

    connect(table_model, &TransModel::SResizeColumnToContents, dialog->View(), &QTableView::resizeColumnToContents);
    connect(table_model, &TransModel::SSearch, tree_model, &NodeModel::RSearch);

    connect(table_model, &TransModel::SUpdateLeafValue, dialog, &InsertNodeOrder::RUpdateLeafValue);
    connect(dialog, &InsertNodeOrder::SUpdateLeafValue, tree_model, &NodeModel::RUpdateLeafValue);

    connect(dialog, &InsertNodeOrder::SSyncBool, tree_model, &NodeModel::RSyncBool);

    connect(dialog, &InsertNodeOrder::SSyncBool, table_model, &TransModel::RSyncBool);
    connect(dialog, &InsertNodeOrder::SSyncInt, table_model, &TransModel::RSyncInt);

    connect(tree_model, &NodeModel::SSyncBool, dialog, &InsertNodeOrder::RSyncBool);
    connect(tree_model, &NodeModel::SSyncInt, dialog, &InsertNodeOrder::RSyncInt);
    connect(tree_model, &NodeModel::SSyncString, dialog, &InsertNodeOrder::RSyncString);

    dialog_list_->append(dialog);

    SetTableView(dialog->View(), std::to_underlying(TransEnumO::kDescription));
    DelegateO(dialog->View(), settings_);
    dialog->show();
}

void MainWindow::REditTransDocument(const QModelIndex& index)
{
    auto* leaf_widget { dynamic_cast<TransWidget*>(ui->tabWidget->currentWidget()) };
    if (!leaf_widget || !index.isValid())
        return;

    const auto document_dir { QDir::homePath() + "/" + settings_->document_dir };
    const int trans_id { index.siblingAtColumn(std::to_underlying(TransEnum::kID)).data().toInt() };

    auto* document_pointer { leaf_widget->Model()->GetDocumentPointer(index) };
    if (!document_pointer)
        return;

    auto* dialog { new EditDocument(document_pointer, document_dir, this) };

    if (dialog->exec() == QDialog::Accepted)
        data_->sql->WriteField(data_->info.trans, kDocument, document_pointer->join(kSemicolon), trans_id);
}

void MainWindow::REditNodeDocument(const QModelIndex& index)
{
    if (!tree_widget_ || !index.isValid())
        return;

    const auto document_dir { QDir::homePath() + "/" + settings_->document_dir };
    const int node_id { index.siblingAtColumn(std::to_underlying(NodeEnum::kID)).data().toInt() };

    auto* document_pointer { tree_widget_->Model()->GetDocumentPointer(node_id) };
    if (!document_pointer)
        return;

    auto* dialog { new EditDocument(document_pointer, document_dir, this) };

    if (dialog->exec() == QDialog::Accepted)
        data_->sql->WriteField(data_->info.node, kDocument, document_pointer->join(kSemicolon), node_id);
}

void MainWindow::RTransRef(const QModelIndex& index)
{
    if (!tree_widget_ || !index.isValid())
        return;

    const int node_id { index.siblingAtColumn(std::to_underlying(NodeEnum::kID)).data().toInt() };
    const int unit { index.siblingAtColumn(std::to_underlying(NodeEnum::kUnit)).data().toInt() };

    switch (start_) {
    case Section::kProduct:
        TransRefP(node_id, unit);
        break;
    case Section::kStakeholder:
        TransRefS(node_id, unit);
        break;
    default:
        break;
    }
}

void MainWindow::TransRefP(int node_id, int unit)
{
    if (unit == std::to_underlying(UnitP::kPos) || start_ != Section::kProduct)
        return;

    if (!sup_wgt_hash_->contains(node_id)) {
        CreateTransRef(tree_widget_->Model(), sup_wgt_hash_, data_, node_id);
    }

    auto widget { sup_wgt_hash_->value(node_id, nullptr) };
    if (!widget)
        return;

    ui->tabWidget->setCurrentWidget(widget);
    widget->activateWindow();
}

void MainWindow::TransRefS(int node_id, int unit)
{
    if (unit != std::to_underlying(UnitS::kCust) || start_ != Section::kStakeholder)
        return;

    if (!sup_wgt_hash_->contains(node_id)) {
        CreateTransRef(tree_widget_->Model(), sup_wgt_hash_, data_, node_id);
    }

    auto widget { sup_wgt_hash_->value(node_id, nullptr) };
    if (!widget)
        return;

    ui->tabWidget->setCurrentWidget(widget);
    widget->activateWindow();
}

void MainWindow::RUpdateName(int node_id, const QString& name, bool branch)
{
    auto model { tree_widget_->Model() };
    auto* widget { ui->tabWidget };

    auto* tab_bar { widget->tabBar() };
    int count { widget->count() };

    QSet<int> nodes;

    if (!branch) {
        nodes.insert(node_id);
        if (start_ == Section::kStakeholder)
            UpdateStakeholderReference(nodes, branch);

        if (!trans_wgt_hash_->contains(node_id))
            return;

    } else {
        nodes = model->ChildrenIDFPTS(node_id);
    }

    int tab_node_id {};
    QString path {};

    for (int index = 0; index != count; ++index) {
        tab_node_id = tab_bar->tabData(index).value<Tab>().node_id;

        if (widget->isTabVisible(index) && nodes.contains(tab_node_id)) {
            path = model->GetPath(tab_node_id);

            if (!branch) {
                tab_bar->setTabText(index, name);
            }

            tab_bar->setTabToolTip(index, path);
        }
    }

    if (start_ == Section::kStakeholder)
        UpdateStakeholderReference(nodes, branch);
}

void MainWindow::RUpdateSettings(const Settings& settings, const Interface& interface)
{
    bool resize_column { false };

    if (interface_ != interface) {
        UpdateInterface(interface);
    }

    if (*settings_ != settings) {
        bool update_default_unit { settings_->default_unit != settings.default_unit };
        resize_column |= settings_->amount_decimal != settings.amount_decimal || settings_->common_decimal != settings.common_decimal
            || settings_->date_format != settings.date_format;

        *settings_ = settings;

        if (update_default_unit)
            tree_widget_->Model()->UpdateDefaultUnit(settings.default_unit);

        tree_widget_->UpdateStatus();
        ytx_sql_->UpdateSettings(settings, start_);
    }

    if (resize_column) {
        auto* current_widget { ui->tabWidget->currentWidget() };

        if (auto* leaf_widget = dynamic_cast<TransWidget*>(current_widget)) {
            auto* header { leaf_widget->View()->horizontalHeader() };

            int column { std::to_underlying(TransEnum::kDescription) };
            auto* model { leaf_widget->Model().data() };

            if (qobject_cast<SupportModel*>(model) || qobject_cast<TransModelO*>(model)) {
                column = std::to_underlying(TransEnumO::kDescription);
            }

            ResizeColumn(header, column);
            return;
        }

        if (auto* tree_widget = dynamic_cast<NodeWidget*>(current_widget)) {
            auto* header { tree_widget->View()->header() };
            ResizeColumn(header, std::to_underlying(NodeEnum::kDescription));
        }
    }
}

void MainWindow::UpdateInterface(CInterface& interface)
{
    auto new_separator { interface.separator };
    auto old_separator { interface_.separator };

    if (old_separator != new_separator) {
        finance_tree_->Model()->UpdateSeparatorFPTS(old_separator, new_separator);
        stakeholder_tree_->Model()->UpdateSeparatorFPTS(old_separator, new_separator);
        product_tree_->Model()->UpdateSeparatorFPTS(old_separator, new_separator);
        task_tree_->Model()->UpdateSeparatorFPTS(old_separator, new_separator);

        auto* widget { ui->tabWidget };
        int count { ui->tabWidget->count() };

        for (int index = 0; index != count; ++index)
            widget->setTabToolTip(index, widget->tabToolTip(index).replace(old_separator, new_separator));
    }

    if (interface_.language != interface.language) {
        MainWindowUtils::Message(QMessageBox::Information, tr("Language Changed"),
            tr("The language has been changed. Please restart the application for the changes to take effect."), kThreeThousand);
    }

    interface_ = interface;

    app_settings_->beginGroup(kInterface);
    app_settings_->setValue(kLanguage, interface.language);
    app_settings_->setValue(kSeparator, interface.separator);
    app_settings_->endGroup();
}

void MainWindow::UpdateStakeholderReference(QSet<int> stakeholder_nodes, bool branch) const
{
    auto* widget { ui->tabWidget };
    auto stakeholder_model { tree_widget_->Model() };
    auto* order_model { static_cast<NodeModelO*>(sales_tree_->Model().data()) };
    auto* tab_bar { widget->tabBar() };
    int count { widget->count() };

    // 使用 QtConcurrent::run 启动后台线程
    auto future = QtConcurrent::run([=]() -> QVector<std::tuple<int, QString, QString>> {
        QVector<std::tuple<int, QString, QString>> updates;

        // 遍历所有选项卡，计算需要更新的项
        for (int index = 0; index != count; ++index) {
            const auto& data { tab_bar->tabData(index).value<Tab>() };
            bool update = data.section == Section::kSales || data.section == Section::kPurchase;

            if (!widget->isTabVisible(index) && update) {
                int order_node_id = data.node_id;
                if (order_node_id == 0)
                    continue;

                int order_party = order_model->Party(order_node_id);
                if (!stakeholder_nodes.contains(order_party))
                    continue;

                QString name = stakeholder_model->Name(order_party);
                QString path = stakeholder_model->GetPath(order_party);

                // 收集需要更新的信息
                updates.append(std::make_tuple(index, name, path));
            }
        }

        return updates;
    });

    // 创建 QFutureWatcher 用于监控任务完成
    auto* watcher = new QFutureWatcher<QVector<std::tuple<int, QString, QString>>>();

    // 连接信号槽，监测任务完成
    connect(watcher, &QFutureWatcher<QVector<std::tuple<int, QString, QString>>>::finished, this, [watcher, tab_bar, branch]() {
        // 获取后台线程的结果
        const auto& updates = watcher->result();

        // 更新 UI
        for (const auto& [index, name, path] : updates) {
            if (!branch)
                tab_bar->setTabText(index, name);

            tab_bar->setTabToolTip(index, path);
        }

        // 删除 watcher，避免内存泄漏
        watcher->deleteLater();
    });

    // 设置未来任务给 watcher
    watcher->setFuture(future);
}

void MainWindow::LoadAndInstallTranslator(CString& language)
{
    if (language == kEnUS)
        return;

    const QString ytx_language { QStringLiteral(":/I18N/I18N/ytx_%1.qm").arg(language) };
    if (ytx_translator_.load(ytx_language))
        qApp->installTranslator(&ytx_translator_);

    const QString qt_language { QStringLiteral(":/I18N/I18N/qt_%1.qm").arg(language) };
    if (qt_translator_.load(qt_language))
        qApp->installTranslator(&qt_translator_);

    if (language == kZhCN)
        QLocale::setDefault(QLocale(QLocale::Chinese, QLocale::China));
}

void MainWindow::ResizeColumn(QHeaderView* header, int stretch_column) const
{
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    header->setSectionResizeMode(stretch_column, QHeaderView::Stretch);
}

void MainWindow::AppSettings()
{
    app_settings_ = std::make_unique<QSettings>(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/ytx.ini", QSettings::IniFormat);

    QString language_code { kEnUS };
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));

    switch (QLocale::system().language()) {
    case QLocale::Chinese:
        language_code = kZhCN;
        break;
    default:
        break;
    }

    app_settings_->beginGroup(kInterface);
    interface_.language = app_settings_->value(kLanguage, language_code).toString();
    interface_.theme = app_settings_->value(kTheme, kSolarizedDark).toString();
    interface_.separator = app_settings_->value(kSeparator, kDash).toString();
    app_settings_->endGroup();

    LoadAndInstallTranslator(interface_.language);

#ifdef Q_OS_WIN
    const QString theme { QStringLiteral("file:///:/theme/theme/%1 Win.qss").arg(interface_.theme) };
#elif defined(Q_OS_MACOS)
    const QString theme { QStringLiteral("file:///:/theme/theme/%1 Mac.qss").arg(interface_.theme) };
#endif

    qApp->setStyleSheet(theme);
}

void MainWindow::on_actionSearch_triggered()
{
    auto* dialog { new Search(tree_widget_->Model(), stakeholder_tree_->Model(), product_tree_->Model(), settings_, data_->sql, data_->info, this) };
    dialog->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);

    connect(dialog, &Search::SNodeLocation, this, &MainWindow::RNodeLocation);
    connect(dialog, &Search::STransLocation, this, &MainWindow::RTransLocation);
    connect(tree_widget_->Model(), &NodeModel::SSearch, dialog, &Search::RSearch);
    connect(dialog, &QDialog::rejected, this, [=, this]() { dialog_list_->removeOne(dialog); });

    dialog_list_->append(dialog);
    dialog->show();
}

void MainWindow::RNodeLocation(int node_id)
{
    auto* widget { tree_widget_ };
    ui->tabWidget->setCurrentWidget(widget);

    if (start_ == Section::kSales || start_ == Section::kPurchase) {
        if (node_id <= 0)
            return;

        tree_widget_->Model()->RetrieveNode(node_id);
    }

    auto index { tree_widget_->Model()->GetIndex(node_id) };
    widget->activateWindow();
    widget->View()->setCurrentIndex(index);
}

void MainWindow::RTransLocation(int trans_id, int lhs_node_id, int rhs_node_id)
{
    int id { lhs_node_id };

    auto Contains = [&](int node_id) {
        if (trans_wgt_hash_->contains(node_id)) {
            id = node_id;
            return true;
        }
        return false;
    };

    switch (start_) {
    case Section::kSales:
    case Section::kPurchase:
        OrderTransLocation(lhs_node_id);
        break;
    case Section::kStakeholder:
        if (!Contains(lhs_node_id))
            CreateLeafFPTS(tree_widget_->Model(), trans_wgt_hash_, data_, settings_, id);
        break;
    case Section::kFinance:
    case Section::kProduct:
    case Section::kTask:
        if (!Contains(lhs_node_id) && !Contains(rhs_node_id))
            CreateLeafFPTS(tree_widget_->Model(), trans_wgt_hash_, data_, settings_, id);
        break;
    default:
        break;
    }

    SwitchToLeaf(id, trans_id);
}

void MainWindow::SalesNodeLocation(int node_id)
{
    RSectionGroup(std::to_underlying(Section::kSales));
    ui->rBtnSales->setChecked(true);

    ui->tabWidget->setCurrentWidget(tree_widget_);

    tree_widget_->Model()->RetrieveNode(node_id);
    tree_widget_->activateWindow();

    auto index { tree_widget_->Model()->GetIndex(node_id) };
    tree_widget_->View()->setCurrentIndex(index);
}

void MainWindow::RSyncInt(int node_id, int column, const QVariant& value)
{
    if (column != std::to_underlying(NodeEnumO::kParty))
        return;

    const int party_id { value.toInt() };

    auto model { stakeholder_tree_->Model() };
    auto* widget { ui->tabWidget };
    auto* tab_bar { widget->tabBar() };
    int count { widget->count() };

    for (int index = 0; index != count; ++index) {
        if (widget->isTabVisible(index) && tab_bar->tabData(index).value<Tab>().node_id == node_id) {
            tab_bar->setTabText(index, model->Name(party_id));
            tab_bar->setTabToolTip(index, model->GetPath(party_id));
        }
    }
}

void MainWindow::on_actionPreferences_triggered()
{
    auto model { tree_widget_->Model() };

    auto* preference { new Preferences(data_->info, model, interface_, *settings_, this) };
    connect(preference, &Preferences::SUpdateSettings, this, &MainWindow::RUpdateSettings);
    preference->exec();
}

void MainWindow::on_actionAbout_triggered()
{
    static About* dialog = nullptr;

    if (!dialog) {
        dialog = new About(this);
        dialog->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);
        connect(dialog, &QDialog::finished, [=]() { dialog = nullptr; });
    }

    dialog->show();
    dialog->activateWindow();
}

void MainWindow::on_actionNewFile_triggered()
{
    auto file_path { QFileDialog::getSaveFileName(this, tr("New File"), QDir::homePath(), "*.ytx", nullptr) };
    if (!MainWindowUtils::CheckFileName(file_path, kDotSuffixYTX))
        return;

    if (ytx_sql_->NewFile(file_path))
        ROpenFile(file_path);
}

void MainWindow::on_actionOpenFile_triggered()
{
    const auto file_path { QFileDialog::getOpenFileName(this, tr("Open File"), QDir::homePath(), "*.ytx", nullptr) };
    ROpenFile(file_path);
}

void MainWindow::SetClearMenuAction()
{
    auto* menu { ui->menuRecent };

    if (!menu->isEmpty()) {
        auto* separator { ui->actionSeparator };
        menu->addAction(separator);
        separator->setSeparator(true);

        menu->addAction(ui->actionClearMenu);
    }
}

void MainWindow::on_actionClearMenu_triggered()
{
    ui->menuRecent->clear();
    recent_file_.clear();
    MainWindowUtils::WriteSettings(app_settings_, recent_file_, kRecent, kFile);
}

void MainWindow::on_tabWidget_tabBarDoubleClicked(int index) { RNodeLocation(ui->tabWidget->tabBar()->tabData(index).value<Tab>().node_id); }

void MainWindow::RUpdateState()
{
    auto* leaf_widget { dynamic_cast<TransWidget*>(ui->tabWidget->currentWidget()) };
    if (!leaf_widget)
        return;

    auto table_model { leaf_widget->Model() };
    table_model->UpdateAllState(Check { QObject::sender()->property(kCheck).toInt() });
}

void MainWindow::SwitchSection(CTab& last_tab) const
{
    auto* tab_widget { ui->tabWidget };
    auto* tab_bar { tab_widget->tabBar() };
    const int count { tab_widget->count() };
    Tab tab {};

    for (int index = 0; index != count; ++index) {
        tab = tab_bar->tabData(index).value<Tab>();
        tab_widget->setTabVisible(index, tab.section == start_);

        if (tab == last_tab)
            tab_widget->setCurrentIndex(index);
    }

    MainWindowUtils::SwitchDialog(dialog_list_, true);
    MainWindowUtils::SwitchDialog(dialog_hash_, true);
}

void MainWindow::UpdateLastTab() const
{
    if (data_) {
        auto index { ui->tabWidget->currentIndex() };
        data_->tab = ui->tabWidget->tabBar()->tabData(index).value<Tab>();
    }
}

void MainWindow::on_tabWidget_currentChanged(int /*index*/)
{
    auto* widget { ui->tabWidget->currentWidget() };
    if (!widget)
        return;

    ui->actionStatement->setEnabled(start_ == Section::kSales || start_ == Section::kPurchase);

    bool is_tree { MainWindowUtils::IsTreeWidget(widget) };
    bool is_order { start_ == Section::kSales || start_ == Section::kPurchase };
    bool is_not_order_table { !is_tree && !is_order };

    ui->actionAppendNode->setEnabled(is_tree);
    ui->actionEditNode->setEnabled(is_tree && !is_order);

    ui->actionCheckAll->setEnabled(is_not_order_table);
    ui->actionCheckNone->setEnabled(is_not_order_table);
    ui->actionCheckReverse->setEnabled(is_not_order_table);
    ui->actionJump->setEnabled(is_not_order_table);
    ui->actionSupportJump->setEnabled(is_not_order_table);

    ui->actionAppendTrans->setEnabled(!is_tree);
}

void MainWindow::on_actionAppendTrans_triggered()
{
    auto* active_window { QApplication::activeWindow() };
    if (auto* edit_node_order = dynamic_cast<InsertNodeOrder*>(active_window)) {
        MainWindowUtils::AppendTrans(edit_node_order, start_);
        return;
    }

    auto* widget { ui->tabWidget->currentWidget() };
    if (!widget)
        return;

    if (auto* leaf_widget = dynamic_cast<TransWidget*>(widget)) {
        MainWindowUtils::AppendTrans(leaf_widget, start_);
    }
}

void MainWindow::on_actionExportYTX_triggered()
{
    CString& source { SqlConnection::Instance().DatabaseName() };
    if (source.isEmpty())
        return;

    QString destination { QFileDialog::getSaveFileName(this, tr("Export Structure"), QDir::homePath(), QStringLiteral("*.ytx")) };
    if (!MainWindowUtils::CheckFileName(destination, kDotSuffixYTX))
        return;

    if (!ytx_sql_->NewFile(destination))
        return;

    auto future = QtConcurrent::run([source, destination]() {
        QSqlDatabase source_db;
        if (!MainWindowUtils::AddDatabase(source_db, source, kSourceConnection))
            return false;

        QSqlDatabase destination_db;
        if (!MainWindowUtils::AddDatabase(destination_db, destination, kDestinationConnection)) {
            MainWindowUtils::RemoveDatabase(kSourceConnection);
            return false;
        }

        try {
            QStringList tables { kFinance, kStakeholder, kTask, kProduct };
            QStringList columns { kName, kRule, kType, kUnit, kRemoved };
            MainWindowUtils::ExportYTX(source, destination, tables, columns);

            tables = { kFinancePath, kStakeholderPath, kTaskPath, kProductPath };
            columns = { kAncestor, kDescendant, kDistance };
            MainWindowUtils::ExportYTX(source, destination, tables, columns);

            MainWindowUtils::RemoveDatabase(kSourceConnection);
            MainWindowUtils::RemoveDatabase(kDestinationConnection);
            return true;
        } catch (...) {
            qWarning() << "Export failed due to an unknown exception.";
            MainWindowUtils::RemoveDatabase(kSourceConnection);
            MainWindowUtils::RemoveDatabase(kDestinationConnection);
            return false;
        }
    });

    auto* watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [watcher, destination]() {
        watcher->deleteLater();

        bool success { watcher->future().result() };
        if (success) {
            MainWindowUtils::Message(QMessageBox::Information, tr("Export Completed"), tr("Export completed successfully."), kThreeThousand);
        } else {
            QFile::remove(destination);
            MainWindowUtils::Message(QMessageBox::Critical, tr("Export Failed"), tr("Export failed. The file has been deleted."), kThreeThousand);
        }
    });

    watcher->setFuture(future);
}

void MainWindow::on_actionExportExcel_triggered()
{
    CString& source { SqlConnection::Instance().DatabaseName() };
    if (source.isEmpty())
        return;

    QString destination { QFileDialog::getSaveFileName(this, tr("Export Excel"), QDir::homePath(), QStringLiteral("*.xlsx")) };
    if (!MainWindowUtils::CheckFileName(destination, kDotSuffixXLSX))
        return;

    auto future = QtConcurrent::run([source, destination, this]() {
        QSqlDatabase source_db;
        if (!MainWindowUtils::AddDatabase(source_db, source, kSourceConnection))
            return false;

        try {
            const QStringList list { tr("Ancestor"), tr("Descendant"), tr("Distance") };

            YXlsx::Document d(destination);

            auto book1 { d.GetWorkbook() };
            book1->AppendSheet(data_->info.node);
            book1->GetCurrentWorksheet()->WriteRow(1, 1, data_->info.node_header);
            MainWindowUtils::ExportExcel(source, data_->info.node, book1->GetCurrentWorksheet());

            book1->AppendSheet(data_->info.path);
            book1->GetCurrentWorksheet()->WriteRow(1, 1, list);
            MainWindowUtils::ExportExcel(source, data_->info.path, book1->GetCurrentWorksheet(), false);

            book1->AppendSheet(data_->info.trans);
            book1->GetCurrentWorksheet()->WriteRow(1, 1, data_->info.excel_trans_header);
            MainWindowUtils::ExportExcel(source, data_->info.trans, book1->GetCurrentWorksheet());

            d.Save();
            MainWindowUtils::RemoveDatabase(kSourceConnection);
            return true;
        } catch (...) {
            qWarning() << "Export failed due to an unknown exception.";
            MainWindowUtils::RemoveDatabase(kSourceConnection);
            return false;
        }
    });

    auto* watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [watcher, destination]() {
        watcher->deleteLater();

        bool success { watcher->future().result() };
        if (success) {
            MainWindowUtils::Message(QMessageBox::Information, tr("Export Completed"), tr("Export completed successfully."), kThreeThousand);
        } else {
            QFile::remove(destination);
            MainWindowUtils::Message(QMessageBox::Critical, tr("Export Failed"), tr("Export failed. The file has been deleted."), kThreeThousand);
        }
    });

    watcher->setFuture(future);
}
