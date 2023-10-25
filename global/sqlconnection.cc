#include "sqlconnection.h"

#include <QDebug>
#include <QFileInfo>
#include <QSqlError>

SqlConnection& SqlConnection::Instance()
{
    static SqlConnection instance {};
    return instance;
}

bool SqlConnection::SetDatabaseName(const QString& file_path)
{
    QMutexLocker locker(&mutex_);

    if (is_initialized_) {
        throw std::runtime_error("Database has already been initialized.");
    }

    QFileInfo file_info(file_path);
    if (!file_info.exists() || !file_info.isFile() || file_info.suffix().toLower() != "ytx") {
        LogError("Invalid file path: must be an existing .ytx file!");
        throw std::invalid_argument("Invalid file path: must be an existing .ytx file.");
    }

    auto db { OpenDatabase(file_path, Section::kFinance) };
    hash_.insert(Section::kFinance, db);
    file_path_ = file_path;
    is_initialized_ = true;

    return true;
}

QSqlDatabase* SqlConnection::Allocate(Section section)
{
    QMutexLocker locker(&mutex_);

    if (!is_initialized_) {
        LogError("Database file path has not been initialized.");
        throw std::runtime_error("Database file path has not been initialized.");
    }

    if (!hash_.contains(section)) {
        auto db { OpenDatabase(file_path_, section) };
        hash_.insert(section, db);
    }

    return &hash_[section];
}

SqlConnection::~SqlConnection()
{
    for (auto& db : hash_)
        if (db.isOpen())
            db.close();

    hash_.clear();
}

void SqlConnection::LogError(const QString& message) const { qCritical() << message; }

QSqlDatabase SqlConnection::OpenDatabase(const QString& file_path, Section section)
{
    auto db { QSqlDatabase::addDatabase("QSQLITE", QString::number(std::to_underlying(section))) };
    db.setDatabaseName(file_path);

    if (!db.open()) {
        LogError("Failed to open database connection: " + db.lastError().text());
        throw std::runtime_error("Failed to open database connection: " + db.lastError().text().toStdString());
    }

    return db;
}
