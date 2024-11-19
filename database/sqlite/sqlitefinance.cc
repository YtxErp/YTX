#include "sqlitefinance.h"

#include <QSqlQuery>

#include "component/constvalue.h"

SqliteFinance::SqliteFinance(CInfo& info, QObject* parent)
    : Sqlite(info, parent)
{
}

void SqliteFinance::WriteNodeBind(Node* node, QSqlQuery& query) const
{
    query.bindValue(":name", node->name);
    query.bindValue(":code", node->code);
    query.bindValue(":description", node->description);
    query.bindValue(":note", node->note);
    query.bindValue(":rule", node->rule);
    query.bindValue(":branch", node->branch);
    query.bindValue(":unit", node->unit);
    query.bindValue(":is_helper", node->is_helper);
}

void SqliteFinance::ReadNodeQuery(Node* node, const QSqlQuery& query) const
{
    node->id = query.value("id").toInt();
    node->name = query.value("name").toString();
    node->code = query.value("code").toString();
    node->description = query.value("description").toString();
    node->note = query.value("note").toString();
    node->rule = query.value("rule").toBool();
    node->branch = query.value("branch").toBool();
    node->unit = query.value("unit").toInt();
    node->is_helper = query.value("is_helper").toBool();
    node->initial_total = query.value("initial_total").toDouble();
    node->final_total = query.value("final_total").toDouble();
}

void SqliteFinance::ReadTransQuery(Trans* trans, const QSqlQuery& query) const
{
    trans->lhs_node = query.value("lhs_node").toInt();
    trans->lhs_ratio = query.value("lhs_ratio").toDouble();
    trans->lhs_debit = query.value("lhs_debit").toDouble();
    trans->lhs_credit = query.value("lhs_credit").toDouble();

    trans->rhs_node = query.value("rhs_node").toInt();
    trans->rhs_ratio = query.value("rhs_ratio").toDouble();
    trans->rhs_debit = query.value("rhs_debit").toDouble();
    trans->rhs_credit = query.value("rhs_credit").toDouble();

    trans->code = query.value("code").toString();
    trans->description = query.value("description").toString();
    trans->document = query.value("document").toString().split(SEMICOLON, Qt::SkipEmptyParts);
    trans->date_time = query.value("date_time").toString();
    trans->state = query.value("state").toBool();
    trans->helper_id = query.value("helper_id").toInt();
}

void SqliteFinance::WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query) const
{
    query.bindValue(":date_time", *trans_shadow->date_time);
    query.bindValue(":lhs_node", *trans_shadow->lhs_node);
    query.bindValue(":lhs_ratio", *trans_shadow->lhs_ratio);
    query.bindValue(":lhs_debit", *trans_shadow->lhs_debit);
    query.bindValue(":lhs_credit", *trans_shadow->lhs_credit);
    query.bindValue(":rhs_node", *trans_shadow->rhs_node);
    query.bindValue(":rhs_ratio", *trans_shadow->rhs_ratio);
    query.bindValue(":rhs_debit", *trans_shadow->rhs_debit);
    query.bindValue(":rhs_credit", *trans_shadow->rhs_credit);
    query.bindValue(":state", *trans_shadow->state);
    query.bindValue(":description", *trans_shadow->description);
    query.bindValue(":code", *trans_shadow->code);
    query.bindValue(":document", trans_shadow->document->join(SEMICOLON));
    query.bindValue(":helper_id", *trans_shadow->helper_id);
}

void SqliteFinance::UpdateTransValueBindFPTO(const TransShadow* trans_shadow, QSqlQuery& query) const
{
    query.bindValue(":lhs_node", *trans_shadow->lhs_node);
    query.bindValue(":lhs_ratio", *trans_shadow->lhs_ratio);
    query.bindValue(":lhs_debit", *trans_shadow->lhs_debit);
    query.bindValue(":lhs_credit", *trans_shadow->lhs_credit);
    query.bindValue(":rhs_node", *trans_shadow->rhs_node);
    query.bindValue(":rhs_ratio", *trans_shadow->rhs_ratio);
    query.bindValue(":rhs_debit", *trans_shadow->rhs_debit);
    query.bindValue(":rhs_credit", *trans_shadow->rhs_credit);
    query.bindValue(":trans_id", *trans_shadow->id);
}

QString SqliteFinance::QSReadNode() const
{
    return QStringLiteral(R"(
    SELECT name, id, code, description, note, rule, branch, unit, is_helper, initial_total, final_total
    FROM finance
    WHERE removed = 0
    )");
}

QString SqliteFinance::QSWriteNode() const
{
    return QStringLiteral(R"(
    INSERT INTO finance (name, code, description, note, rule, branch, unit, is_helper)
    VALUES (:name, :code, :description, :note, :rule, :branch, :unit, :is_helper)
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

QString SqliteFinance::QSHelperReferenceFPTS() const
{
    return QStringLiteral(R"(
    SELECT COUNT(*) FROM finance_transaction
    WHERE helper_id = :helper_id AND removed = 0
    )");
}

QString SqliteFinance::QSReplaceHelperTransFPTS() const
{
    return QStringLiteral(R"(
    UPDATE finance_transaction SET
        helper_id = :new_node_id
    WHERE helper_id = :old_node_id AND removed = 0
    )");
}

QString SqliteFinance::QSRemoveHelperFPTS() const
{
    return QStringLiteral(R"(
    UPDATE finance_transaction SET
        helper_id = 0
    WHERE helper_id = :node_id AND removed = 0
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

QString SqliteFinance::QSHelperTransToMoveFPTS() const
{
    return QStringLiteral(R"(
    SELECT id FROM finance_transaction
    WHERE helper_id = :helper_id AND removed = 0
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

QString SqliteFinance::QSHelperTransToRemoveFPTS() const
{
    return QStringLiteral(R"(
    SELECT helper_id, id FROM finance_transaction
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )");
}

QString SqliteFinance::QSReadNodeTrans() const
{
    return QStringLiteral(R"(
    SELECT id, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, helper_id, code, document, date_time
    FROM finance_transaction
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )");
}

QString SqliteFinance::QSReadHelperTransFPTS() const
{
    return QStringLiteral(R"(
    SELECT id, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, helper_id, code, document, date_time
    FROM finance_transaction
    WHERE helper_id = :node_id AND removed = 0
    )");
}

QString SqliteFinance::QSWriteNodeTrans() const
{
    return QStringLiteral(R"(
    INSERT INTO finance_transaction
    (date_time, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, helper_id, code, document)
    VALUES
    (:date_time, :lhs_node, :lhs_ratio, :lhs_debit, :lhs_credit, :rhs_node, :rhs_ratio, :rhs_debit, :rhs_credit, :state, :description, :helper_id, :code, :document)
    )");
}

QString SqliteFinance::QSReadTransRangeFPTS(CString& in_list) const
{
    return QString(R"(
    SELECT id, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, helper_id, code, document, date_time
    FROM finance_transaction
    WHERE id IN (%1) AND removed = 0
    )")
        .arg(in_list);
}

QString SqliteFinance::QSReadHelperTransRangeFPTS(CString& in_list) const
{
    return QString(R"(
    SELECT id, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, helper_id, code, document, date_time
    FROM finance_transaction
    WHERE helper_id IN (%1) AND removed = 0
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

QString SqliteFinance::QSUpdateTransValueFPTO() const
{
    return QStringLiteral(R"(
    UPDATE finance_transaction SET
        lhs_node = :lhs_node, lhs_ratio = :lhs_ratio, lhs_debit = :lhs_debit, lhs_credit = :lhs_credit,
        rhs_node = :rhs_node, rhs_ratio = :rhs_ratio, rhs_debit = :rhs_debit, rhs_credit = :rhs_credit
    WHERE id = :trans_id
    )");
}

QString SqliteFinance::QSUpdateNodeValueFPTO() const
{
    return QStringLiteral(R"(
    UPDATE finance SET
        initial_total = :initial_total, final_total = :final_total
    WHERE id = :node_id
    )");
}

void SqliteFinance::UpdateNodeValueBindFPTO(const Node* node, QSqlQuery& query) const
{
    query.bindValue(":initial_total", node->initial_total);
    query.bindValue(":final_total", node->final_total);
    query.bindValue(":node_id", node->id);
}

QString SqliteFinance::QSSearchTrans() const
{
    return QStringLiteral(R"(
    SELECT id, lhs_node, lhs_ratio, lhs_debit, lhs_credit, rhs_node, rhs_ratio, rhs_debit, rhs_credit, state, description, helper_id, code, document, date_time
    FROM finance_transaction
    WHERE (lhs_debit = :text OR lhs_credit = :text OR rhs_debit = :text OR rhs_credit = :text OR description LIKE :description) AND removed = 0
    ORDER BY date_time
    )");
}
