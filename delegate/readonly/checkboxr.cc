#include "checkboxr.h"

CheckBoxR::CheckBoxR(QObject* parent)
    : StyledItemDelegate { parent }
{
}

void CheckBoxR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (!index.data().toBool())
        return QStyledItemDelegate::paint(painter, option, index);

    PaintCheckBox(painter, option, index);
}
