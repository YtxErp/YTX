#include "datetimeedit.h"

#include <QKeyEvent>

DateTimeEdit::DateTimeEdit(QWidget* parent)
    : QDateTimeEdit { parent }
{
    this->setButtonSymbols(QAbstractSpinBox::NoButtons);
    this->setAlignment(Qt::AlignCenter);
}

void DateTimeEdit::keyPressEvent(QKeyEvent* event)
{
    const int key { event->key() };
    QDateTime date_time { this->dateTime() };

    if (HandleSpecialKeys(key, date_time)) {
        setDateTime(date_time);
        event->accept();
        return;
    }
    QDateTimeEdit::keyPressEvent(event);
}

bool DateTimeEdit::LastMonthEnd(QDateTime& date_time)
{
    QDate date { date_time.date().addMonths(-1) };
    date.setDate(date.year(), date.month(), date.daysInMonth());
    date_time.setDate(date);
    return true;
}

bool DateTimeEdit::NextMonthStart(QDateTime& date_time)
{
    QDate date { date_time.date().addMonths(1) };
    date.setDate(date.year(), date.month(), 1);
    date_time.setDate(date);
    return true;
}

bool DateTimeEdit::HandleSpecialKeys(int key, QDateTime& date_time)
{
    switch (key) {
    case Qt::Key_J:
        return AdjustDateTime(date_time, -1);
    case Qt::Key_K:
        return AdjustDateTime(date_time, 1);
    case Qt::Key_W:
        return AdjustDateTime(date_time, -7);
    case Qt::Key_B:
        return AdjustDateTime(date_time, 7);
    case Qt::Key_H:
        return LastMonthEnd(date_time);
    case Qt::Key_L:
        return NextMonthStart(date_time);
    case Qt::Key_N:
        return AdjustDateTime(date_time, 0, 0, 1);
    case Qt::Key_E:
        return AdjustDateTime(date_time, 0, 0, -1);
    case Qt::Key_T:
        return SetToCurrentDateTime(date_time);
    default:
        return false;
    }
}

bool DateTimeEdit::AdjustDateTime(QDateTime& date_time, int days, int months, int years)
{
    date_time = date_time.addDays(days).addMonths(months).addYears(years);
    return true;
}

bool DateTimeEdit::SetToCurrentDateTime(QDateTime& date_time)
{
    date_time = QDateTime::currentDateTime();
    return true;
}
