#include "statementwidget.h"

#include <QTimer>

#include "component/constvalue.h"
#include "component/enumclass.h"
#include "component/signalblocker.h"
#include "ui_statementwidget.h"

StatementWidget::StatementWidget(QAbstractItemModel* model, int unit, bool enable_excel, CDateTime& start, CDateTime& end, QWidget* parent)
    : ReportWidget(parent)
    , ui(new Ui::StatementWidget)
    , unit_ { unit }
    , start_ { start }
    , end_ { end }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);
    IniUnitGroup();
    IniWidget(model, enable_excel);
    IniUnit(unit);
    IniConnect();

    QTimer::singleShot(0, this, [this]() { emit SResetModel(unit_, start_, end_); });
}

StatementWidget::~StatementWidget() { delete ui; }

QPointer<QTableView> StatementWidget::View() const { return ui->tableView; }

QPointer<QAbstractItemModel> StatementWidget::Model() const { return ui->tableView->model(); }

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

void StatementWidget::on_pBtnRefresh_clicked() { emit SResetModel(unit_, start_, end_); }

void StatementWidget::RUnitGroupClicked(int id) { unit_ = id; }

void StatementWidget::IniUnitGroup()
{
    unit_group_ = new QButtonGroup(this);
    unit_group_->addButton(ui->rBtnIS, 0);
    unit_group_->addButton(ui->rBtnMS, 1);
    unit_group_->addButton(ui->rBtnPEND, 2);
}

void StatementWidget::IniConnect() { connect(unit_group_, &QButtonGroup::idClicked, this, &StatementWidget::RUnitGroupClicked); }

void StatementWidget::IniUnit(int unit)
{
    const UnitO kUnit { unit };

    switch (kUnit) {
    case UnitO::kIS:
        ui->rBtnIS->setChecked(true);
        break;
    case UnitO::kMS:
        ui->rBtnMS->setChecked(true);
        break;
    case UnitO::kPEND:
        ui->rBtnPEND->setChecked(true);
        break;
    default:
        break;
    }
}

void StatementWidget::IniWidget(QAbstractItemModel* model, bool enable_excel)
{
    ui->start->setDisplayFormat(kDateFST);
    ui->end->setDisplayFormat(kDateFST);

    ui->tableView->setModel(model);
    ui->pBtnRefresh->setFocus();

    ui->start->setDateTime(start_);
    ui->end->setDateTime(end_);

    ui->pBtnExport->setEnabled(enable_excel);
}

void StatementWidget::on_tableView_doubleClicked(const QModelIndex& index)
{
    const int kParty { index.siblingAtColumn(std::to_underlying(StatementEnum::kParty)).data().toInt() };

    if (index.column() == std::to_underlying(StatementEnum::kParty)) {
        emit SStatementPrimary(kParty, unit_, start_, end_);
    }

    if (index.column() == std::to_underlying(StatementEnum::kCSettlement)) {
        emit SStatementSecondary(kParty, unit_, start_, end_);
    }
}

void StatementWidget::on_pBtnExport_clicked() { emit SExport(unit_, start_, end_); }
