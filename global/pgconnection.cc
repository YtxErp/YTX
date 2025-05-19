#include "pgconnection.h"

#include <QDebug>
#include <QSqlError>

PGConnection& PGConnection::Instance()
{
    static PGConnection instance;
    return instance;
}

PGConnection::PGConnection() = default;

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

bool PGConnection::Init(const QString& user, const QString& password, const QString& db_name, int pool_size)
{
    QMutexLocker locker(&mutex_);
    user_ = user;
    password_ = password;
    db_name_ = db_name;

    for (int i = 0; i != pool_size; ++i) {
        QString conn_name = QString("ytx_pg_conn_%1").arg(++connection_counter_);
        QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", conn_name);
        db.setHostName("localhost");
        db.setPort(5432);
        db.setUserName(user_);
        db.setPassword(password_);
        db.setDatabaseName(db_name_);

        if (!db.open()) {
            qCritical() << "❌ Failed to open connection:" << db.lastError().text();
            return false;
        }

        available_dbs_.enqueue(db);
    }

    return true;
}

QSqlDatabase PGConnection::Acquire()
{
    QMutexLocker locker(&mutex_);
    if (available_dbs_.isEmpty()) {
        qCritical() << "❌ PGConnectionPool exhausted!";
        return {};
    }

    QSqlDatabase db = available_dbs_.dequeue();
    used_names_.insert(db.connectionName());
    return db;
}

void PGConnection::Release(const QSqlDatabase& db)
{
    QMutexLocker locker(&mutex_);
    const QString& name = db.connectionName();
    if (used_names_.remove(name)) {
        available_dbs_.enqueue(db);
    }
}
