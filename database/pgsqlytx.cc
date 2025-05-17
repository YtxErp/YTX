#include "pgsqlytx.h"

#include <QSqlError>
#include <QSqlQuery>

#include "component/constvalue.h"

bool PgSqlYtx::NewFile(const QString& user, const QString& db_name, int timeout_ms)
{
    QSqlDatabase db;
    AddDatabase(db, user, "abcd1234EFGH", db_name, "new_file", timeout_ms);
    if (!db.open())
        return false;

    const QString finance { NodeFinance() };
    const QString finance_path { Path(kFinance) };
    const QString finance_trans { TransFinance() };

    const QString product { NodeProduct() };
    const QString product_path { Path(kProduct) };
    const QString product_trans { TransProduct() };

    const QString task { NodeTask() };
    const QString task_path { Path(kTask) };
    const QString task_trans { TransTask() };

    const QString stakeholder { NodeStakeholder() };
    const QString stakeholder_path { Path(kStakeholder) };
    const QString stakeholder_trans { TransStakeholder() };

    const QString purchase { NodeOrder(kPurchase) };
    const QString purchase_path { Path(kPurchase) };
    const QString purchase_trans { TransOrder(kPurchase) };
    const QString purchase_settlement { SettlementOrder(kPurchase) };

    const QString sales { NodeOrder(kSales) };
    const QString sales_path { Path(kSales) };
    const QString sales_trans { TransOrder(kSales) };
    const QString sales_settlement { SettlementOrder(kSales) };

    QSqlQuery query {};
    if (db.transaction()) {
        // Execute each table creation query
        if (query.exec(finance) && query.exec(finance_path) && query.exec(finance_trans) && query.exec(product) && query.exec(product_path)
            && query.exec(product_trans) && query.exec(stakeholder) && query.exec(stakeholder_path) && query.exec(stakeholder_trans) && query.exec(task)
            && query.exec(task_path) && query.exec(task_trans) && query.exec(purchase) && query.exec(purchase_path) && query.exec(purchase_trans)
            && query.exec(sales) && query.exec(sales_path) && query.exec(sales_trans) && query.exec(purchase_settlement) && query.exec(sales_settlement)) {
            // Commit the transaction if all queries are successful
            if (db.commit()) {
                for (int i = 0; i != 6; ++i) {
                    query.exec(QLatin1String("INSERT INTO settings (static_node) VALUES (0);"));
                }
            } else {
                // Handle commit failure
                qDebug() << "Error committing transaction" << db.lastError().text();
                // Rollback the transaction in case of failure
                db.rollback();
            }
        } else {
            // Handle query execution failure
            qDebug() << "Error creating tables" << query.lastError().text();
            // Rollback the transaction in case of failure
            db.rollback();
        }
    } else {
        // Handle transaction start failure
        qDebug() << "Error starting transaction" << db.lastError().text();
    }

    RemoveDatabase("new_file");
    return true;
}

QString PgSqlYtx::NodeFinance()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS finance (
        id               INTEGER PRIMARY KEY AUTOINCREMENT,
        name             TEXT,
        code             TEXT,
        description      TEXT,
        note             TEXT,
        type             INTEGER,
        rule             BOOLEAN    DEFAULT 0,
        unit             INTEGER,
        foreign_total    NUMERIC,
        local_total      NUMERIC,
        removed          BOOLEAN    DEFAULT 0
    );
    )");
}

QString PgSqlYtx::NodeStakeholder()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS stakeholder (
        id                INTEGER PRIMARY KEY AUTOINCREMENT,
        name              TEXT,
        code              TEXT,
        description       TEXT,
        note              TEXT,
        type              INTEGER,
        payment_term      INTEGER,
        unit              INTEGER,
        deadline          TEXT,
        employee          INTEGER,
        tax_rate          NUMERIC,
        amount            NUMERIC,
        removed           BOOLEAN    DEFAULT 0
    );
    )");
}

QString PgSqlYtx::NodeProduct()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS product (
        id               INTEGER PRIMARY KEY AUTOINCREMENT,
        name             TEXT,
        code             TEXT,
        description      TEXT,
        note             TEXT,
        type             INTEGER,
        rule             BOOLEAN    DEFAULT 0,
        unit             INTEGER,
        color            TEXT,
        unit_price       NUMERIC,
        commission       NUMERIC,
        quantity         NUMERIC,
        amount           NUMERIC,
        removed          BOOLEAN    DEFAULT 0
    );
)");
}

QString PgSqlYtx::NodeTask()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS task (
        id               INTEGER PRIMARY KEY AUTOINCREMENT,
        name             TEXT,
        code             TEXT,
        description      TEXT,
        note             TEXT,
        type             INTEGER,
        rule             BOOLEAN    DEFAULT 0,
        unit             INTEGER,
        date_time        TEXT,
        color            TEXT,
        document         TEXT,
        finished         BOOLEAN    DEFAULT 0,
        unit_cost        NUMERIC,
        quantity         NUMERIC,
        amount           NUMERIC,
        removed          BOOLEAN    DEFAULT 0
    );
    )");
}

QString PgSqlYtx::NodeOrder(const QString& order)
{
    return QString(R"(
    CREATE TABLE IF NOT EXISTS %1 (
        id                INTEGER PRIMARY KEY AUTOINCREMENT,
        name              TEXT,
        party             INTEGER,
        description       TEXT,
        employee          INTEGER,
        type              INTEGER,
        rule              BOOLEAN    DEFAULT 0,
        unit              INTEGER,
        date_time         TEXT,
        first             NUMERIC,
        second            NUMERIC,
        finished          BOOLEAN    DEFAULT 0,
        gross_amount      NUMERIC,
        discount          NUMERIC,
        settlement        NUMERIC,
        settlement_id     INTEGER    DEFAULT 0,
        removed           BOOLEAN    DEFAULT 0
    );
    )")
        .arg(order);
}

QString PgSqlYtx::Path(const QString& table_name)
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
        id             INTEGER PRIMARY KEY AUTOINCREMENT,
        date_time      TEXT,
        code           TEXT,
        lhs_node       INTEGER,
        lhs_ratio      NUMERIC                   CHECK (lhs_ratio   > 0),
        lhs_debit      NUMERIC                   CHECK (lhs_debit  >= 0),
        lhs_credit     NUMERIC                   CHECK (lhs_credit >= 0),
        description    TEXT,
        support_id     INTEGER,
        document       TEXT,
        state          BOOLEAN    DEFAULT 0,
        rhs_credit     NUMERIC                   CHECK (rhs_credit >= 0),
        rhs_debit      NUMERIC                   CHECK (rhs_debit  >= 0),
        rhs_ratio      NUMERIC                   CHECK (rhs_ratio   > 0),
        rhs_node       INTEGER,
        removed        BOOLEAN    DEFAULT 0
    );
    )");
}

QString PgSqlYtx::TransOrder(const QString& order)
{
    return QString(R"(
    CREATE TABLE IF NOT EXISTS %1_transaction (
        id                  INTEGER PRIMARY KEY AUTOINCREMENT,
        code                TEXT,
        lhs_node            INTEGER,
        unit_price          NUMERIC,
        first               NUMERIC,
        second              NUMERIC,
        description         TEXT,
        outside_product     INTEGER,
        discount            NUMERIC,
        net_amount          NUMERIC,
        gross_amount        NUMERIC,
        discount_price      NUMERIC,
        inside_product      INTEGER,
        removed             BOOLEAN    DEFAULT 0
    );
    )")
        .arg(order);
}

QString PgSqlYtx::TransStakeholder()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS stakeholder_transaction (
        id                 INTEGER PRIMARY KEY AUTOINCREMENT,
        date_time          TEXT,
        code               TEXT,
        lhs_node           INTEGER,
        unit_price         NUMERIC,
        description        TEXT,
        outside_product    INTEGER,
        document           TEXT,
        state              BOOLEAN    DEFAULT 0,
        inside_product     INTEGER,
        UNIQUE(lhs_node, inside_product)
    );
    )");
}

QString PgSqlYtx::TransTask()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS task_transaction (
        id             INTEGER PRIMARY KEY AUTOINCREMENT,
        date_time      TEXT,
        code           TEXT,
        lhs_node       INTEGER,
        unit_cost      NUMERIC,
        lhs_debit      NUMERIC                  CHECK (lhs_debit  >= 0),
        lhs_credit     NUMERIC                  CHECK (lhs_credit >= 0),
        description    TEXT,
        support_id     INTEGER,
        document       TEXT,
        state          BOOLEAN    DEFAULT 0,
        rhs_credit     NUMERIC                  CHECK (rhs_credit >= 0),
        rhs_debit      NUMERIC                  CHECK (rhs_debit  >= 0),
        rhs_node       INTEGER,
        removed        BOOLEAN    DEFAULT 0
    );
    )");
}

QString PgSqlYtx::TransProduct()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS product_transaction (
        id             INTEGER PRIMARY KEY AUTOINCREMENT,
        date_time      TEXT,
        code           TEXT,
        lhs_node       INTEGER,
        unit_cost      NUMERIC,
        lhs_debit      NUMERIC                  CHECK (lhs_debit  >= 0),
        lhs_credit     NUMERIC                  CHECK (lhs_credit >= 0),
        description    TEXT,
        support_id     INTEGER,
        document       TEXT,
        state          BOOLEAN    DEFAULT 0,
        rhs_credit     NUMERIC                  CHECK (rhs_credit >= 0),
        rhs_debit      NUMERIC                  CHECK (rhs_debit  >= 0),
        rhs_node       INTEGER,
        removed        BOOLEAN    DEFAULT 0
    );
    )");
}

QString PgSqlYtx::SettlementOrder(const QString& order)
{
    return QString(R"(
    CREATE TABLE IF NOT EXISTS %1_settlement (
        id             INTEGER PRIMARY KEY AUTOINCREMENT,
        party          INTEGER,
        date_time      TEXT,
        description    TEXT,
        finished       BOOLEAN    DEFAULT 0,
        gross_amount   NUMERIC,
        removed        BOOLEAN    DEFAULT 0
    );
    )")
        .arg(order);
}

bool PgSqlYtx::AddDatabase(
    QSqlDatabase& db, const QString& user, const QString& password, const QString& db_name, const QString& connection_name, int timeout_ms)
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

    db = QSqlDatabase::addDatabase("QPSQL", connection_name);
    db.setHostName("localhost");
    db.setPort(5432);
    db.setUserName(user);
    db.setPassword(password);
    db.setDatabaseName(db_name);
    db.setConnectOptions(QString("connect_timeout=%1").arg(timeout_ms));

    if (!db.open()) {
        qDebug() << "Failed in CreateConnection:" << db_name;
        QSqlDatabase::removeDatabase(connection_name);
        return false;
    }

    return true;
}

void PgSqlYtx::RemoveDatabase(const QString& connection_name)
{
    {
        QSqlDatabase db = QSqlDatabase::database(connection_name);
        if (db.isOpen()) {
            db.close();
        }
    }

    QSqlDatabase::removeDatabase(connection_name);
}

bool PgSqlYtx::IsPGServerAvailable(const QString& user, const QString& db_name, int timeout_ms)
{
    QSqlDatabase db;
    const bool is_available { AddDatabase(db, user, "abcd1234EFGH", db_name, "pg_check", timeout_ms) };
    RemoveDatabase("pg_check");
    return is_available;
}
