#include "spin.h"

#include "widget/spinbox.h"

Spin::Spin(int min, int max, QObject* parent)
    : StyledItemDelegate { parent }
    , max_ { max }
    , min_ { min }
{
}

QWidget* Spin::createEditor(QWidget* parent, const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/) const
{
    auto* editor { new SpinBox(parent) };
    editor->setMinimum(min_);
    editor->setMaximum(max_);

    return editor;
}

void Spin::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto* cast_editor { static_cast<SpinBox*>(editor) };
    cast_editor->setValue(index.data().toInt());
}

void Spin::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto* cast_editor { static_cast<SpinBox*>(editor) };
    model->setData(index, cast_editor->value());
}

void Spin::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const int value { index.data().toInt() };
    if (value == 0)
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(locale_.toString(value), painter, option, index, Qt::AlignVCenter | Qt::AlignRight);
}

QSize Spin::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const int value { index.data().toInt() };
    return CalculateTextSize(locale_.toString(value), option);
}
