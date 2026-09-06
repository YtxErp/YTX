#include "treewidgeto.h"

#include "component/constantstring.h"
#include "component/constantwebsocket.h"
#include "component/signalblocker.h"
#include "ui_treewidgeto.h"
#include "websocket/jsongen.h"
#include "websocket/websocket.h"

TreeWidgetO::TreeWidgetO(Section section, TreeModel* model, QWidget* parent)
    : TreeWidget(parent)
    , ui(new Ui::TreeWidgetO)
    , section_ { section }
    , model_ { model }
    , range_ { DefaultRange() }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    InitWidget();
    InitTimer();

    ui->treeViewO->setModel(model);
    model->setParent(ui->treeViewO);
}

TreeWidgetO::~TreeWidgetO() { delete ui; }

QTreeView* TreeWidgetO::View() const { return ui->treeViewO; }

void TreeWidgetO::Reset()
{
    range_ = DefaultRange();

    ui->start->setDate(range_.start);
    ui->end->setDate(range_.end);
}

void TreeWidgetO::on_start_dateChanged(const QDate& date)
{
    const bool valid { date <= range_.end };
    range_.start = date;

    cooldown_timer_->stop();
    ui->pBtnFetch->setEnabled(valid);
}

void TreeWidgetO::on_end_dateChanged(const QDate& date)
{
    const bool valid { date >= range_.start };
    range_.end = date;

    cooldown_timer_->stop();
    ui->pBtnFetch->setEnabled(valid);
}

void TreeWidgetO::on_pBtnFetch_clicked()
{
    if (!ui->pBtnFetch->isEnabled()) {
        return;
    }

    if (!range_.IsValid()) {
        return;
    }

    ui->pBtnFetch->setEnabled(false);

    qDebug() << Q_FUNC_INFO << "DateRange:" << range_.ToString();

    const auto query_range { range_.ToQueryRange() };

    qDebug() << Q_FUNC_INFO << "QueryRange:" << query_range.ToString();

    const auto message { JsonGen::TreeAck(section_, query_range) };
    WebSocket::Instance()->SendMessage(WsKey::kOrderTreeAck, message);

    cooldown_timer_->start(time_const::kCooldownMs);
}

void TreeWidgetO::InitWidget()
{
    ui->start->setDisplayFormat(datetime_format::kDashedDate);
    ui->end->setDisplayFormat(datetime_format::kDashedDate);

    ui->start->setDate(range_.start);
    ui->end->setDate(range_.end);
}

void TreeWidgetO::InitTimer()
{
    cooldown_timer_ = new QTimer(this);
    cooldown_timer_->setSingleShot(true);
    connect(cooldown_timer_, &QTimer::timeout, this, [this]() { ui->pBtnFetch->setEnabled(true); });
}
