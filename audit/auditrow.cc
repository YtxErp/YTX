#include "auditrow.h"

#include "component/constant.h"

namespace audit::keys {
constexpr QLatin1StringView kTargetId { "target_id" };
constexpr QLatin1StringView kTargetType { "target_type" };
constexpr QLatin1StringView kTargetOperation { "target_operation" };
constexpr QLatin1StringView kTargetCode { "target_code" };
constexpr QLatin1StringView kBefore { "before" };
constexpr QLatin1StringView kAfter { "after" };
}

void audit::Row::Reset() { *this = Row {}; }

void audit::Row::ReadJson(const QJsonObject& object)
{
    using namespace audit::keys;

    if (const auto val = object.value(kId); val.isString())
        id = QUuid(val.toString());
    if (const auto val = object.value(kTargetId); val.isString())
        target_id = QUuid(val.toString());
    if (const auto val = object.value(kUsername); val.isString())
        username = val.toString();
    if (const auto val = object.value(kLhsNode); val.isString())
        lhs_node = QUuid(val.toString());
    if (const auto val = object.value(kRhsNode); val.isString())
        rhs_node = QUuid(val.toString());
    if (const auto val = object.value(kCreatedTime); val.isString())
        created_time = QDateTime::fromString(val.toString(), Qt::ISODate).toLocalTime();
    if (const auto val = object.value(kTargetOperation); val.isDouble())
        target_operation = val.toInt();
    if (const auto val = object.value(kTargetType); val.isDouble())
        target_type = val.toInt();
    if (const auto val = object.value(kLevel); val.isDouble())
        level = val.toInt();
    if (const auto val = object.value(kTargetCode); val.isString())
        target_code = val.toString();
    if (const auto val = object.value(kBefore); !val.isUndefined())
        before = val;
    if (const auto val = object.value(kAfter); !val.isUndefined())
        after = val;
    if (const auto val = object.value(kTargetField); val.isDouble())
        target_field = val.toInt();
}
