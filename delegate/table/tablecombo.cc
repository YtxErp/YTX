#include "tablecombo.h"

#include "widget/combobox.h"

TableCombo::TableCombo(CTreeModel* tree_model, SortFilterProxyModel* filter_model, QObject* parent)
    : StyledItemDelegate { parent }
    , tree_model_ { tree_model }
    , filter_model_ { filter_model }
{
}

QWidget* TableCombo::createEditor(QWidget* parent, const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/) const
{
    auto* editor { new ComboBox(parent) };
    editor->setModel(filter_model_);

    return editor;
}

void TableCombo::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto* cast_editor { static_cast<ComboBox*>(editor) };

    int key { index.data().toInt() };
    if (key == 0)
        key = last_insert_;

    int item_index { cast_editor->findData(key) };
    cast_editor->setCurrentIndex(item_index);
}

void TableCombo::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto* cast_editor { static_cast<ComboBox*>(editor) };

    int key { cast_editor->currentData().toInt() };
    last_insert_ = key;
    model->setData(index, key);
}

void TableCombo::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QString path { tree_model_->GetPath(index.data().toInt()) };
    if (path.isEmpty())
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(path, painter, option, index, Qt::AlignLeft | Qt::AlignVCenter);

    // 高度自定义
    // const int text_margin { CalculateTextMargin(style, opt.widget) };
    // const QRect text_rect { opt.rect.adjusted(text_margin, 0, -text_margin, 0) };
    // painter->drawText(text_rect, Qt::AlignLeft | Qt::AlignVCenter, path);
}

QSize TableCombo::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QString text = tree_model_->GetPath(index.data().toInt());
    return CalculateTextSize(text, option);
}

void TableCombo::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QSize text_size { CalculateTextSize(tree_model_->GetPath(index.data().toInt()), option) };
    const int width { std::max(option.rect.width(), text_size.width()) };
    const int height { std::max(option.rect.height(), text_size.height()) };

    editor->setFixedHeight(height);
    editor->setMinimumWidth(width);
    editor->setGeometry(option.rect);
}
