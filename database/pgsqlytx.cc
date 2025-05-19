#include "pgsqlytx.h"

#include <QSqlError>
#include <QSqlQuery>

#include "component/constvalue.h"
#if 0
CString PgSqlYtx::kPgDataDir = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath("pg_data");

CString PgSqlYtx::kPgBinBasePath =
#ifdef Q_OS_WIN
    QCoreApplication::applicationDirPath() + "/pgsql/bin/";
#elif defined(Q_OS_MAC)
    QCoreApplication::applicationDirPath() + "/../Resources/pgsql/bin/";
#else
    QString();
#endif
#endif

bool PgSqlYtx::CreateSchema(QSqlDatabase& db)
{
    const std::vector<QString> tables { NodeFinance(), Path(kFinance), TransFinance(), NodeProduct(), Path(kProduct), TransProduct(), NodeStakeholder(),
        Path(kStakeholder), TransStakeholder(), NodeTask(), Path(kTask), TransTask(), NodeOrder(kPurchase), Path(kPurchase), TransOrder(kPurchase),
        SettlementOrder(kPurchase), NodeOrder(kSales), Path(kSales), TransOrder(kSales), SettlementOrder(kSales) };

    QSqlQuery query { db };

    if (!db.transaction()) {
        qDebug() << "Error starting transaction:" << db.lastError().text();
        return false;
    }

    for (const auto& table : tables) {
        if (!query.exec(table)) {
            qDebug() << "Error executing query:" << query.lastError().text();
            db.rollback();
            return false;
        }
    }

    if (!db.commit()) {
        qDebug() << "Error committing transaction:" << db.lastError().text();
        db.rollback();
        return false;
    }

    return true;
}

QString PgSqlYtx::NodeFinance()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS finance (
        id               BIGSERIAL PRIMARY KEY,
        name             TEXT,
        code             TEXT,
        description      TEXT,
        note             TEXT,
        type             INTEGER,
        rule             BOOLEAN DEFAULT FALSE,
        unit             INTEGER,
        foreign_total    NUMERIC(19,6),
        local_total      NUMERIC(19,6),
        removed          BOOLEAN DEFAULT FALSE
    );
    )");
}

QString PgSqlYtx::NodeStakeholder()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS stakeholder (
        id                BIGSERIAL PRIMARY KEY,
        name              TEXT,
        code              TEXT,
        description       TEXT,
        note              TEXT,
        type              INTEGER,
        payment_term      INTEGER,
        unit              INTEGER,
        deadline          INTEGER CHECK (deadline >= 1 AND deadline <= 31),
        employee          INTEGER,
        tax_rate          NUMERIC(19,6),
        amount            NUMERIC(19,6),
        removed           BOOLEAN DEFAULT FALSE
    );
    )");
}

QString PgSqlYtx::NodeProduct()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS product (
        id               BIGSERIAL PRIMARY KEY,
        name             TEXT,
        code             TEXT,
        description      TEXT,
        note             TEXT,
        type             INTEGER,
        rule             BOOLEAN DEFAULT FALSE,
        unit             INTEGER,
        color            TEXT,
        unit_price       NUMERIC(19,6),
        commission       NUMERIC(19,6),
        quantity         NUMERIC(19,6),
        amount           NUMERIC(19,6),
        removed          BOOLEAN DEFAULT FALSE
    );
    )");
}

QString PgSqlYtx::NodeTask()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS task (
        id               BIGSERIAL PRIMARY KEY,
        name             TEXT,
        code             TEXT,
        description      TEXT,
        note             TEXT,
        type             INTEGER,
        rule             BOOLEAN DEFAULT FALSE,
        unit             INTEGER,
        date_time        TIMESTAMPTZ(0),
        color            TEXT,
        document         TEXT,
        finished         BOOLEAN DEFAULT FALSE,
        unit_cost        NUMERIC(19,6),
        quantity         NUMERIC(19,6),
        amount           NUMERIC(19,6),
        removed          BOOLEAN DEFAULT FALSE
    );
    )");
}

QString PgSqlYtx::NodeOrder(CString& order)
{
    return QString(R"(
    CREATE TABLE IF NOT EXISTS %1 (
        id                BIGSERIAL PRIMARY KEY,
        name              TEXT,
        party             INTEGER,
        description       TEXT,
        employee          INTEGER,
        type              INTEGER,
        rule              BOOLEAN DEFAULT FALSE,
        unit              INTEGER,
        date_time         TIMESTAMPTZ(0),
        first             NUMERIC(19,6),
        second            NUMERIC(19,6),
        finished          BOOLEAN DEFAULT FALSE,
        gross_amount      NUMERIC(19,6),
        discount          NUMERIC(19,6),
        settlement        NUMERIC(19,6),
        settlement_id     INTEGER DEFAULT 0,
        removed           BOOLEAN DEFAULT FALSE
    );
    )")
        .arg(order);
}

QString PgSqlYtx::Path(CString& table_name)
{
    return QString(R"(
    CREATE TABLE IF NOT EXISTS %1_path (
        ancestor      INTEGER    CHECK (ancestor   >= 1),
        descendant    INTEGER    CHECK (descendant >= 1),
        distance      INTEGER    CHECK (distance   >= 0)
    );
)")
        .arg(table_name);
}

QString PgSqlYtx::TransFinance()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS finance_transaction (
        id             BIGSERIAL PRIMARY KEY,
        date_time      TEXT,
        code           TEXT,
        lhs_node       INTEGER,
        lhs_ratio      NUMERIC(19,6) CHECK (lhs_ratio > 0),
        lhs_debit      NUMERIC(19,6) CHECK (lhs_debit >= 0),
        lhs_credit     NUMERIC(19,6) CHECK (lhs_credit >= 0),
        description    TEXT,
        support_id     INTEGER,
        document       TEXT,
        state          BOOLEAN DEFAULT FALSE,
        rhs_credit     NUMERIC(19,6) CHECK (rhs_credit >= 0),
        rhs_debit      NUMERIC(19,6) CHECK (rhs_debit >= 0),
        rhs_ratio      NUMERIC(19,6) CHECK (rhs_ratio > 0),
        rhs_node       INTEGER,
        removed        BOOLEAN DEFAULT FALSE
    );
    )");
}

QString PgSqlYtx::TransOrder(CString& order)
{
    return QString(R"(
    CREATE TABLE IF NOT EXISTS %1_transaction (
        id                  BIGSERIAL PRIMARY KEY,
        code                TEXT,
        lhs_node            INTEGER,
        unit_price          NUMERIC(19,6),
        first               NUMERIC(19,6),
        second              NUMERIC(19,6),
        description         TEXT,
        outside_product     INTEGER,
        discount            NUMERIC(19,6),
        net_amount          NUMERIC(19,6),
        gross_amount        NUMERIC(19,6),
        discount_price      NUMERIC(19,6),
        inside_product      INTEGER,
        removed             BOOLEAN DEFAULT FALSE
    );
    )")
        .arg(order);
}

QString PgSqlYtx::TransStakeholder()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS stakeholder_transaction (
        id                 BIGSERIAL PRIMARY KEY,
        date_time          TIMESTAMPTZ(0),
        code               TEXT,
        lhs_node           INTEGER,
        unit_price         NUMERIC(19,6),
        description        TEXT,
        outside_product    INTEGER,
        document           TEXT,
        state              BOOLEAN DEFAULT FALSE,
        inside_product     INTEGER,
        UNIQUE(lhs_node, inside_product)
    );
    )");
}

QString PgSqlYtx::TransTask()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS task_transaction (
        id             BIGSERIAL PRIMARY KEY,
        date_time      TIMESTAMPTZ(0),
        code           TEXT,
        lhs_node       INTEGER,
        unit_cost      NUMERIC(19,6),
        lhs_debit      NUMERIC(19,6) CHECK (lhs_debit >= 0),
        lhs_credit     NUMERIC(19,6) CHECK (lhs_credit >= 0),
        description    TEXT,
        support_id     INTEGER,
        document       TEXT,
        state          BOOLEAN DEFAULT FALSE,
        rhs_credit     NUMERIC(19,6) CHECK (rhs_credit >= 0),
        rhs_debit      NUMERIC(19,6) CHECK (rhs_debit >= 0),
        rhs_node       INTEGER,
        removed        BOOLEAN DEFAULT FALSE
    );
    )");
}

QString PgSqlYtx::TransProduct()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS product_transaction (
        id             BIGSERIAL PRIMARY KEY,
        date_time      TIMESTAMPTZ(0),
        code           TEXT,
        lhs_node       INTEGER,
        unit_cost      NUMERIC(19,6),
        lhs_debit      NUMERIC(19,6) CHECK (lhs_debit  >= 0),
        lhs_credit     NUMERIC(19,6) CHECK (lhs_credit >= 0),
        description    TEXT,
        support_id     INTEGER,
        document       TEXT,
        state          BOOLEAN DEFAULT FALSE,
        rhs_credit     NUMERIC(19,6) CHECK (rhs_credit >= 0),
        rhs_debit      NUMERIC(19,6) CHECK (rhs_debit  >= 0),
        rhs_node       INTEGER,
        removed        BOOLEAN DEFAULT FALSE
    );
    )");
}

QString PgSqlYtx::SettlementOrder(CString& order)
{
    return QString(R"(
    CREATE TABLE IF NOT EXISTS %1_settlement (
        id             BIGSERIAL PRIMARY KEY,
        party          INTEGER,
        date_time      TIMESTAMPTZ(0),
        description    TEXT,
        finished       BOOLEAN DEFAULT FALSE,
        gross_amount   NUMERIC(19,6),
        removed        BOOLEAN DEFAULT FALSE
    );
    )")
        .arg(order);
}

QSqlDatabase PgSqlYtx::SingleConnection(CString& user, CString& password, CString& db_name, CString& connection_name, int timeout_ms)
{
    auto db = QSqlDatabase::addDatabase("QPSQL", connection_name);

    db.setHostName("localhost");
    db.setPort(5432);
    db.setUserName(user);
    db.setPassword(password);
    db.setDatabaseName(db_name);
    db.setConnectOptions(QString("connect_timeout=%1").arg(timeout_ms));

    if (!db.open()) {
        qDebug() << "Failed to open connection to database" << db_name;
        QSqlDatabase::removeDatabase(connection_name);
        return {};
    }

    return db;
}

void PgSqlYtx::RemoveConnection(CString& connection_name)
{
    {
        QSqlDatabase db = QSqlDatabase::database(connection_name);
        if (db.isOpen()) {
            db.close();
        }
    }

    QSqlDatabase::removeDatabase(connection_name);
}

bool PgSqlYtx::CreateRole(QSqlDatabase& db, CString role_name, CString password)
{
    if (!IsValidPgIdentifier(role_name)) {
        qDebug() << "Invalid role name:" << role_name;
        return false;
    }

    if (!IsValidPassword(password)) {
        qDebug() << "Invalid password for role:" << password;
        return false;
    }

    const QString sql { QString("CREATE ROLE %1 LOGIN PASSWORD '%2';").arg(role_name, password) };

    QSqlQuery query(db);

    if (!query.exec(QString("SELECT 1 FROM pg_catalog.pg_roles WHERE rolname = '%1';").arg(role_name))) {
        qDebug() << "Check role existence failed:" << query.lastError().text();
        return false;
    }

    if (query.next()) {
        return true;
    }

    if (!query.exec(sql)) {
        qDebug() << "Error creating role:" << query.lastError().text();
        return false;
    }

    return true;
}

bool PgSqlYtx::CreateDatabase(QSqlDatabase& db, CString db_name, CString owner)
{
    if (!IsValidPgIdentifier(db_name)) {
        qDebug() << "Invalid db name:" << db_name;
        return false;
    }

    if (!IsValidPgIdentifier(owner)) {
        qDebug() << "Invalid owner name:" << owner;
        return false;
    }

    QString sql = QString("CREATE DATABASE %1 OWNER %2;").arg(db_name).arg(owner);

    QSqlQuery query(db);

    if (!query.exec(QString("SELECT 1 FROM pg_database WHERE datname = '%1';").arg(db_name))) {
        qDebug() << "Check database existence failed:" << query.lastError().text();
        return false;
    }

    if (query.next()) {
        return true;
    }

    if (!query.exec(sql)) {
        qDebug() << "Error creating database:" << query.lastError().text();
        return false;
    }

    return true;
}

#if 0
bool PgSqlYtx::IniPGData()
{
    QDir dir(kPgDataDir);
    if (dir.exists()) {
        qInfo() << "PostgreSQL data directory exists:" << kPgDataDir;
        return true;
    }

    if (!dir.mkpath(".")) {
        qWarning() << "Failed to create pg_data directory:" << kPgDataDir;
        return false;
    }

    CString initdb_path { PgBinPath("initdb") };
    if (!QFile::exists(initdb_path)) {
        qWarning() << "initdb not found at:" << initdb_path;
        return false;
    }

    const QStringList args { "-D", kPgDataDir, "-A", "trust", "--no-locale" };

    QString output {};
    if (!RunProcess(initdb_path, args, &output)) {
        return false;
    }

    qInfo() << "PostgreSQL data directory initialized:\n" << output;
    return true;
}

bool PgSqlYtx::StartPGServer()
{
    CString pg_ctl_path { PgBinPath("pg_ctl") };
    if (!QFile::exists(pg_ctl_path)) {
        qWarning() << "pg_ctl not found at:" << pg_ctl_path;
        return false;
    }

    CString log_path = QDir(kPgDataDir).filePath("log.txt");
    const QStringList args { "start", "-D", kPgDataDir, "-l", log_path };

    return RunProcess(pg_ctl_path, args);
}

bool PgSqlYtx::StopPGServer()
{
    CString pg_ctl_path { PgBinPath("pg_ctl") };
    if (!QFile::exists(pg_ctl_path)) {
        qWarning() << "pg_ctl not found at:" << pg_ctl_path;
        return false;
    }

    const QStringList args { "stop", "-D", kPgDataDir, "-m", "fast" };
    return RunProcess(pg_ctl_path, args);
}

bool PgSqlYtx::RunProcess(CString& program, const QStringList& arguments, QString* std_output, QString* std_error, int timeout_ms)
{
    QProcess process {};
    process.start(program, arguments);
    if (!process.waitForFinished(timeout_ms)) {
        qWarning() << program << "timed out.";
        return false;
    }

    if (std_output)
        *std_output = process.readAllStandardOutput();
    if (std_error)
        *std_error = process.readAllStandardError();

    if (process.exitCode() != 0) {
        qWarning() << program << "failed with error:\n" << (std_error ? *std_error : "");
        return false;
    }

    return true;
}

QString PgSqlYtx::PgBinPath(CString& exe_name)
{
#ifdef Q_OS_WIN
    return kPgBinBasePath + exe_name + ".exe";
#else
    return kPgBinBasePath + exe_name;
#endif
}
#endif
