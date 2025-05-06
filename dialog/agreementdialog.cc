#include "agreementdialog.h"

#include "ui_agreementdialog.h"

AgreementDialog::AgreementDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::AgreementDialog)
{
    ui->setupUi(this);

    const QString agreement = tr("Thank you for using this software. To ensure proper usage and protect your rights, "
                                 "please read the following terms carefully before activating the software:\n\n"
                                 "1. Activation Code Instructions\n"
                                 "When activating this software, you will receive a unique activation code. This code is used "
                                 "to verify your purchase and unlock the software features. Please keep it safe and avoid disclosing it.\n\n"
                                 "2. Activation Restrictions\n"
                                 "Each activation code can only be used on one device. Activation requires an internet connection. "
                                 "If the hardware changes, the activation code may become invalid and re-verification will be required.\n\n"
                                 "3. Offline Usage\n"
                                 "You may use this software offline, but you must go online at least once every 7 days for verification.\n\n"
                                 "4. Expired or Invalid Codes\n"
                                 "If your activation code is invalid or expired, please contact us. If the software is updated, "
                                 "re-activation may be required.\n\n"
                                 "5. Returns and Refunds\n"
                                 "We offer a 7-day unconditional return policy. If you return the product within 7 days, "
                                 "your license will be revoked and your activation code invalidated.\n\n"
                                 "6. Privacy\n"
                                 "Your personal information will not be collected or shared without your consent. Activation data "
                                 "is used solely for license verification.\n\n"
                                 "7. Terms Modification\n"
                                 "We reserve the right to modify these terms. Updated terms will apply the next time you start the software.");

    ui->textEdit->setPlainText(agreement);
}

AgreementDialog::~AgreementDialog() { delete ui; }
