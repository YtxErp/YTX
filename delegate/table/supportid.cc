#include "supportid.h"

#include <QPainter>

#include "widget/combobox.h"

SupportID::SupportID(CTreeModel* tree_model, QObject* parent)
    : StyledItemDelegate { parent }
    , tree_model_ { tree_model }
    , support_model_ { tree_model->SupportModel() }
{
}

QWidget* SupportID::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& /*index*/) const
{
    Q_UNUSED(option);

    auto* editor { new ComboBox(parent) };
    editor->setModel(support_model_);

    return editor;
}

void SupportID::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto* cast_editor { static_cast<ComboBox*>(editor) };
    int key { index.data().toInt() };
    int item_index { cast_editor->findData(key) };
    cast_editor->setCurrentIndex(item_index);
}

void SupportID::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto* cast_editor { static_cast<ComboBox*>(editor) };
    int key { cast_editor->currentData().toInt() };
    model->setData(index, key);
}

void SupportID::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QString& text { tree_model_->GetPath(index.data().toInt()) };
    if (text.isEmpty())
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(text, painter, option, index, Qt::AlignLeft | Qt::AlignVCenter);

    // 高度自定义
    // const int text_margin { CalculateTextMargin(style, opt.widget) };
    // const QRect text_rect { opt.rect.adjusted(text_margin, 0, -text_margin, 0) };
    // painter->drawText(text_rect, Qt::AlignLeft | Qt::AlignVCenter, path);
}

QSize SupportID::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QString& text = tree_model_->GetPath(index.data().toInt());
    return CalculateTextSize(text, option);
}

void SupportID::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QSize text_size { CalculateTextSize(tree_model_->GetPath(index.data().toInt()), option) };
    const int width { std::max(option.rect.width(), text_size.width()) };
    const int height { std::max(option.rect.height(), text_size.height()) };

    editor->setFixedHeight(height);
    editor->setMinimumWidth(width);
    editor->setGeometry(option.rect);
}
