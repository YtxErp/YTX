#include "supportwidgetfpts.h"

#include "ui_supportwidgetfpts.h"

SupportWidgetFPTS::SupportWidgetFPTS(QAbstractItemModel* model, QWidget* parent)
    : SupportWidget(parent)
    , ui(new Ui::SupportWidgetFPTS)
    , model_ { model }
{
    ui->setupUi(this);
    ui->tableView->setModel(model);
}

SupportWidgetFPTS::~SupportWidgetFPTS()
{
    delete model_;
    delete ui;
}

QPointer<QTableView> SupportWidgetFPTS::View() const { return ui->tableView; }
