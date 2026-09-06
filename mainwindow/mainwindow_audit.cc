#include "audit/auditdialog.h"
#include "audit/auditenum.h"
#include "mainwindow.h"

void MainWindow::on_actionAuditLog_triggered()
{
    qInfo() << Q_FUNC_INFO;

    const QUuid widget_id { QUuid::createUuidV7() };
    audit::Model* model { new audit::Model(audit_info_, header_info_.audit, start_, this) };

    auto* dialog { new AuditDialog(model, widget_id, audit_info_.section_hash.value(std::to_underlying(start_)), start_, this) };

    {
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        WidgetContext wc { dialog, widget_id, WidgetRole::kDialog };
        widget_hash_.insert(widget_id, wc);
    }

    {
        auto* view { dialog->View() };
        InitTableView(view, std::to_underlying(audit::RowField::kAfter));

        view->horizontalHeader()->setSectionResizeMode(std::to_underlying<>(audit::RowField::kBefore), QHeaderView::Interactive);

        DelegateAuditLog(view);
    }

    dialog->show();
}

void MainWindow::RAuditLogAck(const QUuid& widget_id, const QJsonArray& log_array)
{
    auto widget { widget_hash_.value(widget_id).widget };
    if (!widget)
        return;

    auto* ptr { widget.data() };
    Q_ASSERT(qobject_cast<AuditDialog*>(ptr));

    auto* d_widget { static_cast<AuditDialog*>(ptr) };

    auto* model { d_widget->Model() };
    model->Rebuild(log_array);
}

void MainWindow::InitAuditInfo()
{
    using namespace audit;

    audit_info_.f_leaf_path = sc_f_.tree_model->LeafPath();
    audit_info_.f_branch_path = sc_f_.tree_model->BranchPath();
    audit_info_.i_leaf_path = sc_i_.tree_model->LeafPath();
    audit_info_.i_branch_path = sc_i_.tree_model->BranchPath();
    audit_info_.p_leaf_path = sc_p_.tree_model->LeafPath();
    audit_info_.p_branch_path = sc_p_.tree_model->BranchPath();
    audit_info_.t_leaf_path = sc_t_.tree_model->LeafPath();
    audit_info_.t_branch_path = sc_t_.tree_model->BranchPath();

    audit_info_.section_hash = {
        { std::to_underlying(Section::kFinance), tr("Finance") },
        { std::to_underlying(Section::kTask), tr("Task") },
        { std::to_underlying(Section::kInventory), tr("Inventory") },
        { std::to_underlying(Section::kPartner), tr("Partner") },
        { std::to_underlying(Section::kSale), tr("Sale") },
        { std::to_underlying(Section::kPurchase), tr("Purchase") },
    };

    audit_info_.target_type_hash = {
        { std::to_underlying(TargetType::kNode), tr("Node") },
        { std::to_underlying(TargetType::kEntry), tr("Entry") },
        { std::to_underlying(TargetType::kSettlement), tr("Settlement") },
    };

    audit_info_.level_hash = {
        { std::to_underlying(Level::kInfo), tr("Info") },
        { std::to_underlying(Level::kWarn), tr("Warn") },
        { std::to_underlying(Level::kCritical), tr("Critical") },
    };

    audit_info_.target_operation_hash = {
        { std::to_underlying(TargetOperation::kInsert), tr("Insert") },
        { std::to_underlying(TargetOperation::kUpdate), tr("Update") },
        { std::to_underlying(TargetOperation::kDelete), tr("Delete") },
        { std::to_underlying(TargetOperation::kRecall), tr("Recall") },
        { std::to_underlying(TargetOperation::kRelease), tr("Release") },
        { std::to_underlying(TargetOperation::kMove), tr("Move") },
        { std::to_underlying(TargetOperation::kReplace), tr("Replace") },
        { std::to_underlying(TargetOperation::kPeriodClose), tr("Period Close") },
    };

    audit_info_.target_field_hash = {
        { std::to_underlying(TargetField::kNone), QString() },
        { std::to_underlying(TargetField::kName), tr("Name") },
        { std::to_underlying(TargetField::kDirectionRule), tr("Direction Rule") },
        { std::to_underlying(TargetField::kNumeric), tr("Numeric") },
        { std::to_underlying(TargetField::kRate), tr("Rate") },
        { std::to_underlying(TargetField::kLinkedNode), tr("Linked Node") },
        { std::to_underlying(TargetField::kContent), tr("Content") },
        { std::to_underlying(TargetField::kStatus), tr("Status") },
    };
}
