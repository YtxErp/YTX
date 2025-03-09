#include "nodepathr.h"

#include <QPainter>

NodePathR::NodePathR(CNodeModel* tree_model, QObject* parent)
    : StyledItemDelegate { parent }
    , tree_model_ { tree_model }
{
}

void NodePathR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QString& text { tree_model_->GetPath(index.data().toInt()) };
    if (text.isEmpty())
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(text, painter, option, index, Qt::AlignLeft | Qt::AlignVCenter);
}

QSize NodePathR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QString& text = tree_model_->GetPath(index.data().toInt());
    return CalculateTextSize(text, option);
}
