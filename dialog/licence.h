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
    explicit Licence(QPointer<QNetworkAccessManager> network_manager, QSharedPointer<QSettings> license_settings, CString& hardware_uuid,
        QString& activation_code, QString& activation_url, bool& is_activated, QWidget* parent = nullptr);
    ~Licence();

private slots:
    void on_pBtnActivate_clicked();

private:
    bool IsActivationCodeValid(CString& activation_code);
    void SaveActivationCode(const QString& activation_code);
    void UpdateActivationUI() const;

private:
    Ui::Licence* ui;
    QPointer<QNetworkAccessManager> network_manager_ {};
    QSharedPointer<QSettings> license_settings_ {};
    CString& hardware_uuid_ {};
    QString& activation_code_;
    QString& activation_url_;
    bool& is_activated_;
};

#endif // LICENCE_H
