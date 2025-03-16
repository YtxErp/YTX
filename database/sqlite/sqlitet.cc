#include "sqlitet.h"

#include <QSqlQuery>

#include "component/constvalue.h"

SqliteT::SqliteT(CInfo& info, QObject* parent)
    : Sqlite(info, parent)
{
}

QString SqliteT::QSReadNode() const
{
    return QStringLiteral(R"(
    SELECT name, id, code, description, note, rule, type, unit, color, document, date_time, finished, unit_cost, quantity, amount
    FROM task
    WHERE removed = 0
    )");
}

QString SqliteT::QSWriteNode() const
{
    return QStringLiteral(R"(
    INSERT INTO task (name, code, description, note, rule, type, unit, color, document, date_time, finished, unit_cost)
    VALUES (:name, :code, :description, :note, :rule, :type, :unit, :color, :document, :date_time, :finished, :unit_cost)
    )");
}

QString SqliteT::QSRemoveNodeSecond() const
{
    return QStringLiteral(R"(
    UPDATE task_transaction SET
        removed = 1
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )");
}

QString SqliteT::QSInternalReference() const
{
    return QStringLiteral(R"(
    SELECT COUNT(*) FROM task_transaction
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )");
}

QString SqliteT::QSSupportReference() const
{
    return QStringLiteral(R"(
    SELECT COUNT(*) FROM task_transaction
    WHERE support_id = :support_id AND removed = 0
    )");
}

QString SqliteT::QSReplaceSupport() const
{
    return QStringLiteral(R"(
    UPDATE task_transaction SET
        support_id = :new_node_id
    WHERE support_id = :old_node_id AND removed = 0
    )");
}

QString SqliteT::QSRemoveSupport() const
{
    return QStringLiteral(R"(
    UPDATE task_transaction SET
        support_id = 0
    WHERE support_id = :node_id AND removed = 0
    )");
}

QString SqliteT::QSLeafTotal(int /*unit*/) const
{
    return QStringLiteral(R"(
    WITH node_balance AS (
        SELECT
            lhs_debit AS initial_debit,
            lhs_credit AS initial_credit,
            unit_cost * lhs_debit AS final_debit,
            unit_cost * lhs_credit AS final_credit
        FROM task_transaction
        WHERE lhs_node = :node_id AND removed = 0

        UNION ALL

        SELECT
            rhs_debit,
            rhs_credit,
            unit_cost * rhs_debit,
            unit_cost * rhs_credit
        FROM task_transaction
        WHERE rhs_node = :node_id AND removed = 0
    )
    SELECT
        SUM(initial_credit) - SUM(initial_debit) AS initial_balance,
        SUM(final_credit) - SUM(final_debit) AS final_balance
    FROM node_balance;
    )");
}

QString SqliteT::QSTransToRemove() const
{
    return QStringLiteral(R"(
    SELECT rhs_node AS node_id, id AS trans_id, support_id FROM task_transaction
    WHERE lhs_node = :node_id AND removed = 0
    UNION ALL
    SELECT lhs_node AS node_id, id AS trans_id, support_id FROM task_transaction
    WHERE rhs_node = :node_id AND removed = 0
    )");
}

QString SqliteT::QSReadTrans() const
{
    return QStringLiteral(R"(
    SELECT id, lhs_node, unit_cost, lhs_debit, lhs_credit, rhs_node, rhs_debit, rhs_credit, state, description, support_id, code, document, date_time
    FROM task_transaction
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )");
}

QString SqliteT::QSReadSupportTrans() const
{
    return QStringLiteral(R"(
    SELECT id, lhs_node, unit_cost, lhs_debit, lhs_credit, rhs_node, rhs_debit, rhs_credit, state, description, support_id, code, document, date_time
    FROM task_transaction
    WHERE support_id = :node_id AND removed = 0
    )");
}

void SqliteT::ReadTransQuery(Trans* trans, const QSqlQuery& query) const
{
    trans->lhs_node = query.value(QStringLiteral("lhs_node")).toInt();
    trans->lhs_debit = query.value(QStringLiteral("lhs_debit")).toDouble();
    trans->lhs_credit = query.value(QStringLiteral("lhs_credit")).toDouble();

    trans->rhs_node = query.value(QStringLiteral("rhs_node")).toInt();
    trans->rhs_debit = query.value(QStringLiteral("rhs_debit")).toDouble();
    trans->rhs_credit = query.value(QStringLiteral("rhs_credit")).toDouble();

    trans->lhs_ratio = query.value(QStringLiteral("unit_cost")).toDouble();
    trans->rhs_ratio = trans->lhs_ratio;
    trans->code = query.value(QStringLiteral("code")).toString();
    trans->description = query.value(QStringLiteral("description")).toString();
    trans->document = query.value(QStringLiteral("document")).toString().split(kSemicolon, Qt::SkipEmptyParts);
    trans->date_time = query.value(QStringLiteral("date_time")).toString();
    trans->state = query.value(QStringLiteral("state")).toBool();
    trans->support_id = query.value(QStringLiteral("support_id")).toInt();
}

QString SqliteT::QSWriteTrans() const
{
    return QStringLiteral(R"(
    INSERT INTO task_transaction
    (date_time, lhs_node, unit_cost, lhs_debit, lhs_credit, rhs_node, rhs_debit, rhs_credit, state, description, support_id, code, document)
    VALUES
    (:date_time, :lhs_node, :unit_cost, :lhs_debit, :lhs_credit, :rhs_node, :rhs_debit, :rhs_credit, :state, :description, :support_id, :code, :document)
    )");
}

QString SqliteT::QSReplaceLeaf() const
{
    return QStringLiteral(R"(
    UPDATE task_transaction SET
        lhs_node = CASE WHEN lhs_node = :old_node_id AND rhs_node != :new_node_id THEN :new_node_id ELSE lhs_node END,
        rhs_node = CASE WHEN rhs_node = :old_node_id AND lhs_node != :new_node_id THEN :new_node_id ELSE rhs_node END
    WHERE lhs_node = :old_node_id OR rhs_node = :old_node_id;
    )");
}

QString SqliteT::QSSyncTransValue() const
{
    return QStringLiteral(R"(
    UPDATE task_transaction SET
        lhs_node = :lhs_node, lhs_debit = :lhs_debit, lhs_credit = :lhs_credit,
        rhs_node = :rhs_node, rhs_debit = :rhs_debit, rhs_credit = :rhs_credit
    WHERE id = :trans_id
    )");
}

void SqliteT::WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query) const
{
    query.bindValue(QStringLiteral(":date_time"), *trans_shadow->date_time);
    query.bindValue(QStringLiteral(":unit_cost"), *trans_shadow->lhs_ratio);
    query.bindValue(QStringLiteral(":state"), *trans_shadow->state);
    query.bindValue(QStringLiteral(":description"), *trans_shadow->description);
    query.bindValue(QStringLiteral(":code"), *trans_shadow->code);
    query.bindValue(QStringLiteral(":document"), trans_shadow->document->join(kSemicolon));
    query.bindValue(QStringLiteral(":support_id"), *trans_shadow->support_id);

    query.bindValue(QStringLiteral(":lhs_node"), *trans_shadow->lhs_node);
    query.bindValue(QStringLiteral(":lhs_debit"), *trans_shadow->lhs_debit);
    query.bindValue(QStringLiteral(":lhs_credit"), *trans_shadow->lhs_credit);

    query.bindValue(QStringLiteral(":rhs_node"), *trans_shadow->rhs_node);
    query.bindValue(QStringLiteral(":rhs_debit"), *trans_shadow->rhs_debit);
    query.bindValue(QStringLiteral(":rhs_credit"), *trans_shadow->rhs_credit);
}

void SqliteT::SyncTransValueBind(const TransShadow* trans_shadow, QSqlQuery& query) const
{
    query.bindValue(QStringLiteral(":lhs_node"), *trans_shadow->lhs_node);
    query.bindValue(QStringLiteral(":lhs_debit"), *trans_shadow->lhs_debit);
    query.bindValue(QStringLiteral(":lhs_credit"), *trans_shadow->lhs_credit);
    query.bindValue(QStringLiteral(":rhs_node"), *trans_shadow->rhs_node);
    query.bindValue(QStringLiteral(":rhs_debit"), *trans_shadow->rhs_debit);
    query.bindValue(QStringLiteral(":rhs_credit"), *trans_shadow->rhs_credit);
    query.bindValue(QStringLiteral(":trans_id"), *trans_shadow->id);
}

void SqliteT::WriteNodeBind(Node* node, QSqlQuery& query) const
{
    query.bindValue(QStringLiteral(":name"), node->name);
    query.bindValue(QStringLiteral(":code"), node->code);
    query.bindValue(QStringLiteral(":description"), node->description);
    query.bindValue(QStringLiteral(":note"), node->note);
    query.bindValue(QStringLiteral(":rule"), node->rule);
    query.bindValue(QStringLiteral(":type"), node->type);
    query.bindValue(QStringLiteral(":unit"), node->unit);
    query.bindValue(QStringLiteral(":color"), node->color);
    query.bindValue(QStringLiteral(":date_time"), node->date_time);
    query.bindValue(QStringLiteral(":unit_cost"), node->first);
    query.bindValue(QStringLiteral(":finished"), node->finished);
    query.bindValue(QStringLiteral(":document"), node->document.join(kSemicolon));
}

void SqliteT::ReadNodeQuery(Node* node, const QSqlQuery& query) const
{
    node->id = query.value(QStringLiteral("id")).toInt();
    node->name = query.value(QStringLiteral("name")).toString();
    node->code = query.value(QStringLiteral("code")).toString();
    node->description = query.value(QStringLiteral("description")).toString();
    node->note = query.value(QStringLiteral("note")).toString();
    node->rule = query.value(QStringLiteral("rule")).toBool();
    node->type = query.value(QStringLiteral("type")).toInt();
    node->unit = query.value(QStringLiteral("unit")).toInt();
    node->initial_total = query.value(QStringLiteral("quantity")).toDouble();
    node->final_total = query.value(QStringLiteral("amount")).toDouble();
    node->color = query.value(QStringLiteral("color")).toString();
    node->first = query.value(QStringLiteral("unit_cost")).toDouble();
    node->date_time = query.value(QStringLiteral("date_time")).toString();
    node->finished = query.value(QStringLiteral("finished")).toBool();
    node->document = query.value(QStringLiteral("document")).toString().split(kSemicolon, Qt::SkipEmptyParts);
}

QString SqliteT::QSSyncLeafValue() const
{
    return QStringLiteral(R"(
    UPDATE task SET
        quantity = :quantity, amount = :amount
    WHERE id = :node_id
    )");
}

void SqliteT::SyncLeafValueBind(const Node* node, QSqlQuery& query) const
{
    query.bindValue(QStringLiteral(":quantity"), node->initial_total);
    query.bindValue(QStringLiteral(":amount"), node->final_total);
    query.bindValue(QStringLiteral(":node_id"), node->id);
}

QString SqliteT::QSSearchTransValue() const
{
    return QStringLiteral(R"(
    SELECT id, lhs_node, unit_cost, lhs_debit, lhs_credit, rhs_node, rhs_debit, rhs_credit, state, description, support_id, code, document, date_time
    FROM task_transaction
    WHERE ((lhs_debit BETWEEN :value - :tolerance AND :value + :tolerance)
        OR (lhs_credit BETWEEN :value - :tolerance AND :value + :tolerance)
        OR (rhs_debit BETWEEN :value - :tolerance AND :value + :tolerance)
        OR (rhs_credit BETWEEN :value - :tolerance AND :value + :tolerance))
        AND removed = 0
    )");
}

QString SqliteT::QSSearchTransText() const
{
    return QStringLiteral(R"(
    SELECT id, lhs_node, unit_cost, lhs_debit, lhs_credit, rhs_node, rhs_debit, rhs_credit, state, description, support_id, code, document, date_time
    FROM task_transaction
    WHERE description LIKE :description AND removed = 0
    )");
}
