#include "doublespinunitrnonezero.h"

DoubleSpinUnitRNoneZero::DoubleSpinUnitRNoneZero(const int& decimal, const int& unit, CStringMap& unit_symbol_map, QObject* parent)
    : StyledItemDelegate { parent }
    , decimal_ { decimal }
    , unit_ { unit }
    , unit_symbol_map_ { unit_symbol_map }
{
}

void DoubleSpinUnitRNoneZero::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const double value { index.data().toDouble() };

    if (value == 0.0)
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(Format(index), painter, option, index, Qt::AlignRight | Qt::AlignVCenter);
}

QSize DoubleSpinUnitRNoneZero::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    return { CalculateTextSize(Format(index), option, kCoefficient16) };
}

QString DoubleSpinUnitRNoneZero::Format(const QModelIndex& index) const
{
    auto it { unit_symbol_map_.constFind(unit_) };
    auto symbol { (it != unit_symbol_map_.constEnd()) ? it.value() : kEmptyString };

    return symbol + locale_.toString(index.data().toDouble(), 'f', decimal_);
}
