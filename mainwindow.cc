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
#include "dialog/login.h"
#include "dialog/newdatabase.h"
#include "dialog/preferences.h"
#include "dialog/removenode.h"
#include "document.h"
#include "global/databasemanager.h"
#include "global/leafsstation.h"
#include "global/resourcepool.h"
#include "global/supportsstation.h"
#include "licence/licence.h"
#include "licence/signatureencryptor.h"
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
    ReadAppConfig();
    VerifyActivationOffline();
    // QTimer::singleShot(5 * 60 * 1000, this, &MainWindow::VerifyActivationOnline);

    ui->setupUi(this);
    SignalBlocker blocker(this);

    SetTabWidget();
    IniSectionGroup();
    StringInitializer::SetHeader(finance_data_.info, product_data_.info, stakeholder_data_.info, task_data_.info, sales_data_.info, purchase_data_.info);
    SetAction();
    SetConnect();

    MainWindowUtils::ReadSettings(ui->splitter, &QSplitter::restoreState, app_settings_, kSplitter, kState);
    MainWindowUtils::ReadSettings(this, &QMainWindow::restoreState, app_settings_, kMainwindow, kState, 0);
    MainWindowUtils::ReadSettings(this, &QMainWindow::restoreGeometry, app_settings_, kMainwindow, kGeometry);

    EnableAction(false);

#ifdef Q_OS_WIN
    ui->actionRemove->setShortcut(Qt::Key_Delete);
    qApp->setWindowIcon(QIcon(":/logo/logo/logo.ico"));
#elif defined(Q_OS_MACOS)
    ui->actionRemove->setShortcut(Qt::Key_Backspace);
    qApp->setWindowIcon(QIcon(":/logo/logo/logo.icns"));
#endif

    QTimer::singleShot(0, this, [this]() {
        auto* login { new Login(login_config_, app_settings_, this) };
        login->exec();
    });
}

MainWindow::~MainWindow()
{
    MainWindowUtils::WriteSettings(ui->splitter, &QSplitter::saveState, app_settings_, kSplitter, kState);
    MainWindowUtils::WriteSettings(this, &QMainWindow::saveState, app_settings_, kMainwindow, kState, 0);
    MainWindowUtils::WriteSettings(this, &QMainWindow::saveGeometry, app_settings_, kMainwindow, kGeometry);
    MainWindowUtils::WriteSettings(app_settings_, std::to_underlying(start_), kStart, kSection);

    if (login_config_.is_successful) {
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
    if (!license_config_.is_activated) {
        QMessageBox::critical(this, tr("Activation Required"), tr("The software is not activated. Please activate it first."));
        return false;
    }

    SetFinanceData();
    SetTaskData();
    SetProductData();
    SetStakeholderData();
    SetSalesData();
    SetPurchaseData();

    CreateSection(finance_tree_, finance_trans_wgt_hash_, finance_data_, finance_config_, tr("Finance"));
    CreateSection(stakeholder_tree_, stakeholder_trans_wgt_hash_, stakeholder_data_, stakeholder_config_, tr("Stakeholder"));
    CreateSection(product_tree_, product_trans_wgt_hash_, product_data_, product_config_, tr("Product"));
    CreateSection(task_tree_, task_trans_wgt_hash_, task_data_, task_config_, tr("Task"));
    CreateSection(sales_tree_, sales_trans_wgt_hash_, sales_data_, sales_config_, tr("Sales"));
    CreateSection(purchase_tree_, purchase_trans_wgt_hash_, purchase_data_, purchase_config_, tr("Purchase"));

    RSectionGroup(static_cast<int>(start_));

    EnableAction(true);
    on_tabWidget_currentChanged(0);

    QTimer::singleShot(0, this, [this, file_path]() { MainWindowUtils::ReadPrintTmplate(print_template_); });
    return true;
}

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
        CreateLeafO(node_widget_->Model(), trans_wgt_hash_, data_, section_config_, node_id);
    } else {
        CreateLeafFPTS(node_widget_->Model(), trans_wgt_hash_, data_, section_config_, node_id);
    }
}

void MainWindow::CreateSupportFunction(int type, int node_id)
{
    assert(type == kTypeSupport && "type must be kTypeSupport");
    assert(start_ != Section::kSales && start_ != Section::kPurchase && "start_ must not be kSales or kPurchase");

    if (sup_wgt_hash_->contains(node_id))
        return;

    CreateSupport(node_widget_->Model(), sup_wgt_hash_, data_, section_config_, node_id);
}

void MainWindow::RSectionGroup(int id)
{
    const Section kSection { id };

    start_ = kSection;

    if (!login_config_.is_successful)
        return;

    MainWindowUtils::SwitchDialog(dialog_list_, false);
    UpdateLastTab();

    switch (kSection) {
    case Section::kFinance:
        node_widget_ = finance_tree_;
        trans_wgt_hash_ = &finance_trans_wgt_hash_;
        dialog_list_ = &finance_dialog_list_;
        section_config_ = &finance_config_;
        data_ = &finance_data_;
        sup_wgt_hash_ = &finance_sup_wgt_hash_;
        rpt_wgt_hash_ = nullptr;
        break;
    case Section::kProduct:
        node_widget_ = product_tree_;
        trans_wgt_hash_ = &product_trans_wgt_hash_;
        dialog_list_ = &product_dialog_list_;
        section_config_ = &product_config_;
        data_ = &product_data_;
        sup_wgt_hash_ = &product_sup_wgt_hash_;
        rpt_wgt_hash_ = &product_rpt_wgt_hash_;
        break;
    case Section::kTask:
        node_widget_ = task_tree_;
        trans_wgt_hash_ = &task_trans_wgt_hash_;
        dialog_list_ = &task_dialog_list_;
        section_config_ = &task_config_;
        data_ = &task_data_;
        sup_wgt_hash_ = &task_sup_wgt_hash_;
        rpt_wgt_hash_ = nullptr;
        break;
    case Section::kStakeholder:
        node_widget_ = stakeholder_tree_;
        trans_wgt_hash_ = &stakeholder_trans_wgt_hash_;
        dialog_list_ = &stakeholder_dialog_list_;
        section_config_ = &stakeholder_config_;
        data_ = &stakeholder_data_;
        sup_wgt_hash_ = &stakeholder_sup_wgt_hash_;
        rpt_wgt_hash_ = &stakeholder_rpt_wgt_hash_;
        break;
    case Section::kSales:
        node_widget_ = sales_tree_;
        trans_wgt_hash_ = &sales_trans_wgt_hash_;
        dialog_list_ = &sales_dialog_list_;
        section_config_ = &sales_config_;
        data_ = &sales_data_;
        sup_wgt_hash_ = nullptr;
        rpt_wgt_hash_ = &sales_rpt_wgt_hash_;
        break;
    case Section::kPurchase:
        node_widget_ = purchase_tree_;
        trans_wgt_hash_ = &purchase_trans_wgt_hash_;
        dialog_list_ = &purchase_dialog_list_;
        section_config_ = &purchase_config_;
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
    DelegateStatementPrimary(view, section_config_);

    connect(widget, &StatementWidget::SResetModel, model, &StatementPrimaryModel::RResetModel);

    RegisterRptWgt(widget);
}

void MainWindow::RStatementSecondary(int party_id, int unit, const QDateTime& start, const QDateTime& end)
{
    auto* sql { data_->sql };
    const auto& info { data_->info };

    auto* model { new StatementSecondaryModel(
        sql, info, party_id, product_tree_->Model()->LeafPath(), stakeholder_tree_->Model(), file_config_.company_name, this) };
    auto* widget { new StatementWidget(model, unit, true, start, end, this) };

    const QString name { tr("StatementSecondary-") + stakeholder_tree_->Model()->Name(party_id) };
    const int tab_index { ui->tabWidget->addTab(widget, name) };
    auto* tab_bar { ui->tabWidget->tabBar() };

    tab_bar->setTabData(tab_index, QVariant::fromValue(Tab { start_, report_id_ }));
    tab_bar->setTabToolTip(tab_index, name);

    auto view { widget->View() };
    SetStatementView(view, std::to_underlying(StatementSecondaryEnum::kDescription));
    DelegateStatementSecondary(view, section_config_);

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
        CreateLeafO(node_widget_->Model(), trans_wgt_hash_, data_, section_config_, node_id);
    }
}

void MainWindow::CreateLeafFPTS(PNodeModel tree_model, TransWgtHash* trans_wgt_hash, CData* data, CSectionConfig* settings, int node_id)
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

void MainWindow::CreateSupport(PNodeModel tree_model, SupWgtHash* sup_wgt_hash, CData* data, CSectionConfig* settings, int node_id)
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

void MainWindow::CreateLeafO(PNodeModel tree_model, TransWgtHash* trans_wgt_hash, CData* data, CSectionConfig* settings, int node_id)
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

    auto print_manager = QSharedPointer<PrintManager>::create(app_config_, product_tree_->Model(), stakeholder_tree_->Model());

    auto widget_arg { InsertNodeArgO { node, sql, model, stakeholder_tree_->Model(), section_config_, section } };
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

void MainWindow::TableDelegateFPTS(PTableView table_view, PNodeModel tree_model, CSectionConfig* settings) const
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

void MainWindow::TableDelegateFPT(PTableView table_view, PNodeModel tree_model, CSectionConfig* settings, int node_id) const
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

void MainWindow::TableDelegateO(PTableView table_view, CSectionConfig* settings) const
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

void MainWindow::CreateSection(NodeWidget* node_widget, TransWgtHash& trans_wgt_hash, CData& data, CSectionConfig& settings, CString& name)
{
    const auto& info { data.info };
    auto* tab_widget { ui->tabWidget };

    auto view { node_widget->View() };
    auto model { node_widget->Model() };

    // ReadSettings must before SetTreeView
    MainWindowUtils::ReadSettings(view->header(), &QHeaderView::restoreState, file_settings_, info.node, kHeaderState);

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

    RestoreTab(model, trans_wgt_hash, MainWindowUtils::ReadSettings(file_settings_, info.node, kTabID), data, settings);
}

void MainWindow::SetTreeDelegate(PTreeView tree_view, CInfo& info, CSectionConfig& settings) const
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

void MainWindow::TreeDelegateF(PTreeView tree_view, CInfo& info, CSectionConfig& settings) const
{
    auto* final_total { new DoubleSpinUnitR(settings.amount_decimal, settings.default_unit, info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumF::kLocalTotal), final_total);

    auto* initial_total { new FinanceForeignR(settings.amount_decimal, settings.default_unit, info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumF::kForeignTotal), initial_total);
}

void MainWindow::TreeDelegateT(PTreeView tree_view, CSectionConfig& settings) const
{
    auto* quantity { new DoubleSpinR(settings.common_decimal, kCoefficient16, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumT::kQuantity), quantity);

    auto* amount { new DoubleSpinUnitR(settings.amount_decimal, finance_config_.default_unit, finance_data_.info.unit_symbol_map, tree_view) };
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

void MainWindow::TreeDelegateP(PTreeView tree_view, CSectionConfig& settings) const
{
    auto* quantity { new DoubleSpinR(settings.common_decimal, kCoefficient16, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kQuantity), quantity);

    auto* amount { new DoubleSpinUnitRPS(settings.amount_decimal, finance_config_.default_unit, finance_data_.info.unit_symbol_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kAmount), amount);
    connect(amount, &DoubleSpinUnitRPS::STransRef, this, &MainWindow::RTransRef);

    auto* unit_price { new DoubleSpin(settings.amount_decimal, 0, std::numeric_limits<double>::max(), kCoefficient8, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kUnitPrice), unit_price);
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kCommission), unit_price);

    auto* color { new Color(tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kColor), color);
}

void MainWindow::TreeDelegateS(PTreeView tree_view, CSectionConfig& settings) const
{
    auto* amount { new DoubleSpinUnitRPS(settings.amount_decimal, finance_config_.default_unit, finance_data_.info.unit_symbol_map, tree_view) };
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

void MainWindow::TreeDelegateO(PTreeView tree_view, CSectionConfig& settings) const
{
    auto* amount { new DoubleSpinUnitR(settings.amount_decimal, finance_config_.default_unit, finance_data_.info.unit_symbol_map, tree_view) };
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
    node->unit = parent_id == -1 ? section_config_->default_unit : model->Unit(parent_id);
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

void MainWindow::RestoreTab(PNodeModel tree_model, TransWgtHash& trans_wgt_hash, CIntSet& set, CData& data, CSectionConfig& section_settings)
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

void MainWindow::DelegateSupport(PTableView table_view, PNodeModel tree_model, CSectionConfig* settings) const
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
    DelegateStatement(view, section_config_);

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

    DelegateSettlement(view, section_config_);
    DelegateSettlementPrimary(primary_view, section_config_);

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
    DelegateTransRef(view, &sales_config_);

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

void MainWindow::DelegateTransRef(PTableView table_view, CSectionConfig* settings) const
{
    auto* price { new DoubleSpinRNoneZero(settings->amount_decimal, kCoefficient16, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransRefEnum::kUnitPrice), price);
    table_view->setItemDelegateForColumn(std::to_underlying(TransRefEnum::kDiscountPrice), price);

    auto* quantity { new DoubleSpinRNoneZero(settings->common_decimal, kCoefficient16, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransRefEnum::kFirst), quantity);
    table_view->setItemDelegateForColumn(std::to_underlying(TransRefEnum::kSecond), quantity);

    auto* amount { new DoubleSpinRNoneZero(settings->amount_decimal, kCoefficient16, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(TransRefEnum::kGrossAmount), amount);

    auto* date_time { new DateTimeR(sales_config_.date_format, table_view) };
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

void MainWindow::DelegateStatement(PTableView table_view, CSectionConfig* settings) const
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

void MainWindow::DelegateSettlement(PTableView table_view, CSectionConfig* settings) const
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

void MainWindow::DelegateSettlementPrimary(PTableView table_view, CSectionConfig* settings) const
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

void MainWindow::DelegateStatementPrimary(PTableView table_view, CSectionConfig* settings) const
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

    auto* date_time { new DateTimeR(sales_config_.date_format, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(StatementPrimaryEnum::kDateTime), date_time);
}

void MainWindow::DelegateStatementSecondary(PTableView table_view, CSectionConfig* settings) const
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

    auto* date_time { new DateTimeR(sales_config_.date_format, table_view) };
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

    ReadSectionConfig(finance_config_, kFinance);

    sql = new SqliteF(info, this);

    NodeModelArg arg { sql, info, finance_trans_wgt_hash_, app_config_.separator, finance_config_.default_unit };
    auto* model { new NodeModelF(arg, this) };

    finance_tree_ = new NodeWidgetF(model, info, finance_config_, this);
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

    ReadSectionConfig(product_config_, kProduct);

    sql = new SqliteP(info, this);

    NodeModelArg arg { sql, info, product_trans_wgt_hash_, app_config_.separator, product_config_.default_unit };
    auto* model { new NodeModelP(arg, this) };

    product_tree_ = new NodeWidgetPT(model, product_config_, this);
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

    ReadSectionConfig(stakeholder_config_, kStakeholder);

    sql = new SqliteS(info, this);

    NodeModelArg arg { sql, info, stakeholder_trans_wgt_hash_, app_config_.separator, stakeholder_config_.default_unit };
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

    ReadSectionConfig(task_config_, kTask);

    sql = new SqliteT(info, this);

    NodeModelArg arg { sql, info, task_trans_wgt_hash_, app_config_.separator, task_config_.default_unit };
    auto* model { new NodeModelT(arg, this) };

    task_tree_ = new NodeWidgetPT(model, task_config_, this);
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

    ReadSectionConfig(sales_config_, kSales);

    sql = new SqliteO(info, this);

    NodeModelArg arg { sql, info, sales_trans_wgt_hash_, app_config_.separator, sales_config_.default_unit };
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

    ReadSectionConfig(purchase_config_, kPurchase);

    sql = new SqliteO(info, this);

    NodeModelArg arg { sql, info, purchase_trans_wgt_hash_, app_config_.separator, purchase_config_.default_unit };
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
    ui->actionNewDatabase->setIcon(QIcon(":/solarized_dark/solarized_dark/new.png"));
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
        CreateLeafFPTS(node_widget_->Model(), trans_wgt_hash_, data_, section_config_, rhs_node_id);

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
        CreateSupport(node_widget_->Model(), sup_wgt_hash_, data_, section_config_, id);
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
        parent_path += app_config_.separator;

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
        parent_path += app_config_.separator;

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
        dialog = new InsertNodeTask(arg, section_config_->amount_decimal, section_config_->date_format, this);
        break;
    case Section::kStakeholder:
        node->date_time = QDateTime::currentDateTime().toString(kDateTimeFST);
        employee_model = tree_model->IncludeUnitModel(std::to_underlying(UnitS::kEmp));
        dialog = new InsertNodeStakeholder(arg, employee_model, section_config_->amount_decimal, this);
        break;
    case Section::kProduct:
        dialog = new InsertNodeProduct(arg, section_config_->common_decimal, this);
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

    auto print_manager = QSharedPointer<PrintManager>::create(app_config_, product_tree_->Model(), stakeholder_tree_->Model());

    auto dialog_arg { InsertNodeArgO { node, sql, table_model, stakeholder_tree_->Model(), section_config_, start_ } };
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
    TableDelegateO(dialog->View(), section_config_);
    dialog->show();
}

void MainWindow::REditTransDocument(const QModelIndex& index)
{
    auto* leaf_widget { dynamic_cast<TransWidget*>(ui->tabWidget->currentWidget()) };
    assert(leaf_widget && "leaf_widget must be non-null");
    assert(index.isValid() && "index must be valid");

    const auto document_path { section_config_->document_path };
    const int trans_id { index.siblingAtColumn(std::to_underlying(TransEnum::kID)).data().toInt() };

    auto* document_pointer { leaf_widget->Model()->GetDocumentPointer(index) };
    assert(document_pointer && "document_pointer must be non-null");

    auto* dialog { new EditDocument(document_pointer, document_path, this) };

    if (dialog->exec() == QDialog::Accepted)
        data_->sql->WriteField(data_->info.trans, kDocument, document_pointer->join(kSemicolon), trans_id);
}

void MainWindow::REditNodeDocument(const QModelIndex& index)
{
    assert(node_widget_ && "node_widget_ must be non-null");
    assert(index.isValid() && "index must be valid");

    const auto document_dir { section_config_->document_path };
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

void MainWindow::RUpdateSettings(const AppConfig& app_settings, const FileConfig& file_settings, const SectionConfig& section_settings)
{
    if (app_config_ != app_settings) {
        UpdateAppConfig(app_settings);
    }

    if (*section_config_ != section_settings) {
        UpdateSectionConfig(section_settings);
    }

    if (file_config_ != file_settings) {
        UpdateFileConfig(file_settings);
    }
}

void MainWindow::UpdateAppConfig(CAppConfig& app_config)
{
    auto new_separator { app_config.separator };
    auto old_separator { app_config_.separator };

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

    if (app_config_.language != app_config.language) {
        MainWindowUtils::Message(QMessageBox::Information, tr("Language Changed"),
            tr("The language has been changed. Please restart the application for the changes to take effect."), kThreeThousand);
    }

    app_config_ = app_config;

    app_settings_->beginGroup(kYTX);
    app_settings_->setValue(kLanguage, app_config.language);
    app_settings_->setValue(kSeparator, app_config.separator);
    app_settings_->setValue(kPrinter, app_config.printer);
    app_settings_->setValue(kTheme, app_config.theme);
    app_settings_->endGroup();
}

void MainWindow::UpdateFileConfig(CFileConfig& file_config)
{
    file_config_ = file_config;

    file_settings_->beginGroup(kCompany);
    file_settings_->setValue(kName, file_config.company_name);
    file_settings_->endGroup();
}

void MainWindow::UpdateSectionConfig(CSectionConfig& section_config)
{
    const bool update_default_unit { section_config_->default_unit != section_config.default_unit };
    const bool resize_column { section_config_->amount_decimal != section_config.amount_decimal
        || section_config_->common_decimal != section_config.common_decimal || section_config_->date_format != section_config.date_format };

    *section_config_ = section_config;

    if (update_default_unit)
        node_widget_->Model()->UpdateDefaultUnit(section_config.default_unit);

    node_widget_->UpdateStatus();

    file_settings_->beginGroup(data_->info.node);

    file_settings_->setValue(kStaticLabel, section_config.static_label);
    file_settings_->setValue(kStaticNode, section_config.static_node);
    file_settings_->setValue(kDynamicLabel, section_config.dynamic_label);
    file_settings_->setValue(kDynamicNodeLhs, section_config.dynamic_node_lhs);
    file_settings_->setValue(kOperation, section_config.operation);
    file_settings_->setValue(kDynamicNodeRhs, section_config.dynamic_node_rhs);
    file_settings_->setValue(kDefaultUnit, section_config.default_unit);
    file_settings_->setValue(kDocumentPath, section_config.document_path);
    file_settings_->setValue(kDateFormat, section_config.date_format);
    file_settings_->setValue(kAmountDecimal, section_config.amount_decimal);
    file_settings_->setValue(kCommonDecimal, section_config.common_decimal);

    file_settings_->endGroup();

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

void MainWindow::VerifyActivationOffline()
{
    // Initialize hardware UUID
    license_config_.hardware_uuid = MainWindowUtils::GetHardwareUUID().toLower();
    if (license_config_.hardware_uuid.isEmpty()) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to retrieve hardware UUID."));
        return;
    }

    // Load license info from ini file
    license_settings_ = QSharedPointer<QSettings>::create(
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QDir::separator() + kLicense + kDotSuffixINI, QSettings::IniFormat);

    license_settings_->beginGroup(kLicense);
    license_config_.activation_code = license_settings_->value(kActivationCode, {}).toString();
    license_config_.activation_url = license_settings_->value(kActivationUrl, "https://ytxerp.cc").toString();

    const QByteArray ciphertext { QByteArray::fromBase64(license_settings_->value(kSignatureCiphertext).toByteArray()) };
    const QByteArray iv { QByteArray::fromBase64(license_settings_->value(kSignatureIV).toByteArray()) };
    const QByteArray tag { QByteArray::fromBase64(license_settings_->value(kSignatureTag).toByteArray()) };
    license_settings_->endGroup();

    // Construct encryption key from hardware UUID
    const QByteArray key { QCryptographicHash::hash(license_config_.hardware_uuid.toUtf8(), QCryptographicHash::Sha256).left(32) };
    SignatureEncryptor encryptor(key);

    // Decrypt signature
    const QByteArray decrypted_signature_bytes { encryptor.Decrypt(ciphertext, iv, tag) };
    if (decrypted_signature_bytes.isEmpty()) {
        QMessageBox::critical(this, tr("Fail"), tr("Activation Failed!"));
        return;
    }

    const QString payload { QString("%1:%2:%3").arg(license_config_.activation_code, license_config_.hardware_uuid, "true") };
    const QByteArray payload_bytes { payload.toUtf8() };

    const QString pub_key_path(":/keys/public.pem");
    license_config_.is_activated = Licence::VerifySignature(payload_bytes, decrypted_signature_bytes, pub_key_path);
}

void MainWindow::VerifyActivationOnline()
{
    auto* network_manager_ { new QNetworkAccessManager(this) };

    const QUrl url(license_config_.activation_url + "/" + kActivate);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    const QJsonObject json { { "hardware_uuid", license_config_.hardware_uuid }, { "activation_code", license_config_.activation_code } };
    QNetworkReply* reply { network_manager_->post(request, QJsonDocument(json).toJson()) };

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        auto fail = [this]() {
            license_config_.is_activated = false;
            license_settings_->beginGroup(kLicense);
            license_settings_->setValue(kSignatureCiphertext, {});
            license_settings_->setValue(kSignatureIV, {});
            license_settings_->setValue(kSignatureTag, {});
            license_settings_->endGroup();
        };

        if (reply->error() != QNetworkReply::NoError) {
            fail();
            return;
        }

        const QJsonDocument doc { QJsonDocument::fromJson(reply->readAll()) };
        if (!doc.isObject()) {
            fail();
            return;
        }

        const QJsonObject obj { doc.object() };
        const bool success { obj["success"].toBool() };
        const QString signature { obj["signature"].toString() };

        // Construct payload in the same format as server
        const QString payload { QString("%1:%2:%3").arg(license_config_.activation_code, license_config_.hardware_uuid, success ? "true" : "false") };
        const QByteArray payload_bytes { payload.toUtf8() };
        const QByteArray signature_bytes { QByteArray::fromBase64(signature.toUtf8()) };

        // Read public key
        const QString pub_key_path(":/keys/public.pem");
        if (!Licence::VerifySignature(payload_bytes, signature_bytes, pub_key_path)) {
            fail();
        }
    });
}

void MainWindow::ReadAppConfig()
{
    app_settings_ = QSharedPointer<QSettings>::create(
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

    app_settings_->beginGroup(kYTX);
    app_config_.language = app_settings_->value(kLanguage, language_code).toString();
    app_config_.theme = app_settings_->value(kTheme, kSolarizedDark).toString();
    app_config_.separator = app_settings_->value(kSeparator, kDash).toString();
    app_config_.printer = app_settings_->value(kPrinter, {}).toString();
    app_settings_->endGroup();

    app_settings_->beginGroup(kLogin);
    login_config_.user = app_settings_->value(kUser, {}).toString();
    login_config_.password = app_settings_->value(kPassword, {}).toString();
    login_config_.database = app_settings_->value(kDatabase, {}).toString();
    login_config_.is_saved = app_settings_->value(kIsSaved, {}).toBool();
    app_settings_->endGroup();

    LoadAndInstallTranslator(app_config_.language);

#ifdef Q_OS_WIN
    const QString theme { QStringLiteral("file:///:/theme/theme/%1 Win.qss").arg(app_settings_.theme) };
#elif defined(Q_OS_MACOS)
    const QString theme { QStringLiteral("file:///:/theme/theme/%1 Mac.qss").arg(app_config_.theme) };
#endif

    qApp->setStyleSheet(theme);
}

void MainWindow::ReadFileConfig(CString& complete_base_name)
{
    file_settings_ = QSharedPointer<QSettings>::create(
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QDir::separator() + complete_base_name + kDotSuffixINI, QSettings::IniFormat);

    file_settings_->beginGroup(kCompany);
    file_config_.company_name = app_settings_->value(kName, {}).toString();
    file_settings_->endGroup();
}

void MainWindow::ReadSectionConfig(SectionConfig& section, CString& section_name)
{
    file_settings_->beginGroup(section_name);

    section.static_label = file_settings_->value(kStaticLabel, {}).toString();
    section.static_node = file_settings_->value(kStaticNode, 0).toInt();
    section.dynamic_label = file_settings_->value(kDynamicLabel, {}).toString();
    section.dynamic_node_lhs = file_settings_->value(kDynamicNodeLhs, 0).toInt();
    section.operation = file_settings_->value(kOperation, kPlus).toString();
    section.dynamic_node_rhs = file_settings_->value(kDynamicNodeRhs, 0).toInt();
    section.default_unit = file_settings_->value(kDefaultUnit, 0).toInt();
    section.document_path = file_settings_->value(kDocumentPath, {}).toString();
    section.date_format = file_settings_->value(kDateFormat, kDateTimeFST).toString();
    section.amount_decimal = file_settings_->value(kAmountDecimal, 2).toInt();
    section.common_decimal = file_settings_->value(kCommonDecimal, 2).toInt();

    file_settings_->endGroup();
}

void MainWindow::on_actionSearch_triggered()
{
    auto* dialog { new Search(node_widget_->Model(), stakeholder_tree_->Model(), product_tree_->Model(), section_config_, data_->sql, data_->info, this) };
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
            CreateLeafFPTS(node_widget_->Model(), trans_wgt_hash_, data_, section_config_, id);
        break;
    case Section::kFinance:
    case Section::kProduct:
    case Section::kTask:
        if (!Contains(lhs_node_id) && !Contains(rhs_node_id))
            CreateLeafFPTS(node_widget_->Model(), trans_wgt_hash_, data_, section_config_, id);
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

    auto* preference { new Preferences(data_->info, model, app_config_, file_config_, *section_config_, this) };
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
        dialog = new Licence(license_settings_, license_config_, this);
        dialog->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);
        connect(dialog, &QDialog::finished, [=]() { dialog = nullptr; });
    }

    dialog->show();
    dialog->activateWindow();
}

void MainWindow::on_actionNewDatabase_triggered()
{
    auto* new_database_dlg { new NewDatabase(this) };
    new_database_dlg->exec();
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
            MainWindowUtils::ExportExcel(data_->info.node, book1->GetCurrentWorksheet());

            book1->AppendSheet(data_->info.path);
            book1->GetCurrentWorksheet()->WriteRow(1, 1, list);
            MainWindowUtils::ExportExcel(data_->info.path, book1->GetCurrentWorksheet(), false);

            book1->AppendSheet(data_->info.trans);
            book1->GetCurrentWorksheet()->WriteRow(1, 1, data_->info.excel_trans_header);
            const bool where { start_ == Section::kStakeholder ? false : true };
            MainWindowUtils::ExportExcel(data_->info.trans, book1->GetCurrentWorksheet(), where);

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
