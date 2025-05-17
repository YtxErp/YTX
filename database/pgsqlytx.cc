#include "pgsqlytx.h"

#include <QSqlError>
#include <QSqlQuery>

#include "component/constvalue.h"

bool PgSqlYtx::NewFile(CString& user, CString& db_name, int timeout_ms)
{
    QSqlDatabase db;
    AddDatabase(db, user, "abcd1234EFGH", db_name, "new_file", timeout_ms);
    if (!db.open())
        return false;

    CString finance { NodeFinance() };
    CString finance_path { Path(kFinance) };
    CString finance_trans { TransFinance() };

    CString product { NodeProduct() };
    CString product_path { Path(kProduct) };
    CString product_trans { TransProduct() };

    CString task { NodeTask() };
    CString task_path { Path(kTask) };
    CString task_trans { TransTask() };

    CString stakeholder { NodeStakeholder() };
    CString stakeholder_path { Path(kStakeholder) };
    CString stakeholder_trans { TransStakeholder() };

    CString purchase { NodeOrder(kPurchase) };
    CString purchase_path { Path(kPurchase) };
    CString purchase_trans { TransOrder(kPurchase) };
    CString purchase_settlement { SettlementOrder(kPurchase) };

    CString sales { NodeOrder(kSales) };
    CString sales_path { Path(kSales) };
    CString sales_trans { TransOrder(kSales) };
    CString sales_settlement { SettlementOrder(kSales) };

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

bool PgSqlYtx::AddDatabase(QSqlDatabase& db, CString& user, CString& password, CString& db_name, CString& connection_name, int timeout_ms)
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

void PgSqlYtx::RemoveDatabase(CString& connection_name)
{
    {
        QSqlDatabase db = QSqlDatabase::database(connection_name);
        if (db.isOpen()) {
            db.close();
        }
    }

    QSqlDatabase::removeDatabase(connection_name);
}
