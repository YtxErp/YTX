#include "mainwindow.h"

#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QMessageBox>
#include <QMimeData>
#include <QNetworkReply>
#include <QQueue>
#include <QResource>
#include <QScrollBar>
#include <QtConcurrent>

#include "component/arg/insertnodeargfpts.h"
#include "component/arg/nodemodelarg.h"
#include "component/constvalue.h"
#include "component/enumclass.h"
#include "component/signalblocker.h"
#include "component/stringinitializer.h"
#include "database/sqlite/sqlitef.h"
#include "database/sqlite/sqliteo.h"
#include "database/sqlite/sqlitep.h"
#include "database/sqlite/sqlites.h"
#include "database/sqlite/sqlitet.h"
#include "database/sqliteytx.h"
#include "delegate/boolmap.h"
#include "delegate/checkbox.h"
#include "delegate/document.h"
#include "delegate/doublespin.h"
#include "delegate/line.h"
#include "delegate/readonly/colorr.h"
#include "delegate/readonly/datetimer.h"
#include "delegate/readonly/doublespinr.h"
#include "delegate/readonly/doublespinrnonezero.h"
#include "delegate/readonly/doublespinunitr.h"
#include "delegate/readonly/doublespinunitrps.h"
#include "delegate/readonly/nodenamer.h"
#include "delegate/readonly/nodepathr.h"
#include "delegate/readonly/sectionr.h"
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
#include "dialog/licence.h"
#include "dialog/preferences.h"
#include "dialog/removenode.h"
#include "document.h"
#include "global/databasemanager.h"
#include "global/leafsstation.h"
#include "global/resourcepool.h"
#include "global/supportsstation.h"
#include "mainwindowutils.h"
#include "report/model/settlementmodel.h"
#include "report/model/statementmodel.h"
#include "report/model/statementprimarymodel.h"
#include "report/model/statementsecondarymodel.h"
#include "report/model/transrefmodel.h"
#include "report/widget/refwidget.h"
#include "report/widget/statementwidget.h"
#include "search/dialog/search.h"
#include "support/widget/supportwidgetfpts.h"
#include "table/model/transmodelf.h"
#include "table/model/transmodelp.h"
#include "table/model/transmodels.h"
#include "table/model/transmodelt.h"
#include "table/widget/transwidgetfpts.h"
#include "tree/model/nodemodelf.h"
#include "tree/model/nodemodelo.h"
#include "tree/model/nodemodelp.h"
#include "tree/model/nodemodels.h"
#include "tree/model/nodemodelt.h"
#include "tree/widget/nodewidgetf.h"
#include "tree/widget/nodewidgeto.h"
#include "tree/widget/nodewidgetpt.h"
#include "tree/widget/nodewidgets.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    QResource::registerResource(MainWindowUtils::ResourceFile());
    ReadAppSettings();

    ui->setupUi(this);
    SignalBlocker blocker(this);

    SetTabWidget();
    IniSectionGroup();
    StringInitializer::SetHeader(finance_data_.info, product_data_.info, stakeholder_data_.info, task_data_.info, sales_data_.info, purchase_data_.info);
    SetAction();
    SetConnect();

    this->setAcceptDrops(true);

    MainWindowUtils::ReadSettings(ui->splitter, &QSplitter::restoreState, app_settings_sync_, kSplitter, kState);
    MainWindowUtils::ReadSettings(this, &QMainWindow::restoreState, app_settings_sync_, kMainwindow, kState, 0);
    MainWindowUtils::ReadSettings(this, &QMainWindow::restoreGeometry, app_settings_sync_, kMainwindow, kGeometry);

    RestoreRecentFile();
    VerifyActivation();
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
    MainWindowUtils::WriteSettings(ui->splitter, &QSplitter::saveState, app_settings_sync_, kSplitter, kState);
    MainWindowUtils::WriteSettings(this, &QMainWindow::saveState, app_settings_sync_, kMainwindow, kState, 0);
    MainWindowUtils::WriteSettings(this, &QMainWindow::saveGeometry, app_settings_sync_, kMainwindow, kGeometry);
    MainWindowUtils::WriteSettings(app_settings_sync_, std::to_underlying(start_), kStart, kSection);

    if (lock_file_) {
        MainWindowUtils::WriteSettings(file_settings_sync_, MainWindowUtils::SaveTab(finance_trans_wgt_hash_), kFinance, kTabID);
        MainWindowUtils::WriteSettings(finance_tree_->View()->header(), &QHeaderView::saveState, file_settings_sync_, kFinance, kHeaderState);

        MainWindowUtils::WriteSettings(file_settings_sync_, MainWindowUtils::SaveTab(product_trans_wgt_hash_), kProduct, kTabID);
        MainWindowUtils::WriteSettings(product_tree_->View()->header(), &QHeaderView::saveState, file_settings_sync_, kProduct, kHeaderState);

        MainWindowUtils::WriteSettings(file_settings_sync_, MainWindowUtils::SaveTab(stakeholder_trans_wgt_hash_), kStakeholder, kTabID);
        MainWindowUtils::WriteSettings(stakeholder_tree_->View()->header(), &QHeaderView::saveState, file_settings_sync_, kStakeholder, kHeaderState);

        MainWindowUtils::WriteSettings(file_settings_sync_, MainWindowUtils::SaveTab(task_trans_wgt_hash_), kTask, kTabID);
        MainWindowUtils::WriteSettings(task_tree_->View()->header(), &QHeaderView::saveState, file_settings_sync_, kTask, kHeaderState);

        MainWindowUtils::WriteSettings(sales_tree_->View()->header(), &QHeaderView::saveState, file_settings_sync_, kSales, kHeaderState);
        MainWindowUtils::WriteSettings(purchase_tree_->View()->header(), &QHeaderView::saveState, file_settings_sync_, kPurchase, kHeaderState);
    }

    delete ui;
}

bool MainWindow::ROpenFile(CString& file_path)
{
    if (!is_activated_) {
        QMessageBox::critical(this, tr("Activation Required"), tr("The software is not activated. Please activate it first."));
        return false;
    }

    if (!MainWindowUtils::CheckFileValid(file_path))
        return false;

    if (lock_file_) {
        QProcess::startDetached(qApp->applicationFilePath(), QStringList { file_path });
        return false;
    }

    const QFileInfo file_info(file_path);
    if (!LockFile(file_info))
        return false;

    DatabaseManager::Instance().SetDatabaseName(file_path);

    const auto& complete_base_name { file_info.completeBaseName() };

    this->setWindowTitle(complete_base_name);
    ReadFileSettings(complete_base_name);

    SetFinanceData();
    SetTaskData();
    SetProductData();
    SetStakeholderData();
    SetSalesData();
    SetPurchaseData();

    CreateSection(finance_tree_, finance_trans_wgt_hash_, finance_data_, finance_section_settings_, tr("Finance"));
    CreateSection(stakeholder_tree_, stakeholder_trans_wgt_hash_, stakeholder_data_, stakeholder_section_settings_, tr("Stakeholder"));
    CreateSection(product_tree_, product_trans_wgt_hash_, product_data_, product_section_settings_, tr("Product"));
    CreateSection(task_tree_, task_trans_wgt_hash_, task_data_, task_section_settings_, tr("Task"));
    CreateSection(sales_tree_, sales_trans_wgt_hash_, sales_data_, sales_section_settings_, tr("Sales"));
    CreateSection(purchase_tree_, purchase_trans_wgt_hash_, purchase_data_, purchase_section_settings_, tr("Purchase"));

    RSectionGroup(static_cast<int>(start_));

    EnableAction(true);
    on_tabWidget_currentChanged(0);

    QTimer::singleShot(0, this, [this, file_path]() {
        AddRecentFile(file_path);
        MainWindowUtils::ReadPrintTmplate(print_template_);
    });

    return true;
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        return event->acceptProposedAction();
    }

    event->ignore();
}

void MainWindow::dropEvent(QDropEvent* event) { ROpenFile(event->mimeData()->urls().first().toLocalFile()); }

void MainWindow::on_actionInsertNode_triggered()
{
    assert(node_widget_ && "node_widget_ must be non-null");

    auto current_index { node_widget_->View()->currentIndex() };
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
    assert(node_id >= 1 && "node_id must be greater than 0");

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
    assert(type == kTypeLeaf && "type must be kTypeLeaf");

    if (trans_wgt_hash_->contains(node_id))
        return;

    if (start_ == Section::kSales || start_ == Section::kPurchase) {
        CreateLeafO(node_widget_->Model(), trans_wgt_hash_, data_, section_settings_, node_id);
    } else {
        CreateLeafFPTS(node_widget_->Model(), trans_wgt_hash_, data_, section_settings_, node_id);
    }
}

void MainWindow::CreateSupportFunction(int type, int node_id)
{
    assert(type == kTypeSupport && "type must be kTypeSupport");
    assert(start_ != Section::kSales && start_ != Section::kPurchase && "start_ must not be kSales or kPurchase");

    if (sup_wgt_hash_->contains(node_id))
        return;

    CreateSupport(node_widget_->Model(), sup_wgt_hash_, data_, section_settings_, node_id);
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
    UpdateLastTab();

    switch (kSection) {
    case Section::kFinance:
        node_widget_ = finance_tree_;
        trans_wgt_hash_ = &finance_trans_wgt_hash_;
        dialog_list_ = &finance_dialog_list_;
        section_settings_ = &finance_section_settings_;
        data_ = &finance_data_;
        sup_wgt_hash_ = &finance_sup_wgt_hash_;
        rpt_wgt_hash_ = nullptr;
        break;
    case Section::kProduct:
        node_widget_ = product_tree_;
        trans_wgt_hash_ = &product_trans_wgt_hash_;
        dialog_list_ = &product_dialog_list_;
        section_settings_ = &product_section_settings_;
        data_ = &product_data_;
        sup_wgt_hash_ = &product_sup_wgt_hash_;
        rpt_wgt_hash_ = &product_rpt_wgt_hash_;
        break;
    case Section::kTask:
        node_widget_ = task_tree_;
        trans_wgt_hash_ = &task_trans_wgt_hash_;
        dialog_list_ = &task_dialog_list_;
        section_settings_ = &task_section_settings_;
        data_ = &task_data_;
        sup_wgt_hash_ = &task_sup_wgt_hash_;
        rpt_wgt_hash_ = nullptr;
        break;
    case Section::kStakeholder:
        node_widget_ = stakeholder_tree_;
        trans_wgt_hash_ = &stakeholder_trans_wgt_hash_;
        dialog_list_ = &stakeholder_dialog_list_;
        section_settings_ = &stakeholder_section_settings_;
        data_ = &stakeholder_data_;
        sup_wgt_hash_ = &stakeholder_sup_wgt_hash_;
        rpt_wgt_hash_ = &stakeholder_rpt_wgt_hash_;
        break;
    case Section::kSales:
        node_widget_ = sales_tree_;
        trans_wgt_hash_ = &sales_trans_wgt_hash_;
        dialog_list_ = &sales_dialog_list_;
        section_settings_ = &sales_section_settings_;
        data_ = &sales_data_;
        sup_wgt_hash_ = nullptr;
        rpt_wgt_hash_ = &sales_rpt_wgt_hash_;
        break;
    case Section::kPurchase:
        node_widget_ = purchase_tree_;
        trans_wgt_hash_ = &purchase_trans_wgt_hash_;
        dialog_list_ = &purchase_dialog_list_;
        section_settings_ = &purchase_section_settings_;
        data_ = &purchase_data_;
        sup_wgt_hash_ = nullptr;
        rpt_wgt_hash_ = &purchase_rpt_wgt_hash_;
        break;
    default:
        break;
    }

    SwitchSection(data_->tab);
}

void MainWindow::RTransRefDoubleClicked(const QModelIndex& index)
{
    const int node_id { index.siblingAtColumn(std::to_underlying(TransRefEnum::kOrderNode)).data().toInt() };
    const int kColumn { std::to_underlying(TransRefEnum::kGrossAmount) };

    assert(node_id >= 1 && "node_id must be greater than 0");

    if (index.column() != kColumn)
        return;

    const Section section { index.siblingAtColumn(std::to_underlying(TransRefEnum::kSection)).data().toInt() };
    OrderNodeLocation(section, node_id);
}

void MainWindow::RStatementPrimary(int party_id, int unit, const QDateTime& start, const QDateTime& end)
{
    auto* sql { data_->sql };
    const auto& info { data_->info };

    auto* model { new StatementPrimaryModel(sql, info, party_id, this) };
    auto* widget { new StatementWidget(model, unit, false, start, end, this) };

    const QString name { tr("StatementPrimary-") + stakeholder_tree_->Model()->Name(party_id) };
    const int tab_index { ui->tabWidget->addTab(widget, name) };
    auto* tab_bar { ui->tabWidget->tabBar() };

    tab_bar->setTabData(tab_index, QVariant::fromValue(Tab { start_, report_id_ }));
    tab_bar->setTabToolTip(tab_index, name);

    auto view { widget->View() };
    SetStatementView(view, std::to_underlying(StatementPrimaryEnum::kDescription));
    DelegateStatementPrimary(view, section_settings_);

    connect(widget, &StatementWidget::SResetModel, model, &StatementPrimaryModel::RResetModel);

    RegisterRptWgt(widget);
}

void MainWindow::RStatementSecondary(int party_id, int unit, const QDateTime& start, const QDateTime& end)
{
    auto* sql { data_->sql };
    const auto& info { data_->info };

    auto* model { new StatementSecondaryModel(
        sql, info, party_id, product_tree_->Model()->LeafPath(), stakeholder_tree_->Model(), file_settings_.company_name, this) };
    auto* widget { new StatementWidget(model, unit, true, start, end, this) };

    const QString name { tr("StatementSecondary-") + stakeholder_tree_->Model()->Name(party_id) };
    const int tab_index { ui->tabWidget->addTab(widget, name) };
    auto* tab_bar { ui->tabWidget->tabBar() };

    tab_bar->setTabData(tab_index, QVariant::fromValue(Tab { start_, report_id_ }));
    tab_bar->setTabToolTip(tab_index, name);

    auto view { widget->View() };
    SetStatementView(view, std::to_underlying(StatementSecondaryEnum::kDescription));
    DelegateStatementSecondary(view, section_settings_);

    connect(widget, &StatementWidget::SResetModel, model, &StatementSecondaryModel::RResetModel);
    connect(widget, &StatementWidget::SExport, model, &StatementSecondaryModel::RExport);

    RegisterRptWgt(widget);
}

void MainWindow::REnableAction(bool finished)
{
    ui->actionAppendTrans->setEnabled(!finished);
    ui->actionRemove->setEnabled(!finished);
}

void MainWindow::SwitchToLeaf(int node_id, int trans_id) const
{
    auto widget { trans_wgt_hash_->value(node_id, nullptr) };
    assert(widget && "widget must be non-null");

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
    node_widget_->Model()->ReadNode(node_id);

    if (!trans_wgt_hash_->contains(node_id)) {
        CreateLeafO(node_widget_->Model(), trans_wgt_hash_, data_, section_settings_, node_id);
    }
}

void MainWindow::CreateLeafFPTS(PNodeModel tree_model, TransWgtHash* trans_wgt_hash, CData* data, CSectionSettings* settings, int node_id)
{
    assert(tree_model && "tree_model must be non-null");
    assert(trans_wgt_hash && "trans_wgt_hash must be non-null");
    assert(data && "data must be non-null");
    assert(settings && "settings must be non-null");
    assert(tree_model->Contains(node_id) && "node_id must exist in tree_model");

    CString name { tree_model->Name(node_id) };
    auto* sql { data->sql };
    const Info& info { data->info };
    const Section section { info.section };
    const bool rule { tree_model->Rule(node_id) };

    TransModel* model {};
    TransModelArg arg { sql, info, node_id, rule };

    switch (section) {
    case Section::kFinance:
        model = new TransModelF(arg, this);
        break;
    case Section::kProduct:
        model = new TransModelP(arg, this);
        break;
    case Section::kTask:
        model = new TransModelT(arg, this);
        break;
    case Section::kStakeholder:
        model = new TransModelS(arg, this);
        break;
    default:
        break;
    }

    TransWidgetFPTS* widget { new TransWidgetFPTS(model, this) };

    const int tab_index { ui->tabWidget->addTab(widget, name) };
    auto* tab_bar { ui->tabWidget->tabBar() };

    tab_bar->setTabData(tab_index, QVariant::fromValue(Tab { section, node_id }));
    tab_bar->setTabToolTip(tab_index, tree_model->Path(node_id));

    auto view { widget->View() };

    SetTableView(view, std::to_underlying(TransEnum::kDescription));
    TableDelegateFPTS(view, tree_model, settings);

    switch (section) {
    case Section::kFinance:
    case Section::kProduct:
    case Section::kTask:
        TableDelegateFPT(view, tree_model, settings, node_id);
        TableConnectFPT(view, model, tree_model);
        break;
    case Section::kStakeholder:
        TableDelegateS(view);
        TableConnectS(view, model, tree_model);
        break;
    default:
        break;
    }

    trans_wgt_hash->insert(node_id, widget);
    LeafSStation::Instance().RegisterModel(section, node_id, model);
}

void MainWindow::CreateSupport(PNodeModel tree_model, SupWgtHash* sup_wgt_hash, CData* data, CSectionSettings* settings, int node_id)
{
    assert(tree_model && "tree_model must be non-null");
    assert(sup_wgt_hash && "sup_wgt_hash must be non-null");
    assert(data && "data must be non-null");
    assert(settings && "settings must be non-null");
    assert(tree_model->Contains(node_id) && "node_id must exist in tree_model");

    CString name { tree_model->Name(node_id) };
    auto* sql { data->sql };
    const Info& info { data->info };
    const Section section { info.section };

    auto* model { new SupportModel(sql, node_id, info, this) };
    auto* widget { new SupportWidgetFPTS(model, this) };

    const int tab_index { ui->tabWidget->addTab(widget, name) };
    auto* tab_bar { ui->tabWidget->tabBar() };

    tab_bar->setTabData(tab_index, QVariant::fromValue(Tab { section, node_id }));
    tab_bar->setTabToolTip(tab_index, tree_model->Path(node_id));

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
}

void MainWindow::CreateLeafO(PNodeModel tree_model, TransWgtHash* trans_wgt_hash, CData* data, CSectionSettings* settings, int node_id)
{
    const auto& info { data->info };
    const Section section { info.section };

    assert(section == Section::kSales || section == Section::kPurchase && "section must be kSales or kPurchase");

    Node* node { tree_model->GetNode(node_id) };
    const int party_id { node->party };

    assert(party_id >= 1 && "party_id must be greater than 0");

    auto* sql { data->sql };

    TransModelArg model_arg { sql, info, node_id, node->rule };
    TransModelO* model { new TransModelO(model_arg, node, product_tree_->Model(), stakeholder_data_.sql, this) };

    auto print_manager = QSharedPointer<PrintManager>::create(app_settings_, product_tree_->Model(), stakeholder_tree_->Model());

    auto widget_arg { InsertNodeArgO { node, sql, model, stakeholder_tree_->Model(), section_settings_, section } };
    TransWidgetO* widget { new TransWidgetO(widget_arg, print_template_, print_manager, this) };

    const int tab_index { ui->tabWidget->addTab(widget, stakeholder_tree_->Model()->Name(party_id)) };
    auto* tab_bar { ui->tabWidget->tabBar() };

    tab_bar->setTabData(tab_index, QVariant::fromValue(Tab { section, node_id }));
    tab_bar->setTabToolTip(tab_index, stakeholder_tree_->Model()->Path(party_id));

    auto view { widget->View() };
    SetTableView(view, std::to_underlying(TransEnumO::kDescription));

    TableConnectO(view, model, tree_model, widget);
    TableDelegateO(view, settings);

    trans_wgt_hash->insert(node_id, widget);
}

void MainWindow::TableConnectFPT(PTableView table_view, PTransModel table_model, PNodeModel tree_model) const
{
    connect(table_model, &TransModel::SResizeColumnToContents, table_view, &QTableView::resizeColumnToContents);
    connect(table_model, &TransModel::SSearch, tree_model, &NodeModel::RSearch);

    connect(table_model, &TransModel::SSyncLeafValue, tree_model, &NodeModel::RSyncLeafValue);
    connect(table_model, &TransModel::SSyncDouble, tree_model, &NodeModel::RSyncDouble);

    connect(table_model, &TransModel::SRemoveOneTransL, &LeafSStation::Instance(), &LeafSStation::RRemoveOneTransL);
    connect(table_model, &TransModel::SAppendOneTransL, &LeafSStation::Instance(), &LeafSStation::RAppendOneTransL);
    connect(table_model, &TransModel::SSyncBalance, &LeafSStation::Instance(), &LeafSStation::RSyncBalance);
    connect(table_model, &TransModel::SRemoveOneTransS, &SupportSStation::Instance(), &SupportSStation::RRemoveOneTransS);
    connect(table_model, &TransModel::SAppendOneTransS, &SupportSStation::Instance(), &SupportSStation::RAppendOneTransS);
}

void MainWindow::TableConnectO(PTableView table_view, TransModelO* table_model, PNodeModel tree_model, TransWidgetO* widget) const
{
    connect(table_model, &TransModel::SSearch, tree_model, &NodeModel::RSearch);
    connect(table_model, &TransModel::SResizeColumnToContents, table_view, &QTableView::resizeColumnToContents);

    connect(table_model, &TransModel::SSyncLeafValue, widget, &TransWidgetO::RSyncLeafValue);
    connect(widget, &TransWidgetO::SSyncLeafValue, tree_model, &NodeModel::RSyncLeafValue);

    connect(widget, &TransWidgetO::SSyncInt, table_model, &TransModel::RSyncInt);
    connect(widget, &TransWidgetO::SSyncBoolTrans, table_model, &TransModel::RSyncBoolWD);
    connect(widget, &TransWidgetO::SSyncBoolNode, tree_model, &NodeModel::RSyncBoolWD);

    connect(widget, &TransWidgetO::SSyncInt, this, &MainWindow::RSyncInt);
    connect(widget, &TransWidgetO::SEnableAction, this, &MainWindow::REnableAction);

    connect(tree_model, &NodeModel::SSyncBoolWD, widget, &TransWidgetO::RSyncBoolNode);
    connect(tree_model, &NodeModel::SSyncInt, widget, &TransWidgetO::RSyncInt);
    connect(tree_model, &NodeModel::SSyncString, widget, &TransWidgetO::RSyncString);
}

void MainWindow::TableConnectS(PTableView table_view, PTransModel table_model, PNodeModel tree_model) const
{
    connect(table_model, &TransModel::SResizeColumnToContents, table_view, &QTableView::resizeColumnToContents);
    connect(table_model, &TransModel::SSearch, tree_model, &NodeModel::RSearch);

    connect(table_model, &TransModel::SRemoveOneTransS, &SupportSStation::Instance(), &SupportSStation::RRemoveOneTransS);
    connect(table_model, &TransModel::SAppendOneTransS, &SupportSStation::Instance(), &SupportSStation::RAppendOneTransS);
}

void MainWindow::TableDelegateFPTS(PTableView table_view, PNodeModel tree_model, CSectionSettings* settings) const
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

void MainWindow::TableDelegateFPT(PTableView table_view, PNodeModel tree_model, CSectionSettings* settings, int node_id) const
{
    auto* value { new DoubleSpin(
        settings->common_decimal, -std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), kCoefficient16, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnumF::kDebit), value);
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnumF::kCredit), value);

    auto* subtotal { new DoubleSpinR(settings->common_decimal, kCoefficient16, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnumF::kSubtotal), subtotal);

    auto* filter_model { tree_model->ExcludeLeafModel(node_id) };
    auto* node { new TableCombo(tree_model, filter_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnumF::kRhsNode), node);
}

void MainWindow::TableDelegateS(PTableView table_view) const
{
    auto product_tree_model { product_tree_->Model() };
    auto* filter_model { product_tree_model->ExcludeUnitModel(std::to_underlying(UnitP::kPos)) };

    auto* inside_product { new SpecificUnit(product_tree_model, filter_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransEnumS::kInsideProduct), inside_product);
}

void MainWindow::TableDelegateO(PTableView table_view, CSectionSettings* settings) const
{
    auto* product_tree_model { product_tree_->Model().data() };
    auto* filter_model { product_tree_model->ExcludeUnitModel(std::to_underlying(UnitP::kPos)) };

    auto* inside_product { new SpecificUnit(product_tree_model, filter_model, table_view) };
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

void MainWindow::CreateSection(NodeWidget* node_widget, TransWgtHash& trans_wgt_hash, CData& data, CSectionSettings& settings, CString& name)
{
    const auto& info { data.info };
    auto* tab_widget { ui->tabWidget };

    auto view { node_widget->View() };
    auto model { node_widget->Model() };

    // ReadSettings must before SetTreeView
    MainWindowUtils::ReadSettings(view->header(), &QHeaderView::restoreState, file_settings_sync_, info.node, kHeaderState);

    SetTreeView(view, info);
    SetTreeDelegate(view, info, settings);
    TreeConnect(node_widget, data.sql);

    tab_widget->tabBar()->setTabData(tab_widget->addTab(node_widget, name), QVariant::fromValue(Tab { info.section, 0 }));

    switch (info.section) {
    case Section::kFinance:
    case Section::kTask:
    case Section::kProduct:
        TreeConnectFPT(model, data.sql);
        break;
    case Section::kStakeholder:
        TreeConnectS(model, data.sql);
        break;
    default:
        break;
    }

    RestoreTab(model, trans_wgt_hash, MainWindowUtils::ReadSettings(file_settings_sync_, info.node, kTabID), data, settings);
}

void MainWindow::SetTreeDelegate(PTreeView tree_view, CInfo& info, CSectionSettings& settings) const
{
    TreeDelegate(tree_view, info);

    switch (info.section) {
    case Section::kFinance:
        TreeDelegateF(tree_view, info, settings);
        break;
    case Section::kTask:
        TreeDelegateT(tree_view, settings);
        break;
    case Section::kStakeholder:
        TreeDelegateS(tree_view, settings);
        break;
    case Section::kProduct:
        TreeDelegateP(tree_view, settings);
        break;
    case Section::kSales:
    case Section::kPurchase:
        TreeDelegateO(tree_view, settings);
        break;
    default:
        break;
    }
}

void MainWindow::TreeDelegate(PTreeView tree_view, CInfo& info) const
{
    auto* line { new Line(tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnum::kCode), line);
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnum::kDescription), line);

    auto* plain_text { new TreePlainText(tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnum::kNote), plain_text);

    auto* rule { new BoolMap(info.rule_map, QEvent::MouseButtonDblClick, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnum::kRule), rule);

    auto* unit { new TreeCombo(info.unit_map, info.unit_model, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnum::kUnit), unit);

    auto* type { new TreeCombo(info.type_map, info.type_model, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnum::kType), type);
}

void MainWindow::TreeDelegateF(PTreeView tree_view, CInfo& info, CSectionSettings& settings) const
{
    auto* final_total { new DoubleSpinUnitR(settings.amount_decimal, settings.default_unit, info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumF::kLocalTotal), final_total);

    auto* initial_total { new FinanceForeignR(settings.amount_decimal, settings.default_unit, info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumF::kForeignTotal), initial_total);
}

void MainWindow::TreeDelegateT(PTreeView tree_view, CSectionSettings& settings) const
{
    auto* quantity { new DoubleSpinR(settings.common_decimal, kCoefficient16, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumT::kQuantity), quantity);

    auto* amount { new DoubleSpinUnitR(settings.amount_decimal, finance_section_settings_.default_unit, finance_data_.info.unit_symbol_map, tree_view) };
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

void MainWindow::TreeDelegateP(PTreeView tree_view, CSectionSettings& settings) const
{
    auto* quantity { new DoubleSpinR(settings.common_decimal, kCoefficient16, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kQuantity), quantity);

    auto* amount { new DoubleSpinUnitRPS(settings.amount_decimal, finance_section_settings_.default_unit, finance_data_.info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kAmount), amount);
    connect(amount, &DoubleSpinUnitRPS::STransRef, this, &MainWindow::RTransRef);

    auto* unit_price { new DoubleSpin(settings.amount_decimal, 0, std::numeric_limits<double>::max(), kCoefficient8, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kUnitPrice), unit_price);
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kCommission), unit_price);

    auto* color { new Color(tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kColor), color);
}

void MainWindow::TreeDelegateS(PTreeView tree_view, CSectionSettings& settings) const
{
    auto* amount { new DoubleSpinUnitRPS(settings.amount_decimal, finance_section_settings_.default_unit, finance_data_.info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumS::kAmount), amount);
    connect(amount, &DoubleSpinUnitRPS::STransRef, this, &MainWindow::RTransRef);

    auto* payment_term { new Spin(0, std::numeric_limits<int>::max(), tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumS::kPaymentTerm), payment_term);

    auto* tax_rate { new TaxRate(settings.amount_decimal, 0.0, std::numeric_limits<double>::max(), tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumS::kTaxRate), tax_rate);

    auto* deadline { new TreeDateTime(kDD, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumS::kDeadline), deadline);

    auto stakeholder_tree_model { stakeholder_tree_->Model() };

    auto* filter_model { stakeholder_tree_model->IncludeUnitModel(std::to_underlying(UnitS::kEmp)) };
    auto* employee { new SpecificUnit(stakeholder_tree_model, filter_model, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumS::kEmployee), employee);
}

void MainWindow::TreeDelegateO(PTreeView tree_view, CSectionSettings& settings) const
{
    auto* amount { new DoubleSpinUnitR(settings.amount_decimal, finance_section_settings_.default_unit, finance_data_.info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kGrossAmount), amount);
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kSettlement), amount);
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kDiscount), amount);

    auto* quantity { new DoubleSpinRNoneZero(settings.common_decimal, kCoefficient16, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kSecond), quantity);
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kFirst), quantity);

    auto stakeholder_tree_model { stakeholder_tree_->Model() };

    auto* filter_model { stakeholder_tree_model->IncludeUnitModel(std::to_underlying(UnitS::kEmp)) };
    auto* employee { new SpecificUnit(stakeholder_tree_model, filter_model, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kEmployee), employee);

    auto* name { new OrderNameR(stakeholder_tree_model, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kName), name);

    auto* date_time { new TreeDateTime(settings.date_format, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kDateTime), date_time);

    auto* finished { new CheckBox(QEvent::MouseButtonDblClick, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kFinished), finished);
}

void MainWindow::TreeConnect(NodeWidget* node_widget, const Sqlite* sql) const
{
    auto view { node_widget->View() };
    auto model { node_widget->Model() };

    connect(view, &QTreeView::doubleClicked, this, &MainWindow::RTreeViewDoubleClicked, Qt::UniqueConnection);
    connect(view, &QTreeView::customContextMenuRequested, this, &MainWindow::RTreeViewCustomContextMenuRequested, Qt::UniqueConnection);

    connect(model, &NodeModel::SSyncName, this, &MainWindow::RSyncName, Qt::UniqueConnection);

    connect(model, &NodeModel::SSyncStatusValue, node_widget, &NodeWidget::RSyncStatusValue, Qt::UniqueConnection);

    connect(model, &NodeModel::SResizeColumnToContents, view, &QTreeView::resizeColumnToContents, Qt::UniqueConnection);

    connect(sql, &Sqlite::SRemoveNode, model, &NodeModel::RRemoveNode, Qt::UniqueConnection);
    connect(sql, &Sqlite::SSyncMultiLeafValue, model, &NodeModel::RSyncMultiLeafValue, Qt::UniqueConnection);

    connect(sql, &Sqlite::SFreeWidget, this, &MainWindow::RFreeWidget, Qt::UniqueConnection);
}

void MainWindow::TreeConnectFPT(PNodeModel node_model, const Sqlite* sql) const
{
    connect(sql, &Sqlite::SRemoveMultiTransL, &LeafSStation::Instance(), &LeafSStation::RRemoveMultiTransL, Qt::UniqueConnection);
    connect(sql, &Sqlite::SMoveMultiTransL, &LeafSStation::Instance(), &LeafSStation::RMoveMultiTransL, Qt::UniqueConnection);

    connect(sql, &Sqlite::SRemoveMultiTransS, &SupportSStation::Instance(), &SupportSStation::RRemoveMultiTransS, Qt::UniqueConnection);
    connect(sql, &Sqlite::SMoveMultiTransS, &SupportSStation::Instance(), &SupportSStation::RMoveMultiTransS, Qt::UniqueConnection);

    connect(node_model, &NodeModel::SSyncRule, &LeafSStation::Instance(), &LeafSStation::RSyncRule, Qt::UniqueConnection);
}

void MainWindow::TreeConnectS(PNodeModel node_model, const Sqlite* sql) const
{
    connect(sql, &Sqlite::SSyncStakeholder, node_model, &NodeModel::RSyncStakeholder, Qt::UniqueConnection);
    connect(product_data_.sql, &Sqlite::SSyncProduct, sql, &Sqlite::RSyncProduct, Qt::UniqueConnection);

    connect(sql, &Sqlite::SAppendMultiTrans, &LeafSStation::Instance(), &LeafSStation::RAppendMultiTrans, Qt::UniqueConnection);

    connect(sql, &Sqlite::SRemoveMultiTransS, &SupportSStation::Instance(), &SupportSStation::RRemoveMultiTransS, Qt::UniqueConnection);
    connect(sql, &Sqlite::SMoveMultiTransS, &SupportSStation::Instance(), &SupportSStation::RMoveMultiTransS, Qt::UniqueConnection);
}

void MainWindow::TreeConnectPSO(PNodeModel node_order, const Sqlite* sql_order) const
{
    auto* sqlite_stakeholder { qobject_cast<SqliteS*>(stakeholder_data_.sql) };
    auto* sqlite_order { qobject_cast<const SqliteO*>(sql_order) };

    connect(sqlite_stakeholder, &Sqlite::SSyncStakeholder, sql_order, &Sqlite::RSyncStakeholder, Qt::UniqueConnection);
    connect(product_data_.sql, &Sqlite::SSyncProduct, sql_order, &Sqlite::RSyncProduct, Qt::UniqueConnection);

    connect(node_order, &NodeModel::SSyncDouble, stakeholder_tree_->Model(), &NodeModel::RSyncDouble, Qt::UniqueConnection);
    connect(sqlite_order, &SqliteO::SSyncPrice, sqlite_stakeholder, &SqliteS::RPriceSList, Qt::UniqueConnection);
}

void MainWindow::InsertNodeFunction(const QModelIndex& parent, int parent_id, int row)
{
    auto model { node_widget_->Model() };

    auto* node { ResourcePool<Node>::Instance().Allocate() };
    node->rule = model->Rule(parent_id);
    node->unit = parent_id == -1 ? section_settings_->default_unit : model->Unit(parent_id);
    model->SetParent(node, parent_id);

    if (start_ == Section::kSales || start_ == Section::kPurchase)
        InsertNodeO(node, parent, row);

    if (start_ != Section::kSales && start_ != Section::kPurchase)
        InsertNodeFPTS(node, parent, parent_id, row);
}

void MainWindow::on_actionRemove_triggered()
{
    auto* active_window { QApplication::activeWindow() };
    if (auto* edit_node_order = dynamic_cast<InsertNodeOrder*>(active_window)) {
        MainWindowUtils::RemoveTrans(edit_node_order);
        return;
    }

    auto* widget { ui->tabWidget->currentWidget() };

    assert(widget && "widget must be non-null");

    if (auto* node_widget { dynamic_cast<NodeWidget*>(widget) }) {
        RemoveNode(node_widget);
    }

    if (auto* trans_widget { dynamic_cast<TransWidget*>(widget) }) {
        MainWindowUtils::RemoveTrans(trans_widget);
    }
}

void MainWindow::RemoveNode(NodeWidget* node_widget)
{
    auto view { node_widget->View() };
    assert(view && "view must be non-null");

    if (!MainWindowUtils::HasSelection(view))
        return;

    const auto index { view->currentIndex() };
    if (!index.isValid())
        return;

    auto model { node_widget->Model() };
    assert(model && "model must be non-null");

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

void MainWindow::RemoveNonBranch(PNodeModel tree_model, const QModelIndex& index, int node_id, int node_type)
{
    tree_model->removeRows(index.row(), 1, index.parent());
    data_->sql->RemoveNode(node_id, node_type);

    RFreeWidget(node_id, node_type);
}

void MainWindow::RestoreTab(PNodeModel tree_model, TransWgtHash& trans_wgt_hash, CIntSet& set, CData& data, CSectionSettings& section_settings)
{
    assert(tree_model && "tree_model must be non-null");

    if (set.isEmpty() || data.info.section == Section::kSales || data.info.section == Section::kPurchase)
        return;

    for (int node_id : set) {
        if (tree_model->Contains(node_id) && tree_model->Type(node_id) == kTypeLeaf)
            CreateLeafFPTS(tree_model, &trans_wgt_hash, &data, &section_settings, node_id);
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
    ui->actionSettlement->setEnabled(enable);
}

QStandardItemModel* MainWindow::CreateModelFromMap(CStringMap& map, QObject* parent)
{
    auto* model { new QStandardItemModel(parent) };

    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        auto* item { new QStandardItem(it.value()) };
        item->setData(it.key(), Qt::UserRole);
        model->appendRow(item);
    }

    model->sort(0);
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
    recent_file_ = app_settings_sync_->value(kRecentFile).toStringList();

    auto* recent_menu { ui->menuRecent };
    QStringList valid_recent_file {};

    const long long count { std::min(kMaxRecentFile, recent_file_.size()) };
    const auto mid_file { recent_file_.mid(recent_file_.size() - count, count) };

    for (auto it = mid_file.rbegin(); it != mid_file.rend(); ++it) {
        CString& file_path { *it };

        if (!MainWindowUtils::CheckFileValid(file_path))
            continue;

        auto* action { recent_menu->addAction(file_path) };
        connect(action, &QAction::triggered, this, [file_path, this]() { ROpenFile(file_path); });
        valid_recent_file.prepend(file_path);
    }

    if (recent_file_ != valid_recent_file) {
        recent_file_ = valid_recent_file;
        MainWindowUtils::WriteSettings(app_settings_sync_, recent_file_, kRecent, kFile);
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
        MainWindowUtils::WriteSettings(app_settings_sync_, recent_file_, kRecent, kFile);
    }
}

bool MainWindow::LockFile(const QFileInfo& file_info)
{
    CString lock_file_path { file_info.absolutePath() + QDir::separator() + file_info.completeBaseName() + kDotSuffixLOCK };

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

void MainWindow::RemoveBranch(PNodeModel tree_model, const QModelIndex& index, int node_id)
{
    QMessageBox msg {};
    msg.setIcon(QMessageBox::Question);
    msg.setText(tr("Remove %1").arg(tree_model->Path(node_id)));
    msg.setInformativeText(tr("The branch will be removed, and its direct children will be promoted to the same level."));
    msg.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);

    if (msg.exec() == QMessageBox::Ok) {
        tree_model->removeRows(index.row(), 1, index.parent());
        data_->sql->RemoveNode(node_id, kTypeBranch);
    }
}

void MainWindow::on_tabWidget_tabCloseRequested(int index)
{
    if (index == 0)
        return;

    const int node_id { ui->tabWidget->tabBar()->tabData(index).value<Tab>().node_id };

    if (start_ != Section::kFinance && start_ != Section::kTask)
        MainWindowUtils::FreeWidgetFromHash(node_id, rpt_wgt_hash_);

    const auto node_type { node_widget_->Model()->Type(node_id) };
    RFreeWidget(node_id, node_type);
}

void MainWindow::RFreeWidget(int node_id, int node_type)
{
    switch (node_type) {
    case kTypeLeaf:
        MainWindowUtils::FreeWidgetFromHash(node_id, trans_wgt_hash_);
        LeafSStation::Instance().DeregisterModel(start_, node_id);
        break;
    case kTypeSupport:
        MainWindowUtils::FreeWidgetFromHash(node_id, sup_wgt_hash_);
        SupportSStation::Instance().DeregisterModel(start_, node_id);
        break;
    default:
        return; // No further action if it's not a known type
    }
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

    start_ = Section(app_settings_sync_->value(kStartSection, 0).toInt());

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

void MainWindow::DelegateSupport(PTableView table_view, PNodeModel tree_model, CSectionSettings* settings) const
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

void MainWindow::on_actionStatement_triggered()
{
    assert(start_ == Section::kSales || start_ == Section::kPurchase && "start_ must be kSales or kPurchase");

    auto* sql { data_->sql };
    const auto& info { data_->info };

    auto* model { new StatementModel(sql, info, this) };

    const int unit { std::to_underlying(UnitO::kMS) };
    const auto start { QDateTime(QDate(QDate::currentDate().year(), QDate::currentDate().month(), 1), kStartTime) };
    const auto end { QDateTime(QDate(QDate::currentDate().year(), QDate::currentDate().month(), QDate::currentDate().daysInMonth()), kEndTime) };

    auto* widget { new StatementWidget(model, unit, false, start, end, this) };

    const int tab_index { ui->tabWidget->addTab(widget, tr("Statement")) };
    auto* tab_bar { ui->tabWidget->tabBar() };

    tab_bar->setTabData(tab_index, QVariant::fromValue(Tab { start_, report_id_ }));

    auto view { widget->View() };
    SetStatementView(view, std::to_underlying(StatementEnum::kPlaceholder));
    DelegateStatement(view, section_settings_);

    connect(widget, &StatementWidget::SStatementPrimary, this, &MainWindow::RStatementPrimary);
    connect(widget, &StatementWidget::SStatementSecondary, this, &MainWindow::RStatementSecondary);
    connect(widget, &StatementWidget::SResetModel, model, &StatementModel::RResetModel);

    RegisterRptWgt(widget);
}

void MainWindow::on_actionSettlement_triggered()
{
    assert(start_ == Section::kSales || start_ == Section::kPurchase && "start_ must be kSales or kPurchase");

    if (settlement_widget_) {
        ui->tabWidget->setCurrentWidget(settlement_widget_);
        settlement_widget_->activateWindow();
        return;
    }

    auto* sql { data_->sql };
    const auto& info { data_->info };

    const auto start { QDateTime(QDate(QDate::currentDate().year() - 1, 1, 1), kStartTime) };
    const auto end { QDateTime(QDate(QDate::currentDate().year(), 12, 31), kEndTime) };

    auto* model { new SettlementModel(sql, info, this) };
    model->ResetModel(start, end);

    auto* primary_model { new SettlementPrimaryModel(sql, info, this) };

    settlement_widget_ = new SettlementWidget(model, primary_model, start, end, this);

    const int tab_index { ui->tabWidget->addTab(settlement_widget_, tr("Settlement")) };
    auto* tab_bar { ui->tabWidget->tabBar() };

    tab_bar->setTabData(tab_index, QVariant::fromValue(Tab { start_, report_id_ }));

    auto view { settlement_widget_->View() };
    auto primary_view { settlement_widget_->PrimaryView() };
    SetStatementView(view, std::to_underlying(SettlementEnum::kDescription));
    SetStatementView(primary_view, std::to_underlying(SettlementEnum::kDescription));

    view->setColumnHidden(std::to_underlying(SettlementEnum::kID), false);
    primary_view->setColumnHidden(std::to_underlying(SettlementEnum::kID), false);

    DelegateSettlement(view, section_settings_);
    DelegateSettlementPrimary(primary_view, section_settings_);

    connect(model, &SettlementModel::SResetModel, primary_model, &SettlementPrimaryModel::RResetModel);
    connect(model, &SettlementModel::SSyncFinished, primary_model, &SettlementPrimaryModel::RSyncFinished);
    connect(model, &SettlementModel::SResizeColumnToContents, view, &QTableView::resizeColumnToContents);

    connect(primary_model, &SettlementPrimaryModel::SSyncDouble, model, &SettlementModel::RSyncDouble);
    connect(model, &SettlementModel::SSyncDouble, stakeholder_tree_->Model(), &NodeModel::RSyncDouble);
    connect(settlement_widget_, &SettlementWidget::SNodeLocation, this, &MainWindow::RNodeLocation);

    RegisterRptWgt(settlement_widget_);
}

void MainWindow::CreateTransRef(PNodeModel tree_model, CData* data, int node_id, int unit)
{
    assert(tree_model && "tree_model must be non-null");
    assert(tree_model->Contains(node_id) && "node_id must exist in tree_model");

    CString name { tr("Record-") + tree_model->Name(node_id) };
    auto* sql { data->sql };
    const Info& info { data->info };
    const Section section { info.section };

    auto* model { new TransRefModel(sql, info, unit, this) };

    const auto start { QDateTime(QDate(QDate::currentDate().year() - 1, 1, 1), kStartTime) };
    const auto end { QDateTime(QDate(QDate::currentDate().year(), 12, 31), kEndTime) };
    auto* widget { new RefWidget(model, node_id, start, end, this) };

    const int tab_index { ui->tabWidget->addTab(widget, name) };
    auto* tab_bar { ui->tabWidget->tabBar() };

    tab_bar->setTabData(tab_index, QVariant::fromValue(Tab { section, report_id_ }));

    auto view { widget->View() };
    SetTableView(view, std::to_underlying(TransRefEnum::kDescription));
    DelegateTransRef(view, &sales_section_settings_);

    connect(view, &QTableView::doubleClicked, this, &MainWindow::RTransRefDoubleClicked);
    connect(widget, &RefWidget::SResetModel, model, &TransRefModel::RResetModel);

    RegisterRptWgt(widget);
}

void MainWindow::RegisterRptWgt(ReportWidget* widget)
{
    rpt_wgt_hash_->insert(report_id_, widget);

    ui->tabWidget->setCurrentWidget(widget);
    widget->activateWindow();

    --report_id_;
}

void MainWindow::DelegateTransRef(PTableView table_view, CSectionSettings* settings) const
{
    auto* price { new DoubleSpinRNoneZero(settings->amount_decimal, kCoefficient16, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransRefEnum::kUnitPrice), price);
    table_view->setItemDelegateForColumn(std::to_underlying(TransRefEnum::kDiscountPrice), price);

    auto* quantity { new DoubleSpinRNoneZero(settings->common_decimal, kCoefficient16, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransRefEnum::kFirst), quantity);
    table_view->setItemDelegateForColumn(std::to_underlying(TransRefEnum::kSecond), quantity);

    auto* amount { new DoubleSpinRNoneZero(settings->amount_decimal, kCoefficient16, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransRefEnum::kGrossAmount), amount);

    auto* date_time { new DateTimeR(sales_section_settings_.date_format, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransRefEnum::kDateTime), date_time);

    auto stakeholder_tree_model { stakeholder_tree_->Model() };
    auto* outside_product { new NodePathR(stakeholder_tree_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransRefEnum::kOutsideProduct), outside_product);

    auto* section { new SectionR(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransRefEnum::kSection), section);

    if (start_ == Section::kProduct) {
        auto* name { new NodeNameR(stakeholder_tree_model, table_view) };
        table_view->setItemDelegateForColumn(std::to_underlying(TransRefEnum::kPP), name);
    }

    if (start_ == Section::kStakeholder) {
        auto* inside_product { new NodeNameR(product_tree_->Model(), table_view) };
        table_view->setItemDelegateForColumn(std::to_underlying(TransRefEnum::kPP), inside_product);
    }
}

void MainWindow::DelegateSupportS(PTableView table_view, PNodeModel tree_model, PNodeModel product_tree_model) const
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
    table_view->setColumnHidden(std::to_underlying(TransSearchEnum::kRhsRatio), true);
}

void MainWindow::SetStatementView(PTableView view, int stretch_column) const
{
    view->setSortingEnabled(true);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setAlternatingRowColors(true);

    auto* h_header { view->horizontalHeader() };
    ResizeColumn(h_header, stretch_column);

    auto* v_header { view->verticalHeader() };
    v_header->setDefaultSectionSize(kRowHeight);
    v_header->setSectionResizeMode(QHeaderView::Fixed);
    v_header->setHidden(true);
}

void MainWindow::DelegateStatement(PTableView table_view, CSectionSettings* settings) const
{
    auto* quantity { new DoubleSpinRNoneZero(settings->common_decimal, kCoefficient16, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(StatementEnum::kCFirst), quantity);
    table_view->setItemDelegateForColumn(std::to_underlying(StatementEnum::kCSecond), quantity);

    auto* amount { new DoubleSpinRNoneZero(settings->amount_decimal, kCoefficient16, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(StatementEnum::kCGrossAmount), amount);
    table_view->setItemDelegateForColumn(std::to_underlying(StatementEnum::kCSettlement), amount);
    table_view->setItemDelegateForColumn(std::to_underlying(StatementEnum::kPBalance), amount);
    table_view->setItemDelegateForColumn(std::to_underlying(StatementEnum::kCBalance), amount);

    auto* name { new NodeNameR(stakeholder_tree_->Model(), table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(StatementEnum::kParty), name);
}

void MainWindow::DelegateSettlement(PTableView table_view, CSectionSettings* settings) const
{
    auto* amount { new DoubleSpinRNoneZero(settings->amount_decimal, kCoefficient16, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(SettlementEnum::kGrossAmount), amount);

    auto* finished { new CheckBox(QEvent::MouseButtonDblClick, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(SettlementEnum::kFinished), finished);

    auto* line { new Line(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(SettlementEnum::kDescription), line);

    auto* date_time { new TreeDateTime(kDateFirst, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(SettlementEnum::kDateTime), date_time);

    auto model { stakeholder_tree_->Model() };
    const int unit { start_ == Section::kSales ? std::to_underlying(UnitS::kCust) : std::to_underlying(UnitS::kVend) };

    auto* filter_model { model->IncludeUnitModel(unit) };
    auto* node { new TableCombo(model, filter_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(SettlementEnum::kParty), node);
}

void MainWindow::DelegateSettlementPrimary(PTableView table_view, CSectionSettings* settings) const
{
    auto* amount { new DoubleSpinRNoneZero(settings->amount_decimal, kCoefficient16, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(SettlementEnum::kGrossAmount), amount);

    auto* employee { new NodeNameR(stakeholder_tree_->Model(), table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(SettlementEnum::kParty), employee);

    auto* state { new CheckBox(QEvent::MouseButtonRelease, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(SettlementEnum::kFinished), state);

    auto* date_time { new DateTimeR(kDateFirst, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(SettlementEnum::kDateTime), date_time);
}

void MainWindow::DelegateStatementPrimary(PTableView table_view, CSectionSettings* settings) const
{
    auto* quantity { new DoubleSpinRNoneZero(settings->common_decimal, kCoefficient16, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(StatementPrimaryEnum::kFirst), quantity);
    table_view->setItemDelegateForColumn(std::to_underlying(StatementPrimaryEnum::kSecond), quantity);

    auto* amount { new DoubleSpinRNoneZero(settings->amount_decimal, kCoefficient16, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(StatementPrimaryEnum::kGrossAmount), amount);
    table_view->setItemDelegateForColumn(std::to_underlying(StatementPrimaryEnum::kSettlement), amount);

    auto* employee { new NodeNameR(stakeholder_tree_->Model(), table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(StatementPrimaryEnum::kEmployee), employee);

    auto* state { new CheckBox(QEvent::MouseButtonRelease, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(StatementPrimaryEnum::kState), state);

    auto* date_time { new DateTimeR(sales_section_settings_.date_format, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(StatementPrimaryEnum::kDateTime), date_time);
}

void MainWindow::DelegateStatementSecondary(PTableView table_view, CSectionSettings* settings) const
{
    auto* quantity { new DoubleSpinRNoneZero(settings->common_decimal, kCoefficient16, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(StatementSecondaryEnum::kFirst), quantity);
    table_view->setItemDelegateForColumn(std::to_underlying(StatementSecondaryEnum::kSecond), quantity);

    auto* amount { new DoubleSpinRNoneZero(settings->amount_decimal, kCoefficient16, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(StatementSecondaryEnum::kGrossAmount), amount);
    table_view->setItemDelegateForColumn(std::to_underlying(StatementSecondaryEnum::kSettlement), amount);
    table_view->setItemDelegateForColumn(std::to_underlying(StatementSecondaryEnum::kUnitPrice), amount);

    auto* state { new CheckBox(QEvent::MouseButtonRelease, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(StatementSecondaryEnum::kState), state);

    auto* date_time { new DateTimeR(sales_section_settings_.date_format, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(StatementSecondaryEnum::kDateTime), date_time);

    auto* outside_product { new NodePathR(stakeholder_tree_->Model(), table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(StatementSecondaryEnum::kOutsideProduct), outside_product);

    auto product_tree_model { product_tree_->Model() };
    auto* filter_model { product_tree_model->ExcludeUnitModel(std::to_underlying(UnitP::kPos)) };

    auto* inside_product { new SpecificUnit(product_tree_model, filter_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(StatementSecondaryEnum::kInsideProduct), inside_product);
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

    info.unit_model = CreateModelFromMap(info.unit_map, this);
    info.rule_model = CreateModelFromMap(info.rule_map, this);
    info.type_model = CreateModelFromMap(info.type_map, this);

    ReadSectionSettings(finance_section_settings_, kFinance);

    sql = new SqliteF(info, this);

    NodeModelArg arg { sql, info, finance_trans_wgt_hash_, app_settings_.separator, finance_section_settings_.default_unit };
    auto* model { new NodeModelF(arg, this) };

    finance_tree_ = new NodeWidgetF(model, info, finance_section_settings_, this);
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

    info.unit_model = CreateModelFromMap(info.unit_map, this);
    info.rule_model = CreateModelFromMap(info.rule_map, this);
    info.type_model = CreateModelFromMap(info.type_map, this);

    ReadSectionSettings(product_section_settings_, kProduct);

    sql = new SqliteP(info, this);

    NodeModelArg arg { sql, info, product_trans_wgt_hash_, app_settings_.separator, product_section_settings_.default_unit };
    auto* model { new NodeModelP(arg, this) };

    product_tree_ = new NodeWidgetPT(model, product_section_settings_, this);
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

    // EMP: EMPLOYEE, CUST: CUSTOMER, VEND: VENDOR
    QStringList unit_list { tr("CUST"), tr("EMP"), tr("VEND") };
    QStringList type_list { "L", "B", "S" };

    for (int i = 0; i != unit_list.size(); ++i)
        info.unit_map.insert(i, unit_list.at(i));

    for (int i = 0; i != type_list.size(); ++i)
        info.type_map.insert(i, type_list.at(i));

    info.unit_model = CreateModelFromMap(info.unit_map, this);
    info.type_model = CreateModelFromMap(info.type_map, this);

    ReadSectionSettings(stakeholder_section_settings_, kStakeholder);

    sql = new SqliteS(info, this);

    NodeModelArg arg { sql, info, stakeholder_trans_wgt_hash_, app_settings_.separator, stakeholder_section_settings_.default_unit };
    auto* model { new NodeModelS(arg, this) };

    stakeholder_tree_ = new NodeWidgetS(model, this);
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

    info.unit_model = CreateModelFromMap(info.unit_map, this);
    info.rule_model = CreateModelFromMap(info.rule_map, this);
    info.type_model = CreateModelFromMap(info.type_map, this);

    ReadSectionSettings(task_section_settings_, kTask);

    sql = new SqliteT(info, this);

    NodeModelArg arg { sql, info, task_trans_wgt_hash_, app_settings_.separator, task_section_settings_.default_unit };
    auto* model { new NodeModelT(arg, this) };

    task_tree_ = new NodeWidgetPT(model, task_section_settings_, this);
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
    info.settlement = kSalesSettlement;

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

    info.unit_model = CreateModelFromMap(info.unit_map, this);
    info.rule_model = CreateModelFromMap(info.rule_map, this);
    info.type_model = CreateModelFromMap(info.type_map, this);

    ReadSectionSettings(sales_section_settings_, kSales);

    sql = new SqliteO(info, this);

    NodeModelArg arg { sql, info, sales_trans_wgt_hash_, app_settings_.separator, sales_section_settings_.default_unit };
    auto* model { new NodeModelO(arg, this) };

    sales_tree_ = new NodeWidgetO(model, this);

    TreeConnectPSO(model, sql);
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
    info.settlement = kPurchaseSettlement;

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

    info.unit_model = CreateModelFromMap(info.unit_map, this);
    info.rule_model = CreateModelFromMap(info.rule_map, this);
    info.type_model = CreateModelFromMap(info.type_map, this);

    ReadSectionSettings(purchase_section_settings_, kPurchase);

    sql = new SqliteO(info, this);

    NodeModelArg arg { sql, info, purchase_trans_wgt_hash_, app_settings_.separator, purchase_section_settings_.default_unit };
    auto* model { new NodeModelO(arg, this) };

    purchase_tree_ = new NodeWidgetO(model, this);

    TreeConnectPSO(model, sql);
}

void MainWindow::SetAction() const
{
    ui->actionInsertNode->setIcon(QIcon(":/solarized_dark/solarized_dark/insert.png"));
    ui->actionEditNode->setIcon(QIcon(":/solarized_dark/solarized_dark/edit.png"));
    ui->actionRemove->setIcon(QIcon(":/solarized_dark/solarized_dark/remove.png"));
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
    ui->actionStatement->setIcon(QIcon(":/solarized_dark/solarized_dark/statement.png"));
    ui->actionSettlement->setIcon(QIcon(":/solarized_dark/solarized_dark/settle.png"));

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
    tree_view->setDragDropMode(QAbstractItemView::DragDrop);
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
    assert(node_widget_ && "node_widget_ must be non-null");

    auto view { node_widget_->View() };
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
        CreateLeafFPTS(node_widget_->Model(), trans_wgt_hash_, data_, section_settings_, rhs_node_id);

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
    assert(widget && "widget must be non-null");

    auto* model { widget->Model().data() };
    assert(model && "model must be non-null");

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
    assert(widget && "widget must be non-null");

    auto view { widget->View() };
    if (!MainWindowUtils::HasSelection(view))
        return;

    const auto index { view->currentIndex() };
    if (!index.isValid())
        return;

    auto model { widget->Model() };
    assert(model && "model must be non-null");

    const int id { index.siblingAtColumn(std::to_underlying(TransEnum::kSupportID)).data().toInt() };

    if (id <= 0)
        return;

    if (!sup_wgt_hash_->contains(id)) {
        CreateSupport(node_widget_->Model(), sup_wgt_hash_, data_, section_settings_, id);
    }

    const int trans_id { index.siblingAtColumn(std::to_underlying(TransEnum::kID)).data().toInt() };
    SwitchToSupport(id, trans_id);
}

void MainWindow::SupportToLeaf(SupportWidget* widget)
{
    assert(widget && "widget must be non-null");

    auto view { widget->View() };
    if (!MainWindowUtils::HasSelection(view))
        return;

    const auto index { view->currentIndex() };
    if (!index.isValid())
        return;

    auto model { widget->Model() };
    assert(model && "model must be non-null");

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

    assert(node_widget_ && "node_widget_ must be non-null");

    const auto view { node_widget_->View() };
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
    auto model { node_widget_->Model() };

    const auto& parent { index.parent() };
    const int parent_id { parent.isValid() ? parent.siblingAtColumn(std::to_underlying(NodeEnum::kID)).data().toInt() : -1 };
    auto parent_path { model->Path(parent_id) };

    if (!parent_path.isEmpty())
        parent_path += app_settings_.separator;

    CString name { model->Name(node_id) };
    const auto children_name { model->ChildrenName(parent_id) };

    auto* edit_name { new EditNodeName(name, parent_path, children_name, this) };
    connect(edit_name, &QDialog::accepted, this, [=]() { model->UpdateName(node_id, edit_name->GetName()); });
    edit_name->exec();
}

void MainWindow::InsertNodeFPTS(Node* node, const QModelIndex& parent, int parent_id, int row)
{
    auto tree_model { node_widget_->Model() };
    auto* unit_model { data_->info.unit_model };

    auto parent_path { tree_model->Path(parent_id) };
    if (!parent_path.isEmpty())
        parent_path += app_settings_.separator;

    QDialog* dialog {};
    QSortFilterProxyModel* employee_model {};

    const auto children_name { tree_model->ChildrenName(parent_id) };
    const auto arg { InsertNodeArgFPTS { node, unit_model, parent_path, children_name } };

    switch (start_) {
    case Section::kFinance:
        dialog = new InsertNodeFinance(arg, this);
        break;
    case Section::kTask:
        node->date_time = QDateTime::currentDateTime().toString(kDateTimeFST);
        dialog = new InsertNodeTask(arg, section_settings_->amount_decimal, section_settings_->date_format, this);
        break;
    case Section::kStakeholder:
        node->date_time = QDateTime::currentDateTime().toString(kDateTimeFST);
        employee_model = tree_model->IncludeUnitModel(std::to_underlying(UnitS::kEmp));
        dialog = new InsertNodeStakeholder(arg, employee_model, section_settings_->amount_decimal, this);
        break;
    case Section::kProduct:
        dialog = new InsertNodeProduct(arg, section_settings_->common_decimal, this);
        break;
    default:
        return ResourcePool<Node>::Instance().Recycle(node);
    }

    connect(dialog, &QDialog::accepted, this, [=, this]() {
        if (tree_model->InsertNode(row, parent, node)) {
            auto index = tree_model->index(row, 0, parent);
            node_widget_->View()->setCurrentIndex(index);
        }
    });

    connect(dialog, &QDialog::rejected, this, [=]() { ResourcePool<Node>::Instance().Recycle(node); });
    dialog->exec();
}

void MainWindow::InsertNodeO(Node* node, const QModelIndex& parent, int row)
{
    auto tree_model { node_widget_->Model() };
    auto* sql { data_->sql };

    TransModelArg model_arg { sql, data_->info, 0, 0 };
    auto* table_model { new TransModelO(model_arg, node, product_tree_->Model(), stakeholder_data_.sql, this) };

    auto print_manager = QSharedPointer<PrintManager>::create(app_settings_, product_tree_->Model(), stakeholder_tree_->Model());

    auto dialog_arg { InsertNodeArgO { node, sql, table_model, stakeholder_tree_->Model(), section_settings_, start_ } };
    auto* dialog { new InsertNodeOrder(dialog_arg, print_template_, print_manager, this) };

    dialog->setWindowFlags(Qt::Window);

    connect(dialog, &QDialog::accepted, this, [=, this]() {
        if (tree_model->InsertNode(row, parent, node)) {
            auto index = tree_model->index(row, 0, parent);
            node_widget_->View()->setCurrentIndex(index);
            dialog_list_->removeOne(dialog);
        }
    });
    connect(dialog, &QDialog::rejected, this, [=, this]() {
        if (node->id == 0) {
            ResourcePool<Node>::Instance().Recycle(node);
            dialog_list_->removeOne(dialog);
        }
    });

    connect(table_model, &TransModel::SResizeColumnToContents, dialog->View(), &QTableView::resizeColumnToContents);
    connect(table_model, &TransModel::SSearch, tree_model, &NodeModel::RSearch);

    connect(table_model, &TransModel::SSyncLeafValue, dialog, &InsertNodeOrder::RUpdateLeafValue);
    connect(dialog, &InsertNodeOrder::SSyncLeafValue, tree_model, &NodeModel::RSyncLeafValue);

    connect(dialog, &InsertNodeOrder::SSyncBoolNode, tree_model, &NodeModel::RSyncBoolWD);
    connect(dialog, &InsertNodeOrder::SSyncBoolTrans, table_model, &TransModel::RSyncBoolWD);
    connect(dialog, &InsertNodeOrder::SSyncInt, table_model, &TransModel::RSyncInt);

    connect(tree_model, &NodeModel::SSyncBoolWD, dialog, &InsertNodeOrder::RSyncBoolNode);
    connect(tree_model, &NodeModel::SSyncInt, dialog, &InsertNodeOrder::RSyncInt);
    connect(tree_model, &NodeModel::SSyncString, dialog, &InsertNodeOrder::RSyncString);

    dialog_list_->append(dialog);

    SetTableView(dialog->View(), std::to_underlying(TransEnumO::kDescription));
    TableDelegateO(dialog->View(), section_settings_);
    dialog->show();
}

void MainWindow::REditTransDocument(const QModelIndex& index)
{
    auto* leaf_widget { dynamic_cast<TransWidget*>(ui->tabWidget->currentWidget()) };
    assert(leaf_widget && "leaf_widget must be non-null");
    assert(index.isValid() && "index must be valid");

    const auto document_dir { QDir::homePath() + QDir::separator() + section_settings_->document_dir };
    const int trans_id { index.siblingAtColumn(std::to_underlying(TransEnum::kID)).data().toInt() };

    auto* document_pointer { leaf_widget->Model()->GetDocumentPointer(index) };
    assert(document_pointer && "document_pointer must be non-null");

    auto* dialog { new EditDocument(document_pointer, document_dir, this) };

    if (dialog->exec() == QDialog::Accepted)
        data_->sql->WriteField(data_->info.trans, kDocument, document_pointer->join(kSemicolon), trans_id);
}

void MainWindow::REditNodeDocument(const QModelIndex& index)
{
    assert(node_widget_ && "node_widget_ must be non-null");
    assert(index.isValid() && "index must be valid");

    const auto document_dir { QDir::homePath() + QDir::separator() + section_settings_->document_dir };
    const int node_id { index.siblingAtColumn(std::to_underlying(NodeEnum::kID)).data().toInt() };

    auto* document_pointer { node_widget_->Model()->DocumentPointer(node_id) };
    assert(document_pointer && "document_pointer must be non-null");

    auto* dialog { new EditDocument(document_pointer, document_dir, this) };

    if (dialog->exec() == QDialog::Accepted)
        data_->sql->WriteField(data_->info.node, kDocument, document_pointer->join(kSemicolon), node_id);
}

void MainWindow::RTransRef(const QModelIndex& index)
{
    const int node_id { index.siblingAtColumn(std::to_underlying(NodeEnum::kID)).data().toInt() };
    const int unit { index.siblingAtColumn(std::to_underlying(NodeEnum::kUnit)).data().toInt() };

    assert(node_widget_ && "node_widget_ must be non-null");
    assert(index.isValid() && "index must be valid");
    assert(node_widget_->Model()->Type(node_id) == kTypeLeaf
        && "Node type should be 'kTypeLeaf' at this point. The type check should be performed in the delegate DoubleSpinUnitRPS.");

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

    CreateTransRef(node_widget_->Model(), data_, node_id, unit);
}

void MainWindow::TransRefS(int node_id, int unit)
{
    if (start_ != Section::kStakeholder)
        return;

    CreateTransRef(node_widget_->Model(), data_, node_id, unit);
}

void MainWindow::RSyncName(int node_id, const QString& name, bool branch)
{
    auto model { node_widget_->Model() };
    auto* widget { ui->tabWidget };

    auto* tab_bar { widget->tabBar() };
    int count { widget->count() };

    QSet<int> nodes;

    if (branch) {
        nodes = model->ChildrenID(node_id);
    } else {
        nodes.insert(node_id);

        if (start_ == Section::kStakeholder)
            UpdateStakeholderReference(nodes, branch);

        if (!trans_wgt_hash_->contains(node_id))
            return;
    }

    QString path {};

    for (int index = 0; index != count; ++index) {
        const int node_id { tab_bar->tabData(index).value<Tab>().node_id };

        if (widget->isTabVisible(index) && nodes.contains(node_id)) {
            path = model->Path(node_id);

            if (!branch) {
                tab_bar->setTabText(index, name);
            }

            tab_bar->setTabToolTip(index, path);
        }
    }

    if (start_ == Section::kStakeholder)
        UpdateStakeholderReference(nodes, branch);
}

void MainWindow::RUpdateSettings(const AppSettings& app_settings, const FileSettings& file_settings, const SectionSettings& section_settings)
{
    if (app_settings_ != app_settings) {
        UpdateAppSettings(app_settings);
    }

    if (*section_settings_ != section_settings) {
        UpdateSectionSettings(section_settings);
    }

    if (file_settings_ != file_settings) {
        UpdateFileSettings(file_settings);
    }
}

void MainWindow::UpdateAppSettings(CAppSettings& app_settings)
{
    auto new_separator { app_settings.separator };
    auto old_separator { app_settings_.separator };

    if (old_separator != new_separator) {
        finance_tree_->Model()->UpdateSeparator(old_separator, new_separator);
        stakeholder_tree_->Model()->UpdateSeparator(old_separator, new_separator);
        product_tree_->Model()->UpdateSeparator(old_separator, new_separator);
        task_tree_->Model()->UpdateSeparator(old_separator, new_separator);

        auto* widget { ui->tabWidget };
        int count { ui->tabWidget->count() };

        for (int index = 0; index != count; ++index)
            widget->setTabToolTip(index, widget->tabToolTip(index).replace(old_separator, new_separator));
    }

    if (app_settings_.language != app_settings.language) {
        MainWindowUtils::Message(QMessageBox::Information, tr("Language Changed"),
            tr("The language has been changed. Please restart the application for the changes to take effect."), kThreeThousand);
    }

    app_settings_ = app_settings;

    app_settings_sync_->beginGroup(kYTX);
    app_settings_sync_->setValue(kLanguage, app_settings.language);
    app_settings_sync_->setValue(kSeparator, app_settings.separator);
    app_settings_sync_->setValue(kPrinter, app_settings.printer);
    app_settings_sync_->setValue(kTheme, app_settings.theme);
    app_settings_sync_->endGroup();
}

void MainWindow::UpdateFileSettings(CFileSettings& file_settings)
{
    file_settings_ = file_settings;

    file_settings_sync_->beginGroup(kCompany);
    file_settings_sync_->setValue(kName, file_settings.company_name);
    file_settings_sync_->endGroup();
}

void MainWindow::UpdateSectionSettings(CSectionSettings& section_settings)
{
    const bool update_default_unit { section_settings_->default_unit != section_settings.default_unit };
    const bool resize_column { section_settings_->amount_decimal != section_settings.amount_decimal
        || section_settings_->common_decimal != section_settings.common_decimal || section_settings_->date_format != section_settings.date_format };

    *section_settings_ = section_settings;

    if (update_default_unit)
        node_widget_->Model()->UpdateDefaultUnit(section_settings.default_unit);

    node_widget_->UpdateStatus();

    file_settings_sync_->beginGroup(data_->info.node);

    file_settings_sync_->setValue(kStaticLabel, section_settings.static_label);
    file_settings_sync_->setValue(kStaticNode, section_settings.static_node);
    file_settings_sync_->setValue(kDynamicLabel, section_settings.dynamic_label);
    file_settings_sync_->setValue(kDynamicNodeLhs, section_settings.dynamic_node_lhs);
    file_settings_sync_->setValue(kOperation, section_settings.operation);
    file_settings_sync_->setValue(kDynamicNodeRhs, section_settings.dynamic_node_rhs);
    file_settings_sync_->setValue(kDefaultUnit, section_settings.default_unit);
    file_settings_sync_->setValue(kDocumentDir, section_settings.document_dir);
    file_settings_sync_->setValue(kDateFormat, section_settings.date_format);
    file_settings_sync_->setValue(kAmountDecimal, section_settings.amount_decimal);
    file_settings_sync_->setValue(kCommonDecimal, section_settings.common_decimal);

    file_settings_sync_->endGroup();

    if (resize_column) {
        auto* current_widget { ui->tabWidget->currentWidget() };

        if (const auto* leaf_widget = dynamic_cast<TransWidget*>(current_widget)) {
            auto* header { leaf_widget->View()->horizontalHeader() };

            int column { std::to_underlying(TransEnum::kDescription) };
            auto* model { leaf_widget->Model().data() };

            if (qobject_cast<SupportModel*>(model) || qobject_cast<TransModelO*>(model)) {
                column = std::to_underlying(TransEnumO::kDescription);
            }

            ResizeColumn(header, column);
            return;
        }

        if (const auto* node_widget = dynamic_cast<NodeWidget*>(current_widget)) {
            auto* header { node_widget->View()->header() };
            ResizeColumn(header, std::to_underlying(NodeEnum::kDescription));
        }
    }
}

void MainWindow::UpdateStakeholderReference(QSet<int> stakeholder_nodes, bool branch) const
{
    auto* widget { ui->tabWidget };
    auto stakeholder_model { node_widget_->Model() };
    auto order_model { sales_tree_->Model() };
    auto* tab_bar { widget->tabBar() };
    const int count { widget->count() };

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
                QString path = stakeholder_model->Path(order_party);

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

void MainWindow::VerifyActivation()
{
    // Initialize hardware UUID
    hardware_uuid_ = MainWindowUtils::GetHardwareUUID();
    if (hardware_uuid_.isEmpty()) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to retrieve hardware UUID."));
        return;
    }

    // Initialize network manager if not already created
    if (!network_manager_) {
        network_manager_ = new QNetworkAccessManager(this);
    }

    // Load activation code from ini file
    license_settings_ = QSharedPointer<QSettings>::create(
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QDir::separator() + kLicense + kDotSuffixINI, QSettings::IniFormat);

    license_settings_->beginGroup(kLicense);
    activation_code_ = license_settings_->value(kActivationCode, {}).toString();
    activation_url_ = license_settings_->value(kActivationUrl, "http://127.0.0.1:8080").toString();
    license_settings_->endGroup();

    // Prepare the request
    const QUrl url(activation_url_ + QDir::separator() + kActivate);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Create JSON payload
    const QJsonObject json { { "hardware_uuid", hardware_uuid_ }, { "activation_code", activation_code_ } };

    // Send the request
    QNetworkReply* reply { network_manager_->post(request, QJsonDocument(json).toJson()) };

    // Handle response
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            is_activated_ = true;
        } else {
            is_activated_ = false;
        }
        reply->deleteLater();
    });

    // Handle timeout
    connect(reply, &QNetworkReply::errorOccurred, this, [this](QNetworkReply::NetworkError error) {
        if (error == QNetworkReply::TimeoutError) {
            QMessageBox::critical(this, tr("Error"), tr("Connection timeout. Please try again."));
        }
    });
}

void MainWindow::ReadAppSettings()
{
    app_settings_sync_ = std::make_shared<QSettings>(
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QDir::separator() + kYTX + kDotSuffixINI, QSettings::IniFormat);

    QString language_code { kEnUS };
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));

    switch (QLocale::system().language()) {
    case QLocale::Chinese:
        language_code = kZhCN;
        break;
    default:
        break;
    }

    app_settings_sync_->beginGroup(kYTX);
    app_settings_.language = app_settings_sync_->value(kLanguage, language_code).toString();
    app_settings_.theme = app_settings_sync_->value(kTheme, kSolarizedDark).toString();
    app_settings_.separator = app_settings_sync_->value(kSeparator, kDash).toString();
    app_settings_.printer = app_settings_sync_->value(kPrinter, {}).toString();
    app_settings_sync_->endGroup();

    LoadAndInstallTranslator(app_settings_.language);

#ifdef Q_OS_WIN
    const QString theme { QStringLiteral("file:///:/theme/theme/%1 Win.qss").arg(app_settings_.theme) };
#elif defined(Q_OS_MACOS)
    const QString theme { QStringLiteral("file:///:/theme/theme/%1 Mac.qss").arg(app_settings_.theme) };
#endif

    qApp->setStyleSheet(theme);
}

void MainWindow::ReadFileSettings(CString& complete_base_name)
{
    file_settings_sync_ = std::make_shared<QSettings>(
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QDir::separator() + complete_base_name + kDotSuffixINI, QSettings::IniFormat);

    file_settings_sync_->beginGroup(kCompany);
    file_settings_.company_name = app_settings_sync_->value(kName, {}).toString();
    file_settings_sync_->endGroup();
}

void MainWindow::ReadSectionSettings(SectionSettings& section, CString& section_name)
{
    file_settings_sync_->beginGroup(section_name);

    section.static_label = file_settings_sync_->value(kStaticLabel, {}).toString();
    section.static_node = file_settings_sync_->value(kStaticNode, 0).toInt();
    section.dynamic_label = file_settings_sync_->value(kDynamicLabel, {}).toString();
    section.dynamic_node_lhs = file_settings_sync_->value(kDynamicNodeLhs, 0).toInt();
    section.operation = file_settings_sync_->value(kOperation, kPlus).toString();
    section.dynamic_node_rhs = file_settings_sync_->value(kDynamicNodeRhs, 0).toInt();
    section.default_unit = file_settings_sync_->value(kDefaultUnit, 0).toInt();
    section.document_dir = file_settings_sync_->value(kDocumentDir, {}).toString();
    section.date_format = file_settings_sync_->value(kDateFormat, kDateTimeFST).toString();
    section.amount_decimal = file_settings_sync_->value(kAmountDecimal, 2).toInt();
    section.common_decimal = file_settings_sync_->value(kCommonDecimal, 2).toInt();

    file_settings_sync_->endGroup();
}

void MainWindow::on_actionSearch_triggered()
{
    auto* dialog { new Search(node_widget_->Model(), stakeholder_tree_->Model(), product_tree_->Model(), section_settings_, data_->sql, data_->info, this) };
    dialog->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);

    connect(dialog, &Search::SNodeLocation, this, &MainWindow::RNodeLocation);
    connect(dialog, &Search::STransLocation, this, &MainWindow::RTransLocation);
    connect(node_widget_->Model(), &NodeModel::SSearch, dialog, &Search::RSearch);
    connect(dialog, &QDialog::rejected, this, [=, this]() { dialog_list_->removeOne(dialog); });

    dialog_list_->append(dialog);
    dialog->show();
}

void MainWindow::RNodeLocation(int node_id)
{
    // Ignore report widget
    if (node_id <= 0)
        return;

    auto* widget { node_widget_ };
    ui->tabWidget->setCurrentWidget(widget);

    if (start_ == Section::kSales || start_ == Section::kPurchase) {
        node_widget_->Model()->ReadNode(node_id);
    }

    auto index { node_widget_->Model()->GetIndex(node_id) };
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
            CreateLeafFPTS(node_widget_->Model(), trans_wgt_hash_, data_, section_settings_, id);
        break;
    case Section::kFinance:
    case Section::kProduct:
    case Section::kTask:
        if (!Contains(lhs_node_id) && !Contains(rhs_node_id))
            CreateLeafFPTS(node_widget_->Model(), trans_wgt_hash_, data_, section_settings_, id);
        break;
    default:
        break;
    }

    SwitchToLeaf(id, trans_id);
}

void MainWindow::OrderNodeLocation(Section section, int node_id)
{
    switch (section) {
    case Section::kSales:
        RSectionGroup(std::to_underlying(Section::kSales));
        ui->rBtnSales->setChecked(true);
        break;
    case Section::kPurchase:
        RSectionGroup(std::to_underlying(Section::kPurchase));
        ui->rBtnPurchase->setChecked(true);
        break;
    default:
        return;
    }

    ui->tabWidget->setCurrentWidget(node_widget_);

    node_widget_->Model()->ReadNode(node_id);
    node_widget_->activateWindow();

    auto index { node_widget_->Model()->GetIndex(node_id) };
    node_widget_->View()->setCurrentIndex(index);
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
            tab_bar->setTabToolTip(index, model->Path(party_id));
        }
    }
}

void MainWindow::on_actionPreferences_triggered()
{
    auto model { node_widget_->Model() };

    auto* preference { new Preferences(data_->info, model, app_settings_, file_settings_, *section_settings_, this) };
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

void MainWindow::on_actionLicence_triggered()
{
    static Licence* dialog = nullptr;

    if (!dialog) {
        dialog = new Licence(network_manager_, license_settings_, hardware_uuid_, activation_code_, activation_url_, is_activated_, this);
        dialog->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);
        connect(dialog, &QDialog::finished, [=]() { dialog = nullptr; });
    }

    dialog->show();
    dialog->activateWindow();
}

void MainWindow::on_actionNewFile_triggered()
{
    auto file_path { QFileDialog::getSaveFileName(this, tr("New File"), QDir::homePath(), QStringLiteral("*.ytx"), nullptr) };
    if (!MainWindowUtils::PrepareNewFile(file_path, kDotSuffixYTX))
        return;

    if (SqliteYtx::NewFile(file_path))
        ROpenFile(file_path);
}

void MainWindow::on_actionOpenFile_triggered()
{
    const auto file_path { QFileDialog::getOpenFileName(this, tr("Open File"), QDir::homePath(), QStringLiteral("*.ytx"), nullptr) };
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
    MainWindowUtils::WriteSettings(app_settings_sync_, recent_file_, kRecent, kFile);
}

void MainWindow::on_tabWidget_tabBarDoubleClicked(int index) { RNodeLocation(ui->tabWidget->tabBar()->tabData(index).value<Tab>().node_id); }

void MainWindow::RUpdateState()
{
    auto* leaf_widget { dynamic_cast<TransWidget*>(ui->tabWidget->currentWidget()) };
    assert(leaf_widget && "leaf_widget must be non-null");

    auto table_model { leaf_widget->Model() };
    table_model->UpdateAllState(Check { QObject::sender()->property(kCheck).toInt() });
}

void MainWindow::SwitchSection(CTab& last_tab) const
{
    auto* tab_widget { ui->tabWidget };
    auto* tab_bar { tab_widget->tabBar() };
    const int count { tab_widget->count() };

    for (int index = 0; index != count; ++index) {
        const auto kTab { tab_bar->tabData(index).value<Tab>() };
        tab_widget->setTabVisible(index, kTab.section == start_);

        if (kTab == last_tab)
            tab_widget->setCurrentIndex(index);
    }

    MainWindowUtils::SwitchDialog(dialog_list_, true);
}

void MainWindow::UpdateLastTab() const
{
    if (data_) {
        auto index { ui->tabWidget->currentIndex() };
        data_->tab = ui->tabWidget->tabBar()->tabData(index).value<Tab>();
    }
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    auto* widget { ui->tabWidget->currentWidget() };
    assert(widget && "widget must be non-null");

    const bool is_node { MainWindowUtils::IsNodeWidget(widget) };
    const bool is_leaf_fpts { MainWindowUtils::IsLeafWidgetFPTS(widget) };
    const bool is_leaf_order { MainWindowUtils::IsLeafWidgetO(widget) };
    const bool is_support { MainWindowUtils::IsSupportWidget(widget) };
    const bool is_order_section { start_ == Section::kSales || start_ == Section::kPurchase };

    bool finished {};

    if (is_leaf_order) {
        const int node_id { ui->tabWidget->tabBar()->tabData(index).value<Tab>().node_id };
        finished = node_widget_->Model()->Finished(node_id);
    }

    ui->actionAppendNode->setEnabled(is_node);
    ui->actionEditNode->setEnabled(is_node && !is_order_section);

    ui->actionCheckAll->setEnabled(is_leaf_fpts);
    ui->actionCheckNone->setEnabled(is_leaf_fpts);
    ui->actionCheckReverse->setEnabled(is_leaf_fpts);
    ui->actionJump->setEnabled(is_leaf_fpts);
    ui->actionSupportJump->setEnabled(is_leaf_fpts || is_support);

    ui->actionStatement->setEnabled(is_order_section);
    ui->actionSettlement->setEnabled(is_order_section);

    ui->actionAppendTrans->setEnabled(is_leaf_fpts || (is_leaf_order && !finished));
    ui->actionRemove->setEnabled(is_node || is_leaf_fpts || (is_leaf_order && !finished));
}

void MainWindow::on_actionAppendTrans_triggered()
{
    auto* active_window { QApplication::activeWindow() };
    if (auto* edit_node_order = dynamic_cast<InsertNodeOrder*>(active_window)) {
        MainWindowUtils::AppendTrans(edit_node_order, start_);
        return;
    }

    auto* widget { ui->tabWidget->currentWidget() };
    assert(widget && "widget must be non-null");

    if (auto* leaf_widget = dynamic_cast<TransWidget*>(widget)) {
        MainWindowUtils::AppendTrans(leaf_widget, start_);
    }
}

void MainWindow::on_actionExportYTX_triggered()
{
    CString& source { DatabaseManager::Instance().DatabaseName() };
    if (source.isEmpty())
        return;

    QString destination { QFileDialog::getSaveFileName(this, tr("Export Structure"), QDir::homePath(), QStringLiteral("*.ytx")) };
    if (!MainWindowUtils::PrepareNewFile(destination, kDotSuffixYTX))
        return;

    if (!SqliteYtx::NewFile(destination))
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
    CString& source { DatabaseManager::Instance().DatabaseName() };
    if (source.isEmpty())
        return;

    QString destination { QFileDialog::getSaveFileName(this, tr("Export Excel"), QDir::homePath(), QStringLiteral("*.xlsx")) };
    if (!MainWindowUtils::PrepareNewFile(destination, kDotSuffixXLSX))
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
            book1->GetCurrentWorksheet()->WriteRow(1, 1, data_->info.excel_node_header);
            MainWindowUtils::ExportExcel(source, data_->info.node, book1->GetCurrentWorksheet());

            book1->AppendSheet(data_->info.path);
            book1->GetCurrentWorksheet()->WriteRow(1, 1, list);
            MainWindowUtils::ExportExcel(source, data_->info.path, book1->GetCurrentWorksheet(), false);

            book1->AppendSheet(data_->info.trans);
            book1->GetCurrentWorksheet()->WriteRow(1, 1, data_->info.excel_trans_header);
            const bool where { start_ == Section::kStakeholder ? false : true };
            MainWindowUtils::ExportExcel(source, data_->info.trans, book1->GetCurrentWorksheet(), where);

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
