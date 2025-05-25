#include "sqlf.h"

#include <QSqlQuery>

#include "component/constvalue.h"

SqlF::SqlF(QSqlDatabase& main_db, CInfo& info, QObject* parent)
    : Sql(main_db, info, parent)
{
}

void SqlF::WriteNodeBind(Node* node, QSqlQuery& query) const
{
    query.bindValue(QStringLiteral(":name"), node->name);
    query.bindValue(QStringLiteral(":code"), node->code);
    query.bindValue(QStringLiteral(":description"), node->description);
    query.bindValue(QStringLiteral(":note"), node->note);
    query.bindValue(QStringLiteral(":direction_rule"), node->direction_rule);
    query.bindValue(QStringLiteral(":node_type"), node->node_type);
    query.bindValue(QStringLiteral(":unit"), node->unit);
}

void SqlF::ReadNodeQuery(Node* node, const QSqlQuery& query) const
{
    node->id = query.value(QStringLiteral("id")).toInt();
    node->name = query.value(QStringLiteral("name")).toString();
    node->code = query.value(QStringLiteral("code")).toString();
    node->description = query.value(QStringLiteral("description")).toString();
    node->note = query.value(QStringLiteral("note")).toString();
    node->direction_rule = query.value(QStringLiteral("direction_rule")).toBool();
    node->node_type = query.value(QStringLiteral("node_type")).toInt();
    node->unit = query.value(QStringLiteral("unit")).toInt();
    node->initial_total = query.value(QStringLiteral("foreign_total")).toDouble();
    node->final_total = query.value(QStringLiteral("local_total")).toDouble();
}

void SqlF::ReadTransQuery(Trans* trans, const QSqlQuery& query) const
{
    trans->lhs_node = query.value(QStringLiteral("lhs_node")).toInt();
    trans->lhs_ratio = query.value(QStringLiteral("lhs_ratio")).toDouble();
    trans->lhs_debit = query.value(QStringLiteral("lhs_debit")).toDouble();
    trans->lhs_credit = query.value(QStringLiteral("lhs_credit")).toDouble();

    trans->rhs_node = query.value(QStringLiteral("rhs_node")).toInt();
    trans->rhs_ratio = query.value(QStringLiteral("rhs_ratio")).toDouble();
    trans->rhs_debit = query.value(QStringLiteral("rhs_debit")).toDouble();
    trans->rhs_credit = query.value(QStringLiteral("rhs_credit")).toDouble();

    trans->code = query.value(QStringLiteral("code")).toString();
    trans->description = query.value(QStringLiteral("description")).toString();
    trans->document = query.value(QStringLiteral("document")).toString().split(kSemicolon, Qt::SkipEmptyParts);
    trans->date_time = query.value(QStringLiteral("date_time")).toString();
    trans->state = query.value(QStringLiteral("state")).toBool();
    trans->support_id = query.value(QStringLiteral("support_id")).toInt();
}

void SqlF::WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query) const
{
    query.bindValue(QStringLiteral(":date_time"), *trans_shadow->date_time);
    query.bindValue(QStringLiteral(":lhs_node"), *trans_shadow->lhs_node);
    query.bindValue(QStringLiteral(":lhs_ratio"), *trans_shadow->lhs_ratio);
    query.bindValue(QStringLiteral(":lhs_debit"), *trans_shadow->lhs_debit);
    query.bindValue(QStringLiteral(":lhs_credit"), *trans_shadow->lhs_credit);
    query.bindValue(QStringLiteral(":rhs_node"), *trans_shadow->rhs_node);
    query.bindValue(QStringLiteral(":rhs_ratio"), *trans_shadow->rhs_ratio);
    query.bindValue(QStringLiteral(":rhs_debit"), *trans_shadow->rhs_debit);
    query.bindValue(QStringLiteral(":rhs_credit"), *trans_shadow->rhs_credit);
    query.bindValue(QStringLiteral(":state"), *trans_shadow->state);
    query.bindValue(QStringLiteral(":description"), *trans_shadow->description);
    query.bindValue(QStringLiteral(":code"), *trans_shadow->code);
    query.bindValue(QStringLiteral(":document"), trans_shadow->document->join(kSemicolon));
    query.bindValue(QStringLiteral(":support_id"), *trans_shadow->support_id);
}

void SqlF::UpdateTransValueBind(const TransShadow* trans_shadow, QSqlQuery& query) const
{
    query.bindValue(QStringLiteral(":lhs_node"), *trans_shadow->lhs_node);
    query.bindValue(QStringLiteral(":lhs_ratio"), *trans_shadow->lhs_ratio);
    query.bindValue(QStringLiteral(":lhs_debit"), *trans_shadow->lhs_debit);
    query.bindValue(QStringLiteral(":lhs_credit"), *trans_shadow->lhs_credit);
    query.bindValue(QStringLiteral(":rhs_node"), *trans_shadow->rhs_node);
    query.bindValue(QStringLiteral(":rhs_ratio"), *trans_shadow->rhs_ratio);
    query.bindValue(QStringLiteral(":rhs_debit"), *trans_shadow->rhs_debit);
    query.bindValue(QStringLiteral(":rhs_credit"), *trans_shadow->rhs_credit);
    query.bindValue(QStringLiteral(":trans_id"), *trans_shadow->id);
}

QString SqlF::QSReadNode() const
{
    return QStringLiteral(R"(
    SELECT name, id, code, description, note, rule, type, unit, foreign_total, local_total
    FROM finance
    WHERE removed = false
    )");
}

QString SqlF::QSWriteNode() const
{
    return QStringLiteral(R"(
    INSERT INTO finance (name, code, description, note, rule, type, unit)
    VALUES (:name, :code, :description, :note, :rule, :type, :unit)
    )");
}

QString SqlF::QSRemoveNodeSecond() const
{
    return QStringLiteral(R"(
    UPDATE finance_transaction SET
        removed = true
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )");
}

QString SqlF::QSInternalReference() const
{
    return QStringLiteral(R"(
    SELECT EXISTS(
        SELECT 1 FROM finance_transaction
        WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    ) AS is_referenced
    )");
}

QString SqlF::QSSupportReference() const
{
    return QStringLiteral(R"(
    SELECT 1 FROM finance_transaction
    WHERE support_id = :support_id AND removed = 0
    LIMIT 1
    )");
}

QString SqlF::QSReplaceSupport() const
{
    return QStringLiteral(R"(
    UPDATE finance_transaction SET
        support_id = :new_node_id
    WHERE support_id = :old_node_id AND removed = 0
    )");
}

QString SqlF::QSRemoveSupport() const
{
    return QStringLiteral(R"(
    UPDATE finance_transaction SET
        support_id = 0
    WHERE support_id = :node_id AND removed = 0
    )");
}

QString SqlF::QSLeafTotal(int /*unit*/) const
{
    return QStringLiteral(R"(
    WITH node_balance AS (
        SELECT
            lhs_debit AS initial_debit,
            lhs_credit AS initial_credit,
            lhs_ratio * lhs_debit AS final_debit,
            lhs_ratio * lhs_credit AS final_credit
        FROM finance_transaction
        WHERE lhs_node = :node_id AND removed = 0

        UNION ALL

        SELECT
            rhs_debit,
            rhs_credit,
            rhs_ratio * rhs_debit,
            rhs_ratio * rhs_credit
        FROM finance_transaction
        WHERE rhs_node = :node_id AND removed = 0
    )
    SELECT
        SUM(initial_credit) - SUM(initial_debit) AS initial_balance,
        SUM(final_credit) - SUM(final_debit) AS final_balance
    FROM node_balance;
    )");
}

QString SqlF::QSTransToRemove() const
{
    return QStringLiteral(R"(
    SELECT rhs_node AS node_id, id AS trans_id, support_id FROM finance_transaction
    WHERE lhs_node = :node_id AND removed = 0
    UNION ALL
    SELECT lhs_node AS node_id, id AS trans_id, support_id FROM finance_transaction
    WHERE rhs_node = :node_id AND removed = 0
    )");
}

QString SqlF::QSReadTrans() const
{
    return QStringLiteral(R"(
    SELECT id, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, support_id, code, document, date_time
    FROM finance_transaction
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )");
}

QString SqlF::QSReadSupportTrans() const
{
    return QStringLiteral(R"(
    SELECT id, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, support_id, code, document, date_time
    FROM finance_transaction
    WHERE support_id = :node_id AND removed = 0
    )");
}

QString SqlF::QSWriteTrans() const
{
    return QStringLiteral(R"(
    INSERT INTO finance_transaction
    (date_time, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, support_id, code, document)
    VALUES
    (:date_time, :lhs_node, :lhs_ratio, :lhs_debit, :lhs_credit, :rhs_node, :rhs_ratio, :rhs_debit, :rhs_credit, :state, :description, :support_id, :code, :document)
    )");
}

QString SqlF::QSReplaceLeaf() const
{
    return QStringLiteral(R"(
    UPDATE finance_transaction SET
        lhs_node = CASE WHEN lhs_node = :old_node_id AND rhs_node != :new_node_id THEN :new_node_id ELSE lhs_node END,
        rhs_node = CASE WHEN rhs_node = :old_node_id AND lhs_node != :new_node_id THEN :new_node_id ELSE rhs_node END
    WHERE lhs_node = :old_node_id OR rhs_node = :old_node_id;
    )");
}

QString SqlF::QSUpdateTransValue() const
{
    return QStringLiteral(R"(
    UPDATE finance_transaction SET
        lhs_node = :lhs_node, lhs_ratio = :lhs_ratio, lhs_debit = :lhs_debit, lhs_credit = :lhs_credit,
        rhs_node = :rhs_node, rhs_ratio = :rhs_ratio, rhs_debit = :rhs_debit, rhs_credit = :rhs_credit
    WHERE id = :trans_id
    )");
}

QString SqlF::QSUpdateLeafValue() const
{
    return QStringLiteral(R"(
    UPDATE finance SET
        foreign_total = :foreign_total, local_total = :local_total
    WHERE id = :node_id
    )");
}

void SqlF::UpdateLeafValueBind(const Node* node, QSqlQuery& query) const
{
    query.bindValue(":foreign_total", node->initial_total);
    query.bindValue(":local_total", node->final_total);
    query.bindValue(":node_id", node->id);
}

QString SqlF::QSSearchTransValue() const
{
    return QStringLiteral(R"(
    SELECT id, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, support_id, code, document, date_time
    FROM finance_transaction
    WHERE ((lhs_debit BETWEEN :value - :tolerance AND :value + :tolerance)
        OR (lhs_credit BETWEEN :value - :tolerance AND :value + :tolerance)
        OR (rhs_debit BETWEEN :value - :tolerance AND :value + :tolerance)
        OR (rhs_credit BETWEEN :value - :tolerance AND :value + :tolerance))
        AND removed = 0
    )");
}

QString SqlF::QSSearchTransText() const
{
    return QStringLiteral(R"(
    SELECT id, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, support_id, code, document, date_time
    FROM finance_transaction
    WHERE description LIKE :description AND removed = 0
    )");
}
