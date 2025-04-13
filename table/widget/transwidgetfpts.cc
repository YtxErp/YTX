#include "transwidgetfpts.h"

#include "ui_transwidgetfpts.h"

TransWidgetFPTS::TransWidgetFPTS(TransModel* model, QWidget* parent)
    : TransWidget(parent)
    , ui(new Ui::TransWidgetFPTS)
    , model_ { model }
{
    ui->setupUi(this);
    ui->tableView->setModel(model);
}

TransWidgetFPTS::~TransWidgetFPTS()
{
    delete model_;
    delete ui;
}

QPointer<QTableView> TransWidgetFPTS::View() const { return ui->tableView; }
