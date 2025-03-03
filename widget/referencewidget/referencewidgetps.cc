#include "referencewidgetps.h"

#include "ui_referencewidgetps.h"

ReferenceWidgetPS::ReferenceWidgetPS(QAbstractItemModel* model, QWidget* parent)
    : ReferenceWidget(parent)
    , ui(new Ui::ReferenceWidgetPS)
    , model_ { model }
{
    ui->setupUi(this);
    ui->tableView->setModel(model);
}

ReferenceWidgetPS::~ReferenceWidgetPS() { delete ui; }

QPointer<QTableView> ReferenceWidgetPS::View() const { return ui->tableView; }
