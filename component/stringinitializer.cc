#include "stringinitializer.h"

void StringInitializer::SetHeader(Info& finance, Info& product, Info& stakeholder, Info& task, Info& sales, Info& purchase)
{
    finance.node_header = { QObject::tr("Name"), QObject::tr("ID"), QObject::tr("Code"), QObject::tr("Description"), QObject::tr("Note"), QObject::tr("Rule"),
        QObject::tr("Type"), QObject::tr("Unit"), QObject::tr("ForeignTotal"), QObject::tr("LocalTotal"), {} };
    finance.trans_header
        = { QObject::tr("ID"), QObject::tr("DateTime"), QObject::tr("FXRate"), QObject::tr("Code"), QObject::tr("Description"), QObject::tr("SupportID"),
              QObject::tr("D"), QObject::tr("S"), QObject::tr("RelatedNode"), QObject::tr("Debit"), QObject::tr("Credit"), QObject::tr("Subtotal") };

    finance.search_node_header
        = { QObject::tr("Name"), QObject::tr("ID"), QObject::tr("Code"), QObject::tr("Description"), QObject::tr("Note"), QObject::tr("Rule"),
              QObject::tr("Type"), QObject::tr("Unit"), {}, {}, {}, {}, {}, {}, {}, {}, {}, QObject::tr("ForeignTotal"), QObject::tr("LocalTotal") };
    finance.search_trans_header = { QObject::tr("ID"), QObject::tr("DateTime"), QObject::tr("Code"), QObject::tr("LhsNode"), QObject::tr("LhsFXRate"),
        QObject::tr("LhsDebit"), QObject::tr("LhsCredit"), QObject::tr("Description"), {}, QObject::tr("SupportID"), {}, {}, QObject::tr("D"), QObject::tr("S"),
        QObject::tr("RhsCredit"), QObject::tr("RhsDebit"), QObject::tr("RhsFXRate"), QObject::tr("RhsNode") };

    finance.support_header = { QObject::tr("ID"), QObject::tr("DateTime"), QObject::tr("Code"), QObject::tr("LhsNode"), QObject::tr("LhsRatio"),
        QObject::tr("LhsDebit"), QObject::tr("LhsCredit"), QObject::tr("Description"), QObject::tr("UnitPrice"), QObject::tr("D"), QObject::tr("S"),
        QObject::tr("RhsCredit"), QObject::tr("RhsDebit"), QObject::tr("RhsRatio"), QObject::tr("RhsNode") };

    product.node_header = { QObject::tr("Name"), QObject::tr("ID"), QObject::tr("Code"), QObject::tr("Description"), QObject::tr("Note"), QObject::tr("Rule"),
        QObject::tr("Type"), QObject::tr("Unit"), QObject::tr("Color"), QObject::tr("UnitPrice"), QObject::tr("Commission"), QObject::tr("Quantity"),
        QObject::tr("Amount"), {} };
    product.trans_header
        = { QObject::tr("ID"), QObject::tr("DateTime"), QObject::tr("UnitCost"), QObject::tr("Code"), QObject::tr("Description"), QObject::tr("SupportID"),
              QObject::tr("D"), QObject::tr("S"), QObject::tr("RelatedNode"), QObject::tr("Debit"), QObject::tr("Credit"), QObject::tr("Subtotal") };

    product.search_node_header = { QObject::tr("Name"), QObject::tr("ID"), QObject::tr("Code"), QObject::tr("Description"), QObject::tr("Note"),
        QObject::tr("Rule"), QObject::tr("Type"), QObject::tr("Unit"), {}, {}, {}, QObject::tr("Color"), {}, QObject::tr("UnitPrice"),
        QObject::tr("Commission"), {}, {}, QObject::tr("Quantity"), QObject::tr("Amount") };
    product.search_trans_header = { QObject::tr("ID"), QObject::tr("DateTime"), QObject::tr("Code"), QObject::tr("LhsNode"), {}, QObject::tr("LhsDebit"),
        QObject::tr("LhsCredit"), QObject::tr("Description"), QObject::tr("UnitCost"), QObject::tr("SupportID"), {}, {}, QObject::tr("D"), QObject::tr("S"),
        QObject::tr("RhsCredit"), QObject::tr("RhsDebit"), {}, QObject::tr("RhsNode") };
    product.support_header = finance.support_header;

    stakeholder.node_header = { QObject::tr("Name"), QObject::tr("ID"), QObject::tr("Code"), QObject::tr("Description"), QObject::tr("Note"),
        QObject::tr("Rule"), QObject::tr("Type"), QObject::tr("Category"), QObject::tr("Deadline"), QObject::tr("Employee"), QObject::tr("PaymentPeriod"),
        QObject::tr("TaxRate"), QObject::tr("Amount"), {} };
    stakeholder.trans_header = { QObject::tr("ID"), QObject::tr("DateTime"), QObject::tr("UnitPrice"), QObject::tr("Code"), QObject::tr("Description"),
        QObject::tr("Outside"), QObject::tr("D"), QObject::tr("S"), QObject::tr("Inside"), QObject::tr("PlaceHolder") };

    stakeholder.search_node_header = { QObject::tr("Name"), QObject::tr("ID"), QObject::tr("Code"), QObject::tr("Description"), QObject::tr("Note"),
        QObject::tr("Rule"), QObject::tr("Type"), QObject::tr("Category"), {}, QObject::tr("Employee"), QObject::tr("Deadline"), {}, {},
        QObject::tr("PaymentPeriod"), QObject::tr("TaxRate"), {}, {}, {}, QObject::tr("Amount") };
    stakeholder.search_trans_header
        = { QObject::tr("ID"), QObject::tr("DateTime"), QObject::tr("Code"), QObject::tr("InsideProduct"), {}, {}, {}, QObject::tr("Description"),
              QObject::tr("UnitPrice"), QObject::tr("SupportID"), {}, {}, QObject::tr("D"), QObject::tr("S"), {}, {}, {}, QObject::tr("OutsideProduct") };
    stakeholder.support_header = finance.support_header;

    task.node_header = { QObject::tr("Name"), QObject::tr("ID"), QObject::tr("Code"), QObject::tr("Description"), QObject::tr("Note"), QObject::tr("Rule"),
        QObject::tr("Type"), QObject::tr("Category"), QObject::tr("DateTime"), QObject::tr("Color"), QObject::tr("Document"), QObject::tr("Finished"),
        QObject::tr("UnitCost"), QObject::tr("Quantity"), QObject::tr("Amount"), {} };
    task.trans_header
        = { QObject::tr("ID"), QObject::tr("DateTime"), QObject::tr("UnitCost"), QObject::tr("Code"), QObject::tr("Description"), QObject::tr("SupportID"),
              QObject::tr("D"), QObject::tr("S"), QObject::tr("RelatedNode"), QObject::tr("Debit"), QObject::tr("Credit"), QObject::tr("Subtotal") };

    task.search_node_header = { QObject::tr("Name"), QObject::tr("ID"), QObject::tr("Code"), QObject::tr("Description"), QObject::tr("Note"),
        QObject::tr("Rule"), QObject::tr("Type"), QObject::tr("Category"), {}, {}, QObject::tr("DateTime"), QObject::tr("Color"), QObject::tr("Document"),
        QObject::tr("UnitCost"), {}, {}, {}, QObject::tr("Quantity"), QObject::tr("Amount") };
    task.search_trans_header = product.search_trans_header;
    task.support_header = finance.support_header;

    sales.node_header = { QObject::tr("Name"), QObject::tr("ID"), QObject::tr("Party"), QObject::tr("Description"), QObject::tr("Employee"),
        QObject::tr("Rule"), QObject::tr("Type"), QObject::tr("Unit"), QObject::tr("DateTime"), QObject::tr("First"), QObject::tr("Second"),
        QObject::tr("Finished"), QObject::tr("GrossAmount"), QObject::tr("Discount"), QObject::tr("NetAmount"), {} };
    sales.trans_header = { QObject::tr("ID"), QObject::tr("Inside"), QObject::tr("Outside"), QObject::tr("First"), QObject::tr("Second"),
        QObject::tr("UnitPrice"), QObject::tr("DiscountPrice"), QObject::tr("Description"), QObject::tr("Code"), QObject::tr("Color"),
        QObject::tr("GrossAmount"), QObject::tr("Discount"), QObject::tr("NetAmount") };

    sales.search_node_header = { QObject::tr("Name"), QObject::tr("ID"), QObject::tr("Code"), QObject::tr("Description"), QObject::tr("Note"),
        QObject::tr("Rule"), QObject::tr("Type"), QObject::tr("Unit"), QObject::tr("Party"), QObject::tr("Employee"), QObject::tr("DateTime"), {}, {},
        QObject::tr("First"), QObject::tr("Second"), QObject::tr("Discount"), QObject::tr("Finished"), QObject::tr("GrossAmount"), QObject::tr("NetAmount") };
    sales.search_trans_header = { QObject::tr("ID"), {}, QObject::tr("Code"), QObject::tr("LhsNode"), {}, QObject::tr("First"), QObject::tr("Second"),
        QObject::tr("Description"), QObject::tr("UnitPrice"), QObject::tr("OutsideProduct"), QObject::tr("DiscountPrice"), QObject::tr("Discount"), {}, {},
        QObject::tr("NetAmount"), QObject::tr("GrossAmount"), {}, QObject::tr("InsideProduct") };

    purchase.node_header = sales.node_header;
    purchase.trans_header = sales.trans_header;
    purchase.search_trans_header = sales.search_trans_header;
    purchase.search_node_header = sales.search_node_header;
}
