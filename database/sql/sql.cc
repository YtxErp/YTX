#include "sql.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QTimer>

#include "component/constvalue.h"
#include "global/resourcepool.h"

Sql::Sql(QSqlDatabase& main_db, CInfo& info, QObject* parent)
    : QObject(parent)
    , section_ { info.section }
    , main_db_ { main_db }
    , info_ { info }
{
}

Sql::~Sql() { qDeleteAll(trans_hash_); }

void Sql::RRemoveNode(int node_id, int node_type)
{
    // This function is triggered when removing a node that has internal/support references.
    emit SFreeWidget(node_id, node_type);
    emit SRemoveNode(node_id);

    if (node_type == kTypeSupport) {
        RemoveSupportFunction(node_id);
    }

    if (node_type == kTypeLeaf) {
        QMultiHash<int, int> leaf_trans {};
        QMultiHash<int, int> support_trans {};

        TransToRemove(leaf_trans, support_trans, node_id, node_type);

        emit SRemoveMultiTransL(section_, leaf_trans);

        if (!support_trans.isEmpty())
            emit SRemoveMultiTransS(section_, support_trans);

        RemoveLeafFunction(leaf_trans);

        auto update_list { leaf_trans.uniqueKeys() };
        update_list.removeOne(node_id);

        QTimer::singleShot(0, this, [this, update_list = std::move(update_list)]() { emit SSyncMultiLeafValue(update_list); });
    }

    // Remove node, path, trans from the sqlite3 database
    RemoveNode(node_id, node_type);
}

void Sql::TransToRemove(QMultiHash<int, int>& leaf_trans, QMultiHash<int, int>& support_trans, int node_id, int node_type) const
{
    assert(node_type == kTypeLeaf && "node_type must be kTypeLeaf");

    QSqlQuery query(main_db_);
    query.setForwardOnly(true);

    CString string { QSTransToRemove() };
    query.prepare(string);

    query.bindValue(QStringLiteral(":node_id"), node_id);
    if (!query.exec()) {
        qWarning() << "Failed in TransToRemove" << query.lastError().text();
        return;
    }

    while (query.next()) {
        const int trans_id { query.value("trans_id").toInt() };
        leaf_trans.emplace(query.value("node_id").toInt(), trans_id);

        if (section_ != Section::kSales && section_ != Section::kPurchase) {
            const int support_id { query.value("support_id").toInt() };
            if (support_id)
                support_trans.emplace(support_id, trans_id);
        }
    }
}

void Sql::RemoveSupportFunction(int support_id) const
{
    for (auto* trans : std::as_const(trans_hash_)) {
        if (trans->support_id == support_id) {
            trans->support_id = 0;
        }
    }
}

void Sql::RemoveLeafFunction(const QMultiHash<int, int>& leaf_trans)
{
    // Recycle trans resources
    const auto trans_list { leaf_trans.values() };

    for (int trans_id : trans_list) {
        ResourcePool<Trans>::Instance().Recycle(trans_hash_.take(trans_id));
    }
}

bool Sql::FreeView(int old_node_id, int new_node_id) const
{
    QSqlQuery query(main_db_);
    CString string { QSFreeView() };

    query.prepare(string);
    query.setForwardOnly(true);

    query.bindValue(QStringLiteral(":new_node_id"), new_node_id);
    query.bindValue(QStringLiteral(":old_node_id"), old_node_id);
    if (!query.exec()) {
        qWarning() << "Failed in RReplaceNode" << query.lastError().text();
        return false;
    }

    query.next();
    return query.value(0).toInt() == 0;
}

void Sql::RReplaceNode(int old_node_id, int new_node_id, int node_type, int node_unit)
{
    assert(section_ != Section::kPurchase && section_ != Section::kSales && section_ != Section::kStakeholder
        && "Invalid section: should not be kPurchase, kSales or kStakeholder");

    const bool free { FreeView(old_node_id, new_node_id) };
    QSet<int> trans_id_set {};

    if (node_type == kTypeSupport) {
        ReplaceSupport(old_node_id, new_node_id);

        ReplaceSupportFunction(trans_id_set, old_node_id, new_node_id);
        emit SMoveMultiTransS(section_, 0, new_node_id, trans_id_set);
    }

    if (node_type == kTypeLeaf) {
        ReplaceLeaf(old_node_id, new_node_id, node_unit);
        ReplaceLeafFunction(trans_id_set, old_node_id, new_node_id);

        emit SMoveMultiTransL(section_, old_node_id, new_node_id, trans_id_set);
        emit SSyncMultiLeafValue(free ? QList<int> { new_node_id } : QList<int> { old_node_id, new_node_id });

        if (section_ == Section::kProduct && node_unit != std::to_underlying(UnitP::kPos)) {
            emit SSyncProduct(old_node_id, new_node_id);
        }
    }

    if (free || node_type == kTypeSupport) {
        RemoveNode(old_node_id);
        emit SFreeWidget(old_node_id, node_type);
        emit SRemoveNode(old_node_id);
    }
}

bool Sql::ReplaceLeaf(int old_node_id, int new_node_id, int /*node_unit*/)
{
    QSqlQuery query(main_db_);
    QString string { QSReplaceLeaf() };

    query.prepare(string);
    query.bindValue(QStringLiteral(":new_node_id"), new_node_id);
    query.bindValue(QStringLiteral(":old_node_id"), old_node_id);

    if (!query.exec()) {
        qWarning() << "Failed in ReplaceTrans" << query.lastError().text();
        return false;
    }

    return true;
}

bool Sql::ReplaceSupport(int old_node_id, int new_node_id)
{
    QSqlQuery query(main_db_);

    CString string { QSReplaceSupport() };
    query.prepare(string);

    query.bindValue(QStringLiteral(":new_node_id"), new_node_id);
    query.bindValue(QStringLiteral(":old_node_id"), old_node_id);

    if (!query.exec()) {
        qWarning() << "Failed in ReplaceSupport" << query.lastError().text();
        return false;
    }

    return true;
}

bool Sql::RemoveNode(int old_node_id)
{
    // Remove node and path, ignore trans
    QSqlQuery query(main_db_);

    CString string_frist { QSRemoveNodeFirst() };
    CString string_third { QSRemoveNodeThird() };

    return DBTransaction([&]() {
        query.prepare(string_frist);
        query.bindValue(QStringLiteral(":node_id"), old_node_id);
        if (!query.exec()) {
            qWarning() << "Section: " << std::to_underlying(section_) << "Failed in ReplaceNode 1st" << query.lastError().text();
            return false;
        }

        query.prepare(string_third);
        query.bindValue(QStringLiteral(":node_id"), old_node_id);
        if (!query.exec()) {
            qWarning() << "Section: " << std::to_underlying(section_) << "Failed in ReplaceNode 3rd" << query.lastError().text();
            return false;
        }

        return true;
    });
}

bool Sql::ReadNode(NodeHash& node_hash)
{
    CString string { QSReadNode() };
    if (string.isEmpty())
        return false;

    QSqlQuery query(main_db_);
    query.setForwardOnly(true);
    query.prepare(string);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(section_) << "Failed in ReadNode" << query.lastError().text();
        return false;
    }

    while (query.next()) {
        auto* node { ResourcePool<Node>::Instance().Allocate() };
        ReadNodeQuery(node, query);
        node_hash.insert(node->id, node);
    }

    ReadRelationship(node_hash, query);
    return true;
}

bool Sql::WriteNode(int parent_id, Node* node)
{
    // root_'s id is -1
    CString string { QSWriteNode() };
    if (string.isEmpty() || !node)
        return false;

    QSqlQuery query(main_db_);
    return DBTransaction([&]() {
        // 插入节点记录
        query.prepare(string);
        WriteNodeBind(node, query);

        if (!query.exec()) {
            qWarning() << "Section: " << std::to_underlying(section_) << "Failed in WriteNode" << query.lastError().text();
            return false;
        }

        node->id = query.lastInsertId().toInt();

        WriteRelationship(node->id, parent_id, query);
        return true;
    });
}

void Sql::CalculateLeafTotal(Node* node, QSqlQuery& query) const
{
    bool rule { node->rule };
    int sign = rule ? 1 : -1;

    if (query.next()) {
        const double initial_balance { query.value(QStringLiteral("initial_balance")).toDouble() };
        const double final_balance { query.value(QStringLiteral("final_balance")).toDouble() };

        node->initial_total = sign * initial_balance;
        node->final_total = sign * final_balance;
    }
}

bool Sql::ReadLeafTotal(Node* node) const
{
    assert(node && "Node is null");
    assert(node->id >= 1 && "Node ID must be positive");
    assert(node->type == kTypeLeaf && "Node type must be kTypeLeaf");

    CString string { QSLeafTotal() };
    if (string.isEmpty())
        return false;

    QSqlQuery query(main_db_);
    query.setForwardOnly(true);

    query.prepare(string);
    query.bindValue(QStringLiteral(":node_id"), node->id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(section_) << "Failed in LeafTotal" << query.lastError().text();
        return false;
    }

    CalculateLeafTotal(node, query);
    return true;
}

bool Sql::SearchNodeName(QSet<int>& node_id_set, CString& text) const
{
    QSqlQuery query(main_db_);
    query.setForwardOnly(true);

    QString string {};
    if (text.isEmpty())
        string = QString("SELECT id FROM %1 WHERE removed = false").arg(info_.node);
    else
        string = QString("SELECT id FROM %1 WHERE removed = false AND name LIKE :text").arg(info_.node);

    query.prepare(string);

    if (!text.isEmpty())
        query.bindValue(QStringLiteral(":text"), QString("%%%1%").arg(text));

    if (!query.exec()) {
        qWarning() << "Failed in SearchNodeName" << query.lastError().text();
        return {};
    }

    while (query.next()) {
        int node_id { query.value(QStringLiteral("id")).toInt() };
        node_id_set.insert(node_id);
    }

    return true;
}

bool Sql::RemoveNode(int node_id, int node_type)
{
    QSqlQuery query(main_db_);

    CString string_first { QSRemoveNodeFirst() };
    QString string_second {};
    CString string_third { QSRemoveNodeThird() };

    switch (node_type) {
    case kTypeLeaf:
        string_second = QSRemoveNodeSecond();
        break;
    case kTypeBranch:
        string_second = QSRemoveBranch();
        break;
    case kTypeSupport:
        string_second = QSRemoveSupport();
        break;
    default:
        break;
    }

    return DBTransaction([&]() {
        query.prepare(string_first);
        query.bindValue(QStringLiteral(":node_id"), node_id);
        if (!query.exec()) {
            qWarning() << "Section: " << std::to_underlying(section_) << "Failed in RemoveNode 1st" << query.lastError().text();
            return false;
        }

        query.prepare(string_second);
        query.bindValue(QStringLiteral(":node_id"), node_id);
        if (!query.exec()) {
            qWarning() << "Section: " << std::to_underlying(section_) << "Failed in RemoveNode 2nd" << query.lastError().text();
            return false;
        }

        query.prepare(string_third);
        query.bindValue(QStringLiteral(":node_id"), node_id);
        if (!query.exec()) {
            qWarning() << "Section: " << std::to_underlying(section_) << "Failed in RemoveNode 3rd" << query.lastError().text();
            return false;
        }

        return true;
    });
}

void Sql::ReplaceSupportFunction(QSet<int>& trans_id_set, int old_support_id, int new_support_id)
{
    for (auto* trans : std::as_const(trans_hash_)) {
        if (trans->support_id == old_support_id) {
            trans->support_id = new_support_id;
            trans_id_set.insert(trans->id);
        }
    }
}

QString Sql::QSRemoveNodeFirst() const
{
    return QString(R"(
            UPDATE %1
            SET removed = true
            WHERE id = :node_id
            )")
        .arg(info_.node);
}

QString Sql::QSRemoveBranch() const
{
    return QString(R"(
            WITH related_nodes AS (
                SELECT DISTINCT fp1.ancestor, fp2.descendant
                FROM %1 AS fp1
                INNER JOIN %1 AS fp2 ON fp1.descendant = fp2.ancestor
                WHERE fp2.ancestor = :node_id AND fp2.descendant != :node_id AND fp1.ancestor != :node_id
            )
            UPDATE %1
            SET distance = distance - 1
            WHERE (ancestor, descendant) IN (
            SELECT ancestor, descendant FROM related_nodes)
            )")
        .arg(info_.path);
}

QString Sql::QSRemoveNodeThird() const
{
    return QStringLiteral("DELETE FROM %1 WHERE (descendant = :node_id OR ancestor = :node_id) AND distance !=0").arg(info_.path);
}

QString Sql::QSDragNodeFirst() const
{
    return QString(R"(
            WITH related_nodes AS (
                SELECT DISTINCT fp1.ancestor, fp2.descendant
                FROM %1 AS fp1
                INNER JOIN %1 AS fp2 ON fp1.descendant = fp2.ancestor
                WHERE fp2.ancestor = :node_id AND fp1.ancestor != :node_id
            )
            DELETE FROM %1
            WHERE (ancestor, descendant) IN (
            SELECT ancestor, descendant FROM related_nodes)
            )")
        .arg(info_.path);
}

QString Sql::QSDragNodeSecond() const
{
    return QString(R"(
            INSERT INTO %1 (ancestor, descendant, distance)
            SELECT fp1.ancestor, fp2.descendant, fp1.distance + fp2.distance + 1
            FROM %1 AS fp1
            INNER JOIN %1 AS fp2
            WHERE fp1.descendant = :destination_node_id AND fp2.ancestor = :node_id
            )")
        .arg(info_.path);
}

bool Sql::DragNode(int destination_node_id, int node_id)
{
    QSqlQuery query(main_db_);

    CString string_first { QSDragNodeFirst() };
    CString string_second { QSDragNodeSecond() };

    return DBTransaction([&]() {
        query.prepare(string_first);
        query.bindValue(QStringLiteral(":node_id"), node_id);

        if (!query.exec()) {
            qWarning() << "Failed in DragNode 1st" << query.lastError().text();
            return false;
        }

        query.prepare(string_second);
        query.bindValue(QStringLiteral(":node_id"), node_id);
        query.bindValue(QStringLiteral(":destination_node_id"), destination_node_id);

        if (!query.exec()) {
            qWarning() << "Failed in DragNode 2nd" << query.lastError().text();
            return false;
        }

        return true;
    });
}

bool Sql::InternalReference(int node_id) const
{
    assert(node_id >= 1 && "Node ID must be positive");

    CString string { QSInternalReference() };
    if (string.isEmpty())
        return false;

    QSqlQuery query(main_db_);
    query.setForwardOnly(true);

    query.prepare(string);
    query.bindValue(QStringLiteral(":node_id"), node_id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(section_) << "Failed in InternalReference" << query.lastError().text();
        return false;
    }

    if (query.next())
        return query.value(0).toBool();

    return false;
}

bool Sql::ExternalReference(int node_id) const
{
    assert(node_id >= 1 && "Node ID must be positive");

    CString string { QSExternalReference() };
    if (string.isEmpty())
        return false;

    QSqlQuery query(main_db_);
    query.setForwardOnly(true);

    query.prepare(string);
    query.bindValue(QStringLiteral(":node_id"), node_id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(section_) << "Failed in ExternalReference" << query.lastError().text();
        return false;
    }

    if (query.next())
        return query.value(0).toBool();

    return false;
}

bool Sql::SupportReference(int support_id) const
{
    assert(support_id >= 1 && "support_id must be positive");

    CString string { QSSupportReference() };
    if (string.isEmpty())
        return false;

    QSqlQuery query(main_db_);
    query.setForwardOnly(true);

    query.prepare(string);
    query.bindValue(QStringLiteral(":support_id"), support_id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(section_) << "Failed in SupportReference" << query.lastError().text();
        return false;
    }

    return query.next();
}

bool Sql::ReadTrans(TransShadowList& trans_shadow_list, int node_id)
{
    QSqlQuery query(main_db_);
    query.setForwardOnly(true);

    CString string { QSReadTrans() };
    query.prepare(string);
    query.bindValue(QStringLiteral(":node_id"), node_id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(section_) << "Failed in ReadNodeTrans" << query.lastError().text();
        return false;
    }

    ReadTransFunction(trans_shadow_list, node_id, query);
    return true;
}

void Sql::ConvertTrans(Trans* trans, TransShadow* trans_shadow, bool left) const
{
    trans_shadow->id = &trans->id;
    trans_shadow->state = &trans->state;
    trans_shadow->date_time = &trans->date_time;
    trans_shadow->code = &trans->code;
    trans_shadow->document = &trans->document;
    trans_shadow->description = &trans->description;
    trans_shadow->support_id = &trans->support_id;
    trans_shadow->discount = &trans->discount;

    trans_shadow->lhs_node = &(left ? trans->lhs_node : trans->rhs_node);
    trans_shadow->lhs_ratio = &(left ? trans->lhs_ratio : trans->rhs_ratio);
    trans_shadow->lhs_debit = &(left ? trans->lhs_debit : trans->rhs_debit);
    trans_shadow->lhs_credit = &(left ? trans->lhs_credit : trans->rhs_credit);

    trans_shadow->rhs_node = &(left ? trans->rhs_node : trans->lhs_node);
    trans_shadow->rhs_ratio = &(left ? trans->rhs_ratio : trans->lhs_ratio);
    trans_shadow->rhs_debit = &(left ? trans->rhs_debit : trans->lhs_debit);
    trans_shadow->rhs_credit = &(left ? trans->rhs_credit : trans->lhs_credit);
}

bool Sql::WriteTrans(TransShadow* trans_shadow)
{
    QSqlQuery query(main_db_);
    CString string { QSWriteTrans() };

    query.prepare(string);
    WriteTransBind(trans_shadow, query);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(section_) << "Failed in WriteTrans" << query.lastError().text();
        return false;
    }

    *trans_shadow->id = query.lastInsertId().toInt();
    trans_hash_.insert(*trans_shadow->id, last_trans_);
    return true;
}

bool Sql::RemoveTrans(int trans_id)
{
    QSqlQuery query(main_db_);
    CString part { QSRemoveTrans() };

    query.prepare(part);
    query.bindValue(QStringLiteral(":trans_id"), trans_id);
    if (!query.exec()) {
        qWarning() << "Failed in RemoveTrans" << query.lastError().text();
        return false;
    }

    ResourcePool<Trans>::Instance().Recycle(trans_hash_.take(trans_id));
    return true;
}

bool Sql::UpdateLeafValue(const Node* node) const
{
    if (node->type != kTypeLeaf)
        return false;

    CString string { QSUpdateLeafValue() };
    if (string.isEmpty())
        return false;

    QSqlQuery query(main_db_);

    query.prepare(string);
    UpdateLeafValueBind(node, query);

    if (!query.exec()) {
        qWarning() << "Failed in SyncLeafValue" << query.lastError().text();
        return false;
    }

    return true;
}

bool Sql::UpdateTransValue(const TransShadow* trans_shadow) const
{
    CString string { QSUpdateTransValue() };
    if (string.isEmpty())
        return false;

    QSqlQuery query(main_db_);

    query.prepare(string);
    UpdateTransValueBind(trans_shadow, query);

    if (!query.exec()) {
        qWarning() << "Failed in SyncTransValue" << query.lastError().text();
        return false;
    }

    return true;
}

bool Sql::WriteField(CString& table, CString& field, CVariant& value, int id) const
{
    QSqlQuery query(main_db_);

    auto part = QString(R"(
    UPDATE %1
    SET %2 = :value
    WHERE id = :id
    )")
                    .arg(table, field);

    query.prepare(part);
    query.bindValue(QStringLiteral(":id"), id);
    query.bindValue(QStringLiteral(":value"), value);

    if (!query.exec()) {
        qWarning() << "Failed in UpdateField" << query.lastError().text();
        return false;
    }

    return true;
}

bool Sql::WriteState(Check state) const
{
    QSqlQuery query(main_db_);

    // 使用 is_not_reverse 表示 state != Check::kReverse，避免重复计算
    const bool is_not_reverse { state != Check::kReverse };

    // 构建 SQL 查询字符串，调整三元运算符顺序，避免重复判断
    const auto string { is_not_reverse ? QString("UPDATE %1 SET state = :value").arg(info_.trans)
                                       : QString("UPDATE %1 SET state = NOT state").arg(info_.trans) };

    query.prepare(string);

    // 仅在 is_not_reverse 为 true 时绑定新值
    if (is_not_reverse) {
        query.bindValue(QStringLiteral(":value"), (state != Check::kNone));
    }

    // 执行查询并检查结果
    if (!query.exec()) {
        qWarning() << "Failed in UpdateState" << query.lastError().text();
        return false;
    }

    return true;
}

bool Sql::SearchTrans(TransList& trans_list, CString& text)
{
    if (text.isEmpty())
        return false;

    QSqlQuery query(main_db_);
    query.setForwardOnly(true);

    query.prepare(QSSearchTransText());
    query.bindValue(QStringLiteral(":description"), "%" + text + "%");

    if (!query.exec()) {
        qWarning() << "Failed in SearchTransText" << query.lastError().text();
        return false;
    }

    ReadTransFunction(trans_list, query);

    bool ok { false };
    const double value { text.toDouble(&ok) };

    if (ok && value != 0.0) {
        const double tolerance { value * 0.1 };
        query.prepare(QSSearchTransValue());
        query.bindValue(QStringLiteral(":value"), value);
        query.bindValue(QStringLiteral(":tolerance"), tolerance);

        if (!query.exec()) {
            qWarning() << "Failed in SearchTransValue" << query.lastError().text();
            return false;
        }

        ReadTransFunction(trans_list, query);
    }

    return true;
}

bool Sql::ReadTransRef(TransList& trans_list, int node_id, int unit, const QDateTime& start, const QDateTime& end) const
{
    QSqlQuery query(main_db_);
    query.setForwardOnly(true);

    auto string { QSReadTransRef(unit) };
    if (string.isEmpty())
        return false;

    query.prepare(string);
    query.bindValue(QStringLiteral(":node_id"), node_id);
    query.bindValue(QStringLiteral(":start"), start.toString(kDateTimeFST));
    query.bindValue(QStringLiteral(":end"), end.toString(kDateTimeFST));

    if (!query.exec()) {
        qWarning() << "Failed in TransRefRecord" << query.lastError().text();
        return false;
    }

    ReadTransRefQuery(trans_list, query);

    return true;
}

bool Sql::RetrieveTransRange(TransShadowList& trans_shadow_list, int node_id, const QSet<int>& trans_id_set)
{
    if (trans_id_set.empty() || node_id <= 0)
        return false;

    for (int id : trans_id_set) {
        auto* trans { trans_hash_.value(id) };
        auto* trans_shadow { ResourcePool<TransShadow>::Instance().Allocate() };

        ConvertTrans(trans, trans_shadow, node_id == trans->lhs_node);
        trans_shadow_list.emplaceBack(trans_shadow);
    }

    return true;
}

bool Sql::RetrieveTransRange(TransList& trans_shadow_list, const QSet<int>& trans_id_set)
{
    if (trans_id_set.empty())
        return false;

    for (int id : trans_id_set) {
        auto* trans { trans_hash_.value(id) };
        trans_shadow_list.emplaceBack(trans);
    }

    return true;
}

bool Sql::ReadSupportTrans(TransList& trans_list, int support_id)
{
    QSqlQuery query(main_db_);
    query.setForwardOnly(true);

    CString string { QSReadSupportTrans() };
    if (string.isEmpty())
        return false;

    query.prepare(string);
    query.bindValue(QStringLiteral(":node_id"), support_id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(section_) << "Failed in ReadSupportTrans" << query.lastError().text();
        return false;
    }

    ReadTransFunction(trans_list, query);
    return true;
}

TransShadow* Sql::AllocateTransShadow()
{
    last_trans_ = ResourcePool<Trans>::Instance().Allocate();
    auto* trans_shadow { ResourcePool<TransShadow>::Instance().Allocate() };

    ConvertTrans(last_trans_, trans_shadow, true);
    return trans_shadow;
}

bool Sql::DBTransaction(std::function<bool()> function)
{
    if (main_db_.transaction() && function() && main_db_.commit()) {
        return true;
    } else {
        main_db_.rollback();
        return false;
    }
}

bool Sql::ReadRelationship(const NodeHash& node_hash, QSqlQuery& query) const
{
    if (node_hash.isEmpty())
        return false;

    auto part = QString(R"(
    SELECT ancestor, descendant
    FROM %1
    WHERE distance = 1
)")
                    .arg(info_.path);

    query.prepare(part);
    if (!query.exec()) {
        qWarning() << "Failed in ReadRelationship" << query.lastError().text();
        return false;
    }

    while (query.next()) {
        const int kAncestorID { query.value(QStringLiteral("ancestor")).toInt() };
        const int kDescendantID { query.value(QStringLiteral("descendant")).toInt() };

        Node* ancestor { node_hash.value(kAncestorID) };
        Node* descendant { node_hash.value(kDescendantID) };

        if (!ancestor || !descendant)
            continue;

        ancestor->children.emplaceBack(descendant);
        descendant->parent = ancestor;
    }

    return true;
}

bool Sql::WriteRelationship(int node_id, int parent_id, QSqlQuery& query) const
{
    auto part = QString(R"(
    INSERT INTO %1 (ancestor, descendant, distance)
    SELECT ancestor, :node_id, distance + 1 FROM %1
    WHERE descendant = :parent
    UNION ALL
    SELECT :node_id, :node_id, 0
)")
                    .arg(info_.path);

    query.prepare(part);
    query.bindValue(QStringLiteral(":node_id"), node_id);
    query.bindValue(QStringLiteral(":parent"), parent_id);

    if (!query.exec()) {
        qWarning() << "Failed in WriteRelationship" << query.lastError().text();
        return false;
    }

    return true;
}

QString Sql::QSRemoveTrans() const
{
    return QString(R"(
    UPDATE %1
    SET removed = true
    WHERE id = :trans_id
    )")
        .arg(info_.trans);
}

QString Sql::QSFreeView() const
{
    return QString(R"(
    SELECT COUNT(*) FROM %1
    WHERE ((lhs_node = :old_node_id AND rhs_node = :new_node_id) OR (rhs_node = :old_node_id AND lhs_node = :new_node_id)) AND removed = 0
    )")
        .arg(info_.trans);
}

void Sql::ReadTransFunction(TransShadowList& trans_shadow_list, int node_id, QSqlQuery& query)
{
    TransShadow* trans_shadow {};
    Trans* trans {};

    while (query.next()) {
        const int kID { query.value(QStringLiteral("id")).toInt() };

        trans_shadow = ResourcePool<TransShadow>::Instance().Allocate();

        if (auto it = trans_hash_.constFind(kID); it != trans_hash_.constEnd()) {
            trans = it.value();
        } else {
            trans = ResourcePool<Trans>::Instance().Allocate();
            trans->id = kID;

            ReadTransQuery(trans, query);
            trans_hash_.insert(kID, trans);
        }

        ConvertTrans(trans, trans_shadow, node_id == trans->lhs_node);
        trans_shadow_list.emplaceBack(trans_shadow);
    }
}

void Sql::ReadTransFunction(TransList& trans_list, QSqlQuery& query)
{
    Trans* trans {};

    while (query.next()) {
        const int kID { query.value(QStringLiteral("id")).toInt() };

        if (auto it = trans_hash_.constFind(kID); it != trans_hash_.constEnd()) {
            trans = it.value();

            if (trans_list.contains(trans))
                continue;

        } else {
            trans = ResourcePool<Trans>::Instance().Allocate();
            trans->id = kID;

            ReadTransQuery(trans, query);
            trans_hash_.insert(kID, trans);
        }

        trans_list.emplaceBack(trans);
    }
}

void Sql::ReplaceLeafFunction(QSet<int>& trans_id_set, int old_node_id, int new_node_id) const
{
    for (auto* trans : std::as_const(trans_hash_)) {
        if (trans->lhs_node == old_node_id && trans->rhs_node != new_node_id) {
            trans_id_set.insert(trans->id);
            trans->lhs_node = new_node_id;
        }

        if (trans->rhs_node == old_node_id && trans->lhs_node != new_node_id) {
            trans_id_set.insert(trans->id);
            trans->rhs_node = new_node_id;
        }
    }
}
