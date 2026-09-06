#include "node.h"

#include "component/constant.h"
#include "component/constantint.h"
#include "utils/entryutils.h"

void Node::Reset() { *this = Node {}; }

void Node::InvertTotal()
{
    initial_total = -initial_total;
    final_total = -final_total;
}

void Node::ReadJson(const QJsonObject& object)
{
    // Data loaded from server
    sync_state = SyncState::kSynced;

    if (const auto val = object.value(kName); val.isString())
        name = val.toString();
    if (const auto val = object.value(kId); val.isString())
        id = QUuid(val.toString());
    if (const auto val = object.value(kCode); val.isString())
        code = val.toString();
    if (const auto val = object.value(kDescription); val.isString())
        description = val.toString();
    if (const auto val = object.value(kKind); val.isDouble())
        kind = static_cast<NodeKind>(val.toInt());
    if (const auto val = object.value(kDirectionRule); val.isBool())
        direction_rule = val.toBool();
    if (const auto val = object.value(kUnit); val.isDouble())
        unit = static_cast<NodeUnit>(val.toInt());
    if (const auto val = object.value(kFinalTotal); val.isString())
        final_total = val.toString().toDouble();
    if (const auto val = object.value(kInitialTotal); val.isString())
        initial_total = val.toString().toDouble();
    if (const auto val = object.value(kVersion); val.isDouble())
        version = val.toInt();
    if (const auto val = object.value(kColor); val.isString())
        color = val.toString();
    if (object.value(kTag).isArray())
        tag = utils::ReadStringList(object, kTag);
    if (object.value(kDocument).isArray())
        document = utils::ReadStringList(object, kDocument);
    if (const auto val = object.value(kStatus); val.isDouble())
        status = static_cast<NodeStatus>(val.toInt());
}

QJsonObject Node::WriteJson() const
{
    QJsonObject obj {};
    obj.insert(kName, name);
    obj.insert(kColor, color);
    obj.insert(kId, id.toString(QUuid::WithoutBraces));
    obj.insert(kCode, code);
    obj.insert(kDescription, description);
    obj.insert(kKind, std::to_underlying(kind));
    obj.insert(kDirectionRule, direction_rule);
    obj.insert(kUnit, std::to_underlying(unit));
    obj.insert(kFinalTotal, QString::number(final_total, 'f', numeric_const::kDecimalPlaces4));
    obj.insert(kInitialTotal, QString::number(initial_total, 'f', numeric_const::kDecimalPlaces4));
    obj.insert(kTag, utils::WriteStringList(tag));
    obj.insert(kDocument, utils::WriteStringList(document));
    obj.insert(kVersion, version);
    // Status is initialized to 1 by the database.
    // obj.insert(kStatus, std::to_underlying(status));

    // user_id, created_time, created_by, updated_time, updated_by, version
    // are managed by the server, not written to json
    return obj;
}

void NodeF::Reset() { *this = NodeF {}; }

void NodeF::ReadJson(const QJsonObject& object)
{
    Node::ReadJson(object);

    if (const auto val = object.value(kRoles); val.isDouble())
        roles = static_cast<finance::Roles>(val.toInt());
}

QJsonObject NodeF::WriteJson() const
{
    QJsonObject obj { Node::WriteJson() };
    obj.insert(kRoles, static_cast<int>(roles));
    return obj;
}

void NodeI::Reset() { *this = NodeI {}; }

void NodeI::ReadJson(const QJsonObject& object)
{
    Node::ReadJson(object);
    if (const auto val = object.value(kUnitPrice); val.isString())
        unit_price = val.toString().toDouble();
    if (const auto val = object.value(kCommission); val.isString())
        commission = val.toString().toDouble();
}

QJsonObject NodeI::WriteJson() const
{
    QJsonObject obj { Node::WriteJson() };
    obj.insert(kUnitPrice, QString::number(unit_price, 'f', numeric_const::kDecimalPlaces8));
    obj.insert(kCommission, QString::number(commission, 'f', numeric_const::kDecimalPlaces8));
    return obj;
}

void NodeP::Reset() { *this = NodeP {}; }

void NodeP::ReadJson(const QJsonObject& object)
{
    // Data loaded from server
    sync_state = SyncState::kSynced;

    if (const auto val = object.value(kId); val.isString())
        id = QUuid(val.toString());
    if (const auto val = object.value(kName); val.isString())
        name = val.toString();
    if (const auto val = object.value(kCode); val.isString())
        code = val.toString();
    if (const auto val = object.value(kDescription); val.isString())
        description = val.toString();
    if (const auto val = object.value(kKind); val.isDouble())
        kind = static_cast<NodeKind>(val.toInt());
    if (const auto val = object.value(kUnit); val.isDouble())
        unit = static_cast<NodeUnit>(val.toInt());
    if (const auto val = object.value(kInitialTotal); val.isString())
        initial_total = val.toString().toDouble();
    if (const auto val = object.value(kPaymentTerm); val.isDouble())
        payment_term = val.toInt();
    if (const auto val = object.value(kColor); val.isString())
        color = val.toString();
    if (const auto val = object.value(kStatus); val.isDouble())
        status = static_cast<NodeStatus>(val.toInt());
    if (const auto val = object.value(kVersion); val.isDouble())
        version = val.toInt();

    if (object.value(kTag).isArray())
        tag = utils::ReadStringList(object, kTag);
    if (object.value(kDocument).isArray())
        document = utils::ReadStringList(object, kDocument);
}

QJsonObject NodeP::WriteJson() const
{
    QJsonObject obj {};
    obj.insert(kName, name);
    obj.insert(kId, id.toString(QUuid::WithoutBraces));
    obj.insert(kCode, code);
    obj.insert(kDescription, description);
    obj.insert(kKind, std::to_underlying(kind));
    obj.insert(kUnit, std::to_underlying(unit));
    obj.insert(kInitialTotal, QString::number(initial_total, 'f', numeric_const::kDecimalPlaces4));
    obj.insert(kPaymentTerm, payment_term);
    obj.insert(kColor, color);
    obj.insert(kTag, utils::WriteStringList(tag));
    obj.insert(kDocument, utils::WriteStringList(document));
    obj.insert(kVersion, version);
    obj.insert(kStatus, std::to_underlying(NodeStatus::kActive));

    return obj;
}

void NodeO::Reset() { *this = NodeO {}; }

void NodeO::InvertTotal()
{
    count_total = -count_total;
    measure_total = -measure_total;
    discount_total = -discount_total;
    initial_total = -initial_total;
    final_total = -final_total;
}

void NodeO::ReadJson(const QJsonObject& object)
{
    // Data loaded from server
    sync_state = SyncState::kSynced;

    if (const auto val = object.value(kId); val.isString())
        id = QUuid(val.toString());
    if (const auto val = object.value(kPartnerId); val.isString())
        partner_id = QUuid(val.toString());
    if (const auto val = object.value(kEmployeeId); val.isString())
        employee_id = QUuid(val.toString());
    if (const auto val = object.value(kSettlementId); val.isString())
        settlement_id = QUuid(val.toString());

    if (const auto val = object.value(kName); val.isString())
        name = val.toString();
    if (const auto val = object.value(kCode); val.isString())
        code = val.toString();
    if (const auto val = object.value(kDescription); val.isString())
        description = val.toString();

    if (const auto val = object.value(kKind); val.isDouble())
        kind = static_cast<NodeKind>(val.toInt());
    if (const auto val = object.value(kUnit); val.isDouble())
        unit = static_cast<NodeUnit>(val.toInt());
    if (const auto val = object.value(kStatus); val.isDouble())
        order_status = static_cast<OrderStatus>(val.toInt());
    if (const auto val = object.value(kVersion); val.isDouble())
        version = val.toInt();

    if (const auto val = object.value(kInitialTotal); val.isString())
        initial_total = val.toString().toDouble();
    if (const auto val = object.value(kDiscountTotal); val.isString())
        discount_total = val.toString().toDouble();
    if (const auto val = object.value(kFinalTotal); val.isString())
        final_total = val.toString().toDouble();
    if (const auto val = object.value(kCountTotal); val.isString())
        count_total = val.toString().toDouble();
    if (const auto val = object.value(kMeasureTotal); val.isString())
        measure_total = val.toString().toDouble();

    if (const auto val = object.value(kIssuedTime); val.isString())
        issued_time = QDateTime::fromString(val.toString(), Qt::ISODate).toLocalTime();

    if (const auto val = object.value(kDirectionRule); val.isBool())
        direction_rule = val.toBool();

    if (object.value(kTag).isArray())
        tag = utils::ReadStringList(object, kTag);
}

QJsonObject NodeO::WriteJson() const
{
    QJsonObject obj {};
    obj.insert(kName, name);
    obj.insert(kId, id.toString(QUuid::WithoutBraces));
    obj.insert(kCode, code);
    obj.insert(kDescription, description);
    obj.insert(kKind, std::to_underlying(kind));
    obj.insert(kDirectionRule, direction_rule);
    obj.insert(kUnit, std::to_underlying(unit));
    obj.insert(kFinalTotal, QString::number(final_total, 'f', numeric_const::kDecimalPlaces4));
    obj.insert(kInitialTotal, QString::number(initial_total, 'f', numeric_const::kDecimalPlaces4));
    obj.insert(kDiscountTotal, QString::number(discount_total, 'f', numeric_const::kDecimalPlaces4));
    obj.insert(kEmployeeId, employee_id.toString(QUuid::WithoutBraces));
    obj.insert(kPartnerId, partner_id.toString(QUuid::WithoutBraces));
    obj.insert(kIssuedTime, issued_time.toUTC().toString(Qt::ISODate));
    obj.insert(kCountTotal, QString::number(count_total, 'f', numeric_const::kDecimalPlaces8));
    obj.insert(kMeasureTotal, QString::number(measure_total, 'f', numeric_const::kDecimalPlaces8));
    obj.insert(kStatus, std::to_underlying(order_status));
    obj.insert(kTag, utils::WriteStringList(tag));
    obj.insert(kVersion, version);

    // server will ignore these two values
    obj.insert(kSettlementId, settlement_id.toString(QUuid::WithoutBraces));

    return obj;
}
