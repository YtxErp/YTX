#ifndef LICENCE_H
#define LICENCE_H

#include <QDialog>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QSettings>

#include "component/using.h"

namespace Ui {
class Licence;
}

class Licence : public QDialog {
    Q_OBJECT

public:
    explicit Licence(QSharedPointer<QSettings> license_settings, CString& hardware_uuid, CString& activation_url, QString& activation_code, QString& signature,
        bool& is_activated, QWidget* parent = nullptr);
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

    CString& hardware_uuid_;
    CString& activation_url_;

    QString& activation_code_;
    QString& signature_;
    bool& is_activated_;
};

#endif // LICENCE_H
