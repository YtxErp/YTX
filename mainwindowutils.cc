#include "mainwindowutils.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QSqlError>
#include <QSqlQuery>
#include <QTimer>

QString MainWindowUtils::ResourceFile()
{
    QString path {};

#ifdef Q_OS_WIN
    path = QCoreApplication::applicationDirPath() + "/resource";

    if (!QDir(path).exists() && !QDir().mkpath(path)) {
        qDebug() << "Failed to create directory:" << path;
        return {};
    }

    path += "/resource.brc";

#if 0
    QString command { "E:/Qt/6.8.2/llvm-mingw_64/bin/rcc.exe" };
    QStringList arguments {};
    arguments << "-binary"
              << "E:/Code/YTX/resource/resource.qrc"
              << "-o" << path;

    QProcess process {};

    // 启动终端并执行命令
    process.start(command, arguments);
    process.waitForFinished();
#endif

#elif defined(Q_OS_MACOS)
    path = QCoreApplication::applicationDirPath() + "/../Resources/resource.brc";

#if 0
    QString command { QDir::homePath() + "/Qt6.8/6.8.2/macos/libexec/rcc" + " -binary " + QDir::homePath() + "/Documents/YTX/resource/resource.qrc -o "
        + path };

    QProcess process {};
    process.start("zsh", QStringList() << "-c" << command);
    process.waitForFinished();
#endif

#endif

    return path;
}

QVariantList MainWindowUtils::SaveTab(CTableHash& table_hash)
{
    if (table_hash.isEmpty())
        return {};

    const auto keys { table_hash.keys() };
    QVariantList list {};

    for (int node_id : keys)
        list.emplaceBack(node_id);

    return list;
}

QSet<int> MainWindowUtils::ReadSettings(std::shared_ptr<QSettings> settings, CString& section, CString& property)
{
    if (!settings)
        return {};

    auto variant { settings->value(QString("%1/%2").arg(section, property)) };

    if (!variant.isValid() || !variant.canConvert<QVariantList>())
        return {};

    QSet<int> set {};
    const auto variant_list { variant.value<QVariantList>() };

    for (const auto& node_id : variant_list)
        set.insert(node_id.toInt());

    return set;
}

void MainWindowUtils::WriteSettings(std::shared_ptr<QSettings> settings, const QVariant& value, CString& section, CString& property)
{
    if (!settings) {
        qWarning() << "WriteTabID: Invalid parameters (settings is null)";
        return;
    }

    settings->setValue(QString("%1/%2").arg(section, property), value);
}

bool MainWindowUtils::CopyFile(CString& source, CString& destination)
{
    const QFileInfo source_info(source);
    if (!IsValidFile(source_info)) {
        qDebug() << "Invalid source file, must be an existing .ytx file:" << source;
        return false;
    }

    if (QFile::exists(destination)) {
        qDebug() << "Destination file already exists. Overwriting:" << destination;
        QFile::remove(destination);
    }

    if (!QFile::copy(source, destination)) {
        qDebug() << "Failed to copy file from:" << source << "to:" << destination;
        return false;
    }

    qDebug() << "File copied successfully from:" << source << "to:" << destination;
    return true;
}

bool MainWindowUtils::NewFile(MainwindowSqlite& sql, QString& file_path)
{
    if (file_path.isEmpty())
        return false;

    if (!file_path.endsWith(kSuffixYTX, Qt::CaseInsensitive))
        file_path += kSuffixYTX;

    if (QFile::exists(file_path)) {
        qDebug() << "Destination file already exists. Overwriting:" << file_path;
        QFile::remove(file_path);
    }

    sql.NewFile(file_path);

    return true;
}

bool MainWindowUtils::IsValidFile(const QFileInfo& file_info, CString& suffix)
{
    CString& file_path { file_info.filePath() };

    if (!file_info.exists()) {
        qDebug() << "File does not exist:" << file_path;
        return false;
    }

    if (!file_info.isFile()) {
        qDebug() << "Not a valid file:" << file_path;
        return false;
    }

    if (!suffix.isEmpty() && file_info.suffix().toLower() != suffix.toLower()) {
        qDebug() << "File extension does not match expected suffix:" << suffix;
        return false;
    }

    if (!IsSQLiteFile(file_path)) {
        qDebug() << "File is not a valid SQLite3 database:" << file_path;
        return false;
    }

    return true;
}

bool MainWindowUtils::IsSQLiteFile(CString& file_path)
{
    QFile file(file_path);

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open file for reading:" << file_path;
        return false;
    }

    QByteArray file_header { file.read(16) };
    file.close();

    return file_header.startsWith("SQLite");
}

void MainWindowUtils::ExportColumns(CString& source, CString& destination, CStringList& table_names, CStringList& columns)
{
    if (!IsValidFile(QFileInfo(source)) || !IsValidFile(QFileInfo(destination))) {
        return;
    }

    QSqlDatabase source_db { QSqlDatabase::addDatabase("QSQLITE", "source_connection") };
    source_db.setDatabaseName(source);

    if (!source_db.open()) {
        qDebug() << "Failed to open source database:" << source;
        return;
    }

    QSqlDatabase destination_db { QSqlDatabase::addDatabase("QSQLITE", "destination_connection") };
    destination_db.setDatabaseName(destination);

    if (!destination_db.open()) {
        qDebug() << "Failed to open destination database:" << destination;
        source_db.close();
        QSqlDatabase::removeDatabase("source_connection");
        return;
    }

    QSqlQuery source_query(source_db);
    QSqlQuery destination_query(destination_db);

    const QString column_names { columns.join(", ") };
    QString select_query {};

    for (CString& name : table_names) {
        select_query = QString("SELECT %1 FROM %2;").arg(column_names, name);

        if (!source_query.exec(select_query)) {
            qDebug() << "Failed to execute SELECT query for table:" << name << source_query.lastError().text();
            source_db.close();
            destination_db.close();
            QSqlDatabase::removeDatabase("source_connection");
            QSqlDatabase::removeDatabase("destination_connection");
            return;
        }

        destination_query.exec("BEGIN TRANSACTION;");
        QString insert_query {};

        while (source_query.next()) {
            QVariantList values;
            for (int i = 0; i < columns.size(); ++i) {
                values.append(source_query.value(i).toString());
            }

            insert_query = QString("INSERT INTO %1 (%2) VALUES (%3);").arg(name, column_names, GeneratePlaceholder(values));

            if (!destination_query.exec(insert_query)) {
                qDebug() << "Failed to insert data into destination database for table:" << name << destination_query.lastError().text();
                destination_query.exec("ROLLBACK;");
                source_db.close();
                destination_db.close();
                QSqlDatabase::removeDatabase("source_connection");
                QSqlDatabase::removeDatabase("destination_connection");
                return;
            }
        }

        destination_query.exec("COMMIT;");
    }

    source_db.close();
    destination_db.close();
    QSqlDatabase::removeDatabase("source_connection");
    QSqlDatabase::removeDatabase("destination_connection");

    qDebug() << "Columns copied successfully from" << source << "to" << destination;
}

void MainWindowUtils::Message(QMessageBox::Icon icon, CString& title, CString& text, int timeout)
{
    auto* box { new QMessageBox(icon, title, text, QMessageBox::NoButton) };
    QTimer::singleShot(timeout, box, &QMessageBox::accept);
    QObject::connect(box, &QMessageBox::finished, box, &QMessageBox::deleteLater);

    box->setModal(false);
    box->show();
}

QString MainWindowUtils::GeneratePlaceholder(const QVariantList& values)
{
    QStringList valuePlaceholders {};

    for (const QVariant& value : values) {
        if (value.isNull()) {
            valuePlaceholders.append("NULL");
        } else if (value.canConvert<QString>()) {
            valuePlaceholders.append("'" + value.toString().replace("'", "''") + "'");
        } else {
            valuePlaceholders.append(value.toString());
        }
    }

    return valuePlaceholders.join(", ");
}
