#include "licence.h"

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>

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
#include "component/signalblocker.h"
#include "dialog/agreementdialog.h"
#include "ui_licence.h"

Licence::Licence(QSharedPointer<QSettings> license_settings, CString& hardware_uuid, CString& activation_url, QString& activation_code, QString& signature,
    bool& is_activated, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::Licence)
    , network_manager_(new QNetworkAccessManager(this))
    , license_settings_ { license_settings }
    , hardware_uuid_ { hardware_uuid }
    , activation_url_ { activation_url }
    , activation_code_ { activation_code }
    , signature_ { signature }
    , is_activated_ { is_activated }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    ui->lineEdit->setText(activation_code_);
    ui->pBtnActivate->setEnabled(false);
    UpdateActivationUI();
}

Licence::~Licence() { delete ui; }

void Licence::on_pBtnActivate_clicked()
{
    const QString activation_code { ui->lineEdit->text() };
    if (!IsActivationCodeValid(activation_code)) {
        return;
    }

    // Prepare the request
    const QUrl url(activation_url_ + "/" + kActivate);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Create JSON payload
    const QJsonObject json { { "hardware_uuid", hardware_uuid_ }, { "activation_code", activation_code } };

    // Send the request
    QNetworkReply* reply { network_manager_->post(request, QJsonDocument(json).toJson()) };

    // Handle response
    connect(reply, &QNetworkReply::finished, this, [this, reply, activation_code]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::critical(this, tr("Fail"), tr("Activation Failed!"));
            return;
        }

        const QJsonDocument doc { QJsonDocument::fromJson(reply->readAll()) };
        if (!doc.isObject()) {
            QMessageBox::critical(this, tr("Error"), tr("Invalid server response."));
            return;
        }

        const QJsonObject obj { doc.object() };
        const bool success { obj["success"].toBool() };
        const QString signature { obj["signature"].toString() };

        // Construct payload in the same format as server
        const QString payload { QString("%1:%2:%3").arg(activation_code, hardware_uuid_, success ? "true" : "false") };
        const QByteArray payload_bytes { payload.toUtf8() };
        const QByteArray signature_bytes { QByteArray::fromBase64(signature.toUtf8()) };

        // Read public key
        const QString pub_key_path(":/keys/public.pem");

        const bool ok { VerifySignature(payload_bytes, signature_bytes, pub_key_path) };
        if (ok) {
            QMessageBox::information(this, tr("Success"), tr("Activation Successful!"));
            SaveActivationCode(activation_code, signature);
        } else {
            QMessageBox::critical(this, tr("Fail"), tr("Activation Failed!"));
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

void Licence::SaveActivationCode(CString& activation_code, CString& signature)
{
    activation_code_ = activation_code;
    signature_ = signature;
    is_activated_ = true;

    license_settings_->beginGroup(kLicense);
    license_settings_->setValue(kActivationCode, activation_code_);
    license_settings_->setValue(kSignature, signature_);
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
        ui->chkBoxAgree->setCheckState(Qt::Checked);
        ui->chkBoxAgree->setEnabled(false);
    }
}

bool Licence::VerifySignature(const QByteArray& payload, const QByteArray& signature, const QString& public_key_path)
{
    QFile file(public_key_path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open public key resource:" << public_key_path;
        return false;
    }
    const QByteArray key_data { file.readAll() };
    file.close();

    BIO* bio = BIO_new_mem_buf(key_data.data(), key_data.size());
    if (!bio) {
        qWarning() << "Failed to create BIO:" << ERR_error_string(ERR_get_error(), nullptr);
        return false;
    }

    EVP_PKEY* pub_key = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if (!pub_key) {
        qWarning() << "Failed to read public key:" << ERR_error_string(ERR_get_error(), nullptr);
        return false;
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        EVP_PKEY_free(pub_key);
        qWarning() << "Failed to create digest context:" << ERR_error_string(ERR_get_error(), nullptr);
        return false;
    }

    bool result = false;

    if (EVP_DigestVerifyInit(ctx, nullptr, EVP_sha256(), nullptr, pub_key) == 1) {
        if (EVP_DigestVerifyUpdate(ctx, payload.constData(), payload.size()) == 1) {
            const int verify_ok { EVP_DigestVerifyFinal(ctx, reinterpret_cast<const unsigned char*>(signature.constData()), signature.size()) };
            result = (verify_ok == 1);
        }
    }

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pub_key);

    if (!result) {
        qWarning() << "Signature verification failed:" << ERR_error_string(ERR_get_error(), nullptr);
    }

    return result;
}

void Licence::on_pBtnAgreement_clicked()
{
    AgreementDialog dlg(this);
    dlg.exec();
}

void Licence::on_chkBoxAgree_checkStateChanged(const Qt::CheckState& arg1) { ui->pBtnActivate->setEnabled(arg1 == Qt::Checked); }
