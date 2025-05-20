#include "sqliteo.h"

#include <QDate>
#include <QSqlError>
#include <QSqlQuery>

#include "component/constvalue.h"
#include "global/resourcepool.h"

SqliteO::SqliteO(CInfo& info, QObject* parent)
    : Sqlite(info, parent)
{
}

SqliteO::~SqliteO() { qDeleteAll(node_hash_); }

bool SqliteO::ReadNode(NodeHash& node_hash, const QDateTime& start, const QDateTime& end)
{
    QSqlQuery query(db_);
    query.setForwardOnly(true);

    CString string { QSReadNode() };

    query.prepare(string);
    query.bindValue(QStringLiteral(":start"), start.toString(kDateTimeFST));
    query.bindValue(QStringLiteral(":end"), end.toString(kDateTimeFST));

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(section_) << "Failed in ReadNode" << query.lastError().text();
        return false;
    }

    ReadNodeFunction(node_hash, query);
    return true;
}

void SqliteO::ReadNodeFunction(NodeHash& node_hash, QSqlQuery& query)
{
    for (auto* node : node_hash) {
        node->parent = nullptr;
        node->children.clear();
    }

    node_hash.clear();

    while (query.next()) {
        const int kID { query.value(QStringLiteral("id")).toInt() };

        if (auto it = node_hash_.constFind(kID); it != node_hash_.constEnd()) {
            node_hash.insert(kID, it.value());
            continue;
        }

        Node* node { ResourcePool<Node>::Instance().Allocate() };
        ReadNodeQuery(node, query);
        node_hash.insert(kID, node);
        node_hash_.insert(kID, node);
    }

    if (!node_hash.isEmpty())
        ReadRelationship(node_hash, query);
}

bool SqliteO::SearchNode(QList<const Node*>& node_list, const QList<int>& party_id_list)
{
    if (party_id_list.empty())
        return false;

    QSqlQuery query(db_);
    query.setForwardOnly(true);

    const qsizetype batch_size { kBatchSize };
    const auto total_batches { (party_id_list.size() + batch_size - 1) / batch_size };

    for (int batch_index = 0; batch_index != total_batches; ++batch_index) {
        int start = batch_index * batch_size;
        int end = std::min(start + batch_size, party_id_list.size());

        QList<int> current_batch { party_id_list.mid(start, end - start) };

        QStringList placeholder { current_batch.size(), QStringLiteral("?") };
        QString string { QSSearchNode(placeholder.join(QStringLiteral(","))) };

        query.prepare(string);

        for (int i = 0; i != current_batch.size(); ++i)
            query.bindValue(i, current_batch.at(i));

        if (!query.exec()) {
            qWarning() << "Section: " << std::to_underlying(section_) << "Failed in SearchNode, batch" << batch_index << ": " << query.lastError().text();
            continue;
        }

        SearchNodeFunction(node_list, query);
    }

    return true;
}

void SqliteO::SearchNodeFunction(QList<const Node*>& node_list, QSqlQuery& query)
{
    while (query.next()) {
        const int kID { query.value(QStringLiteral("id")).toInt() };

        if (auto it = node_hash_.constFind(kID); it != node_hash_.constEnd()) {
            node_list.emplaceBack(it.value());
            continue;
        }

        auto* node { ResourcePool<Node>::Instance().Allocate() };
        ReadNodeQuery(node, query);
        node_list.emplaceBack(node);
        node_hash_.insert(kID, node);
    }
}

Node* SqliteO::ReadNode(int node_id)
{
    if (auto it = node_hash_.constFind(node_id); it != node_hash_.constEnd()) {
        return it.value();
    }

    // if search order trans, will read node from sqlite3

    QSqlQuery query(db_);
    query.setForwardOnly(true);

    CString string { QString(R"(
    SELECT name, id, description, rule, type, unit, party, employee, date_time, first, second, discount, finished, gross_amount, settlement, settlement_id
    FROM %1
    WHERE id = :node_id AND removed = 0
    )")
            .arg(info_.node) };

    query.prepare(string);
    query.bindValue(QStringLiteral(":node_id"), node_id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(section_) << "Failed in ReadNode" << query.lastError().text();
        return {};
    }

    if (!query.next())
        return {};

    Node* node = ResourcePool<Node>::Instance().Allocate();
    ReadNodeQuery(node, query);

    node_hash_.insert(node_id, node);
    return node;
}

bool SqliteO::SettlementReference(int settlement_id) const
{
    assert(settlement_id >= 1 && "settlement_id must be positive");

    CString string { QString(R"(
    SELECT 1 FROM %1
    WHERE settlement_id = :settlement_id AND removed = 0
    LIMIT 1
    )")
            .arg(info_.node) };

    QSqlQuery query(db_);
    query.setForwardOnly(true);

    query.prepare(string);
    query.bindValue(QStringLiteral(":settlement_id"), settlement_id);

    if (!query.exec()) {
        qWarning() << "Failed in SettlementReference" << query.lastError().text();
        return false;
    }

    return query.next();
}

int SqliteO::SettlementID(int node_id) const
{
    assert(node_id >= 1 && "node_id must be positive");

    CString string { QString(R"(
    SELECT settlement_id FROM %1
    WHERE id = :node_id AND removed = 0
    )")
            .arg(info_.node) };

    QSqlQuery query(db_);
    query.setForwardOnly(true);

    query.prepare(string);
    query.bindValue(QStringLiteral(":node_id"), node_id);

    if (!query.exec()) {
        qWarning() << "Failed in SettlementID" << query.lastError().text();
        return false;
    }

    if (query.next())
        return query.value(0).toInt();

    return 0;
}

void SqliteO::RRemoveNode(int node_id, int node_type)
{
    // This function is triggered when removing a node that has internal references.

    emit SFreeWidget(node_id, node_type);
    emit SRemoveNode(node_id);

    // Mark Trans for removal
    QMultiHash<int, int> leaf_trans {};
    QMultiHash<int, int> support_trans {};

    TransToRemove(leaf_trans, support_trans, node_id, kTypeLeaf);
    RemoveLeafFunction(leaf_trans);

    // Remove node, path, trans from the sqlite3 database
    RemoveNode(node_id, kTypeLeaf);
}

void SqliteO::RSyncStakeholder(int old_node_id, int new_node_id) const
{
    for (auto* node : std::as_const(node_hash_)) {
        if (node->party == old_node_id)
            node->party = new_node_id;

        if (node->employee == old_node_id)
            node->employee = new_node_id;
    }
}

void SqliteO::RSyncProduct(int old_node_id, int new_node_id) const
{
    for (auto* trans : std::as_const(trans_hash_)) {
        if (trans->rhs_node == old_node_id)
            trans->rhs_node = new_node_id;
    }
}

bool SqliteO::ReadSettlement(NodeList& node_list, const QDateTime& start, const QDateTime& end) const
{
    QSqlQuery query(db_);
    query.setForwardOnly(true);

    auto string { QSReadSettlement() };

    query.prepare(string);
    query.bindValue(QStringLiteral(":start"), start.toString(kDateTimeFST));
    query.bindValue(QStringLiteral(":end"), end.toString(kDateTimeFST));

    if (!query.exec()) {
        qWarning() << "Failed in ReadStatement" << query.lastError().text();
        return false;
    }

    ReadSettlementQuery(node_list, query);
    return true;
}

bool SqliteO::WriteSettlement(Node* node) const
{
    assert(node && "node should not be null");

    QSqlQuery query(db_);

    CString string { QSWriteSettlement() };

    query.prepare(string);
    WriteSettlementBind(node, query);

    if (!query.exec()) {
        qWarning() << "Failed in WriteSettlement" << query.lastError().text();
        return false;
    }

    node->id = query.lastInsertId().toInt();
    return true;
}

bool SqliteO::RemoveSettlement(int settlement_id)
{
    QSqlQuery query(db_);

    CString string_first { QSRemoveSettlementFirst() };
    CString string_second { QSRemoveSettlementSecond() };

    return DBTransaction([&]() {
        query.prepare(string_first);
        query.bindValue(QStringLiteral(":node_id"), settlement_id);
        if (!query.exec()) {
            qWarning() << "Failed in RemoveSettlement 1st" << query.lastError().text();
            return false;
        }

        query.prepare(string_second);
        query.bindValue(QStringLiteral(":node_id"), settlement_id);
        if (!query.exec()) {
            qWarning() << "Failed in RemoveSettlement 2nd" << query.lastError().text();
            return false;
        }

        return true;
    });
}

bool SqliteO::ReadSettlementPrimary(NodeList& node_list, int party_id, int settlement_id, bool finished)
{
    QSqlQuery query(db_);
    query.setForwardOnly(true);

    CString string { QSReadSettlementPrimary(finished) };

    query.prepare(string);
    query.bindValue(QStringLiteral(":party_id"), party_id);
    query.bindValue(QStringLiteral(":settlement_id"), settlement_id);

    if (!query.exec()) {
        qWarning() << "Failed in ReadSettlementPrimary" << query.lastError().text();
        return false;
    }

    ReadSettlementPrimaryQuery(node_list, query);
    return true;
}

bool SqliteO::AddSettlementPrimary(int node_id, int settlement_id) const
{
    QSqlQuery query(db_);
    query.setForwardOnly(true);

    CString string { QString("UPDATE %1 SET settlement_id = :settlement_id, settlement = gross_amount - discount WHERE id = :node_id").arg(info_.node) };

    query.prepare(string);
    query.bindValue(QStringLiteral(":node_id"), node_id);
    query.bindValue(QStringLiteral(":settlement_id"), settlement_id);

    if (!query.exec()) {
        qWarning() << "Failed in SyncNewSettlement" << query.lastError().text();
        return false;
    }

    auto* node { node_hash_.value(node_id) };
    if (node) {
        node->final_total = node->initial_total - node->discount;
    }

    return true;
}

bool SqliteO::RemoveSettlementPrimary(int node_id) const
{
    QSqlQuery query(db_);
    query.setForwardOnly(true);

    CString string { QString("UPDATE %1 SET settlement_id = 0, settlement = 0 WHERE id = :node_id").arg(info_.node) };

    query.prepare(string);
    query.bindValue(QStringLiteral(":node_id"), node_id);

    if (!query.exec()) {
        qWarning() << "Failed in SyncNewSettlement" << query.lastError().text();
        return false;
    }

    auto* node { node_hash_.value(node_id) };
    if (node) {
        node->final_total = 0.0;
    }

    return true;
}

bool SqliteO::SyncPrice(int node_id)
{
    QSqlQuery query(db_);
    query.setForwardOnly(true);

    CString string { QSSyncPriceFirst() };

    query.prepare(string);
    query.bindValue(QStringLiteral(":node_id"), node_id);

    if (!query.exec()) {
        qWarning() << "SQL execution failed in QSSyncStakeholderPriceFirst:" << query.lastError().text();
        return false;
    }

    CString string_second { QSSyncPriceSecond() };

    query.prepare(string_second);
    query.bindValue(QStringLiteral(":node_id"), node_id);

    if (!query.exec()) {
        qWarning() << "SQL execution failed in QSSyncStakeholderPriceSecond:" << query.lastError().text();
        return false;
    }

    QList<PriceS> list {};

    while (query.next()) {
        PriceS item {};
        item.date_time = query.value("date_time").toString();
        item.lhs_node = query.value("lhs_node").toInt();
        item.inside_product = query.value("inside_product").toInt();
        item.unit_price = query.value("unit_price").toDouble();

        list.append(std::move(item));
    }

    emit SSyncPrice(list);
    return true;
}

bool SqliteO::InvertTransValue(int node_id) const
{
    QSqlQuery query(db_);

    CString string { QSInvertTransValue() };

    query.prepare(string);
    query.bindValue(":lhs_node", node_id);

    if (!query.exec()) {
        qWarning() << "Failed in InvertValue" << query.lastError().text();
        return false;
    }

    return true;
}

bool SqliteO::ReadStatement(TransList& trans_list, int unit, const QDateTime& start, const QDateTime& end) const
{
    QSqlQuery query(db_);
    query.setForwardOnly(true);

    CString string { QSReadStatement(unit) };

    query.prepare(string);
    query.bindValue(QStringLiteral(":start"), start.toString(kDateTimeFST));
    query.bindValue(QStringLiteral(":end"), end.toString(kDateTimeFST));

    if (!query.exec()) {
        qWarning() << "Failed in ReadStatement" << query.lastError().text();
        return false;
    }

    ReadStatementQuery(trans_list, query);

    return true;
}

bool SqliteO::ReadBalance(double& pbalance, double& cdelta, int party_id, int unit, const QDateTime& start, const QDateTime& end) const
{
    QSqlQuery query(db_);
    query.setForwardOnly(true);

    CString string { QSReadBalance(unit) };

    query.prepare(string);
    query.bindValue(QStringLiteral(":start"), start.toString(kDateTimeFST));
    query.bindValue(QStringLiteral(":end"), end.toString(kDateTimeFST));
    query.bindValue(QStringLiteral(":party_id"), party_id);

    if (!query.exec()) {
        qWarning() << "Failed in ReadStatement" << query.lastError().text();
        return false;
    }

    if (query.next()) {
        pbalance = query.value(QStringLiteral("pbalance")).toDouble();
        cdelta = query.value(QStringLiteral("cgross_amount")).toDouble() - query.value(QStringLiteral("csettlement")).toDouble();
    }

    return true;
}

bool SqliteO::ReadStatementPrimary(NodeList& node_list, int party_id, int unit, const QDateTime& start, const QDateTime& end) const
{
    QSqlQuery query(db_);
    query.setForwardOnly(true);

    CString string { QSReadStatementPrimary(unit) };

    query.prepare(string);
    query.bindValue(QStringLiteral(":start"), start.toString(kDateTimeFST));
    query.bindValue(QStringLiteral(":end"), end.toString(kDateTimeFST));
    query.bindValue(QStringLiteral(":party"), party_id);
    query.bindValue(QStringLiteral(":unit"), unit);

    if (!query.exec()) {
        qWarning() << "Failed in ReadStatementPrimary" << query.lastError().text();
        return false;
    }

    ReadStatementPrimaryQuery(node_list, query);
    return true;
}

bool SqliteO::ReadStatementSecondary(TransList& trans_list, int party_id, int unit, const QDateTime& start, const QDateTime& end) const
{
    QSqlQuery query(db_);
    query.setForwardOnly(true);

    CString string { QSReadStatementSecondary(unit) };

    query.prepare(string);
    query.bindValue(QStringLiteral(":start"), start.toString(kDateTimeFST));
    query.bindValue(QStringLiteral(":end"), end.toString(kDateTimeFST));
    query.bindValue(QStringLiteral(":party"), party_id);
    query.bindValue(QStringLiteral(":unit"), unit);

    if (!query.exec()) {
        qWarning() << "Failed in ReadStatementPrimary" << query.lastError().text();
        return false;
    }

    ReadStatementSecondaryQuery(trans_list, query);
    return true;
}

bool SqliteO::WriteTransRange(const QList<TransShadow*>& list)
{
    if (list.isEmpty())
        return false;

    QSqlQuery query(db_);

    query.exec(QStringLiteral("PRAGMA synchronous = OFF"));
    query.exec(QStringLiteral("PRAGMA journal_mode = MEMORY"));

    if (!db_.transaction()) {
        qDebug() << "Failed to start transaction" << db_.lastError();
        return false;
    }

    CString string { QSWriteTrans() };

    // 插入多条记录的 SQL 语句
    query.prepare(string);
    WriteTransRangeFunction(list, query);

    // 执行批量插入
    if (!query.execBatch()) {
        qDebug() << "Failed in WriteTransRange" << query.lastError();
        db_.rollback();
        return false;
    }

    // 提交事务
    if (!db_.commit()) {
        qDebug() << "Failed to commit transaction" << db_.lastError();
        db_.rollback();
        return false;
    }

    query.exec(QStringLiteral("PRAGMA synchronous = FULL"));
    query.exec(QStringLiteral("PRAGMA journal_mode = DELETE"));

    int last_id { query.lastInsertId().toInt() };

    for (int i = list.size() - 1; i >= 0; --i) {
        *list.at(i)->id = last_id;
        --last_id;
    }

    return true;
}

/**
 * @brief Read all relevant nodes from the database table.
 *
 * This SQL query retrieves records from the table specified in \a info_.node.
 * It includes:
 * - Branch nodes (\c type = 1)
 * - Unfinished nodes (\c finished = 0)
 * - Pending nodes (\c unit = 2)
 * - Nodes within the specified date range (\c date_time BETWEEN :start AND :end)
 * - Excludes removed nodes (\c removed = 0)
 *
 * The table name is dynamically set by \a info_.node.
 *
 * @return A SQL query string for node selection.
 */
QString SqliteO::QSReadNode() const
{
    return QString(R"(
    SELECT name, id, description, rule, type, unit, party, employee, date_time, first, second, discount, finished, gross_amount, settlement
    FROM %1
    WHERE ((date_time BETWEEN :start AND :end) OR type = 1 OR finished = false OR unit = 2) AND removed = false
    )")
        .arg(info_.node);
}

QString SqliteO::QSWriteNode() const
{
    return QString(R"(
    INSERT INTO %1 (name, description, rule, type, unit, party, employee, date_time, first, second, discount, finished, gross_amount, settlement)
    VALUES (:name, :description, :rule, :type, :unit, :party, :employee, :date_time, :first, :second, :discount, :finished, :gross_amount, :settlement)
    )")
        .arg(info_.node);
}

QString SqliteO::QSRemoveNodeSecond() const
{
    return QString(R"(
    UPDATE %1 SET
        removed = true
    WHERE lhs_node = :node_id
    )")
        .arg(info_.trans);
}

QString SqliteO::QSInternalReference() const
{
    return QString(R"(
    SELECT EXISTS(
        SELECT 1 FROM %1
        WHERE lhs_node = :node_id AND removed = 0
    ) AS is_referenced
    )")
        .arg(info_.trans);
}

QString SqliteO::QSExternalReference() const
{
    return QString(R"(
    SELECT EXISTS(
        SELECT 1 FROM %1
        WHERE id = :node_id AND settlement_id <> 0 AND removed = 0
    ) AS is_referenced
    )")
        .arg(info_.node);
}

QString SqliteO::QSReadTrans() const
{
    return QString(R"(
    SELECT id, code, inside_product, unit_price, second, description, lhs_node, first, gross_amount, discount, net_amount, outside_product, discount_price
    FROM %1
    WHERE lhs_node = :node_id AND removed = 0
    )")
        .arg(info_.trans);
}

QString SqliteO::QSWriteTrans() const
{
    return QString(R"(
    INSERT INTO %1 (code, inside_product, unit_price, second, description, lhs_node, first, gross_amount, discount, net_amount, outside_product, discount_price)
    VALUES (:code, :inside_product, :unit_price, :second, :description, :lhs_node, :first, :gross_amount, :discount, :net_amount, :outside_product, :discount_price)
    )")
        .arg(info_.trans);
}

QString SqliteO::QSSearchTransValue() const
{
    return QString(R"(
    SELECT id, code, inside_product, unit_price, second, description, lhs_node, first, gross_amount, discount, net_amount, outside_product, discount_price
    FROM %1
    WHERE ((first BETWEEN :value - :tolerance AND :value + :tolerance)
        OR (second BETWEEN :value - :tolerance AND :value + :tolerance))
        AND removed = 0
    )")
        .arg(info_.trans);
}

QString SqliteO::QSSearchTransText() const
{
    return QString(R"(
    SELECT id, code, inside_product, unit_price, second, description, lhs_node, first, gross_amount, discount, net_amount, outside_product, discount_price
    FROM %1
    WHERE description LIKE :description AND removed = 0
    )")
        .arg(info_.trans);
}

QString SqliteO::QSUpdateTransValue() const
{
    return QString(R"(
    UPDATE %1 SET
        second = :second, gross_amount = :gross_amount, discount = :discount, net_amount = :net_amount, first = :first
    WHERE id = :trans_id
    )")
        .arg(info_.trans);
}

QString SqliteO::QSTransToRemove() const
{
    return QString(R"(
    SELECT lhs_node AS node_id, id AS trans_id FROM %1
    WHERE lhs_node = :node_id AND removed = 0
    )")
        .arg(info_.trans);
}

QString SqliteO::QSReadStatement(int unit) const
{
    switch (UnitO(unit)) {
    case UnitO::kIS:
        return QString(R"(
            WITH Statement AS (
                SELECT
                    s.id AS party,
                    SUM(CASE WHEN o.date_time BETWEEN :start AND :end THEN o.gross_amount ELSE 0 END) AS cgross_amount,
                    SUM(CASE WHEN o.date_time BETWEEN :start AND :end THEN o.first        ELSE 0 END) AS cfirst,
                    SUM(CASE WHEN o.date_time BETWEEN :start AND :end THEN o.second       ELSE 0 END) AS csecond
                FROM stakeholder s
                INNER JOIN %1 o ON s.id = o.party
                WHERE o.unit = 0 AND o.finished = 1 AND o.removed = 0
                GROUP BY s.id
            )
            SELECT
                party,
                0 AS pbalance,
                0 AS cbalance,
                cgross_amount,
                cgross_amount AS csettlement,
                cfirst,
                csecond
            FROM Statement
            )")
            .arg(info_.node);
    case UnitO::kMS:
        return QString(R"(
            WITH Statement AS (
                SELECT
                    s.id AS party,

                    SUM(CASE WHEN o.date_time < :start AND o.settlement_id = 0 THEN o.gross_amount                 ELSE 0 END) AS pbalance,
                    SUM(CASE WHEN o.date_time BETWEEN :start AND :end THEN o.gross_amount                          ELSE 0 END) AS cgross_amount,
                    SUM(CASE WHEN o.date_time BETWEEN :start AND :end AND o.settlement_id != 0 THEN o.gross_amount ELSE 0 END) AS csettlement,
                    SUM(CASE WHEN o.date_time BETWEEN :start AND :end THEN o.first                                 ELSE 0 END) AS cfirst,
                    SUM(CASE WHEN o.date_time BETWEEN :start AND :end THEN o.second                                ELSE 0 END) AS csecond

                FROM stakeholder s
                INNER JOIN %1 o ON s.id = o.party
                WHERE o.unit = 1 AND o.finished = 1 AND o.removed = 0
                GROUP BY s.id
            )
            SELECT
                party,
                pbalance,
                cgross_amount,
                csettlement,
                pbalance + cgross_amount - csettlement AS cbalance,
                cfirst,
                csecond
            FROM Statement;
            )")
            .arg(info_.node);
    case UnitO::kPEND:
        return QString(R"(
            WITH Statement AS (
                SELECT
                    s.id AS party,
                    SUM(CASE WHEN o.date_time < :start THEN o.gross_amount                ELSE 0 END) AS pbalance,
                    SUM(CASE WHEN o.date_time BETWEEN :start AND :end THEN o.gross_amount ELSE 0 END) AS cgross_amount,
                    SUM(CASE WHEN o.date_time BETWEEN :start AND :end THEN o.first        ELSE 0 END) AS cfirst,
                    SUM(CASE WHEN o.date_time BETWEEN :start AND :end THEN o.second       ELSE 0 END) AS csecond
                FROM stakeholder s
                INNER JOIN %1 o ON s.id = o.party
                WHERE o.unit = 2 AND o.removed = 0
                GROUP BY s.id
            )
            SELECT
                party,
                pbalance,
                cgross_amount,
                0 AS csettlement,
                pbalance + cgross_amount AS cbalance,
                cfirst,
                csecond
            FROM Statement;
            )")
            .arg(info_.node);
    default:
        return {};
    }
}

QString SqliteO::QSReadBalance(int unit) const
{
    switch (UnitO(unit)) {
    case UnitO::kMS:
        return QString(R"(
                SELECT

                    SUM(CASE WHEN o.date_time < :start AND o.settlement_id = 0 THEN o.gross_amount                 ELSE 0 END) AS pbalance,
                    SUM(CASE WHEN o.date_time BETWEEN :start AND :end THEN o.gross_amount                          ELSE 0 END) AS cgross_amount,
                    SUM(CASE WHEN o.date_time BETWEEN :start AND :end AND o.settlement_id != 0 THEN o.gross_amount ELSE 0 END) AS csettlement

                FROM stakeholder s
                INNER JOIN %1 o ON s.id = o.party
                WHERE o.party = :party_id AND o.unit = 1 AND o.finished = 1 AND o.removed = 0
            )")
            .arg(info_.node);
    case UnitO::kPEND:
        return QString(R"(
                SELECT

                    SUM(CASE WHEN o.date_time < :start THEN o.gross_amount                 ELSE 0 END) AS pbalance,
                    SUM(CASE WHEN o.date_time BETWEEN :start AND :end THEN o.gross_amount  ELSE 0 END) AS cgross_amount,
                    0 AS csettlement

                FROM stakeholder s
                INNER JOIN %1 o ON s.id = o.party
                WHERE o.party = :party_id AND o.unit = 2 AND o.removed = 0
            )")
            .arg(info_.node);
    default:
        return {};
    }
}

QString SqliteO::QSReadStatementPrimary(int unit) const
{
    static const QString kBaseQuery = R"(
        SELECT description, employee, date_time, first, second, gross_amount, %1 AS settlement
        FROM %2
        WHERE unit = :unit AND party = :party AND (date_time BETWEEN :start AND :end) %3 AND removed = 0
    )";

    QString settlement_expr {};
    QString finished_condition {};

    switch (UnitO(unit)) {
    case UnitO::kIS:
        settlement_expr = QStringLiteral("gross_amount");
        finished_condition = QStringLiteral("AND finished = 1");
        break;
    case UnitO::kMS:
        settlement_expr = QStringLiteral("0");
        finished_condition = QStringLiteral("AND finished = 1");
        break;
    case UnitO::kPEND:
        settlement_expr = QStringLiteral("0");
        finished_condition = QLatin1String("");
        break;
    default:
        return {};
    }

    return kBaseQuery.arg(settlement_expr, info_.node, finished_condition);
}

QString SqliteO::QSReadStatementSecondary(int unit) const
{
    static const QString kBaseQuery = R"(
        SELECT
            trans.inside_product,
            trans.unit_price,
            trans.second,
            trans.description,
            trans.first,
            trans.gross_amount,
            %1 AS settlement,
            trans.outside_product,
            node.date_time
        FROM %3 trans
        INNER JOIN %2 node ON trans.lhs_node = node.id
        WHERE node.unit = :unit AND node.party = :party AND (node.date_time BETWEEN :start AND :end) %4 AND trans.removed = 0
    )";

    QString settlement_expr {};
    QString finished_condition {};

    switch (UnitO(unit)) {
    case UnitO::kIS:
        settlement_expr = QStringLiteral("trans.gross_amount");
        finished_condition = QStringLiteral("AND node.finished = 1");
        break;
    case UnitO::kMS:
        settlement_expr = QStringLiteral("0");
        finished_condition = QStringLiteral("AND node.finished = 1");
        break;
    case UnitO::kPEND:
        settlement_expr = QStringLiteral("0");
        finished_condition = QLatin1String("");
        break;
    default:
        return {};
    }

    return kBaseQuery.arg(settlement_expr, info_.node, info_.trans, finished_condition);
}

QString SqliteO::QSInvertTransValue() const
{
    return QString(R"(
        UPDATE %1
        SET
            gross_amount = -gross_amount,
            net_amount = -net_amount,
            discount = -discount,
            first = -first,
            second = -second
        WHERE lhs_node = :lhs_node;
        )")
        .arg(info_.trans);
}

QString SqliteO::QSSyncPriceFirst() const
{
    return QString(R"(
        INSERT INTO stakeholder_transaction(date_time, lhs_node, inside_product, unit_price)
        SELECT
            node.date_time,
            node.party AS lhs_node,
            trans.inside_product,
            trans.unit_price
        FROM %2 trans
        JOIN %1 node ON trans.lhs_node = node.id
        JOIN product p ON trans.inside_product = p.id
        WHERE trans.lhs_node = :node_id AND trans.unit_price <> p.unit_price AND trans.removed = 0
        ON CONFLICT(lhs_node, inside_product) DO UPDATE SET
        date_time = excluded.date_time,
        unit_price = excluded.unit_price;
    )")
        .arg(info_.node, info_.trans);
}

QString SqliteO::QSSyncPriceSecond() const
{
    return QString(R"(
        SELECT
            node.date_time,
            node.party AS lhs_node,
            trans.inside_product,
            trans.unit_price
        FROM %2 trans
        JOIN %1 node ON trans.lhs_node = node.id
        JOIN product p ON trans.inside_product = p.id
        WHERE trans.lhs_node = :node_id AND trans.unit_price <> p.unit_price AND trans.removed = 0
    )")
        .arg(info_.node, info_.trans);
}

QString SqliteO::QSWriteSettlement() const
{
    return QString(R"(
    INSERT INTO %1 (date_time)
    VALUES (:date_time)
    )")
        .arg(info_.settlement);
}

QString SqliteO::QSRemoveSettlementFirst() const
{
    return QString(R"(
    UPDATE %1 SET
        removed = true
    WHERE id = :node_id
    )")
        .arg(info_.settlement);
}

QString SqliteO::QSRemoveSettlementSecond() const
{
    return QString(R"(
    UPDATE %1 SET
        settlement_id = 0,
        settlement = 0
    WHERE settlement_id = :node_id
    )")
        .arg(info_.node);
}

QString SqliteO::QSReadSettlementPrimary(bool finished) const
{
    CString finished_string { finished ? QString() : "OR settlement_id = 0" };

    return QString(R"(
    SELECT id, date_time, description, gross_amount, employee, settlement_id
    FROM %1
    WHERE party = :party_id AND unit = 1 AND finished = 1 AND (settlement_id = :settlement_id %2) AND removed = 0
    )")
        .arg(info_.node, finished_string);
}

void SqliteO::ReadSettlementPrimaryQuery(NodeList& node_list, QSqlQuery& query)
{
    // remind to recycle these trans
    while (query.next()) {
        auto* node { ResourcePool<Node>::Instance().Allocate() };

        node->id = query.value(QStringLiteral("id")).toInt();
        node->employee = query.value(QStringLiteral("employee")).toInt();
        node->description = query.value(QStringLiteral("description")).toString();
        node->date_time = query.value(QStringLiteral("date_time")).toString();
        node->initial_total = query.value(QStringLiteral("gross_amount")).toDouble();
        node->finished = query.value(QStringLiteral("settlement_id")).toInt() != 0;

        node_list.emplaceBack(node);
    }
}

void SqliteO::ReadStatementQuery(TransList& trans_list, QSqlQuery& query) const
{
    // remind to recycle these trans
    while (query.next()) {
        auto* trans { ResourcePool<Trans>::Instance().Allocate() };

        trans->id = query.value(QStringLiteral("party")).toInt();
        trans->lhs_ratio = query.value(QStringLiteral("pbalance")).toDouble();
        trans->rhs_debit = query.value(QStringLiteral("cgross_amount")).toDouble();
        trans->rhs_credit = query.value(QStringLiteral("csettlement")).toDouble();
        trans->rhs_ratio = query.value(QStringLiteral("cbalance")).toDouble();
        trans->lhs_debit = query.value(QStringLiteral("cfirst")).toDouble();
        trans->lhs_credit = query.value(QStringLiteral("csecond")).toDouble();

        trans_list.emplaceBack(trans);
    }
}

void SqliteO::ReadStatementPrimaryQuery(NodeList& node_list, QSqlQuery& query) const
{
    // remind to recycle these trans
    while (query.next()) {
        auto* node { ResourcePool<Node>::Instance().Allocate() };

        node->description = query.value(QStringLiteral("description")).toString();
        node->employee = query.value(QStringLiteral("employee")).toInt();
        node->date_time = query.value(QStringLiteral("date_time")).toString();
        node->first = query.value(QStringLiteral("first")).toDouble();
        node->second = query.value(QStringLiteral("second")).toDouble();
        node->initial_total = query.value(QStringLiteral("gross_amount")).toDouble();
        node->final_total = query.value(QStringLiteral("settlement")).toDouble();

        node_list.emplaceBack(node);
    }
}

void SqliteO::ReadStatementSecondaryQuery(TransList& trans_list, QSqlQuery& query) const
{
    // remind to recycle these trans
    while (query.next()) {
        auto* trans { ResourcePool<Trans>::Instance().Allocate() };

        trans->lhs_ratio = query.value(QStringLiteral("unit_price")).toDouble();
        trans->lhs_credit = query.value(QStringLiteral("second")).toDouble();
        trans->description = query.value(QStringLiteral("description")).toString();
        trans->lhs_debit = query.value(QStringLiteral("first")).toDouble();
        trans->rhs_debit = query.value(QStringLiteral("gross_amount")).toDouble();
        trans->rhs_credit = query.value(QStringLiteral("settlement")).toDouble();
        trans->support_id = query.value(QStringLiteral("outside_product")).toInt();
        trans->rhs_node = query.value(QStringLiteral("inside_product")).toInt();
        trans->date_time = query.value(QStringLiteral("date_time")).toString();

        trans_list.emplaceBack(trans);
    }
}

QString SqliteO::QSReadSettlement() const
{
    return QString(R"(
    SELECT id, date_time, description, finished, gross_amount, party
    FROM %1
    WHERE (date_time BETWEEN :start AND :end) AND removed = 0
    )")
        .arg(info_.settlement);
}

void SqliteO::ReadSettlementQuery(NodeList& node_list, QSqlQuery& query) const
{
    // remind to recycle these Node
    while (query.next()) {
        auto* node { ResourcePool<Node>::Instance().Allocate() };

        node->id = query.value(QStringLiteral("id")).toInt();
        node->party = query.value(QStringLiteral("party")).toInt();
        node->description = query.value(QStringLiteral("description")).toString();
        node->date_time = query.value(QStringLiteral("date_time")).toString();
        node->finished = query.value(QStringLiteral("finished")).toBool();
        node->initial_total = query.value(QStringLiteral("gross_amount")).toDouble();

        node_list.emplaceBack(node);
    }
}

void SqliteO::WriteSettlementBind(Node* node, QSqlQuery& query) const { query.bindValue(QStringLiteral(":date_time"), node->date_time); }

QString SqliteO::QSSearchNode(CString& in_list) const
{
    return QString(R"(
    SELECT name, id, description, rule, type, unit, party, employee, date_time, first, second, discount, finished, gross_amount, settlement
    FROM %1
    WHERE party IN (%2) AND removed = 0
    )")
        .arg(info_.node, in_list);
}

void SqliteO::WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query) const
{
    query.bindValue(QStringLiteral(":code"), *trans_shadow->code);
    query.bindValue(QStringLiteral(":inside_product"), *trans_shadow->rhs_node);
    query.bindValue(QStringLiteral(":unit_price"), *trans_shadow->lhs_ratio);
    query.bindValue(QStringLiteral(":second"), *trans_shadow->lhs_credit);
    query.bindValue(QStringLiteral(":description"), *trans_shadow->description);
    query.bindValue(QStringLiteral(":lhs_node"), *trans_shadow->lhs_node);
    query.bindValue(QStringLiteral(":first"), *trans_shadow->lhs_debit);
    query.bindValue(QStringLiteral(":gross_amount"), *trans_shadow->rhs_debit);
    query.bindValue(QStringLiteral(":discount"), *trans_shadow->discount);
    query.bindValue(QStringLiteral(":net_amount"), *trans_shadow->rhs_credit);
    query.bindValue(QStringLiteral(":outside_product"), *trans_shadow->support_id);
    query.bindValue(QStringLiteral(":discount_price"), *trans_shadow->rhs_ratio);
}

void SqliteO::ReadTransQuery(Trans* trans, const QSqlQuery& query) const
{
    trans->code = query.value(QStringLiteral("code")).toString();
    trans->rhs_node = query.value(QStringLiteral("inside_product")).toInt();
    trans->lhs_ratio = query.value(QStringLiteral("unit_price")).toDouble();
    trans->lhs_credit = query.value(QStringLiteral("second")).toDouble();
    trans->description = query.value(QStringLiteral("description")).toString();
    trans->lhs_node = query.value(QStringLiteral("lhs_node")).toInt();
    trans->lhs_debit = query.value(QStringLiteral("first")).toDouble();
    trans->rhs_debit = query.value(QStringLiteral("gross_amount")).toDouble();
    trans->rhs_credit = query.value(QStringLiteral("net_amount")).toDouble();
    trans->discount = query.value(QStringLiteral("discount")).toDouble();
    trans->support_id = query.value(QStringLiteral("outside_product")).toInt();
    trans->rhs_ratio = query.value(QStringLiteral("discount_price")).toDouble();
}

void SqliteO::ReadTransFunction(TransShadowList& trans_shadow_list, int /*node_id*/, QSqlQuery& query)
{
    while (query.next()) {
        const int kID { query.value(QStringLiteral("id")).toInt() };

        auto* trans { ResourcePool<Trans>::Instance().Allocate() };
        auto* trans_shadow { ResourcePool<TransShadow>::Instance().Allocate() };

        trans->id = kID;

        ReadTransQuery(trans, query);
        trans_hash_.insert(kID, trans);

        ConvertTrans(trans, trans_shadow, true);
        trans_shadow_list.emplaceBack(trans_shadow);
    }
}

void SqliteO::UpdateTransValueBind(const TransShadow* trans_shadow, QSqlQuery& query) const
{
    query.bindValue(QStringLiteral(":second"), *trans_shadow->lhs_credit);
    query.bindValue(QStringLiteral(":first"), *trans_shadow->lhs_debit);
    query.bindValue(QStringLiteral(":gross_amount"), *trans_shadow->rhs_debit);
    query.bindValue(QStringLiteral(":discount"), *trans_shadow->discount);
    query.bindValue(QStringLiteral(":net_amount"), *trans_shadow->rhs_credit);
    query.bindValue(QStringLiteral(":trans_id"), *trans_shadow->id);
}

void SqliteO::WriteTransRangeFunction(const QList<TransShadow*>& list, QSqlQuery& query) const
{
    const int size = list.size();

    QVariantList code_list {};
    QVariantList inside_product_list {};
    QVariantList unit_price_list {};
    QVariantList description_list {};
    QVariantList second_list {};
    QVariantList lhs_node_list {};
    QVariantList first_list {};
    QVariantList gross_amount_list {};
    QVariantList discount_list {};
    QVariantList net_amount_list {};
    QVariantList outside_product_list {};
    QVariantList discount_price_list {};

    code_list.reserve(size);
    inside_product_list.reserve(size);
    unit_price_list.reserve(size);
    description_list.reserve(size);
    second_list.reserve(size);
    lhs_node_list.reserve(size);
    first_list.reserve(size);
    gross_amount_list.reserve(size);
    discount_list.reserve(size);
    net_amount_list.reserve(size);
    outside_product_list.reserve(size);
    discount_price_list.reserve(size);

    for (const TransShadow* trans_shadow : list) {
        if (*trans_shadow->rhs_node == 0 || *trans_shadow->lhs_node == 0)
            continue;

        code_list.emplaceBack(*trans_shadow->code);
        inside_product_list.emplaceBack(*trans_shadow->rhs_node);
        unit_price_list.emplaceBack(*trans_shadow->lhs_ratio);
        description_list.emplaceBack(*trans_shadow->description);
        second_list.emplaceBack(*trans_shadow->lhs_credit);
        lhs_node_list.emplaceBack(*trans_shadow->lhs_node);
        first_list.emplaceBack(*trans_shadow->lhs_debit);
        gross_amount_list.emplaceBack(*trans_shadow->rhs_debit);
        discount_list.emplaceBack(*trans_shadow->discount);
        net_amount_list.emplaceBack(*trans_shadow->rhs_credit);
        outside_product_list.emplaceBack(*trans_shadow->support_id);
        discount_price_list.emplaceBack(*trans_shadow->rhs_ratio);
    }

    // 批量绑定 QVariantList
    query.bindValue(QStringLiteral(":code"), code_list);
    query.bindValue(QStringLiteral(":inside_product"), inside_product_list);
    query.bindValue(QStringLiteral(":unit_price"), unit_price_list);
    query.bindValue(QStringLiteral(":description"), description_list);
    query.bindValue(QStringLiteral(":second"), second_list);
    query.bindValue(QStringLiteral(":lhs_node"), lhs_node_list);
    query.bindValue(QStringLiteral(":first"), first_list);
    query.bindValue(QStringLiteral(":gross_amount"), gross_amount_list);
    query.bindValue(QStringLiteral(":discount"), discount_list);
    query.bindValue(QStringLiteral(":net_amount"), net_amount_list);
    query.bindValue(QStringLiteral(":outside_product"), outside_product_list);
    query.bindValue(QStringLiteral(":discount_price"), discount_price_list);
}

QString SqliteO::QSUpdateLeafValue() const
{
    return QStringLiteral(R"(
    UPDATE %1 SET
        gross_amount = :gross_amount, settlement = :settlement, second = :second, discount = :discount, first = :first
    WHERE id = :node_id
    )")
        .arg(info_.node);
}

void SqliteO::UpdateLeafValueBind(const Node* node, QSqlQuery& query) const
{
    query.bindValue(QStringLiteral(":gross_amount"), node->initial_total);
    query.bindValue(QStringLiteral(":second"), node->second);
    query.bindValue(QStringLiteral(":first"), node->first);
    query.bindValue(QStringLiteral(":discount"), node->discount);
    query.bindValue(QStringLiteral(":settlement"), node->final_total);
    query.bindValue(QStringLiteral(":node_id"), node->id);
}

void SqliteO::ReadNodeQuery(Node* node, const QSqlQuery& query) const
{
    node->id = query.value(QStringLiteral("id")).toInt();
    node->name = query.value(QStringLiteral("name")).toString();
    node->description = query.value(QStringLiteral("description")).toString();
    node->rule = query.value(QStringLiteral("rule")).toBool();
    node->type = query.value(QStringLiteral("type")).toInt();
    node->unit = query.value(QStringLiteral("unit")).toInt();
    node->party = query.value(QStringLiteral("party")).toInt();
    node->employee = query.value(QStringLiteral("employee")).toInt();
    node->date_time = query.value(QStringLiteral("date_time")).toString();
    node->first = query.value(QStringLiteral("first")).toDouble();
    node->second = query.value(QStringLiteral("second")).toDouble();
    node->discount = query.value(QStringLiteral("discount")).toDouble();
    node->finished = query.value(QStringLiteral("finished")).toBool();
    node->initial_total = query.value(QStringLiteral("gross_amount")).toDouble();
    node->final_total = query.value(QStringLiteral("settlement")).toDouble();
}

void SqliteO::WriteNodeBind(Node* node, QSqlQuery& query) const
{
    query.bindValue(QStringLiteral(":name"), node->name);
    query.bindValue(QStringLiteral(":description"), node->description);
    query.bindValue(QStringLiteral(":rule"), node->rule);
    query.bindValue(QStringLiteral(":type"), node->type);
    query.bindValue(QStringLiteral(":unit"), node->unit);
    query.bindValue(QStringLiteral(":party"), node->party);
    query.bindValue(QStringLiteral(":employee"), node->employee);
    query.bindValue(QStringLiteral(":date_time"), node->date_time);
    query.bindValue(QStringLiteral(":first"), node->first);
    query.bindValue(QStringLiteral(":second"), node->second);
    query.bindValue(QStringLiteral(":discount"), node->discount);
    query.bindValue(QStringLiteral(":finished"), node->finished);
    query.bindValue(QStringLiteral(":gross_amount"), node->initial_total);
    query.bindValue(QStringLiteral(":settlement"), node->final_total);
}
