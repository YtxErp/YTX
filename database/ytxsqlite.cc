#include "ytxsqlite.h"

#include <QSqlError>
#include <QSqlQuery>

#include "component/constvalue.h"
#include "global/sqlconnection.h"

YtxSqlite::YtxSqlite(Section section)
    : db_ { SqlConnection::Instance().Allocate(section) }
{
}

void YtxSqlite::QuerySettings(Settings& settings, Section section)
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

void YtxSqlite::UpdateSettings(CSettings& settings, Section section)
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

bool YtxSqlite::NewFile(CString& file_path)
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
            && query.exec(sales) && query.exec(sales_path) && query.exec(sales_trans) && query.exec(settings)) {
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

QString YtxSqlite::NodeFinance()
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

QString YtxSqlite::NodeStakeholder()
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

QString YtxSqlite::NodeProduct()
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

QString YtxSqlite::NodeTask()
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

QString YtxSqlite::NodeOrder(CString& order)
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
        payment_id        INTEGER    DEFAULT 0
    );
    )")
        .arg(order);
}

QString YtxSqlite::Path(CString& table_name)
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

QString YtxSqlite::TransFinance()
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

QString YtxSqlite::TransOrder(CString& order)
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

QString YtxSqlite::TransStakeholder()
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
        removed            BOOLEAN    DEFAULT 0
    );
    )");
}

QString YtxSqlite::TransTask()
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

QString YtxSqlite::TransProduct()
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
