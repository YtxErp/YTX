#include "licence.h"

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>

#include "component/constvalue.h"
#include "ui_licence.h"

Licence::Licence(QPointer<QNetworkAccessManager> network_manager, QSharedPointer<QSettings> license_settings, CString& hardware_uuid, QString& activation_code,
    QString& activation_url, bool& is_activated, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::Licence)
    , network_manager_(network_manager)
    , license_settings_ { license_settings }
    , hardware_uuid_ { hardware_uuid }
    , activation_code_ { activation_code }
    , activation_url_ { activation_url }
    , is_activated_ { is_activated }
{
    ui->setupUi(this);

    ui->lineEdit->setText(activation_code_);
    UpdateActivationUI();
}

Licence::~Licence() { delete ui; }

void Licence::on_pBtnActivate_clicked()
{
    const QString activation_code { ui->lineEdit->text() };
    if (!network_manager_ || !IsActivationCodeValid(activation_code)) {
        return;
    }

    // Prepare the request
    const QUrl url(activation_url_ + QDir::separator() + kActivate);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Create JSON payload
    const QJsonObject json { { "hardware_uuid", hardware_uuid_ }, { "activation_code", activation_code } };

    // Send the request
    QNetworkReply* reply { network_manager_->post(request, QJsonDocument(json).toJson()) };

    // Handle response
    connect(reply, &QNetworkReply::finished, this, [this, reply, activation_code]() {
        if (reply->error() == QNetworkReply::NoError) {
            QMessageBox::information(this, tr("Success"), tr("Activation Successful!"));
            SaveActivationCode(activation_code);
        } else {
            QMessageBox::critical(this, tr("Fail"), tr("Activation Failed!"));
        }
        reply->deleteLater();
    });

    // Handle timeout
    connect(reply, &QNetworkReply::errorOccurred, this, [this](QNetworkReply::NetworkError error) {
        if (error == QNetworkReply::TimeoutError) {
            QMessageBox::critical(this, tr("Error"), tr("Connection timeout. Please try again."));
        }
    });
}

bool Licence::IsActivationCodeValid(CString& activation_code)
{
    static const QRegularExpression uuid_regex("^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$");

    if (!uuid_regex.match(activation_code).hasMatch()) {
        QMessageBox::warning(this, tr("Invalid Activation Code"), tr("The activation code must be a valid UUID format (8-4-4-4-12 hexadecimal characters)."));
        return false;
    }
    return true;
}

void Licence::SaveActivationCode(const QString& activation_code)
{
    activation_code_ = activation_code;
    is_activated_ = true;

    license_settings_->beginGroup(kLicense);
    license_settings_->setValue(kActivationCode, activation_code_);
    license_settings_->setValue(kActivationUrl, activation_url_);
    license_settings_->endGroup();

    UpdateActivationUI();
}

void Licence::UpdateActivationUI() const
{
    if (is_activated_) {
        ui->lineEdit->setReadOnly(true);
        ui->pBtnActivate->setText(tr("Activated"));
        ui->pBtnActivate->setEnabled(false);
    }
}
