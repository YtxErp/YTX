#include "search.h"

#include <QHeaderView>

#include "component/enumclass.h"
#include "component/signalblocker.h"
#include "delegate/readonly/checkboxr.h"
#include "delegate/readonly/colorr.h"
#include "delegate/readonly/doublespinrnonezero.h"
#include "delegate/readonly/stringmapr.h"
#include "delegate/search/searchpathtabler.h"
#include "delegate/search/searchpathtreer.h"
#include "ui_search.h"

Search::Search(CNodeModel* tree, CNodeModel* stakeholder_tree, CNodeModel* product_tree, CSectionConfig* settings, Sqlite* sql, CInfo& info, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::Search)
    , sql_ { sql }
    , tree_ { tree }
    , stakeholder_tree_ { stakeholder_tree }
    , product_tree_ { product_tree }
    , settings_ { settings }
    , info_ { info }
    , section_ { info.section }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    search_tree_ = new SearchNodeModel(info, tree_, stakeholder_tree, sql, this);
    search_table_ = new SearchTransModel(info, sql, this);

    TreeViewDelegate(ui->searchViewNode, search_tree_);
    TableViewDelegate(ui->searchViewTrans, search_table_);

    IniView(ui->searchViewNode);
    IniView(ui->searchViewTrans);

    ResizeTreeColumn(ui->searchViewNode->horizontalHeader());
    ResizeTableColumn(ui->searchViewTrans->horizontalHeader());

    IniDialog();
    IniContentGroup();
    HideTreeColumn(ui->searchViewNode, info.section);
    HideTableColumn(ui->searchViewTrans, info.section);

    IniConnect();
}

Search::~Search() { delete ui; }

void Search::IniDialog()
{
    ui->rBtnNode->setChecked(true);
    ui->stackedWidget->setCurrentIndex(0);
    ui->pBtnClose->setAutoDefault(false);
    this->setWindowTitle(tr("Search"));
}

void Search::IniConnect()
{
    connect(ui->lineEdit, &QLineEdit::returnPressed, this, &Search::RSearch);
    connect(ui->searchViewNode, &QTableView::doubleClicked, this, &Search::RNodeDoubleClicked);
    connect(ui->searchViewTrans, &QTableView::doubleClicked, this, &Search::RTransDoubleClicked);
    connect(content_group_, &QButtonGroup::idClicked, this, &Search::RContentGroup);
}

void Search::HideTreeColumn(QTableView* view, Section section)
{
    view->setColumnHidden(std::to_underlying(NodeSearchEnum::kID), false);

    switch (section) {
    case Section::kFinance:
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kParty), true);
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kEmployee), true);
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kDateTime), true);
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kColor), true);
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kFirst), true);
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kSecond), true);
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kDiscount), true);
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kFinished), true);
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kDocument), true);
        break;
    case Section::kTask:
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kParty), true);
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kEmployee), true);
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kSecond), true);
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kDiscount), true);
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kFinished), true);
        break;
    case Section::kProduct:
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kParty), true);
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kEmployee), true);
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kDateTime), true);
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kDiscount), true);
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kFinished), true);
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kDocument), true);
        break;
    case Section::kStakeholder:
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kParty), true);
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kColor), true);
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kDiscount), true);
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kFinished), true);
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kInitialTotal), true);
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kDocument), true);
        break;
    case Section::kSales:
    case Section::kPurchase:
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kColor), true);
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kDocument), true);
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kCode), true);
        view->setColumnHidden(std::to_underlying(NodeSearchEnum::kNote), true);
        break;
    default:
        break;
    }
}

void Search::HideTableColumn(QTableView* view, Section section)
{
    view->setColumnHidden(std::to_underlying(TransSearchEnum::kID), false);

    switch (section) {
    case Section::kFinance:
    case Section::kTask:
    case Section::kProduct:
        view->setColumnHidden(std::to_underlying(TransSearchEnum::kDiscount), true);
        view->setColumnHidden(std::to_underlying(TransSearchEnum::kSupportID), true);
        break;
    case Section::kStakeholder:
        view->setColumnHidden(std::to_underlying(TransSearchEnum::kLhsDebit), true);
        view->setColumnHidden(std::to_underlying(TransSearchEnum::kLhsCredit), true);
        view->setColumnHidden(std::to_underlying(TransSearchEnum::kRhsRatio), true);
        view->setColumnHidden(std::to_underlying(TransSearchEnum::kRhsDebit), true);
        view->setColumnHidden(std::to_underlying(TransSearchEnum::kRhsCredit), true);
        view->setColumnHidden(std::to_underlying(TransSearchEnum::kDiscount), true);
        break;
    case Section::kPurchase:
    case Section::kSales:
        view->setColumnHidden(std::to_underlying(TransSearchEnum::kDateTime), true);
        view->setColumnHidden(std::to_underlying(TransSearchEnum::kDocument), true);
        view->setColumnHidden(std::to_underlying(TransSearchEnum::kState), true);
        view->setColumnHidden(std::to_underlying(TransSearchEnum::kLhsNode), true);
    default:
        break;
    }
}

void Search::IniContentGroup()
{
    content_group_ = new QButtonGroup(this);
    content_group_->addButton(ui->rBtnNode, 0);
    content_group_->addButton(ui->rBtnTrans, 1);
}

void Search::TreeViewDelegate(QTableView* view, SearchNodeModel* model)
{
    view->setModel(model);

    auto* unit { new StringMapR(info_.unit_map, view) };
    view->setItemDelegateForColumn(std::to_underlying(NodeSearchEnum::kUnit), unit);

    auto* rule { new StringMapR(info_.rule_map, view) };
    view->setItemDelegateForColumn(std::to_underlying(NodeSearchEnum::kRule), rule);

    auto* total { new DoubleSpinRNoneZero(settings_->amount_decimal, kCoefficient8, view) };
    view->setItemDelegateForColumn(std::to_underlying(NodeSearchEnum::kInitialTotal), total);
    view->setItemDelegateForColumn(std::to_underlying(NodeSearchEnum::kFinalTotal), total);
    view->setItemDelegateForColumn(std::to_underlying(NodeSearchEnum::kDiscount), total);

    auto* check { new CheckBoxR(view) };
    view->setItemDelegateForColumn(std::to_underlying(NodeSearchEnum::kFinished), check);

    auto* type { new StringMapR(info_.type_map, view) };
    view->setItemDelegateForColumn(std::to_underlying(NodeEnum::kType), type);

    auto* name { new SearchPathTreeR(tree_, std::to_underlying(NodeSearchEnum::kID), view) };
    view->setItemDelegateForColumn(std::to_underlying(NodeSearchEnum::kName), name);

    if (section_ == Section::kProduct || section_ == Section::kTask) {
        auto* color { new ColorR(view) };
        view->setItemDelegateForColumn(std::to_underlying(NodeSearchEnum::kColor), color);
    }

    auto* stakeholder { new SearchPathTableR(stakeholder_tree_, view) };
    view->setItemDelegateForColumn(std::to_underlying(NodeSearchEnum::kParty), stakeholder);
    view->setItemDelegateForColumn(std::to_underlying(NodeSearchEnum::kEmployee), stakeholder);

    auto* value { new DoubleSpinRNoneZero(settings_->amount_decimal, kCoefficient8, view) };
    view->setItemDelegateForColumn(std::to_underlying(NodeSearchEnum::kFirst), value);
    view->setItemDelegateForColumn(std::to_underlying(NodeSearchEnum::kSecond), value);
}

void Search::TableViewDelegate(QTableView* view, SearchTransModel* model)
{
    view->setModel(model);

    auto* value { new DoubleSpinRNoneZero(settings_->amount_decimal, kCoefficient8, view) };
    view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kLhsDebit), value);
    view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kRhsDebit), value);
    view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kLhsCredit), value);
    view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kRhsCredit), value);
    view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kDiscount), value);

    auto* ratio { new DoubleSpinRNoneZero(settings_->common_decimal, kCoefficient8, view) };
    view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kLhsRatio), ratio);
    view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kRhsRatio), ratio);

    if (section_ == Section::kFinance || section_ == Section::kTask || section_ == Section::kProduct) {
        auto* node_name { new SearchPathTableR(tree_, view) };
        view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kLhsNode), node_name);
        view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kRhsNode), node_name);
    }

    if (section_ == Section::kStakeholder) {
        auto* lhs_node_name { new SearchPathTableR(tree_, view) };
        view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kLhsNode), lhs_node_name);
        view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kSupportID), lhs_node_name);

        auto* rhs_node_name { new SearchPathTableR(product_tree_, view) };
        view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kRhsNode), rhs_node_name);
    }

    if (section_ == Section::kSales || section_ == Section::kPurchase) {
        auto* rhs_node_name { new SearchPathTableR(stakeholder_tree_, view) };
        view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kRhsNode), rhs_node_name);

        auto* lhs_node_name { new SearchPathTableR(product_tree_, view) };
        view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kRhsNode), lhs_node_name);

        auto* support_id { new SearchPathTableR(stakeholder_tree_, view) };
        view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kSupportID), support_id);
    }

    auto* state { new CheckBoxR(view) };
    view->setItemDelegateForColumn(std::to_underlying(TransSearchEnum::kState), state);
}

void Search::IniView(QTableView* view)
{
    view->setSortingEnabled(true);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setAlternatingRowColors(true);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->verticalHeader()->setHidden(true);
}

void Search::ResizeTreeColumn(QHeaderView* header)
{
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    header->setSectionResizeMode(std::to_underlying(NodeSearchEnum::kDescription), QHeaderView::Stretch);
}

void Search::ResizeTableColumn(QHeaderView* header)
{
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    header->setSectionResizeMode(std::to_underlying(TransSearchEnum::kDescription), QHeaderView::Stretch);
}

void Search::RSearch()
{
    CString kText { ui->lineEdit->text() };

    if (ui->rBtnNode->isChecked()) {
        search_tree_->Query(kText);
        ResizeTreeColumn(ui->searchViewNode->horizontalHeader());
    }

    if (ui->rBtnTrans->isChecked()) {
        search_table_->Query(kText);
        ResizeTableColumn(ui->searchViewTrans->horizontalHeader());
    }
}

void Search::RNodeDoubleClicked(const QModelIndex& index)
{
    int node_id { index.siblingAtColumn(std::to_underlying(NodeEnum::kID)).data().toInt() };
    emit SNodeLocation(node_id);
}

void Search::RTransDoubleClicked(const QModelIndex& index)
{
    int lhs_node_id { index.siblingAtColumn(std::to_underlying(TransSearchEnum::kLhsNode)).data().toInt() };
    int rhs_node_id { index.siblingAtColumn(std::to_underlying(TransSearchEnum::kRhsNode)).data().toInt() };
    int trans_id { index.siblingAtColumn(std::to_underlying(TransSearchEnum::kID)).data().toInt() };

    switch (section_) {
    case Section::kStakeholder:
    case Section::kSales:
    case Section::kPurchase:
        emit STransLocation(trans_id, lhs_node_id, 0);
        break;
    case Section::kFinance:
    case Section::kProduct:
    case Section::kTask:
        emit STransLocation(trans_id, lhs_node_id, rhs_node_id);
        break;
    default:
        break;
    }
}

void Search::RContentGroup(int id) { ui->stackedWidget->setCurrentIndex(id); }
