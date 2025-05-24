#ifndef LICENCE_H
#define LICENCE_H

#include <QDialog>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QSettings>

#include "component/info.h"
#include "component/using.h"

namespace Ui {
class Licence;
}

class Licence : public QDialog {
    Q_OBJECT

public:
    explicit Licence(QSharedPointer<QSettings> license_settings, LicenseInfo& license_info, QWidget* parent = nullptr);
    ~Licence();

    static bool VerifySignature(const QByteArray& payload, const QByteArray& signature, const QString& public_key_path);

private slots:
    void on_pBtnActivate_clicked();
    void on_pBtnAgreement_clicked();
    void on_chkBoxAgree_checkStateChanged(const Qt::CheckState& arg1);

private:
    bool IsActivationCodeValid(CString& activation_code);
    void SaveActivationCode(CString& activation_code, CString& signature);
    void UpdateActivationUI() const;

private:
    Ui::Licence* ui;

    QNetworkAccessManager* network_manager_ {};
    QSharedPointer<QSettings> license_settings_ {};
    LicenseInfo& license_info_;
};

#endif // LICENCE_H
