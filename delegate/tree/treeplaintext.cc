#include "treeplaintext.h"

#include <QApplication>
#include <QRect>

#include "widget/plaintextedit.h"

TreePlainText::TreePlainText(QObject* parent)
    : StyledItemDelegate { parent }
{
}

QWidget* TreePlainText::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    auto* editor { new PlainTextEdit(parent) };
    return editor;
}

void TreePlainText::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto* cast_editor { static_cast<PlainTextEdit*>(editor) };
    cast_editor->setPlainText(index.data().toString());
}

void TreePlainText::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(index);

    QSize mainwindow_size { qApp->activeWindow()->size() };
    int width { mainwindow_size.width() * 150 / 1920 };
    int height { mainwindow_size.height() * 200 / 1080 };

    editor->setGeometry(QRect(option.rect.bottomLeft(), QSize(width, height)));
}

void TreePlainText::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto* cast_editor { static_cast<PlainTextEdit*>(editor) };
    model->setData(index, cast_editor->toPlainText());
}
