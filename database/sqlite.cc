#include "sqlite.h"

#include <QSqlError>
#include <QSqlQuery>

#include "component/constvalue.h"

bool Sqlite::CreateDatabase(QSqlDatabase& db, CString database, CString owner)
{
    if (!IsValidPgIdentifier(database)) {
        qDebug() << "Invalid database name:" << database;
        return false;
    }

    if (!IsValidPgIdentifier(owner)) {
        qDebug() << "Invalid owner name:" << owner;
        return false;
    }

    QString sql = QString("CREATE DATABASE %1 OWNER %2;").arg(database, owner);

    QSqlQuery query(db);

    if (!query.exec(QString("SELECT 1 FROM pg_database WHERE datname = '%1';").arg(database))) {
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

bool Sqlite::NewFile(const QString& file_path)
{
    QSqlDatabase db { QSqlDatabase::addDatabase(kQSQLITE) };
    db.setDatabaseName(file_path);
    if (!db.open())
        return false;

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

    db.close();
    return true;
}

/**
 * Server-Generated Fields Notice
 *
 * The following fields are strictly generated and managed by the server:
 *
 * - created_at, updated_at
 *   Ensures consistent timestamps across clients and enables proper auditing.
 *
 * - created_by, updated_by
 *   Prevents identity spoofing; reflects authenticated user context on the server.
 *
 * - user_id
 *   Tied to the authenticated session; recommended to be consistent with created_by.
 *
 * Clients MUST NOT generate or override these fields.
 * They are returned by the server and should only be stored or displayed locally.
 */

QString Sqlite::NodeFinance()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS finance (
        id               TEXT PRIMARY KEY,
        name             TEXT NOT NULL,
        code             TEXT,
        description      TEXT,
        note             TEXT,
        node_type        INTEGER NOT NULL,
        direction_rule   BOOLEAN NOT NULL DEFAULT FALSE,
        unit             INTEGER NOT NULL,
        foreign_total    NUMERIC(19,6),
        local_total      NUMERIC(19,6),
        user_id          TEXT NOT NULL,
        created_at       TEXT NOT NULL,
        created_by       TEXT NOT NULL,
        updated_at       TEXT NOT NULL,
        updated_by       TEXT NOT NULL,
        version          INTEGER NOT NULL DEFAULT 1,
        is_valid         BOOLEAN NOT NULL DEFAULT TRUE
    );
    )");
}

QString Sqlite::NodeProduct()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS product (
        id               TEXT PRIMARY KEY,
        name             TEXT NOT NULL,
        code             TEXT,
        description      TEXT,
        note             TEXT,
        node_type        INTEGER NOT NULL,
        direction_rule   BOOLEAN NOT NULL DEFAULT false,
        unit             INTEGER NOT NULL,
        color            TEXT,
        unit_price       NUMERIC(19,6),
        commission       NUMERIC(19,6),
        quantity         NUMERIC(19,6),
        amount           NUMERIC(19,6),
        user_id          TEXT NOT NULL,
        created_at       TEXT NOT NULL,
        created_by       TEXT NOT NULL,
        updated_at       TEXT NOT NULL,
        updated_by       TEXT NOT NULL,
        version          INTEGER NOT NULL DEFAULT 1,
        is_valid         BOOLEAN NOT NULL DEFAULT TRUE
    );
    )");
}

QString Sqlite::NodeTask()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS task (
        id               TEXT PRIMARY KEY,
        name             TEXT NOT NULL,
        code             TEXT,
        description      TEXT,
        note             TEXT,
        node_type        INTEGER NOT NULL,
        direction_rule   BOOLEAN NOT NULL DEFAULT false,
        unit             INTEGER NOT NULL,
        issued_at        TEXT NOT NULL,
        color            TEXT,
        document         TEXT,
        is_finished      BOOLEAN NOT NULL DEFAULT false,
        unit_cost        NUMERIC(19,6),
        quantity         NUMERIC(19,6),
        amount           NUMERIC(19,6),
        user_id          TEXT NOT NULL,
        created_at       TEXT NOT NULL,
        created_by       TEXT NOT NULL,
        updated_at       TEXT NOT NULL,
        updated_by       TEXT NOT NULL,
        version          INTEGER NOT NULL DEFAULT 1,
        is_valid         BOOLEAN NOT NULL DEFAULT TRUE
    );
    )");
}

QString Sqlite::NodeStakeholder()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS stakeholder (
        id                TEXT PRIMARY KEY,
        name              TEXT NOT NULL,
        code              TEXT,
        description       TEXT,
        note              TEXT,
        node_type         INTEGER NOT NULL,
        payment_term      INTEGER NOT NULL,
        unit              INTEGER NOT NULL,
        deadline          INTEGER CHECK (deadline BETWEEN 1 AND 31),
        employee          TEXT NOT NULL,
        tax_rate          NUMERIC(19,6),
        amount            NUMERIC(19,6),
        user_id           TEXT NOT NULL,
        created_at        TEXT NOT NULL,
        created_by        TEXT NOT NULL,
        updated_at        TEXT NOT NULL,
        updated_by        TEXT NOT NULL,
        version           INTEGER NOT NULL DEFAULT 1,
        is_valid          BOOLEAN NOT NULL DEFAULT TRUE
    );
    )");
}

QString Sqlite::NodeOrder(const QString& order)
{
    return QString(R"(
    CREATE TABLE IF NOT EXISTS %1 (
        id                TEXT PRIMARY KEY,
        name              TEXT NOT NULL,
        party             TEXT NOT NULL,
        description       TEXT,
        employee          TEXT NOT NULL,
        node_type         INTEGER NOT NULL,
        direction_rule    BOOLEAN NOT NULL DEFAULT FALSE,
        unit              INTEGER NOT NULL,
        issued_at         TEXT NOT NULL,
        first             NUMERIC(19,6),
        second            NUMERIC(19,6),
        is_finished       BOOLEAN NOT NULL DEFAULT FALSE,
        gross_amount      NUMERIC(19,6),
        discount          NUMERIC(19,6),
        settlement        NUMERIC(19,6),
        settlement_id     TEXT NOT NULL,
        user_id           TEXT NOT NULL,
        created_at        TEXT NOT NULL,
        created_by        TEXT NOT NULL,
        updated_at        TEXT NOT NULL,
        updated_by        TEXT NOT NULL,
        version           INTEGER NOT NULL DEFAULT 1,
        is_valid          BOOLEAN NOT NULL DEFAULT TRUE
    );
    )")
        .arg(order);
}

QString Sqlite::Path(const QString& table_name)
{
    return QString(R"(
    CREATE TABLE IF NOT EXISTS %1_path (
        ancestor      TEXT NOT NULL,
        descendant    TEXT NOT NULL,
        distance      INTEGER NOT NULL CHECK (distance >= 0),
        user_id       TEXT NOT NULL,
        version       INTEGER NOT NULL DEFAULT 1
    );
)")
        .arg(table_name);
}

QString Sqlite::TransFinance()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS finance_transaction (
        id             TEXT PRIMARY KEY,
        issued_at      TEXT NOT NULL,
        code           TEXT,
        lhs_node       TEXT NOT NULL,
        lhs_ratio      NUMERIC(19,6) CHECK (lhs_ratio   > 0),
        lhs_debit      NUMERIC(19,6) CHECK (lhs_debit  >= 0),
        lhs_credit     NUMERIC(19,6) CHECK (lhs_credit >= 0),
        description    TEXT,
        support_id     TEXT,
        document       TEXT,
        is_checked     BOOLEAN DEFAULT FALSE,
        rhs_credit     NUMERIC(19,6) CHECK (rhs_credit >= 0),
        rhs_debit      NUMERIC(19,6) CHECK (rhs_debit  >= 0),
        rhs_ratio      NUMERIC(19,6) CHECK (rhs_ratio   > 0),
        rhs_node       TEXT NOT NULL,
        user_id        TEXT NOT NULL,
        created_at     TEXT NOT NULL,
        created_by     TEXT NOT NULL,
        updated_at     TEXT NOT NULL,
        updated_by     TEXT NOT NULL,
        version        INTEGER NOT NULL DEFAULT 1,
        is_valid       BOOLEAN NOT NULL DEFAULT TRUE
    );
    )");
}

QString Sqlite::TransProduct()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS product_transaction (
        id             TEXT PRIMARY KEY,
        issued_at      TEXT NOT NULL,
        code           TEXT,
        lhs_node       TEXT NOT NULL,
        unit_cost      NUMERIC(19,6),
        lhs_debit      NUMERIC(19,6) CHECK (lhs_debit  >= 0),
        lhs_credit     NUMERIC(19,6) CHECK (lhs_credit >= 0),
        description    TEXT,
        support_id     TEXT,
        document       TEXT,
        is_checked     BOOLEAN DEFAULT FALSE,
        rhs_credit     NUMERIC(19,6) CHECK (rhs_credit >= 0),
        rhs_debit      NUMERIC(19,6) CHECK (rhs_debit  >= 0),
        rhs_node       TEXT NOT NULL,
        user_id        TEXT NOT NULL,
        created_at     TEXT NOT NULL,
        created_by     TEXT NOT NULL,
        updated_at     TEXT NOT NULL,
        updated_by     TEXT NOT NULL,
        version        INTEGER NOT NULL DEFAULT 1,
        is_valid       BOOLEAN NOT NULL DEFAULT TRUE
    );
    )");
}

QString Sqlite::TransTask()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS task_transaction (
        id             TEXT PRIMARY KEY,
        issued_at      TEXT NOT NULL,
        code           TEXT,
        lhs_node       TEXT NOT NULL,
        unit_cost      NUMERIC(19,6),
        lhs_debit      NUMERIC(19,6) CHECK (lhs_debit  >= 0),
        lhs_credit     NUMERIC(19,6) CHECK (lhs_credit >= 0),
        description    TEXT,
        support_id     TEXT,
        document       TEXT,
        is_checked     BOOLEAN DEFAULT FALSE,
        rhs_credit     NUMERIC(19,6) CHECK (rhs_credit >= 0),
        rhs_debit      NUMERIC(19,6) CHECK (rhs_debit  >= 0),
        rhs_node       TEXT NOT NULL,
        user_id        TEXT NOT NULL,
        created_at     TEXT NOT NULL,
        created_by     TEXT NOT NULL,
        updated_at     TEXT NOT NULL,
        updated_by     TEXT NOT NULL,
        version        INTEGER NOT NULL DEFAULT 1,
        is_valid       BOOLEAN NOT NULL DEFAULT TRUE
    );
    )");
}

QString Sqlite::TransStakeholder()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS stakeholder_transaction (
        id                 TEXT PRIMARY KEY,
        issued_at          TEXT NOT NULL,
        code               TEXT,
        lhs_node           TEXT NOT NULL,
        unit_price         NUMERIC(19,6),
        description        TEXT,
        outside_product    TEXT,
        document           TEXT,
        is_checked         BOOLEAN DEFAULT FALSE,
        inside_product     TEXT NOT NULL,
        user_id            TEXT NOT NULL,
        created_at         TEXT NOT NULL,
        created_by         TEXT NOT NULL,
        updated_at         TEXT NOT NULL,
        updated_by         TEXT NOT NULL,
        version            INTEGER NOT NULL DEFAULT 1,
        UNIQUE(lhs_node, inside_product)
    );
    )");
}

QString Sqlite::TransOrder(const QString& order)
{
    return QString(R"(
    CREATE TABLE IF NOT EXISTS %1_transaction (
        id                  TEXT PRIMARY KEY,
        code                TEXT,
        lhs_node            TEXT NOT NULL,
        unit_price          NUMERIC(19,6),
        first               NUMERIC(19,6),
        second              NUMERIC(19,6),
        description         TEXT,
        outside_product     TEXT,
        discount            NUMERIC(19,6),
        net_amount          NUMERIC(19,6),
        gross_amount        NUMERIC(19,6),
        discount_price      NUMERIC(19,6),
        inside_product      TEXT NOT NULL,
        user_id             TEXT NOT NULL,
        created_at          TEXT NOT NULL,
        created_by          TEXT NOT NULL,
        updated_at          TEXT NOT NULL,
        updated_by          TEXT NOT NULL,
        version             INTEGER NOT NULL DEFAULT 1,
        is_valid            BOOLEAN NOT NULL DEFAULT TRUE
    );
    )")
        .arg(order);
}

QString Sqlite::SettlementOrder(const QString& order)
{
    return QString(R"(
    CREATE TABLE IF NOT EXISTS %1_settlement (
        id               TEXT PRIMARY KEY,
        party            TEXT NOT NULL,
        issued_at        TEXT NOT NULL,
        description      TEXT,
        is_finished      BOOLEAN DEFAULT FALSE,
        gross_amount     NUMERIC(19,6),
        user_id          TEXT NOT NULL,
        created_at       TEXT NOT NULL,
        created_by       TEXT NOT NULL,
        updated_at       TEXT NOT NULL,
        updated_by       TEXT NOT NULL,
        version          INTEGER NOT NULL DEFAULT 1,
        is_valid         BOOLEAN NOT NULL DEFAULT TRUE
    );
    )")
        .arg(order);
}
