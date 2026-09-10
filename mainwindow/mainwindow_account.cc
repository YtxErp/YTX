#include "component/constantwebsocket.h"
#include "dialog/userprofiledialog.h"
#include "enum/authenum.h"
#include "global/logininfo.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utils/mainwindowutils.h"
#include "websocket/jsongen.h"
#include "websocket/websocket.h"
#include "workspace/workspacedialog.h"
#include "workspace/workspaceenum.h"

void MainWindow::on_actionProfile_triggered()
{
    qInfo() << Q_FUNC_INFO;

    static QPointer<UserProfileDialog> dialog {};

    if (!dialog) {
        dialog = new UserProfileDialog();
        utils::ManageDialog(widget_hash_, dialog);
    }

    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

void MainWindow::RAccountName(const QString& name) { ui->actionName->setText(tr("Name", "Person") + ": " + name); }

void MainWindow::RAccountUsername(const QJsonObject& obj)
{
    const auto code { static_cast<AccountUsernameOutcome>(obj[kCode].toInt()) };

    QString title {};
    QString message {};

    switch (code) {
    case AccountUsernameOutcome::kSuccess:
        return;

    case AccountUsernameOutcome::kInvalidFormat:
        title = tr("Invalid Input");
        message = tr("Username must be 3-32 characters, start with a letter, and contain only lowercase letters, digits, or underscores. No consecutive or "
                     "trailing underscores.");
        break;

    case AccountUsernameOutcome::kAlreadyExists:
        title = tr("Operation Rejected");
        message = tr("This username is already taken. Please choose another one.");
        break;

    case AccountUsernameOutcome::kUserNotFound:
        title = tr("Not Found");
        message = tr("The user account could not be found.");
        break;
    }

    utils::ShowMessage(QMessageBox::Warning, title, message, time_const::kAutoCloseMs);
}

void MainWindow::on_actionWorkspaceManager_triggered()
{
    qInfo() << Q_FUNC_INFO;

    static QPointer<WorkspaceDialog> dialog {};

    if (!dialog) {
        dialog = new WorkspaceDialog(header_info_.workspace);

        const auto widget_id { utils::ManageDialog(widget_hash_, dialog) };
        const auto message { JsonGen::WorkspaceMemberAck(widget_id, LoginInfo::Instance().Workspace()) };

        WebSocket::Instance()->SendMessage(WsKey::kWorkspaceMemberAck, message);

        auto* view { dialog->View() };
        InitTableView(view, std::to_underlying(workspace::MemberField::kSectionPermissions));
        DelegateWorkspaceMember(view);
    }

    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

void MainWindow::RWorkspaceMemberAck(const QUuid& widget_id, const QJsonArray& array)
{
    auto widget { widget_hash_.value(widget_id).widget };
    if (!widget)
        return;

    auto* ptr { widget.data() };
    Q_ASSERT(qobject_cast<WorkspaceDialog*>(ptr));

    auto* d_widget { static_cast<WorkspaceDialog*>(ptr) };

    auto* model { d_widget->Model() };
    model->Rebuild(array);
}

void MainWindow::RAccountRoleUpdate()
{
    utils::ShowMessage(QMessageBox::Information, tr("Role Updated"),
        tr("Your account role has been updated. Please log in again to apply the new permissions."), time_const::kAutoCloseMs);
}