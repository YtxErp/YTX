#include "nodewidgeto.h"

#include "component/signalblocker.h"
#include "ui_nodewidgeto.h"

NodeWidgetO::NodeWidgetO(NodeModel* model, CInfo& info, CSettings& settings, QWidget* parent)
    : NodeWidget(parent)
    , ui(new Ui::NodeWidgetO)
    , start_ { QDate::currentDate() }
    , end_ { QDate::currentDate() }
    , model_ { static_cast<NodeModelO*>(model) }
    , info_ { info }
    , settings_ { settings }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    ui->dateEditStart->setDisplayFormat(kDateFST);
    ui->dateEditEnd->setDisplayFormat(kDateFST);

    ui->dateEditStart->setDate(start_);
    ui->dateEditEnd->setDate(end_);

    ui->treeViewOrder->setModel(model);
}

NodeWidgetO::~NodeWidgetO() { delete ui; }

QPointer<QTreeView> NodeWidgetO::View() const { return ui->treeViewOrder; }

void NodeWidgetO::on_dateEditStart_dateChanged(const QDate& date)
{
    ui->pBtnRefresh->setEnabled(date <= end_);
    start_ = date;
}

void NodeWidgetO::on_dateEditEnd_dateChanged(const QDate& date)
{
    ui->pBtnRefresh->setEnabled(date >= start_);
    end_ = date;
}

void NodeWidgetO::on_pBtnRefresh_clicked() { model_->UpdateTree(start_, end_); }
