#include "pgconnection.h"

#include <QDebug>
#include <QSqlError>

PGConnection& PGConnection::Instance()
{
    static PGConnection instance;
    return instance;
}

bool PGConnection::InitConnection(CString& user, CString& password, CString& db_name)
{
    if (is_initialized_) {
        ResetConnection();
    }

    db_ = QSqlDatabase::addDatabase("QPSQL", QSqlDatabase::defaultConnection);
    db_.setHostName("localhost");
    db_.setPort(5432);

    db_.setUserName(user);
    db_.setPassword(password);
    db_.setDatabaseName(db_name);

    if (!db_.open()) {
        qDebug() << "Failed to open connection to database" << db_name;
        QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
        return false;
    }

    is_initialized_ = true;
    return true;
}

bool PGConnection::ResetConnection()
{
    if (db_.isOpen()) {
        db_.close();
    }

    const QString conn_name = db_.connectionName();

    if (QSqlDatabase::contains(conn_name)) {
        QSqlDatabase::removeDatabase(conn_name);
    }

    is_initialized_ = false;
    return true;
}

PGConnection::PGConnection() { }

PGConnection::~PGConnection()
{
    if (db_.isOpen()) {
        db_.close();
    }
}

QSqlDatabase* PGConnection::GetConnection()
{
    if (!is_initialized_) {
        qCritical() << ("❌ Database is not initialized yet, please call SetDatabaseName() first");
        throw std::runtime_error("Database is not initialized yet, please call SetDatabaseName() first");
    }

    return &db_;
}
