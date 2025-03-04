#include "reffetcherwidgetps.h"

#include "ui_reffetcherwidgetps.h"

RefFetcherWidgetPS::RefFetcherWidgetPS(QAbstractItemModel* model, QWidget* parent)
    : RefFetcherWidget(parent)
    , ui(new Ui::RefFetcherWidgetPS)
    , model_ { model }
{
    ui->setupUi(this);
    ui->tableView->setModel(model);
}

RefFetcherWidgetPS::~RefFetcherWidgetPS() { delete ui; }

QPointer<QTableView> RefFetcherWidgetPS::View() const { return ui->tableView; }
