#include "searchpathtreer.h"

SearchPathTreeR::SearchPathTreeR(CTreeModel* model, int column, QObject* parent)
    : StyledItemDelegate { parent }
    , model_ { model }
    , column_ { column }
{
}

void SearchPathTreeR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    PaintText(GetPath(index), painter, option, index, Qt::AlignLeft | Qt::AlignVCenter);
}

QSize SearchPathTreeR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const { return CalculateTextSize(GetPath(index), option); }

QString SearchPathTreeR::GetPath(const QModelIndex& index) const { return model_->GetPath(index.siblingAtColumn(column_).data().toInt()); }
