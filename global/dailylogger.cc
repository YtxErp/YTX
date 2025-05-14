#include "dailylogger.h"

#include <QDateTime>
#include <QDir>
#include <QStandardPaths>

DailyLogger& DailyLogger::Instance()
{
    static DailyLogger instance {};
    return instance;
}

DailyLogger::DailyLogger()
{
    const QString log_path { QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath("logs") };
    const QString log_date { QDate::currentDate().toString("yyyy-MM-dd") };
    const QString log_name { log_date + ".log" };
    const QString file_name { QDir(log_path).filePath(log_name) };

    QDir().mkpath(log_path);

    file_.setFileName(file_name);
    if (file_.open(QIODevice::Append | QIODevice::Text)) {
        log_stream_.setDevice(&file_);
    }
}

DailyLogger::~DailyLogger()
{
    if (log_stream_.device()) {
        log_stream_.flush();
    }

    if (file_.isOpen()) {
        file_.close();
    }
}

void DailyLogger::Install() { qInstallMessageHandler(DailyLogger::MessageHandler); }

void DailyLogger::MessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    DailyLogger::Instance().HandleMessage(type, context, msg);
}

void DailyLogger::HandleMessage(QtMsgType type, const QMessageLogContext&, const QString& msg)
{
    if (!file_.isOpen())
        return;

    QString level {};
    switch (type) {
    case QtDebugMsg:
        level = "DEBUG";
        break;
    case QtInfoMsg:
        level = "INFO";
        break;
    case QtWarningMsg:
        level = "WARNING";
        break;
    case QtCriticalMsg:
        level = "CRITICAL";
        break;
    case QtFatalMsg:
        level = "FATAL";
        break;
    }

    const QString time { QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") };
    const QString formatted { QString("%1 [%2] %3").arg(time, level, msg) };

    QMutexLocker locker(&mutex_);
    log_stream_ << formatted << '\n';
    log_stream_.flush();

    fprintf(stderr, "%s\n", formatted.toLocal8Bit().constData());
    fflush(stderr);

    if (type == QtFatalMsg)
        abort();
}
