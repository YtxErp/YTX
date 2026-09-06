#include "auditdialog.h"

#include "component/constantint.h"
#include "component/constantstring.h"
#include "component/signalblocker.h"
#include "global/logininfo.h"
#include "ui_auditdialog.h"
#include "websocket/jsongen.h"
#include "websocket/websocket.h"

AuditDialog::AuditDialog(audit::Model* model, CUuid& widget_id, CString& title, Section section, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::AuditDialog)
    , model_ { model }
    , section_ { section }
    , range_ { DefaultRange() }
    , widget_id_ { widget_id }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    ui->tableView->setModel(model_);
    model_->setParent(ui->tableView);

    InitDialog();
    InitTimer();

    setWindowTitle(tr("Audit") + QStringLiteral(" - ") + title);

    QTimer::singleShot(0, this, &AuditDialog::on_pBtnFetch_clicked);
}

AuditDialog::~AuditDialog() { delete ui; }

QTableView* AuditDialog::View() { return ui->tableView; }

void AuditDialog::on_pBtnFetch_clicked()
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

    const auto message { JsonGen::AuditLogAck(widget_id_, LoginInfo::Instance().Workspace(), query_range, section_) };
    WebSocket::Instance()->SendMessage(WsKey::kAuditLogAck, message);

    cooldown_timer_->start(time_const::kCooldownMs);
}

void AuditDialog::InitTimer()
{
    cooldown_timer_ = new QTimer(this);
    cooldown_timer_->setSingleShot(true);
    connect(cooldown_timer_, &QTimer::timeout, this, [this]() { ui->pBtnFetch->setEnabled(true); });
}

void AuditDialog::on_dateEditStart_dateChanged(const QDate& date)
{
    const bool valid { date <= range_.end };
    range_.start = date;

    cooldown_timer_->stop();
    ui->pBtnFetch->setEnabled(valid);
}

void AuditDialog::on_dateEditEnd_dateChanged(const QDate& date)
{
    const bool valid { date >= range_.start };
    range_.end = date;

    cooldown_timer_->stop();
    ui->pBtnFetch->setEnabled(valid);
}

void AuditDialog::InitDialog()
{
    ui->dateEditStart->setDisplayFormat(datetime_format::kDashedDate);
    ui->dateEditEnd->setDisplayFormat(datetime_format::kDashedDate);

    ui->pBtnFetch->setFocus();

    ui->dateEditStart->setDate(range_.start);
    ui->dateEditEnd->setDate(range_.end);
}
