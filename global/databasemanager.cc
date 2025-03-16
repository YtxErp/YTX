#include "databasemanager.h"

#include <QDebug>
#include <QFileInfo>
#include <QSqlError>

#include "mainwindowutils.h"

DatabaseManager& DatabaseManager::Instance()
{
    static DatabaseManager instance; // **Singleton instance**
    return instance;
}

DatabaseManager::DatabaseManager()
{
    db_ = QSqlDatabase::addDatabase("QSQLITE"); // **Create a global SQLite connection**
}

DatabaseManager::~DatabaseManager()
{
    if (db_.isOpen()) {
        db_.close();
    }
}

bool DatabaseManager::SetDatabaseName(const QString& file_path)
{
    if (is_initialized_) {
        qDebug() << "⚠️ Database is already initialized, no need to reinitialize";
        return false;
    }

    if (!MainWindowUtils::CheckFileValid(file_path))
        return false;

    db_.setDatabaseName(file_path);
    if (!db_.open()) {
        LogError("❌ Failed to open SQLite database: " + db_.lastError().text());
        return false;
    }

    file_path_ = file_path;
    is_initialized_ = true;
    return true;
}

QSqlDatabase* DatabaseManager::GetDatabase()
{
    if (!is_initialized_) {
        LogError("❌ Database is not initialized yet, please call SetDatabaseName() first");
        throw std::runtime_error("Database is not initialized yet, please call SetDatabaseName() first");
    }

    return &db_;
}

void DatabaseManager::LogError(const QString& message) const { qCritical() << message; }
