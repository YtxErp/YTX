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

bool SqliteOrder::ReadNode(NodeHash& node_hash, const QDate& start_date, const QDate& end_date)
{
    CString& string { QSReadNode() };
    if (string.isEmpty())
        return false;

    QSqlQuery query(*db_);
    query.setForwardOnly(true);
    query.prepare(string);

    query.bindValue(QStringLiteral(":start_date"), start_date.toString(kDateFST));
    query.bindValue(QStringLiteral(":end_date"), end_date.toString(kDateFST));

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in ReadNode" << query.lastError().text();
        return false;
    }

    if (!node_hash.isEmpty())
        node_hash.clear();

    Node* node {};
    int id {};

    while (query.next()) {
        id = query.value(QStringLiteral("id")).toInt();

        if (auto it = node_hash_buffer_.constFind(id); it != node_hash_buffer_.constEnd()) {
            it.value()->children.clear();
            it.value()->parent = nullptr;
            node_hash.insert(it.key(), it.value());
            continue;
        }

        node = ResourcePool<Node>::Instance().Allocate();
        ReadNodeQuery(node, query);
        node_hash.insert(id, node);
        node_hash_buffer_.insert(id, node);
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

bool SqliteOrder::RetriveNode(NodeHash& node_hash, int node_id)
{
    auto it = node_hash_buffer_.constFind(node_id);
    if (it != node_hash_buffer_.constEnd() && it.value())
        node_hash.insert(node_id, it.value());

    return true;
}

void SqliteOrder::RRemoveNode(int node_id, int /*node_type*/)
{
    // Notify MainWindow to release the table view
    emit SFreeView(node_id);
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

QString SqliteOrder::QSReadNode() const
{
    return QStringLiteral(R"(
    SELECT name, id, description, rule, type, unit, party, employee, date_time, first, second, discount, finished, gross_amount, net_amount
    FROM %1
    WHERE ((DATE(date_time) BETWEEN :start_date AND :end_date) OR type = 1) AND removed = 0
    )")
        .arg(info_.node);
}

QString SqliteOrder::QSWriteNode() const
{
    return QStringLiteral(R"(
    INSERT INTO %1 (name, description, rule, type, unit, party, employee, date_time, first, second, discount, finished, gross_amount, net_amount)
    VALUES (:name, :description, :rule, :type, :unit, :party, :employee, :date_time, :first, :second, :discount, :finished, :gross_amount, :net_amount)
    )")
        .arg(info_.node);
}

QString SqliteOrder::QSRemoveNodeSecond() const
{
    return QStringLiteral(R"(
    UPDATE %1 SET
        removed = 1
    WHERE lhs_node = :node_id
    )")
        .arg(info_.trans);
}

QString SqliteOrder::QSInternalReference() const
{
    return QStringLiteral(R"(
    SELECT COUNT(*) FROM %1
    WHERE lhs_node = :node_id AND removed = 0
    )")
        .arg(info_.trans);
}

QString SqliteOrder::QSReadNodeTrans() const
{
    return QStringLiteral(R"(
    SELECT id, code, inside_product, unit_price, second, description, lhs_node, first, gross_amount, discount, net_amount, outside_product, discount_price
    FROM %1
    WHERE lhs_node = :node_id AND removed = 0
    )")
        .arg(info_.trans);
}

QString SqliteOrder::QSWriteNodeTrans() const
{
    return QStringLiteral(R"(
    INSERT INTO %1 (code, inside_product, unit_price, second, description, lhs_node, first, gross_amount, discount, net_amount, outside_product, discount_price)
    VALUES (:code, :inside_product, :unit_price, :second, :description, :lhs_node, :first, :gross_amount, :discount, :net_amount, :outside_product, :discount_price)
    )")
        .arg(info_.trans);
}

QString SqliteOrder::QSUpdateProductReferenceSO() const
{
    return QStringLiteral(R"(
    UPDATE %1 SET
        inside_product = :new_node_id
    WHERE inside_product = :old_node_id
    )")
        .arg(info_.trans);
}

QString SqliteOrder::QSUpdateStakeholderReferenceO() const
{
    return QStringLiteral(R"(
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
    return QStringLiteral(R"(
    SELECT id, code, inside_product, unit_price, second, description, lhs_node, first, gross_amount, discount, net_amount, outside_product, discount_price
    FROM %1
    WHERE (first = :text OR second = :text OR description LIKE :description) AND removed = 0
    )")
        .arg(info_.trans);
}

QString SqliteOrder::QSUpdateTransValueFPTO() const
{
    return QStringLiteral(R"(
    UPDATE %1 SET
        second = :second, gross_amount = :gross_amount, discount = :discount, net_amount = :net_amount
    WHERE id = :trans_id
    )")
        .arg(info_.trans);
}

QString SqliteOrder::QSNodeTransToRemove() const
{
    return QStringLiteral(R"(
    SELECT lhs_node, id FROM %1
    WHERE lhs_node = :node_id AND removed = 0
    )")
        .arg(info_.trans);
}

QString SqliteOrder::SearchNodeQS(CString& in_list) const
{
    return QStringLiteral(R"(
    SELECT name, id, description, rule, type, unit, party, employee, date_time, first, second, discount, finished, gross_amount, net_amount
    FROM %1
    WHERE party IN (%2) AND removed = 0
    )")
        .arg(info_.node, in_list);
}

void SqliteOrder::WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query) const
{
    query.bindValue(QStringLiteral(":code"), *trans_shadow->code);
    query.bindValue(QStringLiteral(":inside_product"), *trans_shadow->rhs_node);
    query.bindValue(QStringLiteral(":unit_price"), *trans_shadow->unit_price);
    query.bindValue(QStringLiteral(":second"), *trans_shadow->lhs_credit);
    query.bindValue(QStringLiteral(":description"), *trans_shadow->description);
    query.bindValue(QStringLiteral(":lhs_node"), *trans_shadow->lhs_node);
    query.bindValue(QStringLiteral(":first"), *trans_shadow->lhs_debit);
    query.bindValue(QStringLiteral(":gross_amount"), *trans_shadow->rhs_credit);
    query.bindValue(QStringLiteral(":discount"), *trans_shadow->rhs_debit);
    query.bindValue(QStringLiteral(":net_amount"), *trans_shadow->net_amount);
    query.bindValue(QStringLiteral(":outside_product"), *trans_shadow->support_id);
    query.bindValue(QStringLiteral(":discount_price"), *trans_shadow->discount_price);
}

void SqliteOrder::ReadTransQuery(Trans* trans, const QSqlQuery& query) const
{
    trans->code = query.value(QStringLiteral("code")).toString();
    trans->rhs_node = query.value(QStringLiteral("inside_product")).toInt();
    trans->unit_price = query.value(QStringLiteral("unit_price")).toDouble();
    trans->lhs_credit = query.value(QStringLiteral("second")).toDouble();
    trans->description = query.value(QStringLiteral("description")).toString();
    trans->lhs_node = query.value(QStringLiteral("lhs_node")).toInt();
    trans->lhs_debit = query.value(QStringLiteral("first")).toInt();
    trans->rhs_credit = query.value(QStringLiteral("gross_amount")).toDouble();
    trans->net_amount = query.value(QStringLiteral("net_amount")).toDouble();
    trans->rhs_debit = query.value(QStringLiteral("discount")).toDouble();
    trans->support_id = query.value(QStringLiteral("outside_product")).toInt();
    trans->discount_price = query.value(QStringLiteral("discount_price")).toDouble();
}

void SqliteOrder::ReadTransFunction(TransShadowList& trans_shadow_list, int /*node_id*/, QSqlQuery& query)
{
    TransShadow* trans_shadow {};
    Trans* trans {};
    int id {};

    while (query.next()) {
        id = query.value(QStringLiteral("id")).toInt();

        trans = ResourcePool<Trans>::Instance().Allocate();
        trans_shadow = ResourcePool<TransShadow>::Instance().Allocate();

        trans->id = id;

        ReadTransQuery(trans, query);
        trans_hash_.insert(id, trans);

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

void SqliteOrder::UpdateTransValueBindFPTO(const TransShadow* trans_shadow, QSqlQuery& query) const
{
    query.bindValue(QStringLiteral(":second"), *trans_shadow->lhs_credit);
    query.bindValue(QStringLiteral(":gross_amount"), *trans_shadow->rhs_credit);
    query.bindValue(QStringLiteral(":discount"), *trans_shadow->rhs_debit);
    query.bindValue(QStringLiteral(":net_amount"), *trans_shadow->net_amount);
    query.bindValue(QStringLiteral(":trans_id"), *trans_shadow->id);
}

QString SqliteOrder::QSUpdateNodeValueFPTO() const
{
    return QStringLiteral(R"(
    UPDATE %1 SET
        gross_amount = :gross_amount, net_amount = :net_amount, second = :second, discount = :discount, first = :first
    WHERE id = :node_id
    )")
        .arg(info_.node);
}

void SqliteOrder::UpdateNodeValueBindFPTO(const Node* node, QSqlQuery& query) const
{
    query.bindValue(QStringLiteral(":gross_amount"), node->initial_total);
    query.bindValue(QStringLiteral(":second"), node->second);
    query.bindValue(QStringLiteral(":first"), node->first);
    query.bindValue(QStringLiteral(":discount"), node->discount);
    query.bindValue(QStringLiteral(":net_amount"), node->final_total);
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
    node->final_total = query.value(QStringLiteral("net_amount")).toDouble();
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
    query.bindValue(QStringLiteral(":net_amount"), node->final_total);
}
