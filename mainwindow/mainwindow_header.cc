#include "mainwindow.h"

void MainWindow::InitHeader()
{
    header_info_.audit = {
        tr("Target ID"),
        tr("User"),
        tr("LHS Node"),
        tr("RHS Node"),
        tr("Issued Time"),
        tr("Target Type"),
        tr("Target Code"),
        tr("Target Operation"),
        tr("Target Field"),
        tr("Level"),
        tr("Before Change"),
        tr("After Change"),
    };

    header_info_.workspace = {
        tr("Email"),
        tr("Username"),
        tr("Name", "Person"),
        tr("Workspace Role"),
        tr("Section Permissions"),
        tr("Issued Time"),
    };

    header_info_.cash_flow_statement = {
        tr("Name"),
        tr("Code"),
        tr("Description"),
        tr("Direction Rule"),
        tr("Local Total"),
    };

    header_info_.cash_flow_statement_wrong = {
        tr("Issued Time"),
        tr("LHS Node"),
        tr("LHS Debit"),
        tr("LHS Credit"),
        tr("Description"),
        tr("Cash Kind"),
        tr("RHS Credit"),
        tr("RHS Debit"),
        tr("RHS Node"),
    };

    header_info_.balance_sheet = {
        tr("Name"),
        tr("Code"),
        tr("Description"),
        tr("Direction Rule"),
        tr("Kind"),
        tr("Opening Balance"),
        tr("Closing Balance"),
        tr("Change Amount"),
        tr("Change Rate"),
    };

    header_info_.income_statement = {
        tr("Name"),
        tr("Code"),
        tr("Description"),
        tr("Direction Rule"),
        tr("Kind"),
        tr("Current Period"),
        tr("YoY"),
        tr("YoY %"),
        tr("MoM"),
        tr("MoM %"),
    };

    header_info_.inventory_heat = {
        tr("Name"),
        tr("Placeholder"),
        tr("Order Count"),
        tr("Partner Count"),
        tr("Active Months"),
        tr("Active Days"),
        tr("Total Quantity"),
        tr("Heat Score"),
    };

    header_info_.partner_heat = {
        tr("Name"),
        tr("Placeholder"),
        tr("Order Count"),
        tr("Inventory Diversity"),
        tr("Active Months"),
        tr("Active Days"),
        tr("Total Quantity"),
        tr("Heat Score"),
    };

    header_info_.tag = {
        tr("Name"),
        tr("Color"),
    };

    // Statement
    header_info_.statement_primary = {
        tr("Partner"),
        tr("Count"),
        tr("Measure"),
        tr("Amount"),
        tr("Description"),
    };

    header_info_.statement_secondary = {
        tr("Issued Time"),
        tr("Code"),
        tr("Count"),
        tr("Measure"),
        tr("Amount"),
        tr("Description"),
        tr("Status"),
        tr("Employee"),
    };

    header_info_.statement_tertiary = {
        tr("Issued Time"),
        tr("Code"),
        tr("Internal SKU"),
        tr("Count"),
        tr("Measure"),
        tr("Unit Price"),
        tr("Amount"),
        tr("Description"),
        tr("Status"),
        tr("External SKU"),
    };

    // Settlement
    header_info_.settlement_primary = {
        tr("Partner"),
        tr("Issued Time"),
        tr("Description"),
        tr("Status"),
        tr("Amount"),
    };

    header_info_.settlement_secondary = {
        tr("Issued Time"),
        tr("Amount"),
        tr("Status"),
        tr("Description"),
        tr("Employee"),
    };
}

void MainWindow::InitHeader(SectionInfo& finance, SectionInfo& inventory, SectionInfo& task, SectionInfo& partner, SectionInfo& sale, SectionInfo& purchase)
{
    // Node
    finance.node_header = {
        tr("Name"),
        tr("Code"),
        tr("Description"),
        tr("Tag"),
        tr("Color"),
        tr("Document"),
        tr("Roles"),
        tr("Direction Rule"),
        tr("Kind"),
        tr("Unit"),
        tr("Foreign Total"),
        tr("Local Total"),
    };

    inventory.node_header = {
        tr("Name"),
        tr("Code"),
        tr("Description"),
        tr("Tag"),
        tr("Color"),
        tr("Document"),
        tr("Unit Price"),
        tr("Commission"),
        tr("Direction Rule"),
        tr("Kind"),
        tr("Unit"),
        tr("Quantity Total"),
        tr("Amount Total"),
    };

    task.node_header = {
        tr("Name"),
        tr("Code"),
        tr("Description"),
        tr("Tag"),
        tr("Color"),
        tr("Document"),
        tr("Direction Rule"),
        tr("Kind"),
        tr("Unit"),
        tr("Quantity Total"),
        tr("Amount Total"),
    };

    partner.node_header = {
        tr("Name"),
        tr("Code"),
        tr("Description"),
        tr("Tag"),
        tr("Color"),
        tr("Document"),
        tr("Status"),
        tr("Kind"),
        tr("Unit"),
        tr("Unpaid Total"),
    };

    sale.node_header = {
        tr("Name"),
        tr("Issued Time"),
        tr("Employee"),
        tr("Status"),
        tr("Code"),
        tr("Description"),
        tr("Tag"),
        tr("Direction Rule"),
        tr("Kind"),
        tr("Unit"),
        tr("Count Total"),
        tr("Measure Total"),
        tr("Gross Total"),
    };

    // Entry
    finance.entry_header = {
        tr("LHS Node"),
        tr("Issued Time"),
        tr("FX Rate"),
        tr("Code"),
        tr("Description"),
        tr("Cash Kind"),
        tr("Tag"),
        tr("Document"),
        tr("Status"),
        tr("Linked Node"),
        tr("Debit"),
        tr("Credit"),
        tr("Balance"),
    };

    inventory.entry_header = {
        tr("LHS Node"),
        tr("Issued Time"),
        tr("Unit Cost"),
        tr("Code"),
        tr("Description"),
        tr("Tag"),
        tr("Document"),
        tr("Status"),
        tr("Linked Node"),
        tr("Debit"),
        tr("Credit"),
        tr("Balance"),
    };

    task.entry_header = {
        tr("LHS Node"),
        tr("Issued Time"),
        tr("Unit Cost"),
        tr("Code"),
        tr("Description"),
        tr("Tag"),
        tr("Document"),
        tr("Status"),
        tr("Linked Node"),
        tr("Debit"),
        tr("Credit"),
        tr("Balance"),
    };

    partner.entry_header = {
        tr("LHS Node"),
        tr("Issued Time"),
        tr("Code"),
        tr("Description"),
        tr("Tag"),
        tr("Document"),
        tr("Status"),
        tr("Internal SKU"),
        tr("Unit Price"),
        tr("External SKU"),
    };

    sale.entry_header = {
        tr("LHS Node"),
        tr("Internal SKU"),
        tr("Count"),
        tr("Measure"),
        tr("Unit Price"),
        tr("Status"),
        tr("Description"),
        tr("Tag"),
        tr("External SKU"),
        tr("Gross"),
    };

    // Full Entry
    finance.full_entry_header = {
        tr("Issued Time"),
        tr("Code"),
        tr("LHS Node"),
        tr("LHS FX Rate"),
        tr("LHS Debit"),
        tr("LHS Credit"),
        tr("Description"),
        tr("Cash Kind"),
        tr("Tag"),
        tr("Document"),
        tr("Status"),
        tr("RHS Credit"),
        tr("RHS Debit"),
        tr("RHS FX Rate"),
        tr("RHS Node"),
    };

    inventory.full_entry_header = {
        tr("Issued Time"),
        tr("Code"),
        tr("LHS Node"),
        tr("LHS Unit Cost"),
        tr("LHS Debit"),
        tr("LHS Credit"),
        tr("Description"),
        tr("Tag"),
        tr("Document"),
        tr("Status"),
        tr("RHS Credit"),
        tr("RHS Debit"),
        tr("RHS Unit Cost"),
        tr("RHS Node"),
    };

    task.full_entry_header = {
        tr("Issued Time"),
        tr("Code"),
        tr("LHS Node"),
        tr("LHS Unit Cost"),
        tr("LHS Debit"),
        tr("LHS Credit"),
        tr("Description"),
        tr("Tag"),
        tr("Document"),
        tr("Status"),
        tr("RHS Credit"),
        tr("RHS Debit"),
        tr("RHS Unit Cost"),
        tr("RHS Node"),
    };

    partner.full_entry_header = partner.entry_header;
    sale.full_entry_header = sale.entry_header;

    // SaleReference
    inventory.node_referenced_header = {
        tr("Issued Time"),
        tr("Customer"),
        tr("Count"),
        tr("Measure"),
        tr("Unit Price"),
        tr("Description"),
        tr("Gross"),
    };

    partner.node_referenced_header = {
        tr("Issued Time"),
        tr("Internal SKU"),
        tr("Color"),
        tr("External SKU"),
        tr("Count"),
        tr("Measure"),
        tr("Unit Price"),
        tr("Description"),
        tr("Gross"),
    };

    purchase.node_header = sale.node_header;
    purchase.entry_header = sale.entry_header;
    purchase.full_entry_header = sale.full_entry_header;
}
