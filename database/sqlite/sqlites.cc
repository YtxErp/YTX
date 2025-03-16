#include "sqlites.h"

#include <QSqlError>
#include <QSqlQuery>

#include "component/constvalue.h"
#include "global/resourcepool.h"

SqliteS::SqliteS(CInfo& info, QObject* parent)
    : Sqlite(info, parent)
{
}

void SqliteS::RReplaceNode(int old_node_id, int new_node_id, int node_type, int node_unit)
{
    emit SFreeWidget(old_node_id, node_type);
    emit SRemoveNode(old_node_id);

    if (node_type == kTypeLeaf) {
        ReplaceLeaf(old_node_id, new_node_id, node_unit);
        emit SUpdateStakeholder(old_node_id, new_node_id);
        emit SUpdateMultiLeafTotal({ new_node_id });
    }

    if (node_type == kTypeSupport) {
        QSet<int> trans_id_set {};
        ReplaceSupportFunction(trans_id_set, old_node_id, new_node_id);
        emit SMoveMultiTransS(section_, 0, new_node_id, trans_id_set);

        ReplaceSupport(old_node_id, new_node_id);
    }

    RemoveNode(old_node_id, node_type);
}

void SqliteS::RRemoveNode(int node_id, int node_type)
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

        emit SUpdateStakeholder(node_id, 0); // Update Stakeholder's employee

        if (!support_trans.isEmpty())
            emit SRemoveMultiTransS(section_, support_trans);

        RemoveLeafFunction(leaf_trans);
    }

    // Remove node, path, trans from the sqlite3 database
    RemoveNode(node_id, node_type);
}

void SqliteS::RPriceSList(QList<PriceS>& list)
{
    for (int i = 0; i != list.size(); ++i) {
        Trans* latest_trans { nullptr };

        for (auto* trans : std::as_const(trans_hash_)) {
            if (trans->lhs_node == list[i].lhs_node && trans->rhs_node == list[i].inside_product) {
                latest_trans = trans;
                break;
            }
        }

        if (latest_trans) {
            latest_trans->lhs_ratio = list[i].unit_price;
            latest_trans->rhs_ratio = list[i].unit_price;
            latest_trans->date_time = list[i].date_time;
        }
    }

    QSet<int> set {};
    for (const auto& item : std::as_const(list)) {
        set.insert(item.inside_product);
    }

    ReadTransRange(set);
}

void SqliteS::RUpdateProduct(int old_node_id, int new_node_id)
{
    for (auto* trans : std::as_const(trans_hash_))
        if (trans->rhs_node == old_node_id)
            trans->rhs_node = new_node_id;
}

bool SqliteS::CrossSearch(TransShadow* order_trans_shadow, int party_id, int product_id, bool is_inside) const
{
    const Trans* latest_trans { nullptr };

    for (const auto* trans : std::as_const(trans_hash_)) {
        if (is_inside && trans->lhs_node == party_id && trans->rhs_node == product_id) {
            latest_trans = trans;
            break;
        }

        if (!is_inside && trans->lhs_node == party_id && trans->support_id == product_id) {
            latest_trans = trans;
            break;
        }
    }

    if (!latest_trans) {
        return false;
    }

    *order_trans_shadow->lhs_ratio = latest_trans->lhs_ratio;

    if (is_inside) {
        *order_trans_shadow->support_id = latest_trans->support_id;
    } else {
        *order_trans_shadow->rhs_node = latest_trans->rhs_node;
    }

    return true;
}

bool SqliteS::ReadTransRange(const QSet<int>& set)
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    CString placeholder { QStringList(set.size(), "?").join(", ") };
    CString string { QString(R"(
    SELECT id, date_time, code, outside_product, lhs_node, unit_price, description, document, state, inside_product
    FROM stakeholder_transaction
    WHERE inside_product IN (%1)
    )")
            .arg(placeholder) };

    query.prepare(string);

    for (int value : set) {
        query.addBindValue(value);
    }

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(section_) << "Failed in RetrieveTrans" << query.lastError().text();
        return false;
    }

    TransShadowList trans_shadow_list {};

    while (query.next()) {
        const int kID { query.value(QStringLiteral("id")).toInt() };

        if (trans_hash_.contains(kID))
            continue;

        auto* trans_shadow { ResourcePool<TransShadow>::Instance().Allocate() };
        auto* trans { ResourcePool<Trans>::Instance().Allocate() };
        trans->id = kID;

        ReadTransQuery(trans, query);
        trans_hash_.insert(kID, trans);

        ConvertTrans(trans, trans_shadow, true);
        trans_shadow_list.emplaceBack(trans_shadow);
    }

    if (!trans_shadow_list.isEmpty())
        emit SAppendMultiTrans(section_, *trans_shadow_list[0]->lhs_node, trans_shadow_list);

    return true;
}

bool SqliteS::ReplaceLeafC(QSqlQuery& query, int old_node_id, int new_node_id) const
{
    CString string { QSReplaceLeafOSP() };

    query.prepare(string);
    query.bindValue(QStringLiteral(":new_node_id"), new_node_id);
    query.bindValue(QStringLiteral(":old_node_id"), old_node_id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(section_) << "Failed in ReplaceLeafC" << query.lastError().text();
        return false;
    }

    return true;
}

bool SqliteS::ReplaceLeafE(QSqlQuery& query, int old_node_id, int new_node_id) const
{
    CString stringse { QSReplaceLeafSE() };
    CString stringose { QSReplaceLeafOSE() };
    CString stringope { QSReplaceLeafOPE() };

    if (!DBTransaction([&]() {
            query.prepare(stringse);
            query.bindValue(QStringLiteral(":new_node_id"), new_node_id);
            query.bindValue(QStringLiteral(":old_node_id"), old_node_id);

            if (!query.exec()) {
                qWarning() << "Section: " << std::to_underlying(section_) << "Failed in ReplaceLeaf 1st" << query.lastError().text();
                return false;
            }

            query.clear();

            query.prepare(stringose);
            query.bindValue(QStringLiteral(":new_node_id"), new_node_id);
            query.bindValue(QStringLiteral(":old_node_id"), old_node_id);

            if (!query.exec()) {
                qWarning() << "Section: " << std::to_underlying(section_) << "Failed in ReplaceLeaf 2nd" << query.lastError().text();
                return false;
            }

            query.clear();

            query.prepare(stringope);
            query.bindValue(QStringLiteral(":new_node_id"), new_node_id);
            query.bindValue(QStringLiteral(":old_node_id"), old_node_id);

            if (!query.exec()) {
                qWarning() << "Section: " << std::to_underlying(section_) << "Failed in ReplaceLeaf 3rd" << query.lastError().text();
                return false;
            }

            return true;
        })) {
        qWarning() << "Failed in ReplaceLeaf";
        return false;
    }

    return true;
}

bool SqliteS::ReplaceLeafV(QSqlQuery& query, int old_node_id, int new_node_id) const
{
    CString string { QSReplaceLeafOPP() };

    query.prepare(string);
    query.bindValue(QStringLiteral(":new_node_id"), new_node_id);
    query.bindValue(QStringLiteral(":old_node_id"), old_node_id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(section_) << "Failed in ReplaceLeafV" << query.lastError().text();
        return false;
    }

    return true;
}

bool SqliteS::ReplaceLeaf(int old_node_id, int new_node_id, int node_unit) const
{
    QSqlQuery query(*db_);

    switch (UnitS(node_unit)) {
    case UnitS::kCust:
        ReplaceLeafC(query, old_node_id, new_node_id);
        break;
    case UnitS::kEmp:
        ReplaceLeafE(query, old_node_id, new_node_id);
        break;
    case UnitS::kVend:
        ReplaceLeafV(query, old_node_id, new_node_id);
        break;
    default:
        break;
    }

    return true;
}

bool SqliteS::ReadTrans(int node_id)
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    CString string { QSReadTrans() };
    query.prepare(string);
    query.bindValue(QStringLiteral(":node_id"), node_id);

    if (!query.exec()) {
        qWarning() << "Section: " << std::to_underlying(section_) << "Failed in ReadTrans" << query.lastError().text();
        return false;
    }

    ReadTransS(query);
    return true;
}

QString SqliteS::QSReadNode() const
{
    return QStringLiteral(R"(
    SELECT name, id, code, description, note, type, unit, employee, deadline, payment_term, tax_rate, amount
    FROM stakeholder
    WHERE removed = 0
    )");
}

QString SqliteS::QSWriteNode() const
{
    return QStringLiteral(R"(
    INSERT INTO stakeholder (name, code, description, note, type, unit, employee, deadline, payment_term, tax_rate, amount)
    VALUES (:name, :code, :description, :note, :type, :unit, :employee, :deadline, :payment_term, :tax_rate, :amount)
    )");
}

void SqliteS::WriteNodeBind(Node* node, QSqlQuery& query) const
{
    query.bindValue(QStringLiteral(":name"), node->name);
    query.bindValue(QStringLiteral(":code"), node->code);
    query.bindValue(QStringLiteral(":description"), node->description);
    query.bindValue(QStringLiteral(":note"), node->note);
    query.bindValue(QStringLiteral(":type"), node->type);
    query.bindValue(QStringLiteral(":unit"), node->unit);
    query.bindValue(QStringLiteral(":employee"), node->employee);
    query.bindValue(QStringLiteral(":deadline"), node->date_time);
    query.bindValue(QStringLiteral(":payment_term"), node->first);
    query.bindValue(QStringLiteral(":tax_rate"), node->second);
    query.bindValue(QStringLiteral(":amount"), node->final_total);
}

QString SqliteS::QSRemoveNodeSecond() const
{
    return QStringLiteral(R"(
    DELETE FROM stakeholder_transaction
    WHERE lhs_node = :node_id;
    )");
}

QString SqliteS::QSInternalReference() const
{
    return QStringLiteral(R"(
    SELECT
    (SELECT COUNT(*) FROM stakeholder_transaction WHERE lhs_node = :node_id) +
    (SELECT COUNT(*) FROM stakeholder WHERE employee = :node_id AND removed = 0)
    AS total_count;
    )");
}

QString SqliteS::QSExternalReferencePS() const
{
    return QStringLiteral(R"(
    SELECT
    (SELECT COUNT(*) FROM sales WHERE (party = :node_id OR employee = :node_id) AND removed = 0) +
    (SELECT COUNT(*) FROM purchase WHERE (party = :node_id OR employee = :node_id) AND removed = 0) +
    (SELECT COUNT(*) FROM sales_transaction WHERE outside_product = :node_id AND removed = 0) +
    (SELECT COUNT(*) FROM purchase_transaction WHERE outside_product = :node_id AND removed = 0)
    AS total_count;
    )");
}

QString SqliteS::QSSupportReference() const
{
    return QStringLiteral(R"(
    SELECT COUNT(*) FROM stakeholder_transaction
    WHERE outside_product = :support_id
    )");
}

QString SqliteS::QSReplaceSupport() const
{
    return QStringLiteral(R"(
    UPDATE stakeholder_transaction SET
        outside_product = :new_node_id
    WHERE outside_product = :old_node_id
    )");
}

QString SqliteS::QSSyncLeafValue() const
{
    return QStringLiteral(R"(
    UPDATE stakeholder SET
        amount = :amount
    WHERE id = :node_id
    )");
}

void SqliteS::SyncLeafValueBind(const Node* node, QSqlQuery& query) const
{
    query.bindValue(":amount", node->final_total);
    query.bindValue(":node_id", node->id);
}

QString SqliteS::QSRemoveSupport() const
{
    return QStringLiteral(R"(
    UPDATE stakeholder_transaction SET
        outside_product = 0
    WHERE outside_product = :node_id
    )");
}

QString SqliteS::QSReadTrans() const
{
    return QStringLiteral(R"(
    SELECT id, date_time, code, outside_product, lhs_node, unit_price, description, document, state, inside_product
    FROM stakeholder_transaction
    WHERE lhs_node = :node_id
    )");
}

QString SqliteS::QSReadSupportTrans() const
{
    return QStringLiteral(R"(
    SELECT id, date_time, code, outside_product, lhs_node, unit_price, description, document, state, inside_product
    FROM stakeholder_transaction
    WHERE outside_product = :node_id
    )");
}

QString SqliteS::QSWriteTrans() const
{
    return QStringLiteral(R"(
    INSERT INTO stakeholder_transaction
    (date_time, code, outside_product, lhs_node, unit_price, description, document, state, inside_product)
    VALUES
    (:date_time, :code, :outside_product, :lhs_node, :unit_price, :description, :document, :state, :inside_product)
    )");
}

QString SqliteS::QSReplaceLeafSE() const
{
    return QStringLiteral(R"(
    UPDATE stakeholder
    SET employee = :new_node_id
    WHERE employee = :old_node_id;
    )");
}

QString SqliteS::QSReplaceLeafOSE() const
{
    return QStringLiteral(R"(
    UPDATE sales
    SET employee = :new_node_id
    WHERE employee = :old_node_id;
    )");
}

QString SqliteS::QSReplaceLeafOPE() const
{
    return QStringLiteral(R"(
    UPDATE purchase
    SET employee = :new_node_id
    WHERE employee = :old_node_id;
    )");
}

QString SqliteS::QSReplaceLeafOSP() const
{
    return QStringLiteral(R"(
    UPDATE sales
    SET party = :new_node_id
    WHERE party = :old_node_id;
    )");
}

QString SqliteS::QSReplaceLeafOPP() const
{
    return QStringLiteral(R"(
    UPDATE purchase
    SET party = :new_node_id
    WHERE party = :old_node_id;
    )");
}

QString SqliteS::QSSearchTransValue() const
{
    return QStringLiteral(R"(
    SELECT id, date_time, code, outside_product, lhs_node, unit_price, description, document, state, inside_product
    FROM stakeholder_transaction
    WHERE unit_price BETWEEN :value - :tolerance AND :value + :tolerance
    )");
}

QString SqliteS::QSSearchTransText() const
{
    return QStringLiteral(R"(
    SELECT id, date_time, code, outside_product, lhs_node, unit_price, description, document, state, inside_product
    FROM stakeholder_transaction
    WHERE description LIKE :description
    )");
}

QString SqliteS::QSRemoveNodeFirst() const
{
    return QStringLiteral(R"(
    UPDATE stakeholder
    SET
        removed = CASE WHEN id = :node_id THEN 1 ELSE removed END,
        employee = CASE WHEN employee = :node_id THEN NULL ELSE employee END
    WHERE (id = :node_id OR employee = :node_id) AND removed = 0;
    )");
}

QString SqliteS::QSTransToRemove() const
{
    return QStringLiteral(R"(
    SELECT lhs_node AS node_id, id AS trans_id, outside_product AS support_id FROM stakeholder_transaction
    WHERE lhs_node = :node_id
    )");
}

QString SqliteS::QSReadTransRef() const
{
    return QStringLiteral(R"(
    SELECT
        st.inside_product,
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
        sn.date_time
    FROM sales_transaction st
    INNER JOIN sales sn ON st.lhs_node = sn.id
    WHERE sn.party = :node_id AND sn.finished = 1 AND (sn.date_time BETWEEN :start AND :end) AND st.removed = 0;
    )");
}

QString SqliteS::QSRemoveTrans() const
{
    return QStringLiteral(R"(
    DELETE FROM stakeholder_transaction
    WHERE id = :trans_id
    )");
}

QString SqliteS::QSLeafTotal(int unit) const
{
    switch (UnitS(unit)) {
    case UnitS::kCust:
        return QStringLiteral(R"(
        SELECT SUM(gross_amount) AS amount
        FROM sales
        WHERE party = :node_id AND finished = 1 AND settlement_id = 0 AND unit = 1 AND removed = 0;
        )");
        break;
    case UnitS::kVend:
        return QStringLiteral(R"(
        SELECT SUM(gross_amount) AS amount
        FROM purchase
        WHERE party = :node_id AND finished = 1 AND settlement_id = 0 AND unit = 1 AND removed = 0;
        )");
        break;
    default:
        break;
    }

    return {};
}

void SqliteS::ReadTransS(QSqlQuery& query)
{
    Trans* trans {};

    while (query.next()) {
        const int kID { query.value(QStringLiteral("id")).toInt() };

        if (trans_hash_.contains(kID))
            continue;

        trans = ResourcePool<Trans>::Instance().Allocate();
        trans->id = kID;

        ReadTransQuery(trans, query);
        trans_hash_.insert(kID, trans);
    }
}

void SqliteS::ReadTransRefQuery(TransList& trans_list, QSqlQuery& query) const
{
    // remind to recycle these trans
    while (query.next()) {
        auto* trans { ResourcePool<Trans>::Instance().Allocate() };

        trans->id = query.value(QStringLiteral("inside_product")).toInt();
        trans->lhs_ratio = query.value(QStringLiteral("unit_price")).toDouble();
        trans->lhs_credit = query.value(QStringLiteral("second")).toDouble();
        trans->description = query.value(QStringLiteral("description")).toString();
        trans->lhs_debit = query.value(QStringLiteral("first")).toInt();
        trans->rhs_debit = query.value(QStringLiteral("gross_amount")).toDouble();
        trans->rhs_credit = query.value(QStringLiteral("net_amount")).toDouble();
        trans->discount = query.value(QStringLiteral("discount")).toDouble();
        trans->support_id = query.value(QStringLiteral("outside_product")).toInt();
        trans->rhs_ratio = query.value(QStringLiteral("discount_price")).toDouble();
        trans->date_time = query.value(QStringLiteral("date_time")).toString();
        trans->lhs_node = query.value(QStringLiteral("lhs_node")).toInt();

        trans_list.emplaceBack(trans);
    }
}

void SqliteS::CalculateLeafTotal(Node* node, QSqlQuery& query) const
{
    query.next();
    node->final_total = query.value(QStringLiteral("amount")).toDouble();
}

void SqliteS::WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query) const
{
    query.bindValue(QStringLiteral(":date_time"), *trans_shadow->date_time);
    query.bindValue(QStringLiteral(":code"), *trans_shadow->code);
    query.bindValue(QStringLiteral(":lhs_node"), *trans_shadow->lhs_node);
    query.bindValue(QStringLiteral(":unit_price"), *trans_shadow->lhs_ratio);
    query.bindValue(QStringLiteral(":description"), *trans_shadow->description);
    query.bindValue(QStringLiteral(":state"), *trans_shadow->state);
    query.bindValue(QStringLiteral(":document"), trans_shadow->document->join(kSemicolon));
    query.bindValue(QStringLiteral(":inside_product"), *trans_shadow->rhs_node);
    query.bindValue(QStringLiteral(":outside_product"), *trans_shadow->support_id);
}

void SqliteS::ReadNodeQuery(Node* node, const QSqlQuery& query) const
{
    node->id = query.value(QStringLiteral("id")).toInt();
    node->name = query.value(QStringLiteral("name")).toString();
    node->code = query.value(QStringLiteral("code")).toString();
    node->description = query.value(QStringLiteral("description")).toString();
    node->note = query.value(QStringLiteral("note")).toString();
    node->type = query.value(QStringLiteral("type")).toInt();
    node->unit = query.value(QStringLiteral("unit")).toInt();
    node->employee = query.value(QStringLiteral("employee")).toInt();
    node->date_time = query.value(QStringLiteral("deadline")).toString();
    node->first = query.value(QStringLiteral("payment_term")).toInt();
    node->second = query.value(QStringLiteral("tax_rate")).toDouble();
    node->final_total = query.value(QStringLiteral("amount")).toDouble();
}

void SqliteS::ReadTransQuery(Trans* trans, const QSqlQuery& query) const
{
    trans->support_id = query.value(QStringLiteral("outside_product")).toInt();
    trans->lhs_node = query.value(QStringLiteral("lhs_node")).toInt();
    trans->rhs_node = query.value(QStringLiteral("inside_product")).toInt();
    trans->lhs_ratio = query.value(QStringLiteral("unit_price")).toDouble();
    trans->rhs_ratio = trans->lhs_ratio;
    trans->code = query.value(QStringLiteral("code")).toString();
    trans->description = query.value(QStringLiteral("description")).toString();
    trans->state = query.value(QStringLiteral("state")).toBool();
    trans->document = query.value(QStringLiteral("document")).toString().split(kSemicolon, Qt::SkipEmptyParts);
    trans->date_time = query.value(QStringLiteral("date_time")).toString();
}
