#include "line.h"

#include "widget/lineedit.h"

Line::Line(QObject* parent)
    : StyledItemDelegate { parent }
{
}

QWidget* Line::createEditor(QWidget* parent, const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/) const { return new LineEdit(parent); }

void Line::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto* cast_editor { static_cast<LineEdit*>(editor) };
    cast_editor->setText(index.data().toString());
}

void Line::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto* cast_editor { static_cast<LineEdit*>(editor) };
    model->setData(index, cast_editor->text());
}
