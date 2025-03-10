#include "refwidget.h"

#include "component/constvalue.h"
#include "component/signalblocker.h"
#include "ui_refwidget.h"

RefWidget::RefWidget(TransRefModel* model, int node_id, QWidget* parent)
    : SupportWidget(parent)
    , ui(new Ui::RefWidget)
    , start_ { QDateTime(QDate(QDate::currentDate().year() - 1, 1, 1), kStartTime) }
    , end_ { QDateTime(QDate(QDate::currentDate().year(), 12, 31), kEndTime) }
    , model_ { model }
    , node_id_ { node_id }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);
    IniWidget(model);
    IniData();
}

RefWidget::~RefWidget() { delete ui; }

QPointer<QTableView> RefWidget::View() const { return ui->tableView; }

void RefWidget::on_start_dateChanged(const QDate& date)
{
    ui->pBtnRefresh->setEnabled(date <= end_.date());
    start_.setDate(date);
}

void RefWidget::on_end_dateChanged(const QDate& date)
{
    ui->pBtnRefresh->setEnabled(date >= start_.date());
    end_.setDate(date);
}

void RefWidget::on_pBtnRefresh_clicked() { model_->Query(node_id_, start_, end_); }

void RefWidget::IniWidget(TransRefModel* model)
{
    ui->start->setDisplayFormat(kDateFST);
    ui->end->setDisplayFormat(kDateFST);
    ui->tableView->setModel(model);
    ui->start->setDateTime(start_);
    ui->end->setDateTime(end_);

    ui->pBtnRefresh->setFocus();
}

void RefWidget::IniData() { model_->Query(node_id_, start_, end_); }
