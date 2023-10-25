#include "financeforeignr.h"

#include "component/enumclass.h"

FinanceForeignR::FinanceForeignR(const int& decimal, const int& default_unit, CStringMap& unit_symbol_map, QObject* parent)
    : StyledItemDelegate { parent }
    , decimal_ { decimal }
    , default_unit_ { default_unit }
    , unit_symbol_map_ { unit_symbol_map }
{
}

void FinanceForeignR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    PaintText(Format(index), painter, option, index, Qt::AlignRight | Qt::AlignVCenter);
}

QSize FinanceForeignR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const { return CalculateTextSize(Format(index), option); }

QString FinanceForeignR::Format(const QModelIndex& index) const
{
    int unit { index.siblingAtColumn(std::to_underlying(TreeEnum::kUnit)).data().toInt() };
    if (unit == default_unit_)
        return kEmptyString;

    double value { index.data().toDouble() };

    auto it { unit_symbol_map_.constFind(unit) };
    auto symbol { (it != unit_symbol_map_.constEnd()) ? it.value() : kEmptyString };

    return symbol + locale_.toString(value, 'f', decimal_);
}
