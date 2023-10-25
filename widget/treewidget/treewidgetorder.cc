#include "treewidgetorder.h"

#include "ui_treewidgetorder.h"

TreeWidgetOrder::TreeWidgetOrder(TreeModel* model, CInfo& info, CSettings& settings, QWidget* parent)
    : TreeWidget(parent)
    , ui(new Ui::TreeWidgetOrder)
    , start_ { QDate::currentDate() }
    , end_ { QDate::currentDate() }
    , model_ { static_cast<TreeModelOrder*>(model) }
    , info_ { info }
    , settings_ { settings }
{
    ui->setupUi(this);

    ui->dateEditStart->setDisplayFormat(kDateFST);
    ui->dateEditEnd->setDisplayFormat(kDateFST);

    ui->dateEditStart->setDate(start_);
    ui->dateEditEnd->setDate(end_);

    ui->treeViewOrder->setModel(model);
}

TreeWidgetOrder::~TreeWidgetOrder() { delete ui; }

QPointer<QTreeView> TreeWidgetOrder::View() const { return ui->treeViewOrder; }

void TreeWidgetOrder::on_dateEditStart_dateChanged(const QDate& date)
{
    ui->pBtnRefresh->setEnabled(date <= end_);
    start_ = date;
}

void TreeWidgetOrder::on_dateEditEnd_dateChanged(const QDate& date)
{
    ui->pBtnRefresh->setEnabled(date >= start_);
    end_ = date;
}

void TreeWidgetOrder::on_pBtnRefresh_clicked() { model_->UpdateTree(start_, end_); }
