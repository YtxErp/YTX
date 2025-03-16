#include "stringinitializer.h"

void StringInitializer::SetHeader(Info& finance, Info& product, Info& stakeholder, Info& task, Info& sales, Info& purchase)
{
    finance.node_header = {
        QObject::tr("Name"),
        QObject::tr("ID"),
        QObject::tr("Code"),
        QObject::tr("Description"),
        QObject::tr("Note"),
        QObject::tr("Type"),
        QObject::tr("Rule"),
        QObject::tr("Unit"),
        QObject::tr("ForeignTotal"),
        QObject::tr("LocalTotal"),
    };

    finance.trans_header = {
        QObject::tr("DateTime"),
        QObject::tr("ID"),
        QObject::tr("FXRate"),
        QObject::tr("Code"),
        QObject::tr("Description"),
        QObject::tr("SupportID"),
        QObject::tr("D"),
        QObject::tr("S"),
        QObject::tr("RelatedNode"),
        QObject::tr("Debit"),
        QObject::tr("Credit"),
        QObject::tr("Subtotal"),
    };

    finance.search_node_header = {
        QObject::tr("Name"),
        QObject::tr("ID"),
        QObject::tr("Code"),
        QObject::tr("Description"),
        QObject::tr("Note"),
        QObject::tr("Type"),
        QObject::tr("Rule"),
        QObject::tr("Unit"),
        {},
        {},
        {},
        {},
        {},
        {},
        {},
        {},
        {},
        QObject::tr("ForeignTotal"),
        QObject::tr("LocalTotal"),
    };

    finance.search_trans_header = {
        QObject::tr("DateTime"),
        QObject::tr("ID"),
        QObject::tr("Code"),
        QObject::tr("LhsNode"),
        QObject::tr("LhsFXRate"),
        QObject::tr("LhsDebit"),
        QObject::tr("LhsCredit"),
        QObject::tr("Description"),
        QObject::tr("SupportID"),
        {},
        QObject::tr("D"),
        QObject::tr("S"),
        QObject::tr("RhsCredit"),
        QObject::tr("RhsDebit"),
        QObject::tr("RhsFXRate"),
        QObject::tr("RhsNode"),
    };

    finance.excel_trans_header = {
        QObject::tr("DateTime"),
        QObject::tr("ID"),
        QObject::tr("Code"),
        QObject::tr("LhsNode"),
        QObject::tr("LhsFXRate"),
        QObject::tr("LhsDebit"),
        QObject::tr("LhsCredit"),
        QObject::tr("Description"),
        QObject::tr("SupportID"),
        QObject::tr("D"),
        QObject::tr("S"),
        QObject::tr("RhsCredit"),
        QObject::tr("RhsDebit"),
        QObject::tr("RhsFXRate"),
        QObject::tr("RhsNode"),
    };

    product.node_header = {
        QObject::tr("Name"),
        QObject::tr("ID"),
        QObject::tr("Code"),
        QObject::tr("Description"),
        QObject::tr("Note"),
        QObject::tr("Type"),
        QObject::tr("Rule"),
        QObject::tr("Unit"),
        QObject::tr("Color"),
        QObject::tr("UnitPrice"),
        QObject::tr("Commission"),
        QObject::tr("Quantity"),
        QObject::tr("Amount"),
    };

    product.trans_header = {
        QObject::tr("DateTime"),
        QObject::tr("ID"),
        QObject::tr("UnitCost"),
        QObject::tr("Code"),
        QObject::tr("Description"),
        QObject::tr("SupportID"),
        QObject::tr("D"),
        QObject::tr("S"),
        QObject::tr("RelatedNode"),
        QObject::tr("Debit"),
        QObject::tr("Credit"),
        QObject::tr("Subtotal"),
    };

    product.search_node_header = {
        QObject::tr("Name"),
        QObject::tr("ID"),
        QObject::tr("Code"),
        QObject::tr("Description"),
        QObject::tr("Note"),
        QObject::tr("Type"),
        QObject::tr("Rule"),
        QObject::tr("Unit"),
        {},
        {},
        {},
        QObject::tr("Color"),
        {},
        QObject::tr("UnitPrice"),
        QObject::tr("Commission"),
        {},
        {},
        QObject::tr("Quantity"),
        QObject::tr("Amount"),
    };

    product.search_trans_header = {
        QObject::tr("DateTime"),
        QObject::tr("ID"),
        QObject::tr("Code"),
        QObject::tr("LhsNode"),
        QObject::tr("UnitCost"),
        QObject::tr("LhsDebit"),
        QObject::tr("LhsCredit"),
        QObject::tr("Description"),
        QObject::tr("SupportID"),
        {},
        QObject::tr("D"),
        QObject::tr("S"),
        QObject::tr("RhsCredit"),
        QObject::tr("RhsDebit"),
        QObject::tr("UnitCost"),
        QObject::tr("RhsNode"),
    };

    product.excel_trans_header = {
        QObject::tr("DateTime"),
        QObject::tr("ID"),
        QObject::tr("Code"),
        QObject::tr("LhsNode"),
        QObject::tr("UnitCost"),
        QObject::tr("LhsDebit"),
        QObject::tr("LhsCredit"),
        QObject::tr("Description"),
        QObject::tr("SupportID"),
        QObject::tr("D"),
        QObject::tr("S"),
        QObject::tr("RhsCredit"),
        QObject::tr("RhsDebit"),
        QObject::tr("RhsNode"),
    };

    product.trans_ref_header = {
        QObject::tr("DateTime"),
        QObject::tr("LhsNode"),
        QObject::tr("CUST"),
        QObject::tr("Outside"),
        QObject::tr("First"),
        QObject::tr("Second"),
        QObject::tr("UnitPrice"),
        QObject::tr("DiscountPrice"),
        QObject::tr("Description"),
        QObject::tr("GrossAmount"),
        QObject::tr("Discount"),
        QObject::tr("NetAmount"),
    };

    stakeholder.node_header = {
        QObject::tr("Name"),
        QObject::tr("ID"),
        QObject::tr("Code"),
        QObject::tr("Description"),
        QObject::tr("Note"),
        QObject::tr("Type"),
        QObject::tr("PaymentPeriod"),
        QObject::tr("Unit"),
        QObject::tr("Deadline"),
        QObject::tr("Employee"),
        QObject::tr("TaxRate"),
        QObject::tr("Amount"),
    };

    stakeholder.trans_header = {
        QObject::tr("DateTime"),
        QObject::tr("ID"),
        QObject::tr("UnitPrice"),
        QObject::tr("Code"),
        QObject::tr("Description"),
        QObject::tr("Outside"),
        QObject::tr("D"),
        QObject::tr("S"),
        QObject::tr("Inside"),
        QObject::tr("PlaceHolder"),
    };

    stakeholder.search_node_header = {
        QObject::tr("Name"),
        QObject::tr("ID"),
        QObject::tr("Code"),
        QObject::tr("Description"),
        QObject::tr("Note"),
        QObject::tr("Type"),
        {},
        QObject::tr("Unit"),
        {},
        QObject::tr("Employee"),
        QObject::tr("Deadline"),
        {},
        {},
        QObject::tr("PaymentPeriod"),
        QObject::tr("TaxRate"),
        {},
        {},
        {},
        QObject::tr("Amount"),
    };

    stakeholder.search_trans_header = {
        QObject::tr("DateTime"),
        QObject::tr("ID"),
        QObject::tr("Code"),
        QObject::tr("Party"),
        QObject::tr("UnitPrice"),
        {},
        {},
        QObject::tr("Description"),
        QObject::tr("Outside"),
        {},
        QObject::tr("D"),
        QObject::tr("S"),
        {},
        {},
        {},
        QObject::tr("Inside"),
    };

    stakeholder.excel_trans_header = {
        QObject::tr("DateTime"),
        QObject::tr("ID"),
        QObject::tr("Code"),
        QObject::tr("Party"),
        QObject::tr("UnitPrice"),
        QObject::tr("Description"),
        QObject::tr("Outside"),
        {},
        QObject::tr("D"),
        QObject::tr("S"),
        QObject::tr("Inside"),
    };

    stakeholder.trans_ref_header = {
        QObject::tr("DateTime"),
        QObject::tr("LhsNode"),
        QObject::tr("Inside"),
        QObject::tr("Outside"),
        QObject::tr("First"),
        QObject::tr("Second"),
        QObject::tr("UnitPrice"),
        QObject::tr("DiscountPrice"),
        QObject::tr("Description"),
        QObject::tr("GrossAmount"),
        QObject::tr("Discount"),
        QObject::tr("NetAmount"),
    };

    task.node_header = {
        QObject::tr("Name"),
        QObject::tr("ID"),
        QObject::tr("Code"),
        QObject::tr("Description"),
        QObject::tr("Note"),
        QObject::tr("Type"),
        QObject::tr("Rule"),
        QObject::tr("Unit"),
        QObject::tr("DateTime"),
        QObject::tr("Color"),
        QObject::tr("Document"),
        QObject::tr("Finished"),
        QObject::tr("UnitCost"),
        QObject::tr("Quantity"),
        QObject::tr("Amount"),
    };

    task.trans_header = {
        QObject::tr("DateTime"),
        QObject::tr("ID"),
        QObject::tr("UnitCost"),
        QObject::tr("Code"),
        QObject::tr("Description"),
        QObject::tr("SupportID"),
        QObject::tr("D"),
        QObject::tr("S"),
        QObject::tr("RelatedNode"),
        QObject::tr("Debit"),
        QObject::tr("Credit"),
        QObject::tr("Subtotal"),
    };

    task.search_node_header = {
        QObject::tr("Name"),
        QObject::tr("ID"),
        QObject::tr("Code"),
        QObject::tr("Description"),
        QObject::tr("Note"),
        QObject::tr("Type"),
        QObject::tr("Rule"),
        QObject::tr("Unit"),
        {},
        {},
        QObject::tr("DateTime"),
        QObject::tr("Color"),
        QObject::tr("Document"),
        QObject::tr("UnitCost"),
        {},
        {},
        {},
        QObject::tr("Quantity"),
        QObject::tr("Amount"),
    };
    task.search_trans_header = product.search_trans_header;
    task.excel_trans_header = product.excel_trans_header;

    sales.node_header = {
        QObject::tr("Name"),
        QObject::tr("ID"),
        QObject::tr("Party"),
        QObject::tr("Description"),
        QObject::tr("Employee"),
        QObject::tr("Type"),
        QObject::tr("Rule"),
        QObject::tr("Unit"),
        QObject::tr("DateTime"),
        QObject::tr("First"),
        QObject::tr("Second"),
        QObject::tr("Finished"),
        QObject::tr("GrossAmount"),
        QObject::tr("Discount"),
        QObject::tr("Settlement"),
    };

    sales.trans_header = {
        QObject::tr("Inside"),
        QObject::tr("ID"),
        QObject::tr("Outside"),
        QObject::tr("First"),
        QObject::tr("Second"),
        QObject::tr("UnitPrice"),
        QObject::tr("DiscountPrice"),
        QObject::tr("Description"),
        QObject::tr("Code"),
        QObject::tr("Color"),
        QObject::tr("GrossAmount"),
        QObject::tr("Discount"),
        QObject::tr("NetAmount"),
    };

    sales.search_node_header = {
        QObject::tr("Name"),
        QObject::tr("ID"),
        QObject::tr("Code"),
        QObject::tr("Description"),
        QObject::tr("Note"),
        QObject::tr("Type"),
        QObject::tr("Rule"),
        QObject::tr("Unit"),
        QObject::tr("Party"),
        QObject::tr("Employee"),
        QObject::tr("DateTime"),
        {},
        {},
        QObject::tr("First"),
        QObject::tr("Second"),
        QObject::tr("Discount"),
        QObject::tr("Finished"),
        QObject::tr("GrossAmount"),
        QObject::tr("Settlement"),
    };
    sales.search_trans_header = {
        {},
        QObject::tr("ID"),
        QObject::tr("Code"),
        QObject::tr("LhsNode"),
        QObject::tr("UnitPrice"),
        QObject::tr("First"),
        QObject::tr("Second"),
        QObject::tr("Description"),
        QObject::tr("OutsideProduct"),
        QObject::tr("Discount"),
        {},
        {},
        QObject::tr("NetAmount"),
        QObject::tr("GrossAmount"),
        QObject::tr("DiscountPrice"),
        QObject::tr("InsideProduct"),
    };

    sales.excel_trans_header = {
        QObject::tr("ID"),
        QObject::tr("Code"),
        QObject::tr("LhsNode"),
        QObject::tr("UnitPrice"),
        QObject::tr("First"),
        QObject::tr("Second"),
        QObject::tr("Description"),
        QObject::tr("OutsideProduct"),
        QObject::tr("Discount"),
        QObject::tr("NetAmount"),
        QObject::tr("GrossAmount"),
        QObject::tr("DiscountPrice"),
        QObject::tr("InsideProduct"),
    };

    sales.statement_header = {
        QObject::tr("Party"),
        QObject::tr("PBalance"),
        QObject::tr("CFirst"),
        QObject::tr("CSecond"),
        QObject::tr("CGrossAmount"),
        QObject::tr("CBalance"),
        QObject::tr("Description"),
        QObject::tr("CSettlement"),
    };

    sales.statement_primary_header = {
        QObject::tr("DateTime"),
        QObject::tr("First"),
        QObject::tr("Second"),
        QObject::tr("GrossAmount"),
        "S",
        QObject::tr("Description"),
        QObject::tr("Employee"),
        QObject::tr("Settlement"),
    };

    sales.statement_secondary_header = {
        QObject::tr("DateTime"),
        QObject::tr("Inside"),
        QObject::tr("First"),
        QObject::tr("Second"),
        QObject::tr("UnitPrice"),
        QObject::tr("GrossAmount"),
        "S",
        QObject::tr("Description"),
        QObject::tr("Outside"),
        QObject::tr("Settlement"),
    };

    purchase.node_header = sales.node_header;
    purchase.trans_header = sales.trans_header;
    purchase.search_trans_header = sales.search_trans_header;
    purchase.search_node_header = sales.search_node_header;
    purchase.excel_trans_header = sales.excel_trans_header;
    purchase.statement_header = sales.statement_header;
    purchase.statement_primary_header = sales.statement_primary_header;
    purchase.statement_secondary_header = sales.statement_secondary_header;
}
