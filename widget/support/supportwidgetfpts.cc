#include "supportwidgetfpts.h"

#include "ui_supportwidgetfpts.h"

SupportWidgetFPTS::SupportWidgetFPTS(QAbstractItemModel* model, QWidget* parent)
    : SupportWidget(parent)
    , ui(new Ui::SupportWidgetFPTS)
{
    ui->setupUi(this);
    ui->tableView->setModel(model);
}

SupportWidgetFPTS::~SupportWidgetFPTS() { delete ui; }

QPointer<QAbstractItemModel> SupportWidgetFPTS::Model() const { return ui->tableView->model(); }

QPointer<QTableView> SupportWidgetFPTS::View() const { return ui->tableView; }
