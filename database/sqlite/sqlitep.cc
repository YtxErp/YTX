#include "sqlitep.h"

#include <QSqlError>
#include <QSqlQuery>

#include "component/constvalue.h"
#include "global/resourcepool.h"

SqliteP::SqliteP(CInfo& info, QObject* parent)
    : Sqlite(info, parent)
{
}

QString SqliteP::QSReadNode() const
{
    return QStringLiteral(R"(
    SELECT name, id, code, description, note, rule, type, unit, color, commission, unit_price, quantity, amount
    FROM product
    WHERE removed = 0
    )");
}

QString SqliteP::QSWriteNode() const
{
    return QStringLiteral(R"(
    INSERT INTO product (name, code, description, note, rule, type, unit, color, commission, unit_price)
    VALUES (:name, :code, :description, :note, :rule, :type, :unit, :color, :commission, :unit_price)
    )");
}

QString SqliteP::QSRemoveNodeSecond() const
{
    return QStringLiteral(R"(
    UPDATE product_transaction SET
        removed = 1
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )");
}

QString SqliteP::QSInternalReference() const
{
    return QStringLiteral(R"(
    SELECT EXISTS(
        SELECT 1 FROM product_transaction
        WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    ) AS is_referenced
    )");
}

QString SqliteP::QSExternalReference() const
{
    return QStringLiteral(R"(
   SELECT
        EXISTS(SELECT 1 FROM stakeholder_transaction WHERE inside_product = :node_id) OR
        EXISTS(SELECT 1 FROM sales_transaction WHERE inside_product = :node_id AND removed = 0) OR
        EXISTS(SELECT 1 FROM purchase_transaction WHERE inside_product = :node_id AND removed = 0)
    AS is_referenced;
    )");
}

QString SqliteP::QSSupportReference() const
{
    return QStringLiteral(R"(
    SELECT 1 FROM product_transaction
    WHERE support_id = :support_id AND removed = 0
    LIMIT 1
    )");
}

QString SqliteP::QSReplaceSupport() const
{
    return QStringLiteral(R"(
    UPDATE product_transaction SET
        support_id = :new_node_id
    WHERE support_id = :old_node_id AND removed = 0
    )");
}

QString SqliteP::QSRemoveSupport() const
{
    return QStringLiteral(R"(
    UPDATE product_transaction SET
        support_id = 0
    WHERE support_id = :node_id AND removed = 0
    )");
}

QString SqliteP::QSTransToRemove() const
{
    return QStringLiteral(R"(
    SELECT rhs_node AS node_id, id AS trans_id, support_id FROM product_transaction
    WHERE lhs_node = :node_id AND removed = 0
    UNION ALL
    SELECT lhs_nod AS node_id, id AS trans_id, support_id FROM product_transaction
    WHERE rhs_node = :node_id AND removed = 0
    )");
}

QString SqliteP::QSLeafTotal(int /*unit*/) const
{
    return QStringLiteral(R"(
    WITH node_balance AS (
        SELECT
            lhs_debit AS initial_debit,
            lhs_credit AS initial_credit,
            unit_cost * lhs_debit AS final_debit,
            unit_cost * lhs_credit AS final_credit
        FROM product_transaction
        WHERE lhs_node = :node_id AND removed = 0

        UNION ALL

        SELECT
            rhs_debit,
            rhs_credit,
            unit_cost * rhs_debit,
            unit_cost * rhs_credit
        FROM product_transaction
        WHERE rhs_node = :node_id AND removed = 0
    )
    SELECT
        SUM(initial_credit) - SUM(initial_debit) AS initial_balance,
        SUM(final_credit) - SUM(final_debit) AS final_balance
    FROM node_balance;
    )");
}

void SqliteP::WriteNodeBind(Node* node, QSqlQuery& query) const
{
    query.bindValue(QStringLiteral(":name"), node->name);
    query.bindValue(QStringLiteral(":code"), node->code);
    query.bindValue(QStringLiteral(":description"), node->description);
    query.bindValue(QStringLiteral(":note"), node->note);
    query.bindValue(QStringLiteral(":rule"), node->rule);
    query.bindValue(QStringLiteral(":type"), node->type);
    query.bindValue(QStringLiteral(":unit"), node->unit);
    query.bindValue(QStringLiteral(":color"), node->color);
    query.bindValue(QStringLiteral(":commission"), node->second);
    query.bindValue(QStringLiteral(":unit_price"), node->first);
}

void SqliteP::ReadNodeQuery(Node* node, const QSqlQuery& query) const
{
    node->id = query.value(QStringLiteral("id")).toInt();
    node->name = query.value(QStringLiteral("name")).toString();
    node->code = query.value(QStringLiteral("code")).toString();
    node->description = query.value(QStringLiteral("description")).toString();
    node->note = query.value(QStringLiteral("note")).toString();
    node->rule = query.value(QStringLiteral("rule")).toBool();
    node->type = query.value(QStringLiteral("type")).toInt();
    node->unit = query.value(QStringLiteral("unit")).toInt();
    node->color = query.value(QStringLiteral("color")).toString();
    node->second = query.value(QStringLiteral("commission")).toDouble();
    node->first = query.value(QStringLiteral("unit_price")).toDouble();
    node->initial_total = query.value(QStringLiteral("quantity")).toDouble();
    node->final_total = query.value(QStringLiteral("amount")).toDouble();
}

void SqliteP::ReadTransQuery(Trans* trans, const QSqlQuery& query) const
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

void SqliteP::WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query) const
{
    query.bindValue(QStringLiteral(":date_time"), *trans_shadow->date_time);
    query.bindValue(QStringLiteral(":unit_cost"), *trans_shadow->lhs_ratio);
    query.bindValue(QStringLiteral(":state"), *trans_shadow->state);
    query.bindValue(QStringLiteral(":description"), *trans_shadow->description);
    query.bindValue(QStringLiteral(":support_id"), *trans_shadow->support_id);
    query.bindValue(QStringLiteral(":code"), *trans_shadow->code);
    query.bindValue(QStringLiteral(":document"), trans_shadow->document->join(kSemicolon));

    query.bindValue(QStringLiteral(":lhs_node"), *trans_shadow->lhs_node);
    query.bindValue(QStringLiteral(":lhs_debit"), *trans_shadow->lhs_debit);
    query.bindValue(QStringLiteral(":lhs_credit"), *trans_shadow->lhs_credit);

    query.bindValue(QStringLiteral(":rhs_node"), *trans_shadow->rhs_node);
    query.bindValue(QStringLiteral(":rhs_debit"), *trans_shadow->rhs_debit);
    query.bindValue(QStringLiteral(":rhs_credit"), *trans_shadow->rhs_credit);
}

void SqliteP::UpdateTransValueBind(const TransShadow* trans_shadow, QSqlQuery& query) const
{
    query.bindValue(QStringLiteral(":lhs_node"), *trans_shadow->lhs_node);
    query.bindValue(QStringLiteral(":lhs_debit"), *trans_shadow->lhs_debit);
    query.bindValue(QStringLiteral(":lhs_credit"), *trans_shadow->lhs_credit);
    query.bindValue(QStringLiteral(":rhs_node"), *trans_shadow->rhs_node);
    query.bindValue(QStringLiteral(":rhs_debit"), *trans_shadow->rhs_debit);
    query.bindValue(QStringLiteral(":rhs_credit"), *trans_shadow->rhs_credit);
    query.bindValue(QStringLiteral(":trans_id"), *trans_shadow->id);
}

void SqliteP::ReadTransRefQuery(TransList& trans_list, QSqlQuery& query) const
{
    // remind to recycle these trans
    while (query.next()) {
        auto* trans { ResourcePool<Trans>::Instance().Allocate() };
        trans->id = query.value(QStringLiteral("section")).toInt();
        trans->rhs_node = query.value(QStringLiteral("party")).toInt();
        trans->lhs_ratio = query.value(QStringLiteral("unit_price")).toDouble();
        trans->lhs_credit = query.value(QStringLiteral("second")).toDouble();
        trans->description = query.value(QStringLiteral("description")).toString();
        trans->lhs_debit = query.value(QStringLiteral("first")).toDouble();
        trans->rhs_debit = query.value(QStringLiteral("gross_amount")).toDouble();
        trans->support_id = query.value(QStringLiteral("outside_product")).toInt();
        trans->rhs_ratio = query.value(QStringLiteral("discount_price")).toDouble();
        trans->date_time = query.value(QStringLiteral("date_time")).toString();
        trans->lhs_node = query.value(QStringLiteral("lhs_node")).toInt();

        trans_list.emplaceBack(trans);
    }
}

bool SqliteP::ReplaceLeaf(int old_node_id, int new_node_id, int node_unit)
{
    QSqlQuery query(db_);
    QString string { QSReplaceLeaf() };

    return DBTransaction([&]() {
        query.prepare(string);
        query.bindValue(QStringLiteral(":new_node_id"), new_node_id);
        query.bindValue(QStringLiteral(":old_node_id"), old_node_id);

        if (!query.exec()) {
            qWarning() << "Failed in ReplaceLeaf" << query.lastError().text();
            return false;
        }

        if (node_unit == std::to_underlying(UnitP::kPos))
            return true;

        CString stringsp { QSReplaceLeafSP() };
        query.prepare(stringsp);
        query.bindValue(QStringLiteral(":new_node_id"), new_node_id);
        query.bindValue(QStringLiteral(":old_node_id"), old_node_id);

        if (!query.exec()) {
            qWarning() << "Section: " << std::to_underlying(section_) << "Failed in ReplaceLeaf 1st" << query.lastError().text();
            return false;
        }

        CString stringosp { QSReplaceLeafOSP() };
        query.prepare(stringosp);
        query.bindValue(QStringLiteral(":new_node_id"), new_node_id);
        query.bindValue(QStringLiteral(":old_node_id"), old_node_id);

        if (!query.exec()) {
            qWarning() << "Section: " << std::to_underlying(section_) << "Failed in ReplaceLeaf 2nd" << query.lastError().text();
            return false;
        }

        CString stringopp { QSReplaceLeafOPP() };
        query.prepare(stringopp);
        query.bindValue(QStringLiteral(":new_node_id"), new_node_id);
        query.bindValue(QStringLiteral(":old_node_id"), old_node_id);

        if (!query.exec()) {
            qWarning() << "Section: " << std::to_underlying(section_) << "Failed in ReplaceLeaf 3rd" << query.lastError().text();
            return false;
        }

        return true;
    });
}

QString SqliteP::QSReadTrans() const
{
    return QStringLiteral(R"(
    SELECT id, lhs_node, unit_cost, lhs_debit, lhs_credit, rhs_node, rhs_debit, rhs_credit, state, description, support_id, code, document, date_time
    FROM product_transaction
    WHERE (lhs_node = :node_id OR rhs_node = :node_id) AND removed = 0
    )");
}

QString SqliteP::QSUpdateLeafValue() const
{
    return QStringLiteral(R"(
    UPDATE product SET
        quantity = :quantity, amount = :amount
    WHERE id = :node_id
    )");
}

void SqliteP::UpdateLeafValueBind(const Node* node, QSqlQuery& query) const
{
    query.bindValue(QStringLiteral(":quantity"), node->initial_total);
    query.bindValue(QStringLiteral(":amount"), node->final_total);
    query.bindValue(QStringLiteral(":node_id"), node->id);
}

QString SqliteP::QSWriteTrans() const
{
    return QStringLiteral(R"(
    INSERT INTO product_transaction
    (date_time, lhs_node, unit_cost, lhs_debit, lhs_credit, rhs_node, rhs_debit, rhs_credit, state, description, support_id, code, document)
    VALUES
    (:date_time, :lhs_node, :unit_cost, :lhs_debit, :lhs_credit, :rhs_node, :rhs_debit, :rhs_credit, :state, :description, :support_id, :code, :document)
    )");
}

QString SqliteP::QSReadSupportTrans() const
{
    return QStringLiteral(R"(
    SELECT id, lhs_node, unit_cost, lhs_debit, lhs_credit, rhs_node, rhs_debit, rhs_credit, state, description, support_id, code, document, date_time
    FROM product_transaction
    WHERE support_id = :node_id AND removed = 0
    )");
}

QString SqliteP::QSReplaceLeaf() const
{
    return QStringLiteral(R"(
    UPDATE product_transaction SET
        lhs_node = CASE WHEN lhs_node = :old_node_id AND rhs_node != :new_node_id THEN :new_node_id ELSE lhs_node END,
        rhs_node = CASE WHEN rhs_node = :old_node_id AND lhs_node != :new_node_id THEN :new_node_id ELSE rhs_node END
    WHERE lhs_node = :old_node_id OR rhs_node = :old_node_id;
    )");
}

QString SqliteP::QSUpdateTransValue() const
{
    return QStringLiteral(R"(
    UPDATE product_transaction SET
        lhs_node = :lhs_node, lhs_debit = :lhs_debit, lhs_credit = :lhs_credit,
        rhs_node = :rhs_node, rhs_debit = :rhs_debit, rhs_credit = :rhs_credit
    WHERE id = :trans_id
    )");
}

QString SqliteP::QSSearchTransValue() const
{
    return QStringLiteral(R"(
    SELECT id, lhs_node, unit_cost, lhs_debit, lhs_credit, rhs_node, rhs_debit, rhs_credit, state, description, support_id, code, document, date_time
    FROM product_transaction
    WHERE ((lhs_debit BETWEEN :value - :tolerance AND :value + :tolerance)
        OR (lhs_credit BETWEEN :value - :tolerance AND :value + :tolerance)
        OR (rhs_debit BETWEEN :value - :tolerance AND :value + :tolerance)
        OR (rhs_credit BETWEEN :value - :tolerance AND :value + :tolerance))
        AND removed = 0
    )");
}

QString SqliteP::QSSearchTransText() const
{
    return QStringLiteral(R"(
    SELECT id, lhs_node, unit_cost, lhs_debit, lhs_credit, rhs_node, rhs_debit, rhs_credit, state, description, support_id, code, document, date_time
    FROM product_transaction
    WHERE description LIKE :description AND removed = 0
    )");
}

QString SqliteP::QSReadTransRef(int /*unit*/) const
{
    return QStringLiteral(R"(
    SELECT
        4 AS section,
        st.unit_price,
        st.second,
        st.lhs_node,
        st.description,
        st.first,
        st.gross_amount,
        st.discount,
        st.net_amount,
        st.outside_product,
        st.discount_price,
        sn.party,
        sn.date_time
    FROM sales_transaction st
    INNER JOIN sales sn ON st.lhs_node = sn.id
    WHERE st.inside_product = :node_id AND sn.finished = 1 AND (sn.date_time BETWEEN :start AND :end) AND st.removed = 0

    UNION ALL

    SELECT
        5 AS section,
        pt.unit_price,
        pt.second,
        pt.lhs_node,
        pt.description,
        pt.first,
        pt.gross_amount,
        pt.discount,
        pt.net_amount,
        pt.outside_product,
        pt.discount_price,
        pn.party,
        pn.date_time
    FROM purchase_transaction pt
    INNER JOIN purchase pn ON pt.lhs_node = pn.id
    WHERE pt.inside_product = :node_id AND pn.finished = 1 AND (pn.date_time BETWEEN :start AND :end) AND pt.removed = 0;
    )");
}

QString SqliteP::QSReplaceLeafSP() const
{
    return QStringLiteral(R"(
    UPDATE stakeholder_transaction
    SET inside_product = :new_node_id
    WHERE inside_product = :old_node_id;
    )");
}

QString SqliteP::QSReplaceLeafOSP() const
{
    return QStringLiteral(R"(
    UPDATE sales_transaction
    SET inside_product = :new_node_id
    WHERE inside_product = :old_node_id;
    )");
}

QString SqliteP::QSReplaceLeafOPP() const
{
    return QStringLiteral(R"(
    UPDATE purchase_transaction
    SET inside_product = :new_node_id
    WHERE inside_product = :old_node_id;
    )");
}
