#include "newdatabase.h"

#include <QMessageBox>

#include "database/postgresql.h"
#include "global/pgconnection.h"
#include "ui_newdatabase.h"

NewDatabase::NewDatabase(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::NewDatabase)
{
    ui->setupUi(this);
}

NewDatabase::~NewDatabase() { delete ui; }

void NewDatabase::on_pushButtonCreate_clicked()
{
    const auto user { ui->lineEditUser->text() };
    const auto password { ui->lineEditPassword->text() };
    const auto database { ui->lineEditDatabase->text() };

    auto db { PGConnection::Instance().GetConnection() };

    // Step 1: Create role
    if (!PostgreSql::CreateRole(db, user, password)) {
        QMessageBox::warning(this, "Creation Failed", "Failed to create role. Please check permissions or role name validity.");
        PGConnection::Instance().ReturnConnection(db);
        return;
    }

    // Step 2: Create database owned by the new role
    if (!PostgreSql::CreateDatabase(db, database, user)) {
        QMessageBox::warning(this, "Creation Failed", "Failed to create database. Please check permissions or database name validity.");
        PGConnection::Instance().ReturnConnection(db);
        return;
    }

    // Step 3: Connect to the new database and initialize schema
    auto new_db { PostgreSql::SingleConnection(user, password, database, "on_pushButtonCreate_clicked", 3000) };
    PostgreSql::CreateSchema(new_db);
    PostgreSql::RemoveConnection(new_db.connectionName());

    // Return the original connection to the pool
    PGConnection::Instance().ReturnConnection(db);

    // Step 4: Notify user
    QMessageBox::information(this, "Success", "Role and database were created successfully.");
    close();
}
