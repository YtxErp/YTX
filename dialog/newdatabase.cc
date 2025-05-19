#include "newdatabase.h"

#include "global/pgconnectionpool.h"
#include "ui_newdatabase.h"

NewDatabase::NewDatabase(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::NewDatabase)
{
    ui->setupUi(this);
}

NewDatabase::~NewDatabase() { delete ui; }

void NewDatabase::on_pushButtonCreate_clicked() { auto db { PGConnectionPool::Instance().GetConnection() }; }
