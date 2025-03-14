#include "sqlite.h"

#include <QSqlError>
#include <QSqlQuery>

#include "component/constvalue.h"
#include "global/resourcepool.h"
#include "global/sqlconnection.h"

Sqlite::Sqlite(CInfo& info, QObject* parent)
    : QObject(parent)
    , db_ { SqlConnection::Instance().Allocate(info.section) }
    , info_ { info }
{
}

Sqlite::~Sqlite() { qDeleteAll(trans_hash_); }

void Sqlite::RRemoveNode(int node_id, int node_type)
{
    // Notify MainWindow to release the table view
    emit SFreeWidget(node_id);
    // Notify TreeModel to remove the node
    emit SRemoveNode(node_id);

    // Mark Trans for removal

    QMultiHash<int, int> leaf_trans_hash {};
    QMultiHash<int, int> support_trans_hash {};

    if (node_type == kTypeLeaf) {
        leaf_trans_hash = TransToRemove(node_id, kTypeLeaf);
        support_trans_hash = TransToRemove(node_id, kTypeSupport);
    }

    // Remove node, path, trans from the sqlite3 database
    RemoveNode(node_id, node_type);

    // Process buffered trans
    if (node_type == kTypeSupport) {
        RemoveSupportFunction(node_id);
        return;
    }

    // Handle node and trans based on the current section

    emit SRemoveMultiTrans(leaf_trans_hash);
    emit SUpdateMultiLeafTotal(leaf_trans_hash.uniqueKeys());

    if (!support_trans_hash.isEmpty())
        emit SRemoveMultiSupportTrans(support_trans_hash);

    // Recycle trans resources
    const auto trans_list { leaf_trans_hash.values() };

    for (int trans_id : trans_list)
        ResourcePool<Trans>::Instance().Recycle(trans_hash_.take(trans_id));
}

QMultiHash<int, int> Sqlite::TransToRemove(int node_id, int node_type) const
{
    QMultiHash<int, int> hash {};

    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    QString string {};

    switch (node_type) {
    case kTypeLeaf:
        string = QSTransToRemove();
        break;
    case kTypeSupport:
        string = QSSupportTransToRemove();
        break;
    default:
        break;
    }

    query.prepare(string);
    query.bindValue(QStringLiteral(":node_id"), node_id);
    if (!query.exec()) {
        qWarning() << "Failed in TransToRemove" << query.lastError().text();
        return {};
    }

    while (query.next()) {
        hash.emplace(query.value(0).toInt(), query.value(1).toInt());
    }

    return hash;
}

QList<int> Sqlite::SupportTransToMove(int support_id) const
{
    QList<int> list {};

    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    CString string { QSSupportTransToMove() };

    query.prepare(string);
    query.bindValue(QStringLiteral(":support_id"), support_id);

    if (!query.exec()) {
        qWarning() << "Failed in SupportTransToMoveFPTS" << query.lastError().text();
        return {};
    }

    while (query.next()) {
        list.emplaceBack(query.value(0).toInt());
    }

    return list;
}

void Sqlite::RemoveSupportFunction(int support_id) const
{
    const auto& const_trans_hash { std::as_const(trans_hash_) };

    for (auto* trans : const_trans_hash) {
        if (trans->support_id == support_id) {
            trans->support_id = 0;
        }
    }
}

bool Sqlite::FreeView(int old_node_id, int new_node_id) const
{
    QSqlQuery query(*db_);
    CString string { QSFreeViewFPT() };

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

void Sqlite::RReplaceNode(int old_node_id, int new_node_id, int node_type)
{
    auto section { info_.section };
    if (section == Section::kPurchase || section == Section::kSales)
        return;

    QString string {};
    bool free {};
    QList<int> support_trans {};

    switch (node_type) {
    case kTypeLeaf:
        string = QSReplaceNodeTransFPTS();
        free = FreeView(old_node_id, new_node_id);
        break;
    case kTypeSupport:
        string = QSReplaceSupportTransFPTS();
        support_trans = SupportTransToMove(old_node_id);
        break;
    default:
        break;
    }

    // begin deal with database
    QSqlQuery query(*db_);
    query.prepare(string);

    query.bindValue(QStringLiteral(":new_node_id"), new_node_id);
    query.bindValue(QStringLiteral(":old_node_id"), old_node_id);
    if (!query.exec()) {
        qWarning() << "Failed in RReplaceNode" << query.lastError().text();
        return;
    }
    // end deal with database

    if (node_type == kTypeSupport) {
        emit SFreeWidget(old_node_id);
        emit SRemoveNode(old_node_id);
        emit SMoveMultiSupportTrans(info_.section, new_node_id, support_trans);

        ReplaceSupportFunction(old_node_id, new_node_id);
        RemoveNode(old_node_id, kTypeSupport);

        return;
    }

    // begin deal with trans hash
    auto node_trans { ReplaceNodeFunction(old_node_id, new_node_id) };
    // end deal with trans hash

    emit SMoveMultiTrans(old_node_id, new_node_id, node_trans.values());
    emit SUpdateMultiLeafTotal(QList { old_node_id, new_node_id });

    if (section == Section::kProduct)
        emit SUpdateProduct(old_node_id, new_node_id);

    if (free) {
        emit SFreeWidget(old_node_id);
        emit SRemoveNode(old_node_id);
        RemoveNode(old_node_id, kTypeLeaf);
    }
}

void Sqlite::RUpdateProduct(int old_node_id, int new_node_id)
{
    CString string { QSUpdateProductReferenceSO() };
    if (string.isEmpty())
        return;

    QSqlQuery query(*db_);

    query.prepare(string);
    query.bindValue(QStringLiteral(":old_node_id"), old_node_id);
    query.bindValue(QStringLiteral(":new_node_id"), new_node_id);
    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in RUpdateProductReference" << query.lastError().text();
        return;
    }

    UpdateProductReferenceSO(old_node_id, new_node_id);
}

void Sqlite::RUpdateStakeholder(int old_node_id, int new_node_id)
{
    CString string { QSUpdateStakeholderReferenceO() };
    if (string.isEmpty())
        return;

    QSqlQuery query(*db_);

    query.prepare(string);
    query.bindValue(QStringLiteral(":old_node_id"), old_node_id);
    query.bindValue(QStringLiteral(":new_node_id"), new_node_id);
    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in RUpdateStakeholderReference" << query.lastError().text();
        return;
    }

    UpdateStakeholderReferenceO(old_node_id, new_node_id);
}

bool Sqlite::ReadNode(NodeHash& node_hash)
{
    CString string { QSReadNode() };
    if (string.isEmpty())
        return false;

    QSqlQuery query(*db_);
    query.setForwardOnly(true);
    query.prepare(string);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in ReadNode" << query.lastError().text();
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

bool Sqlite::WriteNode(int parent_id, Node* node) const
{
    // root_'s id is -1
    CString string { QSWriteNode() };
    if (string.isEmpty() || !node)
        return false;

    QSqlQuery query(*db_);
    if (!DBTransaction([&]() {
            // 插入节点记录
            query.prepare(string);
            WriteNodeBind(node, query);

            if (!query.exec()) {
                qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in WriteNode" << query.lastError().text();
                return false;
            }

            // 获取最后插入的ID
            node->id = query.lastInsertId().toInt();

            // 插入节点路径记录
            WriteRelationship(node->id, parent_id, query);
            return true;
        })) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in WriteNode commit";
        return false;
    }

    return true;
}

void Sqlite::CalculateLeafTotal(Node* node, QSqlQuery& query) const
{
    // finance, product, task
    bool rule { node->rule };
    int sign = rule ? 1 : -1;

    if (query.next()) {
        QVariant initial_balance { query.value(QStringLiteral("initial_balance")) };
        QVariant final_balance { query.value(QStringLiteral("final_balance")) };

        node->initial_total = sign * (initial_balance.isNull() ? 0.0 : initial_balance.toDouble());
        node->final_total = sign * (final_balance.isNull() ? 0.0 : final_balance.toDouble());
    }
}

bool Sqlite::ReadLeafTotal(Node* node) const
{
    CString string { QSLeafTotalFPT() };

    if (string.isEmpty() || !node || node->id <= 0 || node->type != kTypeLeaf)
        return false;

    QSqlQuery query(*db_);
    query.setForwardOnly(true);
    query.prepare(string);
    query.bindValue(QStringLiteral(":node_id"), node->id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in LeafTotal" << query.lastError().text();
        return false;
    }

    CalculateLeafTotal(node, query);
    return true;
}

QList<int> Sqlite::SearchNodeName(CString& text) const
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    QString string {};
    if (text.isEmpty())
        string = QString("SELECT id FROM %1 WHERE removed = 0").arg(info_.node);
    else
        string = QString("SELECT id FROM %1 WHERE removed = 0 AND name LIKE :text").arg(info_.node);

    query.prepare(string);

    if (!text.isEmpty())
        query.bindValue(QStringLiteral(":text"), QString("%%%1%").arg(text));

    if (!query.exec()) {
        qWarning() << "Failed in SearchNodeName" << query.lastError().text();
        return {};
    }

    QList<int> node_list {};

    while (query.next()) {
        int node_id { query.value(QStringLiteral("id")).toInt() };
        node_list.emplaceBack(node_id);
    }

    return node_list;
}

bool Sqlite::RemoveNode(int node_id, int node_type) const
{
    QSqlQuery query(*db_);

    CString string_frist { QSRemoveNodeFirst() };
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

    if (!DBTransaction([&]() {
            query.prepare(string_frist);
            query.bindValue(QStringLiteral(":node_id"), node_id);
            if (!query.exec()) {
                qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in RemoveNode 1st" << query.lastError().text();
                return false;
            }

            query.clear();

            query.prepare(string_second);
            query.bindValue(QStringLiteral(":node_id"), node_id);
            if (!query.exec()) {
                qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in RemoveNode 2nd" << query.lastError().text();
                return false;
            }

            query.clear();

            query.prepare(string_third);
            query.bindValue(QStringLiteral(":node_id"), node_id);
            if (!query.exec()) {
                qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in RemoveNode 3rd" << query.lastError().text();
                return false;
            }

            return true;
        })) {
        qWarning() << "Failed in RemoveNode";
        return false;
    }

    return true;
}

void Sqlite::ReplaceSupportFunction(int old_support_id, int new_support_id)
{
    const auto& const_trans_hash { std::as_const(trans_hash_) };

    for (auto* trans : const_trans_hash) {
        if (trans->support_id == old_support_id) {
            trans->support_id = new_support_id;
        }
    }
}

QString Sqlite::QSRemoveNodeFirst() const
{
    return QString(R"(
            UPDATE %1
            SET removed = 1
            WHERE id = :node_id
            )")
        .arg(info_.node);
}

QString Sqlite::QSRemoveBranch() const
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

QString Sqlite::QSRemoveNodeThird() const
{
    return QStringLiteral("DELETE FROM %1 WHERE (descendant = :node_id OR ancestor = :node_id) AND distance !=0").arg(info_.path);
}

QString Sqlite::QSDragNodeFirst() const
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

QString Sqlite::QSDragNodeSecond() const
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

bool Sqlite::DragNode(int destination_node_id, int node_id) const
{
    QSqlQuery query(*db_);

    CString string_first { QSDragNodeFirst() };
    CString string_second { QSDragNodeSecond() };

    if (!DBTransaction([&]() {
            // 第一个查询
            query.prepare(string_first);
            query.bindValue(QStringLiteral(":node_id"), node_id);

            if (!query.exec()) {
                qWarning() << "Failed in DragNode 1st" << query.lastError().text();
                return false;
            }

            query.clear();

            // 第二个查询
            query.prepare(string_second);
            query.bindValue(QStringLiteral(":node_id"), node_id);
            query.bindValue(QStringLiteral(":destination_node_id"), destination_node_id);

            if (!query.exec()) {
                qWarning() << "Failed in DragNode 2nd" << query.lastError().text();
                return false;
            }
            return true;
        })) {
        qWarning() << "Failed in DragNode";
        return false;
    }

    return true;
}

bool Sqlite::InternalReference(int node_id) const
{
    CString string { QSInternalReference() };
    if (string.isEmpty() || node_id <= 0)
        return false;

    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    query.prepare(string);
    query.bindValue(QStringLiteral(":node_id"), node_id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in InternalReference" << query.lastError().text();
        return false;
    }

    query.next();
    return query.value(0).toInt() >= 1;
}

bool Sqlite::ExternalReference(int node_id) const
{
    CString string { QSExternalReferencePS() };
    if (string.isEmpty() || node_id <= 0)
        return false;

    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    query.prepare(string);
    query.bindValue(QStringLiteral(":node_id"), node_id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in ExternalReference" << query.lastError().text();
        return false;
    }

    query.next();
    return query.value(0).toInt() >= 1;
}

bool Sqlite::SupportReference(int support_id) const
{
    CString string { QSSupportReference() };
    if (string.isEmpty() || support_id <= 0)
        return false;

    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    query.prepare(string);
    query.bindValue(QStringLiteral(":support_id"), support_id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in SupportReference" << query.lastError().text();
        return false;
    }

    query.next();
    return query.value(0).toInt() >= 1;
}

bool Sqlite::ReadTrans(TransShadowList& trans_shadow_list, int node_id)
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    CString string { QSReadTrans() };
    query.prepare(string);
    query.bindValue(QStringLiteral(":node_id"), node_id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in ReadNodeTrans" << query.lastError().text();
        return false;
    }

    ReadTransFunction(trans_shadow_list, node_id, query);
    return true;
}

void Sqlite::ConvertTrans(Trans* trans, TransShadow* trans_shadow, bool left) const
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

bool Sqlite::WriteTrans(TransShadow* trans_shadow)
{
    QSqlQuery query(*db_);
    CString string { QSWriteTrans() };

    query.prepare(string);
    WriteTransBind(trans_shadow, query);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in WriteTrans" << query.lastError().text();
        return false;
    }

    *trans_shadow->id = query.lastInsertId().toInt();
    trans_hash_.insert(*trans_shadow->id, last_trans_);
    return true;
}

bool Sqlite::WriteTransRange(const QList<TransShadow*>& list) const
{
    if (list.isEmpty())
        return false;

    QSqlQuery query(*db_);

    query.exec(QStringLiteral("PRAGMA synchronous = OFF"));
    query.exec(QStringLiteral("PRAGMA journal_mode = MEMORY"));

    if (!db_->transaction()) {
        qDebug() << "Failed to start transaction" << db_->lastError();
        return false;
    }

    CString string { QSWriteTrans() };

    // 插入多条记录的 SQL 语句
    query.prepare(string);
    WriteTransRangeFunction(list, query);

    // 执行批量插入
    if (!query.execBatch()) {
        qDebug() << "Failed in WriteTransRange" << query.lastError();
        db_->rollback();
        return false;
    }

    // 提交事务
    if (!db_->commit()) {
        qDebug() << "Failed to commit transaction" << db_->lastError();
        db_->rollback();
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

bool Sqlite::RemoveTrans(int trans_id)
{
    QSqlQuery query(*db_);
    auto part = QString(R"(
    UPDATE %1
    SET removed = 1
    WHERE id = :trans_id
)")
                    .arg(info_.trans);

    query.prepare(part);
    query.bindValue(QStringLiteral(":trans_id"), trans_id);
    if (!query.exec()) {
        qWarning() << "Failed in RemoveTrans" << query.lastError().text();
        return false;
    }

    ResourcePool<Trans>::Instance().Recycle(trans_hash_.take(trans_id));
    return true;
}

bool Sqlite::SyncLeafValue(const Node* node) const
{
    if (node->type != kTypeLeaf)
        return false;

    CString string { QSSyncLeafValue() };
    if (string.isEmpty())
        return false;

    QSqlQuery query(*db_);

    query.prepare(string);
    SyncLeafValueBind(node, query);

    if (!query.exec()) {
        qWarning() << "Failed in SyncLeafValue" << query.lastError().text();
        return false;
    }

    return true;
}

bool Sqlite::SyncTransValue(const TransShadow* trans_shadow) const
{
    CString string { QSSyncTransValue() };
    if (string.isEmpty())
        return false;

    QSqlQuery query(*db_);

    query.prepare(string);
    SyncTransValueBind(trans_shadow, query);

    if (!query.exec()) {
        qWarning() << "Failed in SyncTransValue" << query.lastError().text();
        return false;
    }

    return true;
}

bool Sqlite::InvertTransValue(int node_id) const
{
    CString string { QSInvertTransValue() };
    if (string.isEmpty())
        return false;

    QSqlQuery query(*db_);
    query.prepare(string);
    query.bindValue(":lhs_node", node_id);

    if (!query.exec()) {
        qWarning() << "Failed in InvertValue" << query.lastError().text();
        return false;
    }

    return true;
}

bool Sqlite::WriteField(CString& table, CString& field, CVariant& value, int id) const
{
    QSqlQuery query(*db_);

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

bool Sqlite::WriteState(Check state) const
{
    QSqlQuery query(*db_);

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

bool Sqlite::SearchTrans(TransList& trans_list, CString& text) const
{
    if (text.isEmpty())
        return false;

    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    auto string { QSSearchTrans() };
    query.prepare(string);

    query.bindValue(QStringLiteral(":text"), text);
    query.bindValue(QStringLiteral(":description"), QString("%%%1%").arg(text));

    if (!query.exec()) {
        qWarning() << "Failed in Search Trans" << query.lastError().text();
        return false;
    }

    Trans* trans {};

    while (query.next()) {
        const int kID { query.value(QStringLiteral("id")).toInt() };

        if (auto it = trans_hash_.constFind(kID); it != trans_hash_.constEnd()) {
            trans = it.value();
            trans_list.emplaceBack(trans);
            continue;
        }

        trans = ResourcePool<Trans>::Instance().Allocate();
        trans->id = kID;

        ReadTransQuery(trans, query);
        trans_list.emplaceBack(trans);
    }

    return true;
}

bool Sqlite::ReadTransRef(TransList& trans_list, int node_id, const QDateTime& start, const QDateTime& end) const
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    auto string { QSReadTransRef() };
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

bool Sqlite::ReadStatement(TransList& trans_list, int unit, const QDateTime& start, const QDateTime& end) const
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    auto string { QSReadStatement(unit) };
    if (string.isEmpty())
        return false;

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

bool Sqlite::ReadStatementSecondary(TransList& trans_list, int party_id, int unit, const QDateTime& start, const QDateTime& end) const
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    const auto string { QSReadStatementSecondary(unit) };
    if (string.isEmpty())
        return false;

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

bool Sqlite::ReadStatementPrimary(QList<Node*>& node_list, int party_id, int unit, const QDateTime& start, const QDateTime& end) const
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    const auto string { QSReadStatementPrimary(unit) };
    if (string.isEmpty())
        return false;

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

bool Sqlite::ReadTransRange(TransShadowList& trans_shadow_list, int node_id, const QList<int>& trans_id_list)
{
    if (trans_id_list.empty() || node_id <= 0)
        return false;

    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    const qsizetype batch_size { kBatchSize };
    const auto total_batches { (trans_id_list.size() + batch_size - 1) / batch_size };

    for (int batch_index = 0; batch_index != total_batches; ++batch_index) {
        int start = batch_index * batch_size;
        int end = std::min(start + batch_size, trans_id_list.size());

        QList<int> current_batch { trans_id_list.mid(start, end - start) };

        QStringList placeholder { current_batch.size(), QStringLiteral("?") };
        QString string { QSReadTransRangeFPTS(placeholder.join(QStringLiteral(","))) };

        query.prepare(string);

        for (int i = 0; i != current_batch.size(); ++i)
            query.bindValue(i, current_batch.at(i));

        if (!query.exec()) {
            qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in ReadTransRange, batch" << batch_index << ": "
                       << query.lastError().text();
            continue;
        }

        ReadTransFunction(trans_shadow_list, node_id, query);
    }

    return true;
}

bool Sqlite::ReadSupportTrans(TransShadowList& trans_shadow_list, int support_id)
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    CString string { QSReadSupportTransFPTS() };
    if (string.isEmpty())
        return false;

    query.prepare(string);
    query.bindValue(QStringLiteral(":node_id"), support_id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(info_.section) << "Failed in ReadSupportTransFPTS" << query.lastError().text();
        return false;
    }

    ReadTransFunction(trans_shadow_list, support_id, query, true);
    return true;
}

TransShadow* Sqlite::AllocateTransShadow()
{
    last_trans_ = ResourcePool<Trans>::Instance().Allocate();
    auto* trans_shadow { ResourcePool<TransShadow>::Instance().Allocate() };

    ConvertTrans(last_trans_, trans_shadow, true);
    return trans_shadow;
}

bool Sqlite::DBTransaction(std::function<bool()> function) const
{
    if (db_->transaction() && function() && db_->commit()) {
        return true;
    } else {
        db_->rollback();
        qWarning() << "Failed in Transaction";
        return false;
    }
}

bool Sqlite::ReadRelationship(const NodeHash& node_hash, QSqlQuery& query) const
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

        if (!ancestor || !descendant || descendant->parent)
            continue;

        ancestor->children.emplaceBack(descendant);
        descendant->parent = ancestor;
    }

    return true;
}

bool Sqlite::WriteRelationship(int node_id, int parent_id, QSqlQuery& query) const
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

void Sqlite::ReadTransFunction(TransShadowList& trans_shadow_list, int node_id, QSqlQuery& query, bool is_support)
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

        ConvertTrans(trans, trans_shadow, node_id == trans->lhs_node || is_support);
        trans_shadow_list.emplaceBack(trans_shadow);
    }
}

QMultiHash<int, int> Sqlite::ReplaceNodeFunction(int old_node_id, int new_node_id) const
{
    // finance, product, task
    const auto& const_trans_hash { std::as_const(trans_hash_) };
    QMultiHash<int, int> hash {};

    for (auto* trans : const_trans_hash) {
        if (trans->lhs_node == old_node_id && trans->rhs_node != new_node_id) {
            hash.emplace(trans->rhs_node, trans->id);
            trans->lhs_node = new_node_id;
        }

        if (trans->rhs_node == old_node_id && trans->lhs_node != new_node_id) {
            hash.emplace(trans->lhs_node, trans->id);
            trans->rhs_node = new_node_id;
        }
    }

    return hash;
}
