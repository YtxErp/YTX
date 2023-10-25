#include "taxrate.h"

#include "component/constvalue.h"
#include "widget/doublespinbox.h"

TaxRate::TaxRate(const int& decimal, double min, double max, QObject* parent)
    : StyledItemDelegate { parent }
    , decimal_ { decimal }
    , max_ { max }
    , min_ { min }
{
}

QWidget* TaxRate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    auto* editor { new DoubleSpinBox(parent) };
    editor->setSuffix(kSuffixPERCENT);
    editor->setDecimals(decimal_);
    editor->setMinimum(min_);
    editor->setMaximum(max_);

    return editor;
}

void TaxRate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto* cast_editor { static_cast<DoubleSpinBox*>(editor) };
    cast_editor->setValue(index.data().toDouble() * kHundred);
}

void TaxRate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto* cast_editor { static_cast<DoubleSpinBox*>(editor) };
    model->setData(index, cast_editor->value() / kHundred);
}

void TaxRate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const double value { index.data().toDouble() * kHundred };
    if (value == 0)
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(locale_.toString(value, 'f', decimal_) + kSuffixPERCENT, painter, option, index, Qt::AlignRight | Qt::AlignVCenter);
}

QSize TaxRate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const double value { index.data().toDouble() * kHundred };
    return CalculateTextSize(locale_.toString(value, 'f', decimal_) + kSuffixPERCENT, option);
}
