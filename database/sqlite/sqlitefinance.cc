#include "sqlitefinance.h"

#include <QSqlQuery>

#include "component/constvalue.h"

SqliteFinance::SqliteFinance(CInfo& info, QObject* parent)
    : Sqlite(info, parent)
{
}

void SqliteFinance::WriteNodeBind(Node* node, QSqlQuery& query) const
{
    query.bindValue(QStringLiteral(":name"), node->name);
    query.bindValue(QStringLiteral(":code"), node->code);
    query.bindValue(QStringLiteral(":description"), node->description);
    query.bindValue(QStringLiteral(":note"), node->note);
    query.bindValue(QStringLiteral(":rule"), node->rule);
    query.bindValue(QStringLiteral(":type"), node->type);
    query.bindValue(QStringLiteral(":unit"), node->unit);
}

void SqliteFinance::ReadNodeQuery(Node* node, const QSqlQuery& query) const
{
    node->id = query.value(QStringLiteral("id")).toInt();
    node->name = query.value(QStringLiteral("name")).toString();
    node->code = query.value(QStringLiteral("code")).toString();
    node->description = query.value(QStringLiteral("description")).toString();
    node->note = query.value(QStringLiteral("note")).toString();
    node->rule = query.value(QStringLiteral("rule")).toBool();
    node->type = query.value(QStringLiteral("type")).toInt();
    node->unit = query.value(QStringLiteral("unit")).toInt();
    node->initial_total = query.value(QStringLiteral("foreign_total")).toDouble();
    node->final_total = query.value(QStringLiteral("local_total")).toDouble();
}

void SqliteFinance::ReadTransQuery(Trans* trans, const QSqlQuery& query) const
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

void SqliteFinance::WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query) const
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

void SqliteFinance::WriteTransValueBindFPTO(const TransShadow* trans_shadow, QSqlQuery& query) const
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

QString SqliteFinance::QSReadNode() const
{
    return QStringLiteral(R"(
    SELECT name, id, code, description, note, rule, type, unit, foreign_total, local_total
    FROM finance
    WHERE removed = 0
    )");
}

QString SqliteFinance::QSWriteNode() const
{
    return QStringLiteral(R"(
    INSERT INTO finance (name, code, description, note, rule, type, unit)
    VALUES (:name, :code, :description, :note, :rule, :type, :unit)
    )");
}

QString SqliteFinance::QSRemoveNodeSecond() const
{
    return QStringLiteral(R"(
    UPDATE finance_transaction SET
        removed = 1
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )");
}

QString SqliteFinance::QSInternalReference() const
{
    return QStringLiteral(R"(
    SELECT COUNT(*) FROM finance_transaction
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )");
}

QString SqliteFinance::QSSupportReferenceFPTS() const
{
    return QStringLiteral(R"(
    SELECT COUNT(*) FROM finance_transaction
    WHERE support_id = :support_id AND removed = 0
    )");
}

QString SqliteFinance::QSReplaceSupportTransFPTS() const
{
    return QStringLiteral(R"(
    UPDATE finance_transaction SET
        support_id = :new_node_id
    WHERE support_id = :old_node_id AND removed = 0
    )");
}

QString SqliteFinance::QSRemoveSupportFPTS() const
{
    return QStringLiteral(R"(
    UPDATE finance_transaction SET
        support_id = 0
    WHERE support_id = :node_id AND removed = 0
    )");
}

QString SqliteFinance::QSFreeViewFPT() const
{
    return QStringLiteral(R"(
    SELECT COUNT(*) FROM finance_transaction
    WHERE ((lhs_node = :old_node_id AND rhs_node = :new_node_id) OR (rhs_node = :old_node_id AND lhs_node = :new_node_id)) AND removed = 0
    )");
}

QString SqliteFinance::QSLeafTotalFPT() const
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

QString SqliteFinance::QSSupportTransToMoveFPTS() const
{
    return QStringLiteral(R"(
    SELECT id FROM finance_transaction
    WHERE support_id = :support_id AND removed = 0
    )");
}

QString SqliteFinance::QSNodeTransToRemove() const
{
    return QStringLiteral(R"(
    SELECT rhs_node, id FROM finance_transaction
    WHERE lhs_node = :node_id AND removed = 0
    UNION ALL
    SELECT lhs_node, id FROM finance_transaction
    WHERE rhs_node = :node_id AND removed = 0
    )");
}

QString SqliteFinance::QSSupportTransToRemoveFPTS() const
{
    return QStringLiteral(R"(
    SELECT support_id, id FROM finance_transaction
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )");
}

QString SqliteFinance::QSReadNodeTrans() const
{
    return QStringLiteral(R"(
    SELECT id, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, support_id, code, document, date_time
    FROM finance_transaction
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )");
}

QString SqliteFinance::QSReadSupportTransFPTS() const
{
    return QStringLiteral(R"(
    SELECT id, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, support_id, code, document, date_time
    FROM finance_transaction
    WHERE support_id = :node_id AND removed = 0
    )");
}

QString SqliteFinance::QSWriteNodeTrans() const
{
    return QStringLiteral(R"(
    INSERT INTO finance_transaction
    (date_time, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, support_id, code, document)
    VALUES
    (:date_time, :lhs_node, :lhs_ratio, :lhs_debit, :lhs_credit, :rhs_node, :rhs_ratio, :rhs_debit, :rhs_credit, :state, :description, :support_id, :code, :document)
    )");
}

QString SqliteFinance::QSReadTransRangeFPTS(CString& in_list) const
{
    return QStringLiteral(R"(
    SELECT id, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, support_id, code, document, date_time
    FROM finance_transaction
    WHERE id IN (%1) AND removed = 0
    )")
        .arg(in_list);
}

QString SqliteFinance::QSReplaceNodeTransFPTS() const
{
    return QStringLiteral(R"(
    UPDATE finance_transaction SET
        lhs_node = CASE WHEN lhs_node = :old_node_id AND rhs_node != :new_node_id THEN :new_node_id ELSE lhs_node END,
        rhs_node = CASE WHEN rhs_node = :old_node_id AND lhs_node != :new_node_id THEN :new_node_id ELSE rhs_node END
    WHERE lhs_node = :old_node_id OR rhs_node = :old_node_id;
    )");
}

QString SqliteFinance::QSWriteTransValueFPTO() const
{
    return QStringLiteral(R"(
    UPDATE finance_transaction SET
        lhs_node = :lhs_node, lhs_ratio = :lhs_ratio, lhs_debit = :lhs_debit, lhs_credit = :lhs_credit,
        rhs_node = :rhs_node, rhs_ratio = :rhs_ratio, rhs_debit = :rhs_debit, rhs_credit = :rhs_credit
    WHERE id = :trans_id
    )");
}

QString SqliteFinance::QSWriteLeafValueFPTO() const
{
    return QStringLiteral(R"(
    UPDATE finance SET
        foreign_total = :foreign_total, local_total = :local_total
    WHERE id = :node_id
    )");
}

void SqliteFinance::WriteLeafValueBindFPTO(const Node* node, QSqlQuery& query) const
{
    query.bindValue(":foreign_total", node->initial_total);
    query.bindValue(":local_total", node->final_total);
    query.bindValue(":node_id", node->id);
}

QString SqliteFinance::QSSearchTrans() const
{
    return QStringLiteral(R"(
    SELECT id, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, support_id, code, document, date_time
    FROM finance_transaction
    WHERE (lhs_debit = :text OR lhs_credit = :text OR rhs_debit = :text OR rhs_credit = :text OR description LIKE :description) AND removed = 0
    ORDER BY date_time
    )");
}
