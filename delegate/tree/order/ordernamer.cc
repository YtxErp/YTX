#include "ordernamer.h"

#include <QPainter>

OrderNameR::OrderNameR(CTreeModel* tree_model, QObject* parent)
    : StyledItemDelegate { parent }
    , tree_model_ { tree_model }
{
}

void OrderNameR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QString& text { tree_model_->Name(index.siblingAtColumn(std::to_underlying(NodeEnumO::kParty)).data().toInt()) };
    if (text.isEmpty())
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(text, painter, option, index, Qt::AlignLeft | Qt::AlignVCenter);
}

QSize OrderNameR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QString& text = index.data().toString() + tree_model_->Name(index.siblingAtColumn(std::to_underlying(NodeEnumO::kParty)).data().toInt());
    return CalculateTextSize(text, option);
}
