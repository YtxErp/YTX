#include "color.h"

#include <QColorDialog>
#include <QKeyEvent>
#include <QPainter>

Color::Color(QObject* parent)
    : StyledItemDelegate { parent }
{
}

void Color::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
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
    painter->setPen(Qt::NoPen);

    QColor background_color { (option.state & QStyle::State_Selected) ? option.palette.highlight().color() : option.palette.window().color() };

    painter->setBrush(background_color);
    painter->drawRect(option.rect);

    // 设置画笔颜色
    painter->setBrush(color);
    painter->drawRoundedRect(color_rect, 2, 2);

    painter->restore();
}

bool Color::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    if (event->type() != QEvent::MouseButtonDblClick)
        return StyledItemDelegate::editorEvent(event, model, option, index);

    auto* mouse_event { static_cast<QMouseEvent*>(event) };
    if (!mouse_event || mouse_event->button() != Qt::LeftButton || !option.rect.contains(mouse_event->pos()))
        return false;

    QColor color(index.data().toString());
    if (!color.isValid())
        color = Qt::white;

    QColor selected_color { QColorDialog::getColor(color, nullptr, tr("Choose Color"), QColorDialog::ShowAlphaChannel) };

    if (selected_color.isValid())
        model->setData(index, selected_color.name(QColor::HexRgb));

    return true;
}
