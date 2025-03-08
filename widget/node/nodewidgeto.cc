#include "nodewidgeto.h"

#include "component/signalblocker.h"
#include "ui_nodewidgeto.h"

NodeWidgetO::NodeWidgetO(NodeModel* model, CInfo& info, CSettings& settings, QWidget* parent)
    : NodeWidget(parent)
    , ui(new Ui::NodeWidgetO)
    , start_ { QDateTime(QDate::currentDate(), kStartTime) }
    , end_ { QDateTime(QDate::currentDate(), kEndTime) }
    , model_ { static_cast<NodeModelO*>(model) }
    , info_ { info }
    , settings_ { settings }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    ui->start->setDisplayFormat(kDateFST);
    ui->end->setDisplayFormat(kDateFST);

    ui->start->setDateTime(start_);
    ui->end->setDateTime(end_);

    ui->treeViewOrder->setModel(model);
}

NodeWidgetO::~NodeWidgetO() { delete ui; }

QPointer<QTreeView> NodeWidgetO::View() const { return ui->treeViewOrder; }

void NodeWidgetO::on_start_dateChanged(const QDate& date)
{
    ui->pBtnRefresh->setEnabled(date <= end_.date());
    start_.setDate(date);
}

void NodeWidgetO::on_end_dateChanged(const QDate& date)
{
    ui->pBtnRefresh->setEnabled(date >= start_.date());
    end_.setDate(date);
}

void NodeWidgetO::on_pBtnRefresh_clicked() { model_->UpdateTree(start_, end_); }
