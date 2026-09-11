#include "entry.h"

#include <QJsonArray>
#include <QJsonObject>

#include "component/constant.h"
#include "component/constantint.h"
#include "utils/entryutils.h"

void Entry::Reset() { *this = Entry {}; }

void Entry::ReadJson(const QJsonObject& object)
{
    // Data loaded from server
    sync_state = SyncState::kSynced;

    if (const auto val = object.value(kId); val.isString())
        id = QUuid(val.toString());
    if (const auto val = object.value(kIssuedTime); val.isString())
        issued_time = QDateTime::fromString(val.toString(), Qt::ISODate).toLocalTime();
    if (const auto val = object.value(kCode); val.isString())
        code = val.toString();
    if (const auto val = object.value(kLhsNode); val.isString())
        lhs_node = QUuid(val.toString());
    if (const auto val = object.value(kDescription); val.isString())
        description = val.toString();
    if (const auto val = object.value(kStatus); val.isDouble())
        status = val.toInt();
    if (const auto val = object.value(kRhsNode); val.isString())
        rhs_node = QUuid(val.toString());
    if (const auto val = object.value(kVersion); val.isDouble())
        version = val.toInt();
    if (const auto val = object.value(kLhsRate); val.isString())
        lhs_rate = val.toString().toDouble();
    if (const auto val = object.value(kRhsRate); val.isString())
        rhs_rate = val.toString().toDouble();
    if (const auto val = object.value(kLhsDebit); val.isString())
        lhs_debit = val.toString().toDouble();
    if (const auto val = object.value(kLhsCredit); val.isString())
        lhs_credit = val.toString().toDouble();
    if (const auto val = object.value(kRhsDebit); val.isString())
        rhs_debit = val.toString().toDouble();
    if (const auto val = object.value(kRhsCredit); val.isString())
        rhs_credit = val.toString().toDouble();
    if (object.value(kTag).isArray())
        tag = utils::ReadStringList(object, kTag);
    if (object.value(kDocument).isArray())
        document = utils::ReadStringList(object, kDocument);
}

void EntryF::Reset() { *this = EntryF {}; }

void EntryF::ReadJson(const QJsonObject& object)
{
    Entry::ReadJson(object);

    if (const auto val = object.value(kCashKind); val.isDouble())
        cash_kind = static_cast<finance::CashKind>(val.toInt());
}

void EntryP::Reset() { *this = EntryP {}; }

void EntryP::ReadJson(const QJsonObject& object)
{
    // Data loaded from server
    sync_state = SyncState::kSynced;

    if (const auto val = object.value(kId); val.isString())
        id = QUuid(val.toString());
    if (const auto val = object.value(kIssuedTime); val.isString())
        issued_time = QDateTime::fromString(val.toString(), Qt::ISODate).toLocalTime();
    if (const auto val = object.value(kCode); val.isString())
        code = val.toString();
    if (const auto val = object.value(kLhsNode); val.isString())
        lhs_node = QUuid(val.toString());
    if (const auto val = object.value(kRhsNode); val.isString())
        rhs_node = QUuid(val.toString());
    if (const auto val = object.value(kExternalSku); val.isString())
        external_sku = val.toString();
    if (const auto val = object.value(kDescription); val.isString())
        description = val.toString();
    if (const auto val = object.value(kStatus); val.isDouble())
        status = val.toInt();
    if (const auto val = object.value(kVersion); val.isDouble())
        version = val.toInt();
    if (const auto val = object.value(kUnitPrice); val.isString())
        unit_price = val.toString().toDouble();

    if (object.value(kTag).isArray())
        tag = utils::ReadStringList(object, kTag);
    if (object.value(kDocument).isArray())
        document = utils::ReadStringList(object, kDocument);
}

QJsonObject EntryP::WriteJson() const
{
    QJsonObject obj {};

    obj.insert(kId, id.toString(QUuid::WithoutBraces));
    obj.insert(kIssuedTime, issued_time.toUTC().toString(Qt::ISODate));
    obj.insert(kCode, code);
    obj.insert(kLhsNode, lhs_node.toString(QUuid::WithoutBraces));
    obj.insert(kDescription, description);
    obj.insert(kExternalSku, external_sku);
    obj.insert(kStatus, status);
    obj.insert(kRhsNode, rhs_node.toString(QUuid::WithoutBraces));
    obj.insert(kTag, utils::WriteStringList(tag));
    obj.insert(kDocument, utils::WriteStringList(document));
    obj.insert(kVersion, version);

    obj.insert(kUnitPrice, QString::number(unit_price, 'f', numeric_const::kDecimalPlaces8));
    return obj;
}

void EntryO::Reset() { *this = EntryO {}; }

// Note: Fields like issued_time, document, code and status are ignored for Order entries
void EntryO::ReadJson(const QJsonObject& object)
{
    // Data loaded from server
    sync_state = SyncState::kSynced;

    if (const auto val = object.value(kId); val.isString())
        id = QUuid(val.toString());
    if (const auto val = object.value(kDescription); val.isString())
        description = val.toString();
    if (const auto val = object.value(kLhsNode); val.isString())
        lhs_node = QUuid(val.toString());
    if (const auto val = object.value(kRhsNode); val.isString())
        rhs_node = QUuid(val.toString());
    if (const auto val = object.value(kVersion); val.isDouble())
        version = val.toInt();
    if (const auto val = object.value(kUnitPrice); val.isString())
        unit_price = val.toString().toDouble();
    if (const auto val = object.value(kCount); val.isString())
        count = val.toString().toDouble();
    if (const auto val = object.value(kMeasure); val.isString())
        measure = val.toString().toDouble();
    if (const auto val = object.value(kInitial); val.isString())
        initial = val.toString().toDouble();
    if (object.value(kTag).isArray())
        tag = utils::ReadStringList(object, kTag);
}

QJsonObject EntryO::WriteJson() const
{
    QJsonObject obj {};

    obj.insert(kId, id.toString(QUuid::WithoutBraces));
    obj.insert(kDescription, description);
    obj.insert(kLhsNode, lhs_node.toString(QUuid::WithoutBraces));
    obj.insert(kRhsNode, rhs_node.toString(QUuid::WithoutBraces));

    obj.insert(kUnitPrice, QString::number(unit_price, 'f', numeric_const::kDecimalPlaces8));
    obj.insert(kCount, QString::number(count, 'f', numeric_const::kDecimalPlaces8));
    obj.insert(kMeasure, QString::number(measure, 'f', numeric_const::kDecimalPlaces8));
    obj.insert(kInitial, QString::number(initial, 'f', numeric_const::kDecimalPlaces4));
    obj.insert(kTag, utils::WriteStringList(tag));
    obj.insert(kVersion, version);

    return obj;
}
