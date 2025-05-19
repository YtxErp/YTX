#include "pgconnectionpool.h"

#include <QDebug>
#include <QSqlError>

PGConnectionPool& PGConnectionPool::Instance()
{
    static PGConnectionPool instance;
    return instance;
}

PGConnectionPool::~PGConnectionPool()
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

bool PGConnectionPool::Initialize(const QString& user, const QString& password, const QString& db_name, int pool_size)
{
    QMutexLocker locker(&mutex_);
    Reset();

    for (int i = 0; i != pool_size; ++i) {
        const QString conn_name { QString("ytx_pg_conn_%1").arg(i) };
        QSqlDatabase db { QSqlDatabase::addDatabase("QPSQL", conn_name) };
        db.setHostName("localhost");
        db.setPort(5432);
        db.setUserName(user);
        db.setPassword(password);
        db.setDatabaseName(db_name);

        if (!db.open()) {
            qCritical() << "Failed to open connection:" << db.lastError().text();
            Reset();
            return false;
        }

        available_dbs_.enqueue(db);
    }

    return true;
}

QSqlDatabase PGConnectionPool::GetConnection()
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

void PGConnectionPool::ReturnConnection(const QSqlDatabase& db)
{
    QMutexLocker locker(&mutex_);
    const QString name { db.connectionName() };
    if (used_names_.remove(name)) {
        available_dbs_.enqueue(db);
    }
}

void PGConnectionPool::Reset()
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
