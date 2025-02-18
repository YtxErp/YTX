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
#include "delegate/readonly/doublespinr.h"
#include "delegate/readonly/doublespinunitr.h"
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
#include "dialog/editnode/editnodefinance.h"
#include "dialog/editnode/editnodeorder.h"
#include "dialog/editnode/editnodeproduct.h"
#include "dialog/editnode/editnodestakeholder.h"
#include "dialog/editnode/editnodetask.h"
#include "dialog/preferences.h"
#include "dialog/removenode.h"
#include "dialog/search.h"
#include "document.h"
#include "global/resourcepool.h"
#include "global/signalstation.h"
#include "global/sqlconnection.h"
#include "mainwindowutils.h"
#include "table/model/sortfilterproxymodel.h"
#include "table/model/tablemodelfinance.h"
#include "table/model/tablemodelproduct.h"
#include "table/model/tablemodelstakeholder.h"
#include "table/model/tablemodelsupport.h"
#include "table/model/tablemodeltask.h"
#include "tree/model/treemodelfinance.h"
#include "tree/model/treemodelorder.h"
#include "tree/model/treemodelproduct.h"
#include "tree/model/treemodelstakeholder.h"
#include "tree/model/treemodeltask.h"
#include "ui_mainwindow.h"
#include "widget/tablewidget/tablewidgetfpts.h"
#include "widget/treewidget/treewidgetfpt.h"
#include "widget/treewidget/treewidgetorder.h"
#include "widget/treewidget/treewidgetstakeholder.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    QResource::registerResource(MainWindowUtils::ResourceFile());
    AppSettings();

    ui->setupUi(this);
    SignalBlocker blocker(this);

    SetTabWidget();
    SetConnect();
    StringInitializer::SetHeader(finance_data_.info, product_data_.info, stakeholder_data_.info, task_data_.info, sales_data_.info, purchase_data_.info);
    SetAction();

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
        MainWindowUtils::WriteSettings(file_settings_, MainWindowUtils::SaveTab(finance_table_hash_), kFinance, kTabID);
        MainWindowUtils::WriteSettings(finance_tree_->View()->header(), &QHeaderView::saveState, file_settings_, kFinance, kHeaderState);

        MainWindowUtils::WriteSettings(file_settings_, MainWindowUtils::SaveTab(product_table_hash_), kProduct, kTabID);
        MainWindowUtils::WriteSettings(product_tree_->View()->header(), &QHeaderView::saveState, file_settings_, kProduct, kHeaderState);

        MainWindowUtils::WriteSettings(file_settings_, MainWindowUtils::SaveTab(stakeholder_table_hash_), kStakeholder, kTabID);
        MainWindowUtils::WriteSettings(stakeholder_tree_->View()->header(), &QHeaderView::saveState, file_settings_, kStakeholder, kHeaderState);

        MainWindowUtils::WriteSettings(file_settings_, MainWindowUtils::SaveTab(task_table_hash_), kTask, kTabID);
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

    if (!MainWindowUtils::IsValidFile(file_info)) {
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
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + kSlash + complete_base_name + kSuffixINI, QSettings::IniFormat);

    sql_ = YtxSqlite(start_);
    SetFinanceData();
    SetTaskData();
    SetProductData();
    SetStakeholderData();
    SetSalesData();
    SetPurchaseData();

    CreateSection(finance_tree_, finance_table_hash_, finance_data_, finance_settings_, tr("Finance"));
    CreateSection(stakeholder_tree_, stakeholder_table_hash_, stakeholder_data_, stakeholder_settings_, tr("Stakeholder"));
    CreateSection(product_tree_, product_table_hash_, product_data_, product_settings_, tr("Product"));
    CreateSection(task_tree_, task_table_hash_, task_data_, task_settings_, tr("Task"));
    CreateSection(sales_tree_, sales_table_hash_, sales_data_, sales_settings_, tr("Sales"));
    CreateSection(purchase_tree_, purchase_table_hash_, purchase_data_, purchase_settings_, tr("Purchase"));

    switch (start_) {
    case Section::kFinance:
        on_rBtnFinance_toggled(true);
        break;
    case Section::kStakeholder:
        on_rBtnStakeholder_toggled(true);
        break;
    case Section::kProduct:
        on_rBtnProduct_toggled(true);
        break;
    case Section::kTask:
        on_rBtnTask_toggled(true);
        break;
    case Section::kSales:
        on_rBtnSales_toggled(true);
        break;
    case Section::kPurchase:
        on_rBtnPurchase_toggled(true);
        break;
    default:
        break;
    }

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

    const int parent_id { parent_index.isValid() ? parent_index.siblingAtColumn(std::to_underlying(TreeEnum::kID)).data().toInt() : -1 };
    InsertNodeFunction(parent_index, parent_id, current_index.row() + 1);
}

void MainWindow::RTreeViewDoubleClicked(const QModelIndex& index)
{
    if (index.column() != 0)
        return;

    const int type { index.siblingAtColumn(std::to_underlying(TreeEnum::kType)).data().toInt() };
    if (type == kTypeBranch)
        return;

    const int node_id { index.siblingAtColumn(std::to_underlying(TreeEnum::kID)).data().toInt() };
    if (node_id <= 0)
        return;

    if (!table_hash_->contains(node_id)) {
        if (start_ == Section::kSales || start_ == Section::kPurchase) {
            if (auto it { dialog_hash_->constFind(node_id) }; it != dialog_hash_->constEnd()) {
                auto dialog { *it };
                dialog->show();
                dialog->raise();
                dialog->activateWindow();
                return;
            }

            const int party_id { index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kParty)).data().toInt() };
            if (party_id <= 0)
                return;

            CreateTableOrder(tree_widget_->Model(), table_hash_, data_, settings_, node_id, party_id);
        }

        if (start_ != Section::kSales && start_ != Section::kPurchase) {
            if (type == kTypeSupport)
                CreateTableSupport(tree_widget_->Model(), table_hash_, data_, settings_, node_id);
            else
                CreateTableFPTS(tree_widget_->Model(), table_hash_, data_, settings_, node_id);
        }
    }

    SwitchTab(node_id);
}

void MainWindow::SwitchTab(int node_id, int trans_id) const
{
    auto* widget { table_hash_->value(node_id, nullptr) };
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
    view->scrollTo(index.siblingAtColumn(std::to_underlying(TableEnum::kDateTime)), QAbstractItemView::PositionAtCenter);
}

void MainWindow::CreateTableFPTS(PTreeModel tree_model, TableHash* table_hash, CData* data, CSettings* settings, int node_id)
{
    if (!tree_model || !table_hash || !data || !settings || !tree_model->Contains(node_id))
        return;

    CString name { tree_model->Name(node_id) };
    auto* sql { data->sql };
    const Info& info { data->info };
    const Section section { info.section };
    const bool rule { tree_model->Rule(node_id) };

    TableModel* model {};

    switch (section) {
    case Section::kFinance:
        model = new TableModelFinance(sql, rule, node_id, info, this);
        break;
    case Section::kProduct:
        model = new TableModelProduct(sql, rule, node_id, info, this);
        break;
    case Section::kTask:
        model = new TableModelTask(sql, rule, node_id, info, this);
        break;
    case Section::kStakeholder:
        model = new TableModelStakeholder(sql, rule, node_id, info, this);
        break;
    default:
        break;
    }

    TableWidgetFPTS* widget { new TableWidgetFPTS(model, this) };

    const int tab_index { ui->tabWidget->addTab(widget, name) };
    auto* tab_bar { ui->tabWidget->tabBar() };

    tab_bar->setTabData(tab_index, QVariant::fromValue(Tab { section, node_id }));
    tab_bar->setTabToolTip(tab_index, tree_model->GetPath(node_id));

    auto view { widget->View() };
    SetView(view, section);
    DelegateFPTS(view, tree_model, settings);

    switch (section) {
    case Section::kFinance:
    case Section::kProduct:
    case Section::kTask:
        TableConnectFPT(view, model, tree_model, data);
        DelegateFPT(view, tree_model, settings, node_id);
        break;
    case Section::kStakeholder:
        TableConnectStakeholder(view, model, tree_model, data);
        DelegateStakeholder(view);
        break;
    default:
        break;
    }

    table_hash->insert(node_id, widget);
    SignalStation::Instance().RegisterModel(section, node_id, model);
}

void MainWindow::CreateTableSupport(PTreeModel tree_model, TableHash* table_hash, CData* data, CSettings* settings, int node_id)
{
    if (!tree_model || !table_hash || !data || !settings || !tree_model->Contains(node_id))
        return;

    CString name { tree_model->Name(node_id) };
    auto* sql { data->sql };
    const Info& info { data->info };
    const Section section { info.section };
    const bool rule { tree_model->Rule(node_id) };

    auto* model { new TableModelSupport(sql, rule, node_id, info, this) };
    TableWidgetFPTS* widget { new TableWidgetFPTS(model, this) };

    const int tab_index { ui->tabWidget->addTab(widget, name) };
    auto* tab_bar { ui->tabWidget->tabBar() };

    tab_bar->setTabData(tab_index, QVariant::fromValue(Tab { section, node_id }));
    tab_bar->setTabToolTip(tab_index, tree_model->GetPath(node_id));

    auto view { widget->View() };
    SetSupportView(view);
    DelegateSupport(view, tree_model, settings);

    table_hash->insert(node_id, widget);
    SignalStation::Instance().RegisterModel(section, node_id, model);

    connect(data->sql, &Sqlite::SRemoveMultiTrans, model, &TableModel::RRemoveMultiTrans);
}

void MainWindow::CreateTableOrder(PTreeModel tree_model, TableHash* table_hash, CData* data, CSettings* settings, int node_id, int party_id)
{
    const auto& info { data->info };
    const Section section { info.section };

    if (section != Section::kSales && section != Section::kPurchase)
        return;

    auto* node_shadow { ResourcePool<NodeShadow>::Instance().Allocate() };
    tree_model->SetNodeShadowOrder(node_shadow, node_id);

    auto* sql { data->sql };

    TableModelOrder* model { new TableModelOrder(sql, true, node_id, info, node_shadow, product_tree_->Model(), stakeholder_data_.sql, this) };
    auto params { EditNodeParamsOrder { node_shadow, sql, model, stakeholder_tree_->Model(), settings_, section } };

    TableWidgetOrder* widget { new TableWidgetOrder(std::move(params), this) };

    const int tab_index { ui->tabWidget->addTab(widget, stakeholder_tree_->Model()->Name(party_id)) };
    auto* tab_bar { ui->tabWidget->tabBar() };

    tab_bar->setTabData(tab_index, QVariant::fromValue(Tab { section, node_id }));
    tab_bar->setTabToolTip(tab_index, stakeholder_tree_->Model()->GetPath(party_id));

    auto view { widget->View() };
    SetView(view, section);

    TableConnectOrder(view, model, tree_model, widget);
    DelegateOrder(view, settings);

    table_hash->insert(node_id, widget);
}

void MainWindow::TableConnectFPT(PQTableView table_view, PTableModel table_model, PTreeModel tree_model, const Data* data) const
{
    connect(table_model, &TableModel::SResizeColumnToContents, table_view, &QTableView::resizeColumnToContents);
    connect(table_model, &TableModel::SSearch, tree_model, &TreeModel::RSearch);

    connect(table_model, &TableModel::SUpdateLeafValue, tree_model, &TreeModel::RUpdateLeafValue);
    connect(table_model, &TableModel::SSyncOneValue, tree_model, &TreeModel::RSyncOneValue);

    connect(table_model, &TableModel::SRemoveOneTrans, &SignalStation::Instance(), &SignalStation::RRemoveOneTrans);
    connect(table_model, &TableModel::SAppendOneTrans, &SignalStation::Instance(), &SignalStation::RAppendOneTrans);
    connect(table_model, &TableModel::SUpdateBalance, &SignalStation::Instance(), &SignalStation::RUpdateBalance);
    connect(table_model, &TableModel::SRemoveSupportTrans, &SignalStation::Instance(), &SignalStation::RRemoveSupportTrans);
    connect(table_model, &TableModel::SAppendSupportTrans, &SignalStation::Instance(), &SignalStation::RAppendSupportTrans);

    connect(data->sql, &Sqlite::SRemoveMultiTrans, table_model, &TableModel::RRemoveMultiTrans);
    connect(data->sql, &Sqlite::SMoveMultiTrans, table_model, &TableModel::RMoveMultiTrans);
}

void MainWindow::TableConnectOrder(PQTableView table_view, TableModelOrder* table_model, PTreeModel tree_model, TableWidgetOrder* widget) const
{
    connect(table_model, &TableModel::SSearch, tree_model, &TreeModel::RSearch);
    connect(table_model, &TableModel::SResizeColumnToContents, table_view, &QTableView::resizeColumnToContents);
    connect(table_model, &TableModel::SUpdateLeafValue, widget, &TableWidgetOrder::RUpdateLeafValue);

    connect(tree_model, &TreeModel::SSyncOneValue, widget, &TableWidgetOrder::RSyncOneValue);

    connect(widget, &TableWidgetOrder::SSyncOneValue, tree_model, &TreeModel::RSyncOneValue);
    connect(widget, &TableWidgetOrder::SUpdateLeafValue, tree_model, &TreeModel::RUpdateLeafValue);

    connect(widget, &TableWidgetOrder::SSyncOneValue, table_model, &TableModel::RSyncOneValue);
    connect(widget, &TableWidgetOrder::SSyncFinished, table_model, &TableModel::RSyncOneValue);
    connect(widget, &TableWidgetOrder::SSyncOneValue, this, &MainWindow::RSyncOneValue);
}

void MainWindow::TableConnectStakeholder(PQTableView table_view, PTableModel table_model, PTreeModel tree_model, const Data* data) const
{
    connect(table_model, &TableModel::SResizeColumnToContents, table_view, &QTableView::resizeColumnToContents);
    connect(table_model, &TableModel::SSearch, tree_model, &TreeModel::RSearch);

    connect(data->sql, &Sqlite::SMoveMultiTrans, table_model, &TableModel::RMoveMultiTrans);
    connect(data->sql, &Sqlite::SRemoveMultiTrans, table_model, &TableModel::RRemoveMultiTrans);

    connect(table_model, &TableModel::SRemoveSupportTrans, &SignalStation::Instance(), &SignalStation::RRemoveSupportTrans);
    connect(table_model, &TableModel::SAppendSupportTrans, &SignalStation::Instance(), &SignalStation::RAppendSupportTrans);
}

void MainWindow::DelegateFPTS(PQTableView table_view, PTreeModel tree_model, CSettings* settings) const
{
    auto* date_time { new TableDateTime(settings->date_format, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnum::kDateTime), date_time);

    auto* line { new Line(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnum::kDescription), line);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnum::kCode), line);

    auto* state { new CheckBox(QEvent::MouseButtonRelease, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnum::kState), state);

    auto* document { new Document(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnum::kDocument), document);
    connect(document, &Document::SEditDocument, this, &MainWindow::REditTransDocument);

    auto* lhs_ratio { new DoubleSpin(settings->common_decimal, 0, std::numeric_limits<double>::max(), table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnum::kLhsRatio), lhs_ratio);

    auto* support_node { new SupportID(tree_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnum::kSupportID), support_node);
}

void MainWindow::DelegateFPT(PQTableView table_view, PTreeModel tree_model, CSettings* settings, int node_id) const
{
    auto* value { new DoubleSpin(settings->common_decimal, -std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumFinance::kDebit), value);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumFinance::kCredit), value);

    auto* subtotal { new DoubleSpinR(settings->common_decimal, false, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumFinance::kSubtotal), subtotal);

    auto* filter_model { new SortFilterProxyModel(node_id, table_view) };
    filter_model->setSourceModel(tree_model->LeafModel());

    auto* node { new TableCombo(tree_model, filter_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumFinance::kRhsNode), node);
}

void MainWindow::DelegateStakeholder(PQTableView table_view) const
{
    auto* product_tree_model { product_tree_->Model().data() };
    auto* inside_product { new SpecificUnit(product_tree_model, product_tree_model->UnitModelPS(), table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumStakeholder::kInsideProduct), inside_product);
}

void MainWindow::DelegateOrder(PQTableView table_view, CSettings* settings) const
{
    auto* product_tree_model { product_tree_->Model().data() };
    auto* inside_product { new SpecificUnit(product_tree_model, product_tree_model->UnitModelPS(), table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumOrder::kInsideProduct), inside_product);

    auto stakeholder_tree_model { stakeholder_tree_->Model() };
    auto* support_node { new SupportID(stakeholder_tree_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumOrder::kOutsideProduct), support_node);

    auto* color { new ColorR(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumOrder::kColor), color);

    auto* line { new Line(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumOrder::kDescription), line);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumOrder::kCode), line);

    auto* price { new DoubleSpin(settings->amount_decimal, 0, std::numeric_limits<double>::max(), table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumOrder::kUnitPrice), price);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumOrder::kDiscountPrice), price);

    auto* quantity { new DoubleSpin(settings->common_decimal, -std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumOrder::kFirst), quantity);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumOrder::kSecond), quantity);

    auto* amount { new DoubleSpinR(settings->amount_decimal, true, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumOrder::kGrossAmount), amount);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumOrder::kDiscount), amount);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumOrder::kNetAmount), amount);
}

void MainWindow::CreateSection(TreeWidget* tree_widget, TableHash& table_hash, CData& data, CSettings& settings, CString& name)
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
        RestoreTab(model, table_hash, MainWindowUtils::ReadSettings(file_settings_, info.node, kTabID), data, settings);
        break;
    default:
        break;
    }

    SetView(view, info);
}

void MainWindow::SetDelegate(PQTreeView tree_view, CInfo& info, CSettings& settings) const
{
    DelegateFPTSO(tree_view, info);

    switch (info.section) {
    case Section::kFinance:
        DelegateFinance(tree_view, info, settings);
        break;
    case Section::kTask:
        DelegateTask(tree_view, settings);
        break;
    case Section::kStakeholder:
        DelegateStakeholder(tree_view, settings);
        break;
    case Section::kProduct:
        DelegateProduct(tree_view, settings);
        break;
    case Section::kSales:
    case Section::kPurchase:
        DelegateOrder(tree_view, info, settings);
        break;
    default:
        break;
    }
}

void MainWindow::DelegateFPTSO(PQTreeView tree_view, CInfo& info) const
{
    auto* line { new Line(tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnum::kCode), line);
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnum::kDescription), line);

    auto* plain_text { new TreePlainText(tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnum::kNote), plain_text);

    auto* rule { new TreeCombo(info.rule_map, info.rule_model, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnum::kRule), rule);

    auto* unit { new TreeCombo(info.unit_map, info.unit_model, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnum::kUnit), unit);

    auto* type { new TreeCombo(info.type_map, info.type_model, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnum::kType), type);
}

void MainWindow::DelegateFinance(PQTreeView tree_view, CInfo& info, CSettings& settings) const
{
    auto* final_total { new DoubleSpinUnitR(settings.amount_decimal, false, settings.default_unit, info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumFinance::kLocalTotal), final_total);

    auto* initial_total { new FinanceForeignR(settings.amount_decimal, settings.default_unit, info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumFinance::kForeignTotal), initial_total);
}

void MainWindow::DelegateTask(PQTreeView tree_view, CSettings& settings) const
{
    auto* quantity { new DoubleSpinR(settings.common_decimal, false, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumTask::kQuantity), quantity);

    auto* amount { new DoubleSpinUnitR(settings.amount_decimal, false, finance_settings_.default_unit, finance_data_.info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumTask::kAmount), amount);

    auto* unit_cost { new DoubleSpin(settings.amount_decimal, 0, std::numeric_limits<double>::max(), tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumTask::kUnitCost), unit_cost);

    auto* color { new Color(tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumTask::kColor), color);

    auto* date_time { new TreeDateTime(settings.date_format, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumTask::kDateTime), date_time);

    auto* finished { new CheckBox(QEvent::MouseButtonDblClick, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumTask::kFinished), finished);

    auto* document { new Document(tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumTask::kDocument), document);
    connect(document, &Document::SEditDocument, this, &MainWindow::REditNodeDocument);
}

void MainWindow::DelegateProduct(PQTreeView tree_view, CSettings& settings) const
{
    auto* quantity { new DoubleSpinR(settings.common_decimal, false, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumProduct::kQuantity), quantity);

    auto* amount { new DoubleSpinUnitR(settings.amount_decimal, false, finance_settings_.default_unit, finance_data_.info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumProduct::kAmount), amount);

    auto* unit_price { new DoubleSpin(settings.amount_decimal, 0, std::numeric_limits<double>::max(), tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumProduct::kUnitPrice), unit_price);
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumProduct::kCommission), unit_price);

    auto* color { new Color(tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumProduct::kColor), color);
}

void MainWindow::DelegateStakeholder(PQTreeView tree_view, CSettings& settings) const
{
    auto* amount { new DoubleSpinUnitR(settings.amount_decimal, false, finance_settings_.default_unit, finance_data_.info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumStakeholder::kAmount), amount);

    auto* payment_term { new Spin(0, std::numeric_limits<int>::max(), tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumStakeholder::kPaymentTerm), payment_term);

    auto* tax_rate { new TaxRate(settings.amount_decimal, 0.0, std::numeric_limits<double>::max(), tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumStakeholder::kTaxRate), tax_rate);

    auto* deadline { new TreeDateTime(kDD, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumStakeholder::kDeadline), deadline);

    auto* employee { new SpecificUnit(stakeholder_tree_->Model(), stakeholder_tree_->Model()->UnitModelPS(kUnitEmp), tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumStakeholder::kEmployee), employee);
}

void MainWindow::DelegateOrder(PQTreeView tree_view, CInfo& info, CSettings& settings) const
{
    auto* rule { new TreeCombo(info.rule_map, info.rule_model, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kRule), rule);

    auto* amount { new DoubleSpinUnitR(settings.amount_decimal, false, finance_settings_.default_unit, finance_data_.info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kGrossAmount), amount);
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kNetAmount), amount);

    auto* discount { new DoubleSpinUnitR(settings.amount_decimal, true, finance_settings_.default_unit, finance_data_.info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kDiscount), discount);

    auto* quantity { new DoubleSpinR(settings.common_decimal, true, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kSecond), quantity);
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kFirst), quantity);

    auto stakeholder_tree_model { stakeholder_tree_->Model() };

    auto* employee { new SpecificUnit(stakeholder_tree_model, stakeholder_tree_model->UnitModelPS(kUnitEmp), tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kEmployee), employee);

    auto* name { new OrderNameR(stakeholder_tree_model, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kName), name);

    auto* date_time { new TreeDateTime(settings.date_format, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kDateTime), date_time);

    auto* finished { new CheckBox(QEvent::MouseButtonDblClick, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(TreeEnumOrder::kFinished), finished);
}

void MainWindow::TreeConnect(TreeWidget* tree_widget, const Sqlite* sql) const
{
    auto view { tree_widget->View() };
    auto model { tree_widget->Model() };

    connect(view, &QTreeView::doubleClicked, this, &MainWindow::RTreeViewDoubleClicked);
    connect(view, &QTreeView::customContextMenuRequested, this, &MainWindow::RTreeViewCustomContextMenuRequested);

    connect(model, &TreeModel::SUpdateName, this, &MainWindow::RUpdateName);

    connect(model, &TreeModel::SUpdateStatusValue, tree_widget, &TreeWidget::RUpdateStatusValue);

    connect(model, &TreeModel::SResizeColumnToContents, view, &QTreeView::resizeColumnToContents);

    connect(model, &TreeModel::SRule, &SignalStation::Instance(), &SignalStation::RRule);

    connect(sql, &Sqlite::SRemoveNode, model, &TreeModel::RRemoveNode);
    connect(sql, &Sqlite::SUpdateMultiLeafTotal, model, &TreeModel::RUpdateMultiLeafTotal);

    connect(sql, &Sqlite::SFreeView, this, &MainWindow::RFreeView);
}

void MainWindow::InsertNodeFunction(const QModelIndex& parent, int parent_id, int row)
{
    auto model { tree_widget_->Model() };

    auto* node { ResourcePool<Node>::Instance().Allocate() };
    node->rule = model->Rule(parent_id);
    node->unit = model->Unit(parent_id);
    model->SetParent(node, parent_id);

    if (start_ == Section::kSales || start_ == Section::kPurchase)
        InsertNodeOrder(node, parent, row);

    if (start_ != Section::kSales && start_ != Section::kPurchase)
        InsertNodeFPTS(node, parent, parent_id, row);
}

void MainWindow::on_actionRemove_triggered()
{
    auto* widget { ui->tabWidget->currentWidget() };
    if (!widget)
        return;

    if (auto* tree_widget = dynamic_cast<TreeWidget*>(widget)) {
        RemoveNode(tree_widget);
        return;
    }

    if (auto* table_widget = dynamic_cast<TableWidget*>(widget)) {
        RemoveTrans(table_widget);
    }
}

void MainWindow::RemoveNode(TreeWidget* tree_widget)
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

    const int node_id { index.siblingAtColumn(std::to_underlying(TreeEnum::kID)).data().toInt() };
    const int node_type { index.siblingAtColumn(std::to_underlying(TreeEnum::kType)).data().toInt() };

    if (node_type == kTypeBranch) {
        if (model->ChildrenEmpty(node_id))
            model->RemoveNode(index.row(), index.parent());
        else
            RemoveBranch(model, index, node_id);

        return;
    }

    auto* sql { data_->sql };
    bool interal_reference { sql->InternalReference(node_id) };
    bool exteral_reference { sql->ExternalReference(node_id) };
    bool support_reference { sql->SupportReferenceFPTS(node_id) };

    if (!interal_reference && !exteral_reference && !support_reference) {
        RemoveNonBranch(model, index, node_id, node_type);
        return;
    }

    const int unit { index.siblingAtColumn(std::to_underlying(TreeEnum::kUnit)).data().toInt() };

    auto* dialog { new class RemoveNode(model, start_, node_id, node_type, unit, exteral_reference, this) };
    connect(dialog, &RemoveNode::SRemoveNode, sql, &Sqlite::RRemoveNode);
    connect(dialog, &RemoveNode::SReplaceNode, sql, &Sqlite::RReplaceNode);
    dialog->exec();
}

void MainWindow::RemoveTrans(TableWidget* table_widget)
{
    auto view { table_widget->View() };
    if (!view || !MainWindowUtils::HasSelection(view)) {
        return;
    }

    const QModelIndex current_index { view->currentIndex() };
    if (!current_index.isValid()) {
        return;
    }

    auto model { table_widget->Model() };
    if (!model || model->IsSupport()) {
        return;
    }

    const int current_row { current_index.row() };
    if (!model->removeRows(current_row, 1)) {
        qDebug() << "Failed to remove row:" << current_row;
        return;
    }

    const int new_row_count { model->rowCount() };
    if (new_row_count == 0) {
        return;
    }

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
    auto* widget { table_hash_->value(node_id) };

    if (widget) {
        MainWindowUtils::FreeWidget(widget);
        table_hash_->remove(node_id);
        SignalStation::Instance().DeregisterModel(start_, node_id);
    }
}

void MainWindow::RestoreTab(PTreeModel tree_model, TableHash& table_hash, CIntSet& set, CData& data, CSettings& settings)
{
    if (!tree_model || set.isEmpty())
        return;

    for (int node_id : set) {
        switch (tree_model->TypeFPTS(node_id)) {
        case kTypeSupport:
            CreateTableSupport(tree_model, &table_hash, &data, &settings, node_id);
            break;
        case kTypeLeaf:
            CreateTableFPTS(tree_model, &table_hash, &data, &settings, node_id);
            break;
        default:
            break;
        }
    }
}

void MainWindow::EnableAction(bool enable)
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
    ui->actionExportXlsx->setEnabled(enable);
    ui->actionExportStructure->setEnabled(enable);
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
    CString lock_file_path { file_info.dir().filePath(file_info.completeBaseName() + kSuffixLOCK) };

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
        tree_widget_->RUpdateStatusValue();
    }
}

void MainWindow::on_tabWidget_tabCloseRequested(int index)
{
    if (index == 0)
        return;

    const int node_id { ui->tabWidget->tabBar()->tabData(index).value<Tab>().node_id };
    auto* widget { table_hash_->value(node_id) };

    MainWindowUtils::FreeWidget(widget);
    table_hash_->remove(node_id);

    SignalStation::Instance().DeregisterModel(start_, node_id);
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

void MainWindow::SetView(PQTableView view, Section section) const
{
    view->setSortingEnabled(true);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setAlternatingRowColors(true);
    view->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::CurrentChanged);
    view->setColumnHidden(std::to_underlying(TableEnum::kID), false);

    const int column { (section == Section::kSales || section == Section::kPurchase) ? std::to_underlying(TableEnumOrder::kDescription)
                                                                                     : std::to_underlying(TableEnum::kDescription) };

    auto* h_header { view->horizontalHeader() };
    h_header->setSectionResizeMode(QHeaderView::ResizeToContents);
    h_header->setSectionResizeMode(column, QHeaderView::Stretch);

    auto* v_header { view->verticalHeader() };
    v_header->setDefaultSectionSize(kRowHeight);
    v_header->setSectionResizeMode(QHeaderView::Fixed);
    v_header->setHidden(true);

    view->scrollToBottom();
    view->setCurrentIndex(QModelIndex());
    view->sortByColumn(std::to_underlying(TableEnum::kDateTime), Qt::AscendingOrder); // will run function: AccumulateSubtotal while sorting
}

void MainWindow::SetSupportView(PQTableView view) const
{
    view->setSortingEnabled(true);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setAlternatingRowColors(true);
    view->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::CurrentChanged);
    view->setColumnHidden(std::to_underlying(TableEnumSupport::kID), false);

    auto* h_header { view->horizontalHeader() };
    h_header->setSectionResizeMode(QHeaderView::ResizeToContents);
    h_header->setSectionResizeMode(std::to_underlying(TableEnumSupport::kDescription), QHeaderView::Stretch);

    auto* v_header { view->verticalHeader() };
    v_header->setDefaultSectionSize(kRowHeight);
    v_header->setSectionResizeMode(QHeaderView::Fixed);
    v_header->setHidden(true);

    view->scrollToBottom();
    view->setCurrentIndex(QModelIndex());
    view->sortByColumn(std::to_underlying(TableEnum::kDateTime), Qt::AscendingOrder);
}

void MainWindow::DelegateSupport(PQTableView table_view, PTreeModel tree_model, CSettings* settings) const
{
    auto* line { new Line(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumSupport::kDescription), line);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumSupport::kCode), line);

    auto* date_time { new TableDateTime(settings->date_format, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumSupport::kDateTime), date_time);

    auto* state { new CheckBox(QEvent::MouseButtonRelease, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumSupport::kState), state);

    auto* document { new Document(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumSupport::kDocument), document);
    connect(document, &Document::SEditDocument, this, &MainWindow::REditNodeDocument);

    auto* value { new DoubleSpinR(settings->amount_decimal, true, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumSupport::kLhsDebit), value);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumSupport::kRhsDebit), value);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumSupport::kLhsCredit), value);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumSupport::kRhsCredit), value);

    auto* ratio { new DoubleSpinR(settings->common_decimal, true, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumSupport::kLhsRatio), ratio);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumSupport::kRhsRatio), ratio);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumSupport::kUnitPrice), ratio);

    auto* node_name { new SearchPathTableR(tree_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumSupport::kLhsNode), node_name);
    table_view->setItemDelegateForColumn(std::to_underlying(TableEnumSupport::kRhsNode), node_name);
}

void MainWindow::SetConnect() const
{
    connect(ui->actionCheckAll, &QAction::triggered, this, &MainWindow::RUpdateState);
    connect(ui->actionCheckNone, &QAction::triggered, this, &MainWindow::RUpdateState);
    connect(ui->actionCheckReverse, &QAction::triggered, this, &MainWindow::RUpdateState);
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

    QStringList unit_list { tr("CNY"), tr("HKD"), tr("USD"), tr("GBP"), tr("JPY"), tr("CAD"), tr("AUD"), tr("EUR") };
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

    sql_.QuerySettings(finance_settings_, section);

    sql = new SqliteFinance(info, this);

    auto* model { new TreeModelFinance(sql, info, finance_settings_.default_unit, finance_table_hash_, interface_.separator, this) };
    finance_tree_ = new TreeWidgetFPT(model, info, finance_settings_, this);

    connect(sql, &Sqlite::SMoveMultiSupportTransFPTS, &SignalStation::Instance(), &SignalStation::RMoveMultiSupportTransFPTS);
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
    QStringList unit_list { {}, tr("POS"), tr("BOX"), tr("PC"), tr("SET"), tr("SF") };
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

    sql_.QuerySettings(product_settings_, section);

    sql = new SqliteProduct(info, this);

    auto* model { new TreeModelProduct(sql, info, product_settings_.default_unit, product_table_hash_, interface_.separator, this) };
    product_tree_ = new TreeWidgetFPT(model, info, product_settings_, this);

    connect(sql, &Sqlite::SMoveMultiSupportTransFPTS, &SignalStation::Instance(), &SignalStation::RMoveMultiSupportTransFPTS);
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
    QStringList unit_list { tr("CUST"), tr("EMP"), tr("VEND"), tr("PROD") };
    // IM：Immediate, MS（Monthly Settlement）
    QStringList rule_list { "IS", "MS" };
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

    sql_.QuerySettings(stakeholder_settings_, section);

    sql = new SqliteStakeholder(info, this);

    auto* model { new TreeModelStakeholder(sql, info, stakeholder_settings_.default_unit, stakeholder_table_hash_, interface_.separator, this) };
    stakeholder_tree_ = new TreeWidgetStakeholder(model, info, stakeholder_settings_, this);

    connect(product_data_.sql, &Sqlite::SUpdateProduct, sql, &Sqlite::RUpdateProduct);
    connect(sql, &Sqlite::SUpdateStakeholder, model, &TreeModel::RUpdateStakeholder);
    connect(static_cast<SqliteStakeholder*>(sql), &SqliteStakeholder::SAppendPrice, &SignalStation::Instance(), &SignalStation::RAppendPrice);
    connect(sql, &Sqlite::SMoveMultiSupportTransFPTS, &SignalStation::Instance(), &SignalStation::RMoveMultiSupportTransFPTS);
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
    QStringList unit_list { tr("CUST"), tr("EMP"), tr("VEND"), tr("PROD") };
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

    sql_.QuerySettings(task_settings_, section);

    sql = new SqliteTask(info, this);

    auto* model { new TreeModelTask(sql, info, task_settings_.default_unit, task_table_hash_, interface_.separator, this) };
    task_tree_ = new TreeWidgetFPT(model, info, task_settings_, this);
    connect(sql, &Sqlite::SMoveMultiSupportTransFPTS, &SignalStation::Instance(), &SignalStation::RMoveMultiSupportTransFPTS);
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
    QStringList rule_list { "SO", "RO" };
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

    sql_.QuerySettings(sales_settings_, section);

    sql = new SqliteOrder(info, this);

    auto* model { new TreeModelOrder(sql, info, sales_settings_.default_unit, sales_table_hash_, interface_.separator, this) };
    sales_tree_ = new TreeWidgetOrder(model, info, sales_settings_, this);

    connect(stakeholder_data_.sql, &Sqlite::SUpdateStakeholder, model, &TreeModel::RUpdateStakeholder);
    connect(product_data_.sql, &Sqlite::SUpdateProduct, sql, &Sqlite::RUpdateProduct);
    connect(model, &TreeModel::SSyncOneValue, stakeholder_tree_->Model(), &TreeModel::RSyncOneValue);
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
    QStringList unit_list { tr("IM"), tr("MS"), tr("PEND") };
    // SO: SALES ORDER, RO: REFUND ORDER
    QStringList rule_list { "SO", "RO" };
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

    sql_.QuerySettings(purchase_settings_, section);

    sql = new SqliteOrder(info, this);

    auto* model { new TreeModelOrder(sql, info, purchase_settings_.default_unit, purchase_table_hash_, interface_.separator, this) };
    purchase_tree_ = new TreeWidgetOrder(model, info, purchase_settings_, this);

    connect(stakeholder_data_.sql, &Sqlite::SUpdateStakeholder, model, &TreeModel::RUpdateStakeholder);
    connect(product_data_.sql, &Sqlite::SUpdateProduct, sql, &Sqlite::RUpdateProduct);
    connect(model, &TreeModel::SSyncOneValue, stakeholder_tree_->Model(), &TreeModel::RSyncOneValue);
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

void MainWindow::SetView(PQTreeView tree_view, CInfo& info) const
{
    tree_view->setColumnHidden(std::to_underlying(TreeEnum::kID), false);
    if (info.section == Section::kSales || info.section == Section::kPurchase)
        tree_view->setColumnHidden(std::to_underlying(TreeEnumOrder::kParty), false);

    tree_view->setSelectionMode(QAbstractItemView::SingleSelection);
    tree_view->setDragDropMode(QAbstractItemView::InternalMove);
    tree_view->setEditTriggers(QAbstractItemView::DoubleClicked);
    tree_view->setDropIndicatorShown(true);
    tree_view->setSortingEnabled(true);
    tree_view->setContextMenuPolicy(Qt::CustomContextMenu);
    tree_view->setExpandsOnDoubleClick(true);

    auto* header { tree_view->header() };
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    header->setSectionResizeMode(std::to_underlying(TreeEnum::kDescription), QHeaderView::Stretch);
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

    const int type { parent_index.siblingAtColumn(std::to_underlying(TreeEnum::kType)).data().toInt() };
    if (type != kTypeBranch)
        return;

    const int parent_id { parent_index.siblingAtColumn(std::to_underlying(TreeEnum::kID)).data().toInt() };
    InsertNodeFunction(parent_index, parent_id, 0);
}

template <TableWidgetLike T> void MainWindow::AppendTrans(T* widget)
{
    if (!widget)
        return;

    auto model { widget->Model() };
    if (!model || model->IsSupport())
        return;

    constexpr int ID_ZERO = 0;
    const int empty_row = model->GetNodeRow(ID_ZERO);

    QModelIndex target_index {};

    if (empty_row == -1) {
        const int new_row = model->rowCount();
        if (!model->insertRows(new_row, 1))
            return;

        target_index = model->index(new_row, std::to_underlying(TableEnum::kDateTime));
    } else if (start_ != Section::kSales && start_ != Section::kPurchase)
        target_index = model->index(empty_row, std::to_underlying(TableEnum::kRhsNode));

    if (target_index.isValid()) {
        widget->View()->setCurrentIndex(target_index);
    }
}

void MainWindow::on_actionJump_triggered()
{
    if (start_ == Section::kSales || start_ == Section::kPurchase)
        return;

    auto* table_widget { dynamic_cast<TableWidget*>(ui->tabWidget->currentWidget()) };
    if (!table_widget)
        return;

    auto view { table_widget->View() };
    if (!MainWindowUtils::HasSelection(view))
        return;

    const auto index { view->currentIndex() };
    if (!index.isValid())
        return;

    int row { index.row() };
    const int rhs_node_id { index.sibling(row, std::to_underlying(TableEnum::kRhsNode)).data().toInt() };
    if (rhs_node_id == 0)
        return;

    if (!table_hash_->contains(rhs_node_id))
        CreateTableFPTS(tree_widget_->Model(), table_hash_, data_, settings_, rhs_node_id);

    const int trans_id { index.sibling(row, std::to_underlying(TableEnum::kID)).data().toInt() };
    SwitchTab(rhs_node_id, trans_id);
}

void MainWindow::on_actionSupportJump_triggered()
{
    if (start_ == Section::kSales || start_ == Section::kPurchase)
        return;

    auto* table_widget { dynamic_cast<TableWidget*>(ui->tabWidget->currentWidget()) };
    if (!table_widget)
        return;

    auto view { table_widget->View() };
    if (!MainWindowUtils::HasSelection(view))
        return;

    const auto index { view->currentIndex() };
    if (!index.isValid())
        return;

    auto model { table_widget->Model() };
    if (!model)
        return;

    const int row { index.row() };
    const int id { model->IsSupport() ? index.sibling(row, std::to_underlying(TableEnumSupport::kRhsNode)).data().toInt()
                                      : index.sibling(row, std::to_underlying(TableEnum::kSupportID)).data().toInt() };

    if (id == 0)
        return;

    if (!table_hash_->contains(id)) {
        if (model->IsSupport())
            CreateTableFPTS(tree_widget_->Model(), table_hash_, data_, settings_, id);
        else
            CreateTableSupport(tree_widget_->Model(), table_hash_, data_, settings_, id);
    }

    const int trans_id { index.sibling(row, std::to_underlying(TableEnum::kID)).data().toInt() };
    SwitchTab(id, trans_id);
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

    const int node_id { index.siblingAtColumn(std::to_underlying(TreeEnum::kID)).data().toInt() };
    EditNodeFPTS(index, node_id);
}

void MainWindow::EditNodeFPTS(const QModelIndex& index, int node_id)
{
    auto* unit_model { data_->info.unit_model };

    QDialog* dialog {};
    auto model { tree_widget_->Model() };

    auto* tmp_node { ResourcePool<Node>::Instance().Allocate() };
    model->CopyNodeFPTS(tmp_node, node_id);

    const auto& parent { index.parent() };
    const int parent_id { parent.isValid() ? parent.siblingAtColumn(std::to_underlying(TreeEnum::kID)).data().toInt() : -1 };
    auto parent_path { model->GetPath(parent_id) };
    if (!parent_path.isEmpty())
        parent_path += interface_.separator;

    const auto& name_list { model->ChildrenNameFPTS(parent_id, node_id) };
    const auto* sqlite { data_->sql };

    bool is_not_referenced { !sqlite->InternalReference(node_id) && !sqlite->ExternalReference(node_id) };
    bool branch_enable { is_not_referenced && model->ChildrenEmpty(node_id) && !table_hash_->contains(node_id) };
    bool unit_enable { is_not_referenced };

    auto params { EditNodeParamsFPTS { tmp_node, unit_model, parent_path, name_list, branch_enable, unit_enable } };

    switch (start_) {
    case Section::kFinance:
        dialog = new EditNodeFinance(std::move(params), this);
        break;
    case Section::kTask:
        dialog = new EditNodeTask(std::move(params), settings_->amount_decimal, settings_->date_format, this);
        break;
    case Section::kStakeholder:
        params.unit_enable = is_not_referenced && model->ChildrenEmpty(node_id);
        dialog = new EditNodeStakeholder(std::move(params), model->UnitModelPS(kUnitEmp), settings_->amount_decimal, this);
        break;
    case Section::kProduct:
        params.unit_enable = is_not_referenced && model->ChildrenEmpty(node_id);
        dialog = new EditNodeProduct(std::move(params), settings_->amount_decimal, this);
        break;
    default:
        return ResourcePool<Node>::Instance().Recycle(tmp_node);
    }

    connect(dialog, &QDialog::accepted, this, [=]() { model->UpdateNodeFPTS(tmp_node); });
    dialog->exec();
    ResourcePool<Node>::Instance().Recycle(tmp_node);
}

void MainWindow::InsertNodeFPTS(Node* node, const QModelIndex& parent, int parent_id, int row)
{
    auto tree_model { tree_widget_->Model() };
    auto* unit_model { data_->info.unit_model };

    auto parent_path { tree_model->GetPath(parent_id) };
    if (!parent_path.isEmpty())
        parent_path += interface_.separator;

    const auto& name_list { tree_model->ChildrenNameFPTS(parent_id, 0) };

    QDialog* dialog {};
    const auto params { EditNodeParamsFPTS { node, unit_model, parent_path, name_list, true, true } };

    switch (start_) {
    case Section::kFinance:
        dialog = new EditNodeFinance(std::move(params), this);
        break;
    case Section::kTask:
        node->date_time = QDateTime::currentDateTime().toString(kDateTimeFST);
        dialog = new EditNodeTask(std::move(params), settings_->amount_decimal, settings_->date_format, this);
        break;
    case Section::kStakeholder:
        node->unit = settings_->default_unit;
        dialog = new EditNodeStakeholder(std::move(params), tree_model->UnitModelPS(kUnitEmp), settings_->amount_decimal, this);
        break;
    case Section::kProduct:
        dialog = new EditNodeProduct(std::move(params), settings_->common_decimal, this);
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

void MainWindow::InsertNodeOrder(Node* node, const QModelIndex& parent, int row)
{
    auto tree_model { tree_widget_->Model() };

    auto* node_shadow { ResourcePool<NodeShadow>::Instance().Allocate() };
    tree_model->SetNodeShadowOrder(node_shadow, node);
    if (!node_shadow->id) {
        ResourcePool<NodeShadow>::Instance().Recycle(node_shadow);
        ResourcePool<Node>::Instance().Recycle(node);
        return;
    }

    auto* sql { data_->sql };

    auto* table_model { new TableModelOrder(sql, node->rule, 0, data_->info, node_shadow, product_tree_->Model(), stakeholder_data_.sql, this) };

    auto params { EditNodeParamsOrder { node_shadow, sql, table_model, stakeholder_tree_->Model(), settings_, start_ } };
    auto* dialog { new EditNodeOrder(std::move(params), this) };

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
        ResourcePool<NodeShadow>::Instance().Recycle(node_shadow);
        if (node->id == 0) {
            ResourcePool<Node>::Instance().Recycle(node);
            dialog_list_->removeOne(dialog);
        } else {
            dialog_hash_->remove(node->id);
        }
    });

    connect(table_model, &TableModel::SResizeColumnToContents, dialog->View(), &QTableView::resizeColumnToContents);
    connect(table_model, &TableModel::SUpdateLeafValue, dialog, &EditNodeOrder::RUpdateLeafValue);

    connect(tree_model, &TreeModel::SSyncOneValue, dialog, &EditNodeOrder::RSyncOneValue);

    connect(dialog, &EditNodeOrder::SSyncOneValue, tree_model, &TreeModel::RSyncOneValue);
    connect(dialog, &EditNodeOrder::SSyncOneValue, table_model, &TableModel::RSyncOneValue);
    connect(dialog, &EditNodeOrder::SSyncFinished, table_model, &TableModel::RSyncOneValue);
    connect(dialog, &EditNodeOrder::SUpdateLeafValue, tree_model, &TreeModel::RUpdateLeafValue);

    dialog_list_->append(dialog);

    SetView(dialog->View(), Section::kSales);
    DelegateOrder(dialog->View(), settings_);
    dialog->show();
}

void MainWindow::REditTransDocument()
{
    auto* table_widget { dynamic_cast<TableWidget*>(ui->tabWidget->currentWidget()) };
    if (!table_widget)
        return;

    auto view { table_widget->View() };
    if (!MainWindowUtils::HasSelection(view))
        return;

    const auto index { view->currentIndex() };
    if (!index.isValid())
        return;

    const auto document_dir { QDir::homePath() + "/" + settings_->document_dir };
    const int trans_id { index.siblingAtColumn(std::to_underlying(TableEnum::kID)).data().toInt() };

    auto* document_pointer { table_widget->Model()->GetDocumentPointer(index) };
    auto* dialog { new EditDocument(document_pointer, document_dir, this) };

    if (dialog->exec() == QDialog::Accepted)
        data_->sql->UpdateField(data_->info.trans, document_pointer->join(kSemicolon), kDocument, trans_id);
}

void MainWindow::REditNodeDocument()
{
    if (!tree_widget_)
        return;

    auto view { tree_widget_->View() };
    if (!MainWindowUtils::HasSelection(view))
        return;

    const auto index { view->currentIndex() };
    if (!index.isValid())
        return;

    const auto document_dir { QDir::homePath() + "/" + settings_->document_dir };
    const int id { index.siblingAtColumn(std::to_underlying(TreeEnum::kID)).data().toInt() };

    auto* document_pointer { tree_widget_->Model()->GetDocumentPointer(index) };
    auto* dialog { new EditDocument(document_pointer, document_dir, this) };

    if (dialog->exec() == QDialog::Accepted)
        data_->sql->UpdateField(data_->info.node, document_pointer->join(kSemicolon), kDocument, id);
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

        if (!table_hash_->contains(node_id))
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
        sql_.UpdateSettings(settings, start_);
    }

    if (resize_column) {
        auto* current_widget { ui->tabWidget->currentWidget() };

        if (auto* table_widget = dynamic_cast<TableWidget*>(current_widget)) {
            auto* header { table_widget->View()->horizontalHeader() };
            ResizeColumn(header, true);
            return;
        }

        if (auto* tree_widget = dynamic_cast<TreeWidget*>(current_widget)) {
            auto* header { tree_widget->View()->header() };
            ResizeColumn(header, false);
        }
    }
}
void MainWindow::RFreeView(int node_id)
{
    auto* view { table_hash_->value(node_id) };

    if (view) {
        MainWindowUtils::FreeWidget(view);
        table_hash_->remove(node_id);
        SignalStation::Instance().DeregisterModel(start_, node_id);
    }
}

void MainWindow::UpdateInterface(CInterface& interface)
{
    auto new_language { interface.language };
    if (interface_.language != new_language) {
        if (new_language == kEnUS) {
            qApp->removeTranslator(&ytx_translator_);
            qApp->removeTranslator(&qt_translator_);
        } else
            LoadAndInstallTranslator(new_language);

        ui->retranslateUi(this);
        StringInitializer::SetHeader(finance_data_.info, product_data_.info, stakeholder_data_.info, task_data_.info, sales_data_.info, purchase_data_.info);
        UpdateTranslate();
    }

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

    interface_ = interface;

    app_settings_->beginGroup(kInterface);
    app_settings_->setValue(kLanguage, interface.language);
    app_settings_->setValue(kSeparator, interface.separator);
    app_settings_->endGroup();
}

void MainWindow::UpdateTranslate() const
{
    QWidget* widget {};
    Tab tab_id {};
    auto* tab_widget { ui->tabWidget };
    auto* tab_bar { tab_widget->tabBar() };
    int count { tab_widget->count() };

    for (int index = 0; index != count; ++index) {
        widget = tab_widget->widget(index);
        tab_id = tab_bar->tabData(index).value<Tab>();

        if (MainWindowUtils::IsTreeWidget(widget)) {
            Section section { tab_id.section };
            switch (section) {
            case Section::kFinance:
                tab_widget->setTabText(index, tr("Finance"));
                break;
            case Section::kStakeholder:
                tab_widget->setTabText(index, tr("Stakeholder"));
                break;
            case Section::kProduct:
                tab_widget->setTabText(index, tr("Product"));
                break;
            case Section::kTask:
                tab_widget->setTabText(index, tr("Task"));
                break;
            case Section::kSales:
                tab_widget->setTabText(index, tr("Sales"));
                break;
            case Section::kPurchase:
                tab_widget->setTabText(index, tr("Purchase"));
                break;
            default:
                break;
            }
        }
    }
}

void MainWindow::UpdateStakeholderReference(QSet<int> stakeholder_nodes, bool branch) const
{
    auto* widget { ui->tabWidget };
    auto stakeholder_model { tree_widget_->Model() };
    auto* order_model { static_cast<TreeModelOrder*>(sales_tree_->Model().data()) };
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
    const QString ytx_language { QString(":/I18N/I18N/") + ytx + "_" + language + kSuffixQM };
    if (ytx_translator_.load(ytx_language))
        qApp->installTranslator(&ytx_translator_);

    const QString qt_language { ":/I18N/I18N/qt_" + language + kSuffixQM };
    if (qt_translator_.load(qt_language))
        qApp->installTranslator(&qt_translator_);
}

void MainWindow::ResizeColumn(QHeaderView* header, bool table_view) const
{
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    table_view ? header->setSectionResizeMode(std::to_underlying(TableEnum::kDescription), QHeaderView::Stretch)
               : header->setSectionResizeMode(std::to_underlying(TreeEnum::kDescription), QHeaderView::Stretch);
    ;
}

void MainWindow::AppSettings()
{
    app_settings_ = std::make_unique<QSettings>(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/ytx.ini", QSettings::IniFormat);

    QString language_code {};

    switch (QLocale::system().language()) {
    case QLocale::English:
        language_code = kEnUS;
        break;
    case QLocale::Chinese:
        language_code = kZhCN;
        break;
    default:
        language_code = kEnUS;
        break;
    }

    app_settings_->beginGroup(kInterface);
    interface_.language = app_settings_->value(kLanguage, language_code).toString();
    interface_.theme = app_settings_->value(kTheme, kSolarizedDark).toString();
    interface_.separator = app_settings_->value(kSeparator, kDash).toString();
    app_settings_->endGroup();

    LoadAndInstallTranslator(interface_.language);

#ifdef Q_OS_WIN
    QString theme { "file:///:/theme/theme/" + interface_.theme + " Win.qss" };
#elif defined(Q_OS_MACOS)
    QString theme { "file:///:/theme/theme/" + interface_.theme + " Mac.qss" };
#endif

    qApp->setStyleSheet(theme);
}

void MainWindow::on_actionSearch_triggered()
{
    auto* dialog { new Search(tree_widget_->Model(), stakeholder_tree_->Model(), product_tree_->Model(), settings_, data_->sql, data_->info, this) };
    dialog->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);

    connect(dialog, &Search::SNodeLocation, this, &MainWindow::RNodeLocation);
    connect(dialog, &Search::STransLocation, this, &MainWindow::RTransLocation);
    connect(tree_widget_->Model(), &TreeModel::SSearch, dialog, &Search::RSearch);
    connect(dialog, &QDialog::rejected, this, [=, this]() { dialog_list_->removeOne(dialog); });

    dialog_list_->append(dialog);
    dialog->show();
}

void MainWindow::RNodeLocation(int node_id)
{
    auto* widget { tree_widget_ };
    ui->tabWidget->setCurrentWidget(widget);

    if (start_ == Section::kSales || start_ == Section::kPurchase)
        tree_widget_->Model()->RetriveNodeOrder(node_id);

    auto index { tree_widget_->Model()->GetIndex(node_id) };
    widget->activateWindow();
    widget->View()->setCurrentIndex(index);
}

void MainWindow::RTransLocation(int trans_id, int lhs_node_id, int rhs_node_id)
{
    int id { lhs_node_id };

    auto Contains = [&](int node_id) {
        if (table_hash_->contains(node_id)) {
            id = node_id;
            return true;
        }
        return false;
    };

    if (!Contains(lhs_node_id) && !Contains(rhs_node_id))
        CreateTableFPTS(tree_widget_->Model(), table_hash_, data_, settings_, id);

    SwitchTab(id, trans_id);
}

void MainWindow::RSyncOneValue(int node_id, int column, const QVariant& value)
{
    if (column != std::to_underlying(TreeEnumOrder::kParty))
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
    if (MainWindowUtils::NewFile(sql_, file_path))
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
    auto* table_widget { dynamic_cast<TableWidget*>(ui->tabWidget->currentWidget()) };
    if (!table_widget)
        return;

    auto table_model { table_widget->Model() };
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

void MainWindow::on_rBtnFinance_toggled(bool checked)
{
    if (!checked)
        return;

    start_ = Section::kFinance;

    if (!lock_file_) {
        MainWindowUtils::Message(QMessageBox::Information, tr("File Required"), tr("Please open the file first."), kThreeThousand);
        return;
    }

    MainWindowUtils::SwitchDialog(dialog_list_, false);
    MainWindowUtils::SwitchDialog(dialog_hash_, false);
    UpdateLastTab();

    tree_widget_ = finance_tree_;
    table_hash_ = &finance_table_hash_;
    dialog_list_ = &finance_dialog_list_;
    dialog_hash_ = &finance_dialog_hash_;
    settings_ = &finance_settings_;
    data_ = &finance_data_;

    SwitchSection(data_->tab);
}

void MainWindow::on_rBtnSales_toggled(bool checked)
{
    if (!checked)
        return;

    start_ = Section::kSales;

    if (!lock_file_) {
        MainWindowUtils::Message(QMessageBox::Information, tr("File Required"), tr("Please open the file first."), kThreeThousand);
        return;
    }

    MainWindowUtils::SwitchDialog(dialog_list_, false);
    MainWindowUtils::SwitchDialog(dialog_hash_, false);
    UpdateLastTab();

    tree_widget_ = sales_tree_;
    table_hash_ = &sales_table_hash_;
    dialog_list_ = &sales_dialog_list_;
    dialog_hash_ = &sales_dialog_hash_;
    settings_ = &sales_settings_;
    data_ = &sales_data_;

    SwitchSection(data_->tab);
}

void MainWindow::on_rBtnTask_toggled(bool checked)
{
    if (!checked)
        return;

    start_ = Section::kTask;

    if (!lock_file_) {
        MainWindowUtils::Message(QMessageBox::Information, tr("File Required"), tr("Please open the file first."), kThreeThousand);
        return;
    }

    MainWindowUtils::SwitchDialog(dialog_list_, false);
    MainWindowUtils::SwitchDialog(dialog_hash_, false);
    UpdateLastTab();

    tree_widget_ = task_tree_;
    table_hash_ = &task_table_hash_;
    dialog_list_ = &task_dialog_list_;
    dialog_hash_ = &task_dialog_hash_;
    settings_ = &task_settings_;
    data_ = &task_data_;

    SwitchSection(data_->tab);
}

void MainWindow::on_rBtnStakeholder_toggled(bool checked)
{
    if (!checked)
        return;

    start_ = Section::kStakeholder;

    if (!lock_file_) {
        MainWindowUtils::Message(QMessageBox::Information, tr("File Required"), tr("Please open the file first."), kThreeThousand);
        return;
    }

    MainWindowUtils::SwitchDialog(dialog_list_, false);
    MainWindowUtils::SwitchDialog(dialog_hash_, false);
    UpdateLastTab();

    tree_widget_ = stakeholder_tree_;
    table_hash_ = &stakeholder_table_hash_;
    dialog_list_ = &stakeholder_dialog_list_;
    dialog_hash_ = &stakeholder_dialog_hash_;
    settings_ = &stakeholder_settings_;
    data_ = &stakeholder_data_;

    SwitchSection(data_->tab);
}

void MainWindow::on_rBtnProduct_toggled(bool checked)
{
    if (!checked)
        return;

    start_ = Section::kProduct;

    if (!lock_file_) {
        MainWindowUtils::Message(QMessageBox::Information, tr("File Required"), tr("Please open the file first."), kThreeThousand);
        return;
    }

    MainWindowUtils::SwitchDialog(dialog_list_, false);
    MainWindowUtils::SwitchDialog(dialog_hash_, false);
    UpdateLastTab();

    tree_widget_ = product_tree_;
    table_hash_ = &product_table_hash_;
    dialog_list_ = &product_dialog_list_;
    dialog_hash_ = &product_dialog_hash_;
    settings_ = &product_settings_;
    data_ = &product_data_;

    SwitchSection(data_->tab);
}

void MainWindow::on_rBtnPurchase_toggled(bool checked)
{
    if (!checked)
        return;

    start_ = Section::kPurchase;

    if (!lock_file_) {
        MainWindowUtils::Message(QMessageBox::Information, tr("File Required"), tr("Please open the file first."), kThreeThousand);
        return;
    }

    MainWindowUtils::SwitchDialog(dialog_list_, false);
    MainWindowUtils::SwitchDialog(dialog_hash_, false);
    UpdateLastTab();

    tree_widget_ = purchase_tree_;
    table_hash_ = &purchase_table_hash_;
    dialog_list_ = &purchase_dialog_list_;
    dialog_hash_ = &purchase_dialog_hash_;
    settings_ = &purchase_settings_;
    data_ = &purchase_data_;

    SwitchSection(data_->tab);
}

void MainWindow::on_tabWidget_currentChanged(int /*index*/)
{
    auto* widget { ui->tabWidget->currentWidget() };
    if (!widget)
        return;

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
    if (auto* edit_node_order = dynamic_cast<EditNodeOrder*>(active_window)) {
        AppendTrans(edit_node_order);
        return;
    }

    auto* widget { ui->tabWidget->currentWidget() };
    if (!widget)
        return;

    if (auto* table_widget = dynamic_cast<TableWidget*>(widget)) {
        AppendTrans(table_widget);
    }
}

void MainWindow::on_actionExportStructure_triggered()
{
    CString& source { SqlConnection::Instance().DatabaseName() };
    if (source.isEmpty())
        return;

    QString destination { QFileDialog::getSaveFileName(this, tr("Export Structure"), QDir::homePath(), "*.ytx") };
    if (!MainWindowUtils::NewFile(sql_, destination))
        return;

    auto future = QtConcurrent::run([source, destination]() {
        try {
            QStringList tables { kFinance, kStakeholder, kTask, kProduct };
            QStringList columns { kName, kRule, kType, kUnit, kRemoved };
            MainWindowUtils::ExportColumns(source, destination, tables, columns);

            tables = { kFinancePath, kStakeholderPath, kTaskPath, kProductPath };
            columns = { kAncestor, kDescendant, kDistance };
            MainWindowUtils::ExportColumns(source, destination, tables, columns);

            return true;
        } catch (const std::exception& e) {
            qWarning() << "Export failed:" << e.what();
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

void MainWindow::on_actionExportXlsx_triggered()
{
    QString destination { QFileDialog::getSaveFileName(this, tr("Export Section"), QDir::homePath(), "*.xlsx") };

    if (!destination.endsWith(".xlsx", Qt::CaseInsensitive))
        destination += ".xlsx";

    if (QFile::exists(destination)) {
        qDebug() << "Destination file already exists. Overwriting:" << destination;
        QFile::remove(destination);
    }

    yxlsx::Document d(destination, this);
    auto book1 { d.GetWorkbook() };
    book1->GetCurrentWorksheet()->Write(1, 1, "Hello YTX!");
    d.Save();
}
