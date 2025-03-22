#include "sectionr.h"

SectionR::SectionR(QObject* parent)
    : StyledItemDelegate { parent }
{
}

void SectionR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const int section { index.data().toInt() };
    const QString text { section == 4 ? QObject::tr("Sales") : QObject::tr("Purchase") };
    PaintText(text, painter, option, index, Qt::AlignCenter);
}
