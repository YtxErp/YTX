#include "refwidget.h"

#include <QTimer>

#include "component/constvalue.h"
#include "component/signalblocker.h"
#include "ui_refwidget.h"

RefWidget::RefWidget(QAbstractItemModel* model, int node_id, CDateTime& start, CDateTime& end, QWidget* parent)
    : ReportWidget(parent)
    , ui(new Ui::RefWidget)
    , start_ { start }
    , end_ { end }
    , node_id_ { node_id }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);
    IniWidget(model);

    QTimer::singleShot(0, this, [this]() { emit SRetrieveData(node_id_, start_, end_); });
}

RefWidget::~RefWidget() { delete ui; }

QPointer<QTableView> RefWidget::View() const { return ui->tableView; }

QPointer<QAbstractItemModel> RefWidget::Model() const { return ui->tableView->model(); }

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

void RefWidget::on_pBtnRefresh_clicked() { emit SRetrieveData(node_id_, start_, end_); }

void RefWidget::IniWidget(QAbstractItemModel* model)
{
    ui->start->setDisplayFormat(kDateFST);
    ui->end->setDisplayFormat(kDateFST);
    ui->tableView->setModel(model);
    ui->start->setDateTime(start_);
    ui->end->setDateTime(end_);

    ui->pBtnRefresh->setFocus();
}
