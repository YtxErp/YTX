#include "doublespinunitrps.h"

#include <QMouseEvent>

#include "component/enumclass.h"

DoubleSpinUnitRPS::DoubleSpinUnitRPS(const int& decimal, const int& unit, CStringMap& unit_symbol_map, QObject* parent)
    : StyledItemDelegate { parent }
    , decimal_ { decimal }
    , unit_ { unit }
    , unit_symbol_map_ { unit_symbol_map }
{
}

void DoubleSpinUnitRPS::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    PaintText(Format(index), painter, option, index, Qt::AlignRight | Qt::AlignVCenter);
}

QSize DoubleSpinUnitRPS::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    return { CalculateTextSize(Format(index), option, kCoefficient16) };
}

QString DoubleSpinUnitRPS::Format(const QModelIndex& index) const
{
    auto it { unit_symbol_map_.constFind(unit_) };
    auto symbol { (it != unit_symbol_map_.constEnd()) ? it.value() : kEmptyString };

    return symbol + locale_.toString(index.data().toDouble(), 'f', decimal_);
}

bool DoubleSpinUnitRPS::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    const bool leaf { index.siblingAtColumn(std::to_underlying(NodeEnum::kType)).data().toInt() == kTypeLeaf };

    if (leaf && event->type() == QEvent::MouseButtonDblClick && option.rect.contains(static_cast<QMouseEvent*>(event)->pos()))
        emit STransRef(index);

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
