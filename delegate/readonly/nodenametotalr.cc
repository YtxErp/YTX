#include "nodenametotalr.h"

#include <QPainter>

NodeNameTotalR::NodeNameTotalR(CTreeModel* tree_model, QObject* parent)
    : StyledItemDelegate { parent }
    , tree_model_ { tree_model }
{
}

void NodeNameTotalR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QString text { DisplayText(index) };

    if (text.isEmpty())
        return PaintEmpty(painter, option, index);

    PaintText(text, painter, option, index, Qt::AlignLeft | Qt::AlignVCenter);
}

QSize NodeNameTotalR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const { return CalculateTextSize(DisplayText(index), option); }

QString NodeNameTotalR::DisplayText(const QModelIndex& index) const
{
    const auto data { index.data() };
    const QUuid id { data.toUuid() };

    return id.isNull() ? data.toString() : tree_model_->Name(id);
}
