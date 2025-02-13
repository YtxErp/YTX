#include "stringinitializer.h"

void StringInitializer::SetHeader(Info& finance, Info& product, Info& stakeholder, Info& task, Info& sales, Info& purchase)
{
    finance.node_header = { QObject::tr("Name"), QObject::tr("ID"), QObject::tr("Code"), QObject::tr("Description"), QObject::tr("Note"), QObject::tr("Rule"),
        QObject::tr("Type"), QObject::tr("Unit"), QObject::tr("Foreign Total"), QObject::tr("Local Total"), {} };
    finance.trans_header
        = { QObject::tr("ID"), QObject::tr("DateTime"), QObject::tr("FXRate"), QObject::tr("Code"), QObject::tr("Description"), QObject::tr("SupportID"),
              QObject::tr("D"), QObject::tr("S"), QObject::tr("RelatedNode"), QObject::tr("Debit"), QObject::tr("Credit"), QObject::tr("Subtotal") };
    finance.search_trans_header = { QObject::tr("ID"), QObject::tr("DateTime"), QObject::tr("Code"), QObject::tr("LhsNode"), QObject::tr("LhsFXRate"),
        QObject::tr("LhsDebit"), QObject::tr("LhsCredit"), QObject::tr("Description"), {}, QObject::tr("SupportID"), {}, {}, QObject::tr("D"), QObject::tr("S"),
        QObject::tr("RhsCredit"), QObject::tr("RhsDebit"), QObject::tr("RhsFXRate"), QObject::tr("RhsNode") };
    finance.search_node_header
        = { QObject::tr("Name"), QObject::tr("ID"), QObject::tr("Code"), QObject::tr("Description"), QObject::tr("Note"), QObject::tr("Rule"),
              QObject::tr("Type"), QObject::tr("Unit"), {}, {}, {}, {}, {}, {}, {}, {}, {}, QObject::tr("Foreign Total"), QObject::tr("Local Total") };
    finance.support_header = { QObject::tr("ID"), QObject::tr("DateTime"), QObject::tr("Code"), QObject::tr("LhsNode"), QObject::tr("LhsRatio"),
        QObject::tr("LhsDebit"), QObject::tr("LhsCredit"), QObject::tr("Description"), QObject::tr("UnitPrice"), QObject::tr("D"), QObject::tr("S"),
        QObject::tr("RhsCredit"), QObject::tr("RhsDebit"), QObject::tr("RhsRatio"), QObject::tr("RhsNode") };

    product.node_header = { QObject::tr("Name"), QObject::tr("ID"), QObject::tr("Code"), QObject::tr("Description"), QObject::tr("Note"), QObject::tr("Rule"),
        QObject::tr("Type"), QObject::tr("Unit"), QObject::tr("Color"), QObject::tr("UnitPrice"), QObject::tr("Commission"), QObject::tr("Quantity"),
        QObject::tr("Amount"), {} };
    product.trans_header
        = { QObject::tr("ID"), QObject::tr("DateTime"), QObject::tr("UnitCost"), QObject::tr("Code"), QObject::tr("Description"), QObject::tr("SupportID"),
              QObject::tr("D"), QObject::tr("S"), QObject::tr("RelatedNode"), QObject::tr("Debit"), QObject::tr("Credit"), QObject::tr("Subtotal") };
    product.search_trans_header = { QObject::tr("ID"), QObject::tr("DateTime"), QObject::tr("Code"), QObject::tr("LhsNode"), {}, QObject::tr("LhsDebit"),
        QObject::tr("LhsCredit"), QObject::tr("Description"), QObject::tr("UnitCost"), QObject::tr("SupportID"), {}, {}, QObject::tr("D"), QObject::tr("S"),
        QObject::tr("RhsCredit"), QObject::tr("RhsDebit"), {}, QObject::tr("RhsNode") };
    product.search_node_header = { QObject::tr("Name"), QObject::tr("ID"), QObject::tr("Code"), QObject::tr("Description"), QObject::tr("Note"),
        QObject::tr("Rule"), QObject::tr("Type"), QObject::tr("Unit"), {}, {}, {}, QObject::tr("Color"), {}, QObject::tr("UnitPrice"),
        QObject::tr("Commission"), {}, {}, QObject::tr("Quantity"), QObject::tr("Amount") };
    product.support_header = finance.support_header;

    stakeholder.node_header = { QObject::tr("Name"), QObject::tr("ID"), QObject::tr("Code"), QObject::tr("Description"), QObject::tr("Note"),
        QObject::tr("Rule"), QObject::tr("Type"), QObject::tr("Unit"), QObject::tr("Deadline"), QObject::tr("Employee"), QObject::tr("PaymentPeriod"),
        QObject::tr("TaxRate"), {} };
    stakeholder.trans_header = { QObject::tr("ID"), QObject::tr("DateTime"), QObject::tr("UnitPrice"), QObject::tr("Code"), QObject::tr("Description"),
        QObject::tr("OutsideProduct"), QObject::tr("D"), QObject::tr("S"), QObject::tr("InsideProduct"), QObject::tr("PlaceHolder") };
    stakeholder.search_trans_header
        = { QObject::tr("ID"), QObject::tr("DateTime"), QObject::tr("Code"), QObject::tr("InsideProduct"), {}, {}, {}, QObject::tr("Description"),
              QObject::tr("UnitPrice"), QObject::tr("SupportID"), {}, {}, QObject::tr("D"), QObject::tr("S"), {}, {}, {}, QObject::tr("OutsideProduct") };
    stakeholder.search_node_header = { QObject::tr("Name"), QObject::tr("ID"), QObject::tr("Code"), QObject::tr("Description"), QObject::tr("Note"),
        QObject::tr("Rule"), QObject::tr("Type"), QObject::tr("Unit"), {}, QObject::tr("Employee"), QObject::tr("Deadline"), {}, {},
        QObject::tr("PaymentPeriod"), QObject::tr("TaxRate"), {}, {}, {}, {} };
    stakeholder.support_header = finance.support_header;

    task.node_header = { QObject::tr("Name"), QObject::tr("ID"), QObject::tr("Code"), QObject::tr("Description"), QObject::tr("Note"), QObject::tr("Rule"),
        QObject::tr("Type"), QObject::tr("Unit"), QObject::tr("DateTime"), QObject::tr("Color"), QObject::tr("Document"), QObject::tr("Finished"),
        QObject::tr("UnitCost"), QObject::tr("Quantity"), QObject::tr("Amount"), {} };
    task.trans_header
        = { QObject::tr("ID"), QObject::tr("DateTime"), QObject::tr("UnitCost"), QObject::tr("Code"), QObject::tr("Description"), QObject::tr("SupportID"),
              QObject::tr("D"), QObject::tr("S"), QObject::tr("RelatedNode"), QObject::tr("Debit"), QObject::tr("Credit"), QObject::tr("Subtotal") };
    task.search_trans_header = product.search_trans_header;
    task.search_node_header = { QObject::tr("Name"), QObject::tr("ID"), QObject::tr("Code"), QObject::tr("Description"), QObject::tr("Note"),
        QObject::tr("Rule"), QObject::tr("Type"), QObject::tr("Unit"), {}, {}, QObject::tr("DateTime"), QObject::tr("Color"), QObject::tr("Document"),
        QObject::tr("UnitCost"), {}, {}, {}, QObject::tr("Quantity"), QObject::tr("Amount") };
    task.support_header = finance.support_header;

    sales.node_header = { QObject::tr("Name"), QObject::tr("ID"), QObject::tr("Code"), QObject::tr("Description"), QObject::tr("Note"), QObject::tr("Rule"),
        QObject::tr("Type"), QObject::tr("Unit"), QObject::tr("Party"), QObject::tr("Employee"), QObject::tr("DateTime"), QObject::tr("First"),
        QObject::tr("Second"), QObject::tr("Finished"), QObject::tr("Amount"), QObject::tr("Discount"), QObject::tr("Settled"), {} };
    sales.trans_header = { QObject::tr("ID"), QObject::tr("InsideProduct"), QObject::tr("OutsideProduct"), QObject::tr("Code"), QObject::tr("Description"),
        QObject::tr("Color"), QObject::tr("First"), QObject::tr("Second"), QObject::tr("UnitPrice"), QObject::tr("Amount"), QObject::tr("DiscountPrice"),
        QObject::tr("Discount"), QObject::tr("Settled") };
    sales.search_trans_header = { QObject::tr("ID"), {}, QObject::tr("Code"), QObject::tr("InsideProduct"), {}, QObject::tr("First"), QObject::tr("Second"),
        QObject::tr("Description"), QObject::tr("UnitPrice"), QObject::tr("LhsNode"), QObject::tr("DiscountPrice"), QObject::tr("Settled"), {}, {},
        QObject::tr("Amount"), QObject::tr("Discount"), {}, QObject::tr("OutsideProduct") };
    sales.search_node_header = { QObject::tr("Name"), QObject::tr("ID"), QObject::tr("Code"), QObject::tr("Description"), QObject::tr("Note"),
        QObject::tr("Rule"), QObject::tr("Type"), QObject::tr("Unit"), QObject::tr("Party"), QObject::tr("Employee"), QObject::tr("DateTime"), {}, {},
        QObject::tr("First"), QObject::tr("Second"), QObject::tr("Finished"), QObject::tr("Amount"), QObject::tr("Discount"), QObject::tr("Settled") };

    purchase.node_header = sales.node_header;
    purchase.trans_header = sales.trans_header;
    purchase.search_trans_header = sales.search_trans_header;
    purchase.search_node_header = sales.search_node_header;
}
