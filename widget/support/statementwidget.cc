#include "statementwidget.h"

#include "component/constvalue.h"
#include "component/signalblocker.h"
#include "ui_statementwidget.h"

StatementWidget::StatementWidget(StatementModel* model, QWidget* parent)
    : SupportWidget(parent)
    , ui(new Ui::StatementWidget)
    , start_ { QDateTime(QDate(QDate::currentDate().year(), QDate::currentDate().month(), 1), kStartTime) }
    , end_ { QDateTime(QDate(QDate::currentDate().year(), QDate::currentDate().month(), QDate::currentDate().daysInMonth()), kEndTime) }
    , unit_ { UnitO::kMS }
    , model_ { model }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);
    IniUnitGroup();
    IniWidget(model);
    IniData();
    IniConnect();
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

void StatementWidget::on_pBtnRefresh_clicked() { model_->Query(start_, end_, unit_); }

void StatementWidget::RUnitGroupClicked(int id) { unit_ = UnitO(id); }

void StatementWidget::IniUnitGroup()
{
    unit_group_ = new QButtonGroup(this);
    unit_group_->addButton(ui->rBtnIS, 0);
    unit_group_->addButton(ui->rBtnMS, 1);
    unit_group_->addButton(ui->rBtnPEND, 2);
}

void StatementWidget::IniConnect() { connect(unit_group_, &QButtonGroup::idClicked, this, &StatementWidget::RUnitGroupClicked); }

void StatementWidget::IniWidget(StatementModel* model)
{
    ui->start->setDisplayFormat(kDateFST);
    ui->end->setDisplayFormat(kDateFST);
    ui->rBtnMS->setChecked(true);
    ui->tableViewStatement->setModel(model);
    ui->start->setDateTime(start_);
    ui->end->setDateTime(end_);

    ui->pBtnRefresh->setFocus();
}

void StatementWidget::IniData() { model_->Query(start_, end_, unit_); }

void StatementWidget::on_tableViewStatement_doubleClicked(const QModelIndex& index)
{
    const int kParty { index.siblingAtColumn(std::to_underlying(StatementEnum::kParty)).data().toInt() };
    const double pbalance { index.siblingAtColumn(std::to_underlying(StatementEnum::kPBalance)).data().toDouble() };
    const double cbalance { index.siblingAtColumn(std::to_underlying(StatementEnum::kCBalance)).data().toDouble() };

    if (index.column() == std::to_underlying(StatementEnum::kParty)) {
        emit SPrimaryStatement(kParty, start_, end_, pbalance, cbalance);
    }

    if (index.column() == std::to_underlying(StatementEnum::kCBalance)) {
        emit SSecondaryStatement(kParty, start_, end_, pbalance, cbalance);
    }
}
