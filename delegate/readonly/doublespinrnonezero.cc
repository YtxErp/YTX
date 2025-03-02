#include "doublespinrnonezero.h"

DoubleSpinRNoneZero::DoubleSpinRNoneZero(const int& decimal, int coefficient, QObject* parent)
    : StyledItemDelegate { parent }
    , decimal_ { decimal }
    , coefficient_ { coefficient }
{
}

void DoubleSpinRNoneZero::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const double value { index.data().toDouble() };

    if (value == 0.0)
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(locale_.toString(value, 'f', decimal_), painter, option, index, Qt::AlignRight | Qt::AlignVCenter);
}

QSize DoubleSpinRNoneZero::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const double value { index.data().toDouble() };
    return CalculateTextSize(locale_.toString(value, 'f', decimal_), option, coefficient_);
}
