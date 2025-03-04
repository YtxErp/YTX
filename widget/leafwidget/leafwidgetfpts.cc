#include "leafwidgetfpts.h"

#include "ui_leafwidgetfpts.h"

LeafWidgetFPTS::LeafWidgetFPTS(TableModel* model, QWidget* parent)
    : LeafWidget(parent)
    , ui(new Ui::LeafWidgetFPTS)
    , model_ { model }
{
    ui->setupUi(this);
    ui->tableView->setModel(model);
}

LeafWidgetFPTS::~LeafWidgetFPTS()
{
    delete model_;
    delete ui;
}

QPointer<QTableView> LeafWidgetFPTS::View() const { return ui->tableView; }
