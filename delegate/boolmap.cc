#include "boolmap.h"

#include <QMouseEvent>

BoolMap::BoolMap(CStringMap& map, QEvent::Type type, QObject* parent)
    : StyledItemDelegate { parent }
    , map_ { map }
    , type_ { type }
{
}

void BoolMap::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    PaintText(MapValue(index.data().toInt()), painter, option, index, Qt::AlignCenter);
}

bool BoolMap::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    if (event->type() != type_)
        return false;

    auto* mouse_event { static_cast<QMouseEvent*>(event) };
    if (mouse_event->button() != Qt::LeftButton || !option.rect.contains(mouse_event->pos()))
        return false;

    const bool checked { index.data().toBool() };
    return model->setData(index, !checked, Qt::EditRole);
}

QString BoolMap::MapValue(int key) const
{
    auto it { map_.constFind(key) };
    return (it != map_.constEnd()) ? it.value() : kEmptyString;
}
