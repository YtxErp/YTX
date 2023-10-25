#include "colorr.h"

#include <QPainter>

ColorR::ColorR(QObject* parent)
    : StyledItemDelegate { parent }
{
}

void ColorR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QString string { index.data().toString() };
    if (string.isEmpty())
        return QStyledItemDelegate::paint(painter, option, index);

    QColor color(string);
    if (!color.isValid())
        return QStyledItemDelegate::paint(painter, option, index);

    QRect color_rect { option.rect.adjusted(2, 2, -2, -2) };

    // 保存当前的 painter 状态
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // 设置画笔颜色
    painter->setBrush(color);
    painter->setPen(Qt::NoPen);

    // 在单元格矩形内填充颜色
    painter->drawRoundedRect(color_rect, 2, 2);
    painter->restore();
}
