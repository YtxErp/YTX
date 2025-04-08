#include "mainwindowutils.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
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
    QString command { "D:/Qt/6.8.3/llvm-mingw_64/bin/rcc.exe" };
    QStringList arguments {};
    arguments << "-binary"
              << "E:/YTX/resource/resource.qrc"
              << "-o" << path;

    QProcess process {};

    // 启动终端并执行命令
    process.start(command, arguments);
    process.waitForFinished();
#endif

#elif defined(Q_OS_MACOS)
    path = QCoreApplication::applicationDirPath() + "/../Resources/resource.brc";

#if 0
    QString command { QDir::homePath() + "/Qt6.8/6.8.3/macos/libexec/rcc" + " -binary " + QDir::homePath() + "/Documents/YTX/resource/resource.qrc -o "
        + path };

    QProcess process {};
    process.start("zsh", QStringList() << "-c" << command);
    process.waitForFinished();
#endif

#endif

    return path;
}

QVariantList MainWindowUtils::SaveTab(CTransWgtHash& trans_wgt_hash)
{
    if (trans_wgt_hash.isEmpty())
        return {};

    const auto keys { trans_wgt_hash.keys() };
    QVariantList list {};

    for (int node_id : keys)
        list.emplaceBack(node_id);

    return list;
}

QSet<int> MainWindowUtils::ReadSettings(std::shared_ptr<QSettings> settings, CString& section, CString& property)
{
    assert(settings && "settings must be non-null");

    auto variant { settings->value(QString("%1/%2").arg(section, property)) };

    if (!variant.isValid() || !variant.canConvert<QVariantList>())
        return {};

    QSet<int> set {};
    const auto variant_list { variant.value<QVariantList>() };

    for (const auto& node_id : variant_list)
        set.insert(node_id.toInt());

    return set;
}

void MainWindowUtils::ReadPrintTmplate(QMap<QString, QString>& print_template)
{
#ifdef Q_OS_MAC
    constexpr auto folder_name { "../Resources/print" };
#elif defined(Q_OS_WIN32)
    constexpr auto folder_name { "print" };
#else
    return;
#endif

    const QString folder_path { QCoreApplication::applicationDirPath() + QDir::separator() + QString::fromUtf8(folder_name) };
    QDir dir(folder_path);

    if (!dir.exists()) {
        return;
    }

    const QStringList name_filters { "*.ini" };
    const QDir::Filters entry_filters { QDir::Files | QDir::NoSymLinks };
    const QFileInfoList file_list { dir.entryInfoList(name_filters, entry_filters) };

    for (const auto& fileInfo : file_list) {
        print_template.insert(fileInfo.baseName(), fileInfo.absoluteFilePath());
    }
}

void MainWindowUtils::WriteSettings(std::shared_ptr<QSettings> settings, const QVariant& value, CString& section, CString& property)
{
    assert(settings && "settings must be non-null");
    settings->setValue(QString("%1/%2").arg(section, property), value);
}

#if 0
bool MainWindowUtils::CopyFile(CString& source, QString& destination)
{
    if (!CheckFileValid(source, kSuffixYTX)) {
        return false;
    }

    if (!PrepareNewFile(destination, kDotSuffixYTX)) {
        return false;
    }

    QSqlDatabase db;
    if (AddDatabase(db, source, "copy_file"))
        return false;

    if (!db.open()) {
        qDebug() << "Failed to open source database!";
        return false;
    }

    QString string = QString("VACUUM INTO '%1'").arg(destination);
    QSqlQuery query(db);
    if (!query.exec(string)) {
        qDebug() << "VACUUM INTO failed:" << query.lastError().text();
    } else {
        qDebug() << "Database export complete!";
    }

    RemoveDatabase("copy_file");
    return true;
}
#endif

bool MainWindowUtils::AddDatabase(QSqlDatabase& db, CString& db_path, CString& connection_name)
{
    if (QSqlDatabase::contains(connection_name)) {
        db = QSqlDatabase::database(connection_name);

        if (db.isOpen()) {
            return true;
        }

        if (db.open()) {
            return true;
        }

        QSqlDatabase::removeDatabase(connection_name);
    }

    db = QSqlDatabase::addDatabase("QSQLITE", connection_name);
    db.setDatabaseName(db_path);

    if (!db.open()) {
        qDebug() << "Failed in AddDatabase:" << db_path;
        QSqlDatabase::removeDatabase(connection_name);
        return false;
    }

    return true;
}

QSqlDatabase MainWindowUtils::GetDatabase(CString& connection_name)
{
    if (QSqlDatabase::contains(connection_name)) {
        QSqlDatabase db { QSqlDatabase::database(connection_name) };
        if (db.isOpen()) {
            return db;
        }
    }

    return QSqlDatabase();
}

void MainWindowUtils::RemoveDatabase(CString& connection_name)
{
    {
        QSqlDatabase db = QSqlDatabase::database(connection_name);
        if (db.isOpen()) {
            db.close();
        }
    }

    QSqlDatabase::removeDatabase(connection_name);
}

bool MainWindowUtils::CheckFileValid(CString& file_path, CString& suffix)
{
    if (file_path.isEmpty())
        return false;

    const QFileInfo file_info(file_path);

    if (!file_info.exists() || !file_info.isFile()) {
        Message(QMessageBox::Critical, QObject::tr("Invalid File"), QObject::tr("The specified file does not exist or is not a valid file:\n%1").arg(file_path),
            kThreeThousand);
        return false;
    }

    if (file_info.suffix().compare(suffix, Qt::CaseInsensitive) != 0) {
        Message(QMessageBox::Critical, QObject::tr("Extension Mismatch"),
            QObject::tr("The file extension does not match the expected type:\n%1").arg(file_path), kThreeThousand);
        return false;
    }

    if (!CheckFileSQLite(file_path)) {
        Message(
            QMessageBox::Critical, QObject::tr("Invalid Database"), QObject::tr("The file is not a valid SQLite database:\n%1").arg(file_path), kThreeThousand);
        return false;
    }

    return true;
}

bool MainWindowUtils::PrepareNewFile(QString& file_path, CString& suffix)
{
    if (file_path.isEmpty())
        return false;

    if (!file_path.endsWith(suffix, Qt::CaseInsensitive))
        file_path += suffix;

    if (QFile::exists(file_path)) {
        qDebug() << "Destination file already exists. Overwriting:" << file_path;
        QFile::remove(file_path);
    }

    return true;
}

bool MainWindowUtils::CheckFileSQLite(CString& file_path)
{
    if (file_path.isEmpty())
        return false;

    QFile file(file_path);

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed in CheckFileSQLite:" << file_path;
        return false;
    }

    QByteArray file_header { file.read(16) };
    file.close();

    return file_header.startsWith("SQLite");
}

void MainWindowUtils::ExportYTX(CString& source, CString& destination, CStringList& table_names, CStringList& columns)
{
    if (!CheckFileValid(source) || !CheckFileValid(destination)) {
        return;
    }

    QSqlDatabase source_db = GetDatabase(kSourceConnection);
    if (!source_db.isValid())
        return;

    QSqlDatabase destination_db = GetDatabase(kDestinationConnection);
    if (!destination_db.isValid())
        return;

    QSqlQuery source_query(source_db);
    QSqlQuery destination_query(destination_db);

    const QString column_names { columns.join(", ") };

    for (CString& name : table_names) {
        const auto select_query { QString("SELECT %1 FROM %2;").arg(column_names, name) };

        if (!source_query.exec(select_query)) {
            qDebug() << "Failed to execute SELECT query for table:" << name << source_query.lastError().text();
            return;
        }

        destination_query.exec(QStringLiteral("BEGIN TRANSACTION;"));

        while (source_query.next()) {
            QVariantList values;
            for (int i = 0; i < columns.size(); ++i) {
                values.append(source_query.value(i).toString());
            }

            const auto insert_query { QString("INSERT INTO %1 (%2) VALUES (%3);").arg(name, column_names, GeneratePlaceholder(values)) };

            if (!destination_query.exec(insert_query)) {
                qDebug() << "Failed to insert data into destination database for table:" << name << destination_query.lastError().text();
                destination_query.exec(QStringLiteral("ROLLBACK;"));
                return;
            }
        }

        destination_query.exec(QStringLiteral("COMMIT;"));
    }
}

void MainWindowUtils::ExportExcel(CString& source, CString& table, QSharedPointer<YXlsx::Worksheet> worksheet, bool where)
{
    if (!CheckFileValid(source) || !worksheet) {
        return;
    }

    QSqlDatabase source_db = GetDatabase(kSourceConnection);
    if (!source_db.isValid())
        return;

    QSqlQuery source_query(source_db);
    QString select_query = QString("SELECT * FROM %1 WHERE removed = 0;").arg(table);

    if (!where)
        select_query = QString("SELECT * FROM %1;").arg(table);

    if (!source_query.exec(select_query)) {
        qDebug() << "Failed to execute SELECT query for table in ExportExcel" << source_query.lastError().text();
        return;
    }

    const int column { source_query.record().count() };
    QList<QVariantList> list(column);

    while (source_query.next()) {
        for (int col = 0; col != column; ++col) {
            list[col].append(source_query.value(col));
        }
    }

    for (int col = 0; col != column; ++col) {
        worksheet->WriteColumn(2, col + 1, list.at(col));
    }
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
