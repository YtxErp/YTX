#include "stringmapr.h"

#include <widget/combobox.h>

StringMapR::StringMapR(CStringMap& map, QObject* parent)
    : StyledItemDelegate { parent }
    , map_ { map }
{
}

void StringMapR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    PaintText(MapValue(index.data().toInt()), painter, option, index, Qt::AlignCenter);
}

QSize StringMapR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QString text = MapValue(index.data().toInt());
    return CalculateTextSize(text, option);
}

QString StringMapR::MapValue(int key) const
{
    auto it { map_.constFind(key) };
    return (it != map_.constEnd()) ? it.value() : kEmptyString;
}
