#include "statementwidget.h"

#include "component/signalblocker.h"
#include "ui_statementwidget.h"

StatementWidget::StatementWidget(NodeModel* model, CInfo& info, CSettings& settings, QWidget* parent)
    : SupportWidget(parent)
    , ui(new Ui::StatementWidget)
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

    ui->tableViewStatement->setModel(model);
}

StatementWidget::~StatementWidget() { delete ui; }

QPointer<QTableView> StatementWidget::View() const { return ui->tableViewStatement; }

void StatementWidget::on_start_dateChanged(const QDate& date)
{
    ui->pBtnRefresh->setEnabled(date <= end_.date());
    start_.setDate(date);
}

void StatementWidget::on_end_dateChanged(const QDate& date)
{
    ui->pBtnRefresh->setEnabled(date >= start_.date());
    end_.setDate(date);
}

void StatementWidget::on_pBtnRefresh_clicked() { model_->UpdateTree(start_, end_); }

void StatementWidget::RUnitGroupClicked(int id) { }

void StatementWidget::IniUnitGroup()
{
    unit_group_ = new QButtonGroup(this);
    unit_group_->addButton(ui->rBtnIS, 0);
    unit_group_->addButton(ui->rBtnMS, 1);
    unit_group_->addButton(ui->rBtnPEND, 2);
    unit_group_->addButton(ui->rBtnAll, 3);
}
