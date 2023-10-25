#include "doublespinr.h"

DoubleSpinR::DoubleSpinR(const int& decimal, int coefficient, QObject* parent)
    : StyledItemDelegate { parent }
    , decimal_ { decimal }
    , coefficient_ { coefficient }
{
}

void DoubleSpinR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const double value { index.data().toDouble() };
    PaintText(locale_.toString(value, 'f', decimal_), painter, option, index, Qt::AlignRight | Qt::AlignVCenter);
}

QSize DoubleSpinR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const double value { index.data().toDouble() };
    return CalculateTextSize(locale_.toString(value, 'f', decimal_), option, coefficient_);
}
