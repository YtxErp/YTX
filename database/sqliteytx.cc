#include "sqliteytx.h"

#include <QSqlError>
#include <QSqlQuery>

#include "component/constvalue.h"

bool SqliteYtx::NewFile(const QString& file_path)
{
    QSqlDatabase db { QSqlDatabase::addDatabase(kQSQLITE) };
    db.setDatabaseName(file_path);
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
            && query.exec(sales) && query.exec(sales_path) && query.exec(sales_trans) && query.exec(purchase_settlement) && query.exec(sales_settlement)
            && NodeIndex(query)) {
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

    db.close();
    return true;
}

QString SqliteYtx::NodeFinance()
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

QString SqliteYtx::NodeStakeholder()
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

QString SqliteYtx::NodeProduct()
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

QString SqliteYtx::NodeTask()
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

QString SqliteYtx::NodeOrder(const QString& order)
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

QString SqliteYtx::Path(const QString& table_name)
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

QString SqliteYtx::TransFinance()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS finance_transaction (
        id             INTEGER PRIMARY KEY AUTOINCREMENT,
        date_time      DATE,
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

QString SqliteYtx::TransOrder(const QString& order)
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

QString SqliteYtx::TransStakeholder()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS stakeholder_transaction (
        id                 INTEGER PRIMARY KEY AUTOINCREMENT,
        date_time          DATE,
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

QString SqliteYtx::TransTask()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS task_transaction (
        id             INTEGER PRIMARY KEY AUTOINCREMENT,
        date_time      DATE,
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

QString SqliteYtx::TransProduct()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS product_transaction (
        id             INTEGER PRIMARY KEY AUTOINCREMENT,
        date_time      DATE,
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

bool SqliteYtx::NodeIndex(QSqlQuery& query)
{
    // Create an index on (party, date_time)
    QString sql1 = QStringLiteral("CREATE INDEX IF NOT EXISTS idx_sales_party_datetime ON sales (party, date_time);");
    if (!query.exec(sql1)) {
        qDebug() << QString("Failed to create idx_sales_party_datetime index: %1").arg(query.lastError().text());
        return false;
    }

    sql1 = QStringLiteral("CREATE INDEX IF NOT EXISTS idx_purchase_party_datetime ON purchase (party, date_time);");
    if (!query.exec(sql1)) {
        qDebug() << QString("Failed to create idx_purchase_party_datetime index: %1").arg(query.lastError().text());
        return false;
    }

    // Create an index on (unit, party, date_time)
    QString sql2 = QStringLiteral("CREATE INDEX IF NOT EXISTS idx_sales_unit_party_datetime ON sales (unit, party, date_time);");
    if (!query.exec(sql2)) {
        qDebug() << QString("Failed to create idx_sales_unit_party_datetime index: %1").arg(query.lastError().text());
        return false;
    }

    sql2 = QStringLiteral("CREATE INDEX IF NOT EXISTS idx_purchase_unit_party_datetime ON purchase (unit, party, date_time);");
    if (!query.exec(sql2)) {
        qDebug() << QString("Failed to create idx_purchase_unit_party_datetime index: %1").arg(query.lastError().text());
        return false;
    }

    return true;
}

QString SqliteYtx::SettlementOrder(const QString& order)
{
    return QString(R"(
    CREATE TABLE IF NOT EXISTS %1_settlement (
        id             INTEGER PRIMARY KEY AUTOINCREMENT,
        party          INTEGER,
        date_time      DATE,
        description    TEXT,
        finished       BOOLEAN    DEFAULT 0,
        gross_amount   NUMERIC,
        removed        BOOLEAN    DEFAULT 0
    );
    )")
        .arg(order);
}

#if 0
bool SqliteYtx::TransIndex(QSqlQuery& /*query*/)
{
    bool success = true;

    // Create an index on (lhs_node)
    QStringList lhs_tables {
        kTaskTrans,
        kProductTrans,
        kFinanceTrans,
        kStakeholderTrans,
        kSalesTrans,
        kPurchaseTrans,
    };
    for (const auto& table : lhs_tables) {
        QString sql = QString("CREATE INDEX IF NOT EXISTS idx_%1_lhs_node ON %1 (lhs_node);").arg(table);
        if (!query.exec(sql)) {
            qDebug() << "Failed to create idx_" << table << "_lhs_node index: " << query.lastError().text();
            success = false;
        }
    }

    // Create an index on (inside_product)
    QStringList inside_product_tables {
        kSalesTrans,
        kPurchaseTrans,
    };
    for (const auto& table : inside_product_tables) {
        QString sql = QString("CREATE INDEX IF NOT EXISTS idx_%1_inside_product ON %1 (inside_product);").arg(table);
        if (!query.exec(sql)) {
            qDebug() << "Failed to create idx_" << table << "_inside_product index: " << query.lastError().text();
            success = false;
        }
    }

    // Create an index on (rhs_node)
    QStringList rhs_tables {
        kTaskTrans,
        kProductTrans,
        kFinanceTrans,
    };
    for (const auto& table : rhs_tables) {
        QString sql = QString("CREATE INDEX IF NOT EXISTS idx_%1_rhs_node ON %1 (rhs_node);").arg(table);
        if (!query.exec(sql)) {
            qDebug() << "Failed to create idx_" << table << "_rhs_node index: " << query.lastError().text();
            success = false;
        }
    }

    QStringList value_tables {
        kTaskTrans,
        kProductTrans,
        kFinanceTrans,
    };

    for (const auto& table : value_tables) {
        QStringList index_columns { "lhs_debit", "lhs_credit", "rhs_debit", "rhs_credit" };

        for (const auto& column : index_columns) {
            const QString sql { QString("CREATE INDEX IF NOT EXISTS idx_%1_%2 ON %1 (%2);").arg(table, column) };

            if (!query.exec(sql)) {
                qDebug() << "Failed to create index " << table << ": " << query.lastError().text();
                success = false;
            }
        }
    }

    const QString idx_stakeholder { QString("CREATE INDEX IF NOT EXISTS idx_%1_%2 ON %1 (%2);").arg(kStakeholderTrans, kUnitPrice) };
    if (!query.exec(idx_stakeholder)) {
        qDebug() << "Failed to create index " << kStakeholderTrans << ": " << query.lastError().text();
        success = false;
    }

    QStringList order_tables {
        kSalesTrans,
        kPurchaseTrans,
    };

    for (const auto& table : order_tables) {
        QStringList index_columns { kFirst, kSecond };

        for (const auto& column : index_columns) {
            const QString sql { QString("CREATE INDEX IF NOT EXISTS idx_%1_%2 ON %1 (%2);").arg(table, column) };

            if (!query.exec(sql)) {
                qDebug() << "Failed to create index " << table << ": " << query.lastError().text();
                success = false;
            }
        }
    }

    return true;
}
#endif
