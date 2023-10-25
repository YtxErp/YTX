#include "styleditemdelegate.h"

#include <QApplication>
#include <QFontMetrics>

const QLocale StyledItemDelegate::locale_ { QLocale::English, QLocale::UnitedStates };

StyledItemDelegate::StyledItemDelegate(QObject* parent)
    : QStyledItemDelegate { parent }
{
}

void StyledItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& /*index*/) const
{
    editor->setFixedHeight(option.rect.height());
    editor->setFixedWidth(option.rect.width());
    editor->setGeometry(option.rect);
}

QSize StyledItemDelegate::CalculateTextSize(CString& text, const QStyleOptionViewItem& option)
{
    const int text_margin { QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, option.widget) };
    const QFontMetrics fm(option.font);
    const int width { std::max(fm.horizontalAdvance(text) + coefficient_ * text_margin, option.rect.width()) };
    const int height { std::max(fm.height(), option.rect.height()) };

    return QSize(width, height);
}

void StyledItemDelegate::PaintText(
    CString& text, QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index, Qt::Alignment alignment) const
{
    QStyleOptionViewItem opt(option);
    opt.displayAlignment = alignment;

    initStyleOption(&opt, index);

    opt.text = text;

    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
}

void StyledItemDelegate::PaintCheckBox(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt { option };
    initStyleOption(&opt, index);

    auto* style { QApplication::style() };
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);

    QStyleOptionButton check_box {};
    check_box.state = option.state;
    check_box.state |= index.data().toBool() ? QStyle::State_On : QStyle::State_Off;

    auto rect { style->subElementRect(QStyle::SE_CheckBoxIndicator, &check_box, opt.widget) };
    rect.moveCenter(opt.rect.center());
    check_box.rect = rect;

    style->drawPrimitive(QStyle::PE_IndicatorCheckBox, &check_box, painter, option.widget);
}
