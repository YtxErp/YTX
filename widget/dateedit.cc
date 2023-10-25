#include "dateedit.h"

#include <QKeyEvent>

DateEdit::DateEdit(QWidget* parent)
    : QDateEdit { parent }
{
    setButtonSymbols(QAbstractSpinBox::NoButtons);
    setAlignment(Qt::AlignCenter);
}

void DateEdit::keyPressEvent(QKeyEvent* event)
{
    const int key { event->key() };
    QDate date { this->date() };

    if (HandleSpecialKeys(key, date)) {
        setDate(date);
        event->accept();
        return;
    }

    QDateEdit::keyPressEvent(event);
}

bool DateEdit::LastMonthEnd(QDate& date)
{
    date = date.addMonths(-1);
    date.setDate(date.year(), date.month(), date.daysInMonth());
    return true;
}

bool DateEdit::NextMonthStart(QDate& date)
{
    date = date.addMonths(1);
    date.setDate(date.year(), date.month(), 1);
    return true;
}

bool DateEdit::HandleSpecialKeys(int key, QDate& date)
{
    switch (key) {
    case Qt::Key_J:
        return AdjustDate(date, -1);
    case Qt::Key_K:
        return AdjustDate(date, 1);
    case Qt::Key_W:
        return AdjustDate(date, -7);
    case Qt::Key_B:
        return AdjustDate(date, 7);
    case Qt::Key_H:
        return LastMonthEnd(date);
    case Qt::Key_L:
        return NextMonthStart(date);
    case Qt::Key_N:
        return AdjustDate(date, 0, 0, 1);
    case Qt::Key_E:
        return AdjustDate(date, 0, 0, -1);
    case Qt::Key_T:
        return SetToCurrentDate(date);
    default:
        return false;
    }
}

bool DateEdit::AdjustDate(QDate& date, int days, int months, int years)
{
    date = date.addDays(days).addMonths(months).addYears(years);
    return true;
}

bool DateEdit::SetToCurrentDate(QDate& date)
{
    date = QDate::currentDate();
    return true;
}
