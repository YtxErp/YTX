#include "doublespinr.h"

DoubleSpinR::DoubleSpinR(const int& decimal, bool ignore_zero, QObject* parent)
    : StyledItemDelegate { parent }
    , decimal_ { decimal }
    , ignore_zero_ { ignore_zero }
{
}

void DoubleSpinR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const double value { index.data().toDouble() };
    if (ignore_zero_ && value == 0.0)
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(locale_.toString(value, 'f', decimal_), painter, option, index, Qt::AlignRight | Qt::AlignVCenter);
}

QSize DoubleSpinR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const double value { index.data().toDouble() };
    return CalculateTextSize(locale_.toString(value, 'f', decimal_), option);
}
