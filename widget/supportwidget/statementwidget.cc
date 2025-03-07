#include "statementwidget.h"

#include "component/signalblocker.h"
#include "ui_statementwidget.h"

StatementWidget::StatementWidget(TreeModel* model, CInfo& info, CSettings& settings, QWidget* parent)
    : SupportWidget(parent)
    , ui(new Ui::StatementWidget)
    , start_ { QDate::currentDate() }
    , end_ { QDate::currentDate() }
    , model_ { static_cast<TreeModelOrder*>(model) }
    , info_ { info }
    , settings_ { settings }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    ui->dateEditStart->setDisplayFormat(kDateFST);
    ui->dateEditEnd->setDisplayFormat(kDateFST);

    ui->dateEditStart->setDate(start_);
    ui->dateEditEnd->setDate(end_);

    ui->tableViewStatement->setModel(model);
}

StatementWidget::~StatementWidget() { delete ui; }

QPointer<QTableView> StatementWidget::View() const { return ui->tableViewStatement; }

void StatementWidget::on_dateEditStart_dateChanged(const QDate& date)
{
    ui->pBtnRefresh->setEnabled(date <= end_);
    start_ = date;
}

void StatementWidget::on_dateEditEnd_dateChanged(const QDate& date)
{
    ui->pBtnRefresh->setEnabled(date >= start_);
    end_ = date;
}

void StatementWidget::on_pBtnRefresh_clicked() { model_->UpdateTree(start_, end_); }

void StatementWidget::RUnitGroupClicked(int id) { }

void StatementWidget::IniUnitGroup()
{
    unit_group_ = new QButtonGroup(this);
    unit_group_->addButton(ui->rBtnIS, 0);
    unit_group_->addButton(ui->rBtnMS, 1);
    unit_group_->addButton(ui->rBtnPEND, 2);
    unit_group_->addButton(ui->rBtnAll, 2);
}
