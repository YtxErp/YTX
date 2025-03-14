#include "sqliteorder.h"

#include <QDate>
#include <QSqlError>
#include <QSqlQuery>

#include "component/constvalue.h"
#include "global/resourcepool.h"

SqliteOrder::SqliteOrder(CInfo& info, QObject* parent)
    : Sqlite(info, parent)
{
}

SqliteOrder::~SqliteOrder() { qDeleteAll(node_hash_buffer_); }

bool SqliteOrder::ReadNode(NodeHash& node_hash, const QDateTime& start, const QDateTime& end)
{
    CString string { QSReadNode() };
    if (string.isEmpty())
        return false;

    QSqlQuery query(*db_);
    query.setForwardOnly(true);
    query.prepare(string);

    query.bindValue(QStringLiteral(":start"), start.toString(kDateTimeFST));
    query.bindValue(QStringLiteral(":end"), end.toString(kDateTimeFST));

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in ReadNode" << query.lastError().text();
        return false;
    }

    if (!node_hash.isEmpty())
        node_hash.clear();

    Node* node {};

    while (query.next()) {
        const int kID { query.value(QStringLiteral("id")).toInt() };

        if (auto it = node_hash_buffer_.constFind(kID); it != node_hash_buffer_.constEnd()) {
            it.value()->children.clear();
            it.value()->parent = nullptr;
            node_hash.insert(it.key(), it.value());
            continue;
        }

        node = ResourcePool<Node>::Instance().Allocate();
        ReadNodeQuery(node, query);
        node_hash.insert(kID, node);
        node_hash_buffer_.insert(kID, node);
    }

    if (!node_hash.isEmpty())
        ReadRelationship(node_hash, query);

    return true;
}

bool SqliteOrder::SearchNode(QList<const Node*>& node_list, const QList<int>& party_id_list)
{
    if (party_id_list.empty())
        return false;

    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    const qsizetype batch_size { kBatchSize };
    const auto total_batches { (party_id_list.size() + batch_size - 1) / batch_size };

    Node* node {};
    int id {};

    for (int batch_index = 0; batch_index != total_batches; ++batch_index) {
        int start = batch_index * batch_size;
        int end = std::min(start + batch_size, party_id_list.size());

        QList<int> current_batch { party_id_list.mid(start, end - start) };

        QStringList placeholder { current_batch.size(), QStringLiteral("?") };
        QString string { SearchNodeQS(placeholder.join(QStringLiteral(","))) };

        query.prepare(string);

        for (int i = 0; i != current_batch.size(); ++i)
            query.bindValue(i, current_batch.at(i));

        if (!query.exec()) {
            qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in SearchNode, batch" << batch_index << ": " << query.lastError().text();
            continue;
        }

        while (query.next()) {
            id = query.value(QStringLiteral("id")).toInt();

            if (auto it = node_hash_buffer_.constFind(id); it != node_hash_buffer_.constEnd()) {
                node_list.emplaceBack(it.value());
                continue;
            }

            node = ResourcePool<Node>::Instance().Allocate();
            ReadNodeQuery(node, query);
            node_list.emplaceBack(node);
            node_hash_buffer_.insert(id, node);
        }
    }

    return true;
}

bool SqliteOrder::RetrieveNode(NodeHash& node_hash, int node_id)
{
    auto it = node_hash_buffer_.constFind(node_id);
    if (it == node_hash_buffer_.constEnd())
        node_hash.insert(node_id, ReadNode(node_id));

    return true;
}

void SqliteOrder::RRemoveNode(int node_id, int /*node_type*/)
{
    // Notify MainWindow to release the table view
    emit SFreeWidget(node_id);
    // Notify TreeModel to remove the node
    emit SRemoveNode(node_id);

    // Mark Trans for removal

    const QMultiHash<int, int> node_trans { TransToRemove(node_id, kTypeLeaf) };
    const auto trans { node_trans.values() };

    // Remove node, path, trans from the sqlite3 database
    RemoveNode(node_id, kTypeLeaf);

    // Recycle trans resources
    for (int trans_id : trans)
        ResourcePool<Trans>::Instance().Recycle(trans_hash_.take(trans_id));
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
QString SqliteOrder::QSReadNode() const
{
    return QString(R"(
    SELECT name, id, description, rule, type, unit, party, employee, date_time, first, second, discount, finished, gross_amount, settlement
    FROM %1
    WHERE ((date_time BETWEEN :start AND :end) OR type = 1 OR finished = 0 OR unit = 2) AND removed = 0
    )")
        .arg(info_.node);
}

QString SqliteOrder::QSWriteNode() const
{
    return QString(R"(
    INSERT INTO %1 (name, description, rule, type, unit, party, employee, date_time, first, second, discount, finished, gross_amount, settlement)
    VALUES (:name, :description, :rule, :type, :unit, :party, :employee, :date_time, :first, :second, :discount, :finished, :gross_amount, :settlement)
    )")
        .arg(info_.node);
}

QString SqliteOrder::QSRemoveNodeSecond() const
{
    return QString(R"(
    UPDATE %1 SET
        removed = 1
    WHERE lhs_node = :node_id
    )")
        .arg(info_.trans);
}

QString SqliteOrder::QSInternalReference() const
{
    return QString(R"(
    SELECT COUNT(*) FROM %1
    WHERE lhs_node = :node_id AND removed = 0
    )")
        .arg(info_.trans);
}

QString SqliteOrder::QSReadTrans() const
{
    return QString(R"(
    SELECT id, code, inside_product, unit_price, second, description, lhs_node, first, gross_amount, discount, net_amount, outside_product, discount_price
    FROM %1
    WHERE lhs_node = :node_id AND removed = 0
    )")
        .arg(info_.trans);
}

QString SqliteOrder::QSWriteTrans() const
{
    return QString(R"(
    INSERT INTO %1 (code, inside_product, unit_price, second, description, lhs_node, first, gross_amount, discount, net_amount, outside_product, discount_price)
    VALUES (:code, :inside_product, :unit_price, :second, :description, :lhs_node, :first, :gross_amount, :discount, :net_amount, :outside_product, :discount_price)
    )")
        .arg(info_.trans);
}

QString SqliteOrder::QSUpdateProductReferenceSO() const
{
    return QString(R"(
    UPDATE %1 SET
        inside_product = :new_node_id
    WHERE inside_product = :old_node_id
    )")
        .arg(info_.trans);
}

QString SqliteOrder::QSUpdateStakeholderReferenceO() const
{
    return QString(R"(
    BEGIN TRANSACTION;

    -- Update the outside_product in transaction table
    UPDATE %2 SET
        outside_product = :new_node_id
    WHERE outside_product = :old_node_id;

    -- Update the party and employee in node table
    UPDATE %1 SET
        party = CASE WHEN party = :old_node_id THEN :new_node_id ELSE party END,
        employee = CASE WHEN employee = :old_node_id THEN :new_node_id ELSE employee END
    WHERE party = :old_node_id OR employee = :old_node_id;

    COMMIT;
    )")
        .arg(info_.node, info_.trans);
}

QString SqliteOrder::QSSearchTrans() const
{
    return QString(R"(
    SELECT id, code, inside_product, unit_price, second, description, lhs_node, first, gross_amount, discount, net_amount, outside_product, discount_price
    FROM %1
    WHERE (first = :text OR second = :text OR description LIKE :description) AND removed = 0
    )")
        .arg(info_.trans);
}

QString SqliteOrder::QSSyncTransValue() const
{
    return QString(R"(
    UPDATE %1 SET
        second = :second, gross_amount = :gross_amount, discount = :discount, net_amount = :net_amount, first = :first
    WHERE id = :trans_id
    )")
        .arg(info_.trans);
}

QString SqliteOrder::QSTransToRemove() const
{
    return QString(R"(
    SELECT lhs_node, id FROM %1
    WHERE lhs_node = :node_id AND removed = 0
    )")
        .arg(info_.trans);
}

QString SqliteOrder::QSReadStatement(int unit) const
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

                    SUM(CASE WHEN o.date_time < :start AND o.payment_id = 0 THEN o.gross_amount                 ELSE 0 END) AS pbalance,
                    SUM(CASE WHEN o.date_time BETWEEN :start AND :end THEN o.gross_amount                       ELSE 0 END) AS cgross_amount,
                    SUM(CASE WHEN o.date_time BETWEEN :start AND :end AND o.payment_id != 0 THEN o.gross_amount ELSE 0 END) AS csettlement,
                    SUM(CASE WHEN o.date_time BETWEEN :start AND :end THEN o.first                              ELSE 0 END) AS cfirst,
                    SUM(CASE WHEN o.date_time BETWEEN :start AND :end THEN o.second                             ELSE 0 END) AS csecond

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

QString SqliteOrder::QSReadStatementPrimary(int unit) const
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

QString SqliteOrder::QSReadStatementSecondary(int unit) const
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

QString SqliteOrder::QSInvertTransValue() const
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

void SqliteOrder::ReadStatementQuery(TransList& trans_list, QSqlQuery& query) const
{
    // remind to recycle these trans
    while (query.next()) {
        auto* trans { ResourcePool<Trans>::Instance().Allocate() };

        trans->id = query.value(QStringLiteral("party")).toInt();
        trans->lhs_ratio = query.value(QStringLiteral("pbalance")).toDouble();
        trans->rhs_debit = query.value(QStringLiteral("cgross_amount")).toDouble();
        trans->rhs_credit = query.value(QStringLiteral("csettlement")).toInt();
        trans->rhs_ratio = query.value(QStringLiteral("cbalance")).toDouble();
        trans->lhs_debit = query.value(QStringLiteral("cfirst")).toDouble();
        trans->lhs_credit = query.value(QStringLiteral("csecond")).toDouble();

        trans_list.emplaceBack(trans);
    }
}

void SqliteOrder::ReadStatementPrimaryQuery(QList<Node*>& node_list, QSqlQuery& query) const
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

void SqliteOrder::ReadStatementSecondaryQuery(TransList& trans_list, QSqlQuery& query) const
{
    // remind to recycle these trans
    while (query.next()) {
        auto* trans { ResourcePool<Trans>::Instance().Allocate() };

        trans->lhs_ratio = query.value(QStringLiteral("unit_price")).toDouble();
        trans->lhs_credit = query.value(QStringLiteral("second")).toDouble();
        trans->description = query.value(QStringLiteral("description")).toString();
        trans->lhs_debit = query.value(QStringLiteral("first")).toInt();
        trans->rhs_debit = query.value(QStringLiteral("gross_amount")).toDouble();
        trans->rhs_credit = query.value(QStringLiteral("settlement")).toDouble();
        trans->support_id = query.value(QStringLiteral("outside_product")).toInt();
        trans->rhs_node = query.value(QStringLiteral("inside_product")).toInt();
        trans->date_time = query.value(QStringLiteral("date_time")).toString();

        trans_list.emplaceBack(trans);
    }
}

QString SqliteOrder::SearchNodeQS(CString& in_list) const
{
    return QString(R"(
    SELECT name, id, description, rule, type, unit, party, employee, date_time, first, second, discount, finished, gross_amount, settlement
    FROM %1
    WHERE party IN (%2) AND removed = 0
    )")
        .arg(info_.node, in_list);
}

Node* SqliteOrder::ReadNode(int node_id)
{
    if (auto it = node_hash_buffer_.constFind(node_id); it != node_hash_buffer_.constEnd())
        return it.value();

    CString string { QString(R"(
    SELECT name, id, description, rule, type, unit, party, employee, date_time, first, second, discount, finished, gross_amount, settlement
    FROM %1
    WHERE (id = :node_id) AND removed = 0
    )")
            .arg(info_.node) };

    QSqlQuery query(*db_);
    query.setForwardOnly(true);
    query.prepare(string);

    query.bindValue(QStringLiteral(":node_id"), node_id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in ReadNode" << query.lastError().text();
        return nullptr;
    }

    Node* node {};

    if (query.next()) {
        node = ResourcePool<Node>::Instance().Allocate();
        ReadNodeQuery(node, query);
        node_hash_buffer_.insert(node_id, node);
    }

    return node;
}

void SqliteOrder::WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query) const
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

void SqliteOrder::ReadTransQuery(Trans* trans, const QSqlQuery& query) const
{
    trans->code = query.value(QStringLiteral("code")).toString();
    trans->rhs_node = query.value(QStringLiteral("inside_product")).toInt();
    trans->lhs_ratio = query.value(QStringLiteral("unit_price")).toDouble();
    trans->lhs_credit = query.value(QStringLiteral("second")).toDouble();
    trans->description = query.value(QStringLiteral("description")).toString();
    trans->lhs_node = query.value(QStringLiteral("lhs_node")).toInt();
    trans->lhs_debit = query.value(QStringLiteral("first")).toInt();
    trans->rhs_debit = query.value(QStringLiteral("gross_amount")).toDouble();
    trans->rhs_credit = query.value(QStringLiteral("net_amount")).toDouble();
    trans->discount = query.value(QStringLiteral("discount")).toDouble();
    trans->support_id = query.value(QStringLiteral("outside_product")).toInt();
    trans->rhs_ratio = query.value(QStringLiteral("discount_price")).toDouble();
}

void SqliteOrder::ReadTransFunction(TransShadowList& trans_shadow_list, int /*node_id*/, QSqlQuery& query, bool /*is_support*/)
{
    TransShadow* trans_shadow {};
    Trans* trans {};

    while (query.next()) {
        const int kID { query.value(QStringLiteral("id")).toInt() };

        trans = ResourcePool<Trans>::Instance().Allocate();
        trans_shadow = ResourcePool<TransShadow>::Instance().Allocate();

        trans->id = kID;

        ReadTransQuery(trans, query);
        trans_hash_.insert(kID, trans);

        ConvertTrans(trans, trans_shadow, true);
        trans_shadow_list.emplaceBack(trans_shadow);
    }
}

void SqliteOrder::UpdateProductReferenceSO(int old_node_id, int new_node_id) const
{
    const auto& const_trans_hash { std::as_const(trans_hash_) };

    for (auto* trans : const_trans_hash) {
        if (trans->rhs_node == old_node_id)
            trans->rhs_node = new_node_id;
    }
}

void SqliteOrder::UpdateStakeholderReferenceO(int old_node_id, int new_node_id) const
{
    // for party's product reference
    const auto& const_trans_hash { std::as_const(trans_hash_) };

    for (auto* trans : const_trans_hash) {
        if (trans->support_id == old_node_id)
            trans->support_id = new_node_id;
    }
}

void SqliteOrder::SyncTransValueBind(const TransShadow* trans_shadow, QSqlQuery& query) const
{
    query.bindValue(QStringLiteral(":second"), *trans_shadow->lhs_credit);
    query.bindValue(QStringLiteral(":first"), *trans_shadow->lhs_debit);
    query.bindValue(QStringLiteral(":gross_amount"), *trans_shadow->rhs_debit);
    query.bindValue(QStringLiteral(":discount"), *trans_shadow->discount);
    query.bindValue(QStringLiteral(":net_amount"), *trans_shadow->rhs_credit);
    query.bindValue(QStringLiteral(":trans_id"), *trans_shadow->id);
}

void SqliteOrder::WriteTransRangeFunction(const QList<TransShadow*>& list, QSqlQuery& query) const
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

QString SqliteOrder::QSSyncLeafValue() const
{
    return QStringLiteral(R"(
    UPDATE %1 SET
        gross_amount = :gross_amount, settlement = :settlement, second = :second, discount = :discount, first = :first
    WHERE id = :node_id
    )")
        .arg(info_.node);
}

void SqliteOrder::SyncLeafValueBind(const Node* node, QSqlQuery& query) const
{
    query.bindValue(QStringLiteral(":gross_amount"), node->initial_total);
    query.bindValue(QStringLiteral(":second"), node->second);
    query.bindValue(QStringLiteral(":first"), node->first);
    query.bindValue(QStringLiteral(":discount"), node->discount);
    query.bindValue(QStringLiteral(":settlement"), node->final_total);
    query.bindValue(QStringLiteral(":node_id"), node->id);
}

void SqliteOrder::ReadNodeQuery(Node* node, const QSqlQuery& query) const
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
    node->first = query.value(QStringLiteral("first")).toInt();
    node->second = query.value(QStringLiteral("second")).toDouble();
    node->discount = query.value(QStringLiteral("discount")).toDouble();
    node->finished = query.value(QStringLiteral("finished")).toBool();
    node->initial_total = query.value(QStringLiteral("gross_amount")).toDouble();
    node->final_total = query.value(QStringLiteral("settlement")).toDouble();
}

void SqliteOrder::WriteNodeBind(Node* node, QSqlQuery& query) const
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
