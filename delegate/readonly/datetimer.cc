#include "datetimer.h"

DateTimeR::DateTimeR(const QString& date_format, QObject* parent)
    : StyledItemDelegate { parent }
    , date_format_ { date_format }
{
}

void DateTimeR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const auto date_time { index.data().toDateTime() };
    if (!date_time.isValid())
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(date_time.toString(date_format_), painter, option, index, Qt::AlignCenter);
}

QSize DateTimeR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto text { index.data().toDateTime().toString(date_format_) };
    return CalculateTextSize(text, option);
}
