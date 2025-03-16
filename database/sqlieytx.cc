#include "sqlieytx.h"

#include <QSqlError>
#include <QSqlQuery>

#include "component/constvalue.h"
#include "global/databasemanager.h"

SqlieYtx::SqlieYtx()
    : db_ { DatabaseManager::Instance().GetDatabase() }
{
}

void SqlieYtx::QuerySettings(Settings& settings, Section section)
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    auto part = QStringLiteral(R"(
    SELECT static_label, static_node, dynamic_label, dynamic_node_lhs, operation, dynamic_node_rhs, default_unit, document_dir, date_format, amount_decimal, common_decimal
    FROM settings
    WHERE id = :section
)");

    query.prepare(part);
    query.bindValue(QStringLiteral(":section"), std::to_underlying(section) + 1);
    if (!query.exec()) {
        qWarning() << "Failed to query section settings: " << query.lastError().text();
        return;
    }

    while (query.next()) {
        settings.static_label = query.value(QStringLiteral("static_label")).toString();
        settings.static_node = query.value(QStringLiteral("static_node")).toInt();
        settings.dynamic_label = query.value(QStringLiteral("dynamic_label")).toString();
        settings.dynamic_node_lhs = query.value(QStringLiteral("dynamic_node_lhs")).toInt();
        settings.operation = query.value(QStringLiteral("operation")).toString();
        settings.dynamic_node_rhs = query.value(QStringLiteral("dynamic_node_rhs")).toInt();
        settings.default_unit = query.value(QStringLiteral("default_unit")).toInt();
        settings.document_dir = query.value(QStringLiteral("document_dir")).toString();
        settings.date_format = query.value(QStringLiteral("date_format")).toString();
        settings.amount_decimal = query.value(QStringLiteral("amount_decimal")).toInt();
        settings.common_decimal = query.value(QStringLiteral("common_decimal")).toInt();
    }
}

void SqlieYtx::UpdateSettings(CSettings& settings, Section section)
{
    auto part = QStringLiteral(R"(
    UPDATE settings SET
        static_label = :static_label, static_node = :static_node, dynamic_label = :dynamic_label, dynamic_node_lhs = :dynamic_node_lhs,
        operation = :operation, dynamic_node_rhs = :dynamic_node_rhs, default_unit = :default_unit, document_dir = :document_dir,
        date_format = :date_format, amount_decimal = :amount_decimal, common_decimal = :common_decimal
    WHERE id = :section
)");

    QSqlQuery query(*db_);

    query.prepare(part);
    query.bindValue(QStringLiteral(":section"), std::to_underlying(section) + 1);
    query.bindValue(QStringLiteral(":static_label"), settings.static_label);
    query.bindValue(QStringLiteral(":static_node"), settings.static_node);
    query.bindValue(QStringLiteral(":dynamic_label"), settings.dynamic_label);
    query.bindValue(QStringLiteral(":dynamic_node_lhs"), settings.dynamic_node_lhs);
    query.bindValue(QStringLiteral(":operation"), settings.operation);
    query.bindValue(QStringLiteral(":dynamic_node_rhs"), settings.dynamic_node_rhs);
    query.bindValue(QStringLiteral(":default_unit"), settings.default_unit);
    query.bindValue(QStringLiteral(":document_dir"), settings.document_dir);
    query.bindValue(QStringLiteral(":date_format"), settings.date_format);
    query.bindValue(QStringLiteral(":amount_decimal"), settings.amount_decimal);
    query.bindValue(QStringLiteral(":common_decimal"), settings.common_decimal);

    if (!query.exec()) {
        qWarning() << "Failed to update section settings: " << query.lastError().text();
        return;
    }
}

bool SqlieYtx::NewFile(CString& file_path)
{
    QSqlDatabase db { QSqlDatabase::addDatabase(kQSQLITE) };
    db.setDatabaseName(file_path);
    if (!db.open())
        return false;

    QString finance = NodeFinance();
    QString finance_path = Path(kFinancePath);
    QString finance_trans = TransFinance();

    QString product = NodeProduct();
    QString product_path = Path(kProductPath);
    QString product_trans = TransProduct();

    QString task = NodeTask();
    QString task_path = Path(kTaskPath);
    QString task_trans = TransTask();

    QString stakeholder = NodeStakeholder();
    QString stakeholder_path = Path(kStakeholderPath);
    QString stakeholder_trans = TransStakeholder();

    QString purchase = NodeOrder(kPurchase);
    QString purchase_path = Path(kPurchasePath);
    QString purchase_trans = TransOrder(kPurchaseTrans);

    QString sales = NodeOrder(kSales);
    QString sales_path = Path(kSalesPath);
    QString sales_trans = TransOrder(kSalesTrans);

    QString settings = QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS settings (
        id                  INTEGER PRIMARY KEY AUTOINCREMENT,
        static_label        TEXT,
        static_node         INTEGER,
        dynamic_label       TEXT,
        dynamic_node_lhs    INTEGER,
        operation           TEXT,
        dynamic_node_rhs    INTEGER,
        default_unit        INTEGER,
        document_dir        TEXT,
        date_format         TEXT       DEFAULT 'yyyy-MM-dd HH:mm',
        amount_decimal      INTEGER    DEFAULT 2,
        common_decimal      INTEGER    DEFAULT 2
    );
    )");

    QSqlQuery query {};
    if (db.transaction()) {
        // Execute each table creation query
        if (query.exec(finance) && query.exec(finance_path) && query.exec(finance_trans) && query.exec(product) && query.exec(product_path)
            && query.exec(product_trans) && query.exec(stakeholder) && query.exec(stakeholder_path) && query.exec(stakeholder_trans) && query.exec(task)
            && query.exec(task_path) && query.exec(task_trans) && query.exec(purchase) && query.exec(purchase_path) && query.exec(purchase_trans)
            && query.exec(sales) && query.exec(sales_path) && query.exec(sales_trans) && query.exec(settings) && NodeIndex(query) && TransIndex(query)) {
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

QString SqlieYtx::NodeFinance()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS finance (
        name             TEXT,
        id               INTEGER PRIMARY KEY AUTOINCREMENT,
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

QString SqlieYtx::NodeStakeholder()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS stakeholder (
        name              TEXT,
        id                INTEGER PRIMARY KEY AUTOINCREMENT,
        code              TEXT,
        description       TEXT,
        note              TEXT,
        type              INTEGER,
        unit              INTEGER,
        deadline          TEXT,
        employee          INTEGER,
        payment_term      INTEGER,
        tax_rate          NUMERIC,
        amount            NUMERIC,
        removed           BOOLEAN    DEFAULT 0
    );
    )");
}

QString SqlieYtx::NodeProduct()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS product (
        name             TEXT,
        id               INTEGER PRIMARY KEY AUTOINCREMENT,
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

QString SqlieYtx::NodeTask()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS task (
        name             TEXT,
        id               INTEGER PRIMARY KEY AUTOINCREMENT,
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

QString SqlieYtx::NodeOrder(CString& order)
{
    return QString(R"(
    CREATE TABLE IF NOT EXISTS %1 (
        name              TEXT,
        id                INTEGER PRIMARY KEY AUTOINCREMENT,
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
        removed           BOOLEAN    DEFAULT 0,
        settlement_id     INTEGER    DEFAULT 0
    );
    )")
        .arg(order);
}

QString SqlieYtx::Path(CString& table_name)
{
    return QString(R"(
    CREATE TABLE IF NOT EXISTS %1 (
        ancestor      INTEGER    CHECK (ancestor   >= 1),
        descendant    INTEGER    CHECK (descendant >= 1),
        distance      INTEGER    CHECK (distance   >= 0)
    );
)")
        .arg(table_name);
}

QString SqlieYtx::TransFinance()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS finance_transaction (
        date_time      DATE,
        id             INTEGER PRIMARY KEY AUTOINCREMENT,
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

QString SqlieYtx::TransOrder(CString& order)
{
    return QString(R"(
    CREATE TABLE IF NOT EXISTS %1 (
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

QString SqlieYtx::TransStakeholder()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS stakeholder_transaction (
        date_time          DATE,
        id                 INTEGER PRIMARY KEY AUTOINCREMENT,
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

QString SqlieYtx::TransTask()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS task_transaction (
        date_time      DATE,
        id             INTEGER PRIMARY KEY AUTOINCREMENT,
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

QString SqlieYtx::TransProduct()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS product_transaction (
        date_time      DATE,
        id             INTEGER PRIMARY KEY AUTOINCREMENT,
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

bool SqlieYtx::NodeIndex(QSqlQuery& query)
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

bool SqlieYtx::TransIndex(QSqlQuery& /*query*/)
{
    // bool success = true;

    // // Create an index on (lhs_node)
    // QStringList lhs_tables {
    //     kTaskTrans,
    //     kProductTrans,
    //     kFinanceTrans,
    //     kStakeholderTrans,
    //     kSalesTrans,
    //     kPurchaseTrans,
    // };
    // for (const auto& table : lhs_tables) {
    //     QString sql = QString("CREATE INDEX IF NOT EXISTS idx_%1_lhs_node ON %1 (lhs_node);").arg(table);
    //     if (!query.exec(sql)) {
    //         qDebug() << "Failed to create idx_" << table << "_lhs_node index: " << query.lastError().text();
    //         success = false;
    //     }
    // }

    // // Create an index on (inside_product)
    // QStringList inside_product_tables {
    //     kSalesTrans,
    //     kPurchaseTrans,
    // };
    // for (const auto& table : inside_product_tables) {
    //     QString sql = QString("CREATE INDEX IF NOT EXISTS idx_%1_inside_product ON %1 (inside_product);").arg(table);
    //     if (!query.exec(sql)) {
    //         qDebug() << "Failed to create idx_" << table << "_inside_product index: " << query.lastError().text();
    //         success = false;
    //     }
    // }

    // // Create an index on (rhs_node)
    // QStringList rhs_tables {
    //     kTaskTrans,
    //     kProductTrans,
    //     kFinanceTrans,
    // };
    // for (const auto& table : rhs_tables) {
    //     QString sql = QString("CREATE INDEX IF NOT EXISTS idx_%1_rhs_node ON %1 (rhs_node);").arg(table);
    //     if (!query.exec(sql)) {
    //         qDebug() << "Failed to create idx_" << table << "_rhs_node index: " << query.lastError().text();
    //         success = false;
    //     }
    // }

    // QStringList value_tables {
    //     kTaskTrans,
    //     kProductTrans,
    //     kFinanceTrans,
    // };

    // for (const auto& table : value_tables) {
    //     QStringList index_columns { "lhs_debit", "lhs_credit", "rhs_debit", "rhs_credit" };

    //     for (const auto& column : index_columns) {
    //         CString sql { QString("CREATE INDEX IF NOT EXISTS idx_%1_%2 ON %1 (%2);").arg(table, column) };

    //         if (!query.exec(sql)) {
    //             qDebug() << "Failed to create index " << table << ": " << query.lastError().text();
    //             success = false;
    //         }
    //     }
    // }

    // CString idx_stakeholder { QString("CREATE INDEX IF NOT EXISTS idx_%1_%2 ON %1 (%2);").arg(kStakeholderTrans, kUnitPrice) };
    // if (!query.exec(idx_stakeholder)) {
    //     qDebug() << "Failed to create index " << kStakeholderTrans << ": " << query.lastError().text();
    //     success = false;
    // }

    // QStringList order_tables {
    //     kSalesTrans,
    //     kPurchaseTrans,
    // };

    // for (const auto& table : order_tables) {
    //     QStringList index_columns { kFirst, kSecond };

    //     for (const auto& column : index_columns) {
    //         CString sql { QString("CREATE INDEX IF NOT EXISTS idx_%1_%2 ON %1 (%2);").arg(table, column) };

    //         if (!query.exec(sql)) {
    //             qDebug() << "Failed to create index " << table << ": " << query.lastError().text();
    //             success = false;
    //         }
    //     }
    // }

    return true;
}
