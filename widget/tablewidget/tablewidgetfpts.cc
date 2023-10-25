#include "tablewidgetfpts.h"

#include "ui_tablewidgetfpts.h"

TableWidgetFPTS::TableWidgetFPTS(TableModel* model, QWidget* parent)
    : TableWidget(parent)
    , ui(new Ui::TableWidgetFPTS)
    , model_ { model }
{
    ui->setupUi(this);
    ui->tableView->setModel(model);
}

TableWidgetFPTS::~TableWidgetFPTS() { delete ui; }

QPointer<QTableView> TableWidgetFPTS::View() const { return ui->tableView; }
