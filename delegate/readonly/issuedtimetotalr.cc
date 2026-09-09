#include "issuedtimetotalr.h"

#include <QtCore/qdatetime.h>

IssuedTimeTotalR::IssuedTimeTotalR(const QString& date_format, QObject* parent)
    : StyledItemDelegate { parent }
    , date_format_ { date_format }
{
}

void IssuedTimeTotalR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    PaintText(DisplayText(index), painter, option, index, Qt::AlignCenter);
}

QSize IssuedTimeTotalR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const { return CalculateTextSize(DisplayText(index), option); }

QString IssuedTimeTotalR::DisplayText(const QModelIndex& index) const
{
    const auto data { index.data() };
    const auto issued_time { data.toDateTime() };

    return issued_time.isValid() ? issued_time.toString(date_format_) : data.toString();
}
