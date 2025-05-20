#include "pgconnection.h"

#include <QDebug>
#include <QSqlError>

PGConnection& PGConnection::Instance()
{
    static PGConnection instance;
    return instance;
}

PGConnection::~PGConnection()
{
    QMutexLocker locker(&mutex_);

    for (auto& name : std::as_const(used_names_)) {
        QSqlDatabase db { QSqlDatabase::database(name) };
        if (db.isOpen()) {
            db.close();
        }
    }

    while (!available_dbs_.isEmpty()) {
        QSqlDatabase db { available_dbs_.dequeue() };
        if (db.isOpen()) {
            db.close();
        }
    }

    used_names_.clear();
}

bool PGConnection::Initialize(CString& host, int port, CString& user, CString& password, CString& db_name, int pool_size)
{
    QMutexLocker locker(&mutex_);
    Reset();

    for (int i = 0; i != pool_size; ++i) {
        CString conn_name { QString("ytx_pg_conn_%1").arg(i) };
        QSqlDatabase db { QSqlDatabase::addDatabase("QPSQL", conn_name) };
        db.setHostName(host);
        db.setPort(port);
        db.setUserName(user);
        db.setPassword(password);
        db.setDatabaseName(db_name);

        if (!db.open()) {
            qWarning() << "Failed to open PG connection:" << db.lastError().text();
            db = QSqlDatabase();
            QSqlDatabase::removeDatabase(conn_name);
            continue;
        }

        available_dbs_.enqueue(db);
    }

    return available_dbs_.size() >= 1;
}

QSqlDatabase PGConnection::GetConnection()
{
    QMutexLocker locker(&mutex_);
    if (available_dbs_.isEmpty()) {
        qCritical() << "PGConnectionPool exhausted!";
        return {};
    }

    QSqlDatabase db { available_dbs_.dequeue() };
    used_names_.insert(db.connectionName());
    return db;
}

void PGConnection::ReturnConnection(const QSqlDatabase& db)
{
    QMutexLocker locker(&mutex_);
    CString name { db.connectionName() };
    if (used_names_.remove(name)) {
        available_dbs_.enqueue(db);
    }
}

void PGConnection::Reset()
{
    for (const auto& name : std::as_const(used_names_)) {
        {
            QSqlDatabase db { QSqlDatabase::database(name, false) };
            if (db.isOpen()) {
                db.close();
            }
        }

        QSqlDatabase::removeDatabase(name);
    }

    while (!available_dbs_.isEmpty()) {
        QSqlDatabase db { available_dbs_.dequeue() };
        if (db.isOpen()) {
            db.close();
        }

        QSqlDatabase::removeDatabase(db.connectionName());
    }

    used_names_.clear();
}
