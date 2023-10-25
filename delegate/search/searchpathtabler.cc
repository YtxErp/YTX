#include "searchpathtabler.h"

SearchPathTableR::SearchPathTableR(CTreeModel* model, QObject* parent)
    : StyledItemDelegate { parent }
    , model_ { model }
{
}

void SearchPathTableR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    PaintText(GetPath(index), painter, option, index, Qt::AlignLeft | Qt::AlignVCenter);
}

QSize SearchPathTableR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const { return CalculateTextSize(GetPath(index), option); }

QString SearchPathTableR::GetPath(const QModelIndex& index) const { return model_->GetPath(index.data().toInt()); }
