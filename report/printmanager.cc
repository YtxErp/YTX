#include "printmanager.h"

#include <QFile>
#include <QFont>
#include <QPainter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinterInfo>
#include <QVariant>

PrintManager::PrintManager(CAppConfig& app_settings, NodeModel* product, NodeModel* stakeholder)
    : app_settings_ { app_settings }
    , product_ { product }
    , stakeholder_ { stakeholder }
{
}

void PrintManager::SetData(const PrintData& data, const QList<TransShadow*>& trans_shadow_list)
{
    data_ = data;
    trans_shadow_list_ = trans_shadow_list;
}

void PrintManager::Preview()
{
    QPrinter printer { QPrinter::ScreenResolution };
    ApplyConfig(&printer);

    printer.setPrinterName(app_settings_.printer);

    QPrintPreviewDialog preview(&printer);

    QObject::connect(&preview, &QPrintPreviewDialog::paintRequested, &preview, [this](QPrinter* printer) { this->RenderAllPages(printer); });
    preview.exec();
}

void PrintManager::Print()
{
    QPrinter printer(QPrinter::ScreenResolution);
    ApplyConfig(&printer);

    const auto available_printers { QPrinterInfo::availablePrinterNames() };
    const QString& printer_name { app_settings_.printer };

    if (printer_name.isEmpty() || !available_printers.contains(printer_name)) {
        QPrintDialog dialog(&printer);
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }
    } else {
        printer.setPrinterName(printer_name);
    }

    RenderAllPages(&printer);
}

bool PrintManager::LoadIni(const QString& file_path)
{
    // Page settings
    QSettings settings(file_path, QSettings::IniFormat);

    const QList<QString> page_settings { "paper_size", "orientation", "font_size" };

    for (const QString& setting : page_settings) {
        page_settings_[setting] = settings.value("page_settings/" + setting);
    }

    const QList<QString> header_settings { "party", "date_time" };
    const QList<QString> content_settings { "left_top", "heigh_width", "rows_columns" };
    const QList<QString> footer_settings { "employee", "unit", "gross_amount", "page_info" };

    // Read fields for header section
    for (const QString& setting : header_settings) {
        ReadFieldPosition(settings, "header", setting);
    }

    // Read fields for content section
    for (const QString& setting : content_settings) {
        ReadFieldPosition(settings, "table", setting);
    }

    // Read fields for footer section
    for (const QString& setting : footer_settings) {
        ReadFieldPosition(settings, "footer", setting);
    }

    return true;
}

void PrintManager::RenderAllPages(QPrinter* printer)
{
    // Fetch configuration values for rows and columns
    int rows { field_settings_.value("rows_columns").x };

    // Calculate total pages required based on the total rows and rows per page
    const long long total_pages { (trans_shadow_list_.size() + rows - 1) / rows }; // Ceiling division to determine total pages

    QPainter painter(printer);

    // Start rendering each page
    for (long long page_num = 0; page_num != total_pages; ++page_num) {
        // Begin a new page
        if (page_num != 0) {
            printer->newPage();
        }

        // Draw header (e.g., title, date, etc.)
        DrawHeader(&painter);

        // Draw content on the page
        const long long start_index = page_num * rows;
        const long long end_index = qMin((page_num + 1) * rows, trans_shadow_list_.size());
        DrawTable(&painter, start_index, end_index);

        // Draw footer (e.g., page number, etc.)
        DrawFooter(&painter, page_num + 1, total_pages);
    }
}

void PrintManager::DrawHeader(QPainter* painter)
{
    // Example: Draw a header at the specified position

    if (field_settings_.contains("party")) {
        const auto& party { field_settings_.value("party") };
        painter->drawText(party.x, party.y, data_.party);
    }

    const auto& date_time { field_settings_.value("date_time") };
    painter->drawText(date_time.x, date_time.y, data_.date_time);
}

void PrintManager::DrawTable(QPainter* painter, long long start_index, long long end_index)
{
    int columns { field_settings_.value("rows_columns").y };
    int left { field_settings_.value("left_top").x };
    int top { field_settings_.value("left_top").y };
    int heigh { field_settings_.value("heigh_width").x };
    int width { field_settings_.value("heigh_width").y };

    for (int row = 0; row != end_index - start_index; ++row) {
        const auto* trans_shadow { trans_shadow_list_.at(start_index + row) };

        for (int col = 0; col != columns; ++col) {
            QRect cellRect(left + col * width, top + row * heigh, width, heigh);
            // painter->drawRect(cellRect);
            painter->drawText(cellRect, Qt::AlignCenter, GetColumnText(col, trans_shadow));
        }
    }
}

void PrintManager::DrawFooter(QPainter* painter, int page_num, int total_pages)
{
    // Example: Draw footer with page number at the bottom of the page

    if (field_settings_.contains("employee")) {
        const auto& employee { field_settings_.value("employee") };
        painter->drawText(employee.x, employee.y, data_.employee);
    }

    const auto& unit { field_settings_.value("unit") };
    painter->drawText(unit.x, unit.y, data_.unit);

    const auto& gross_amount { field_settings_.value("gross_amount") };
    painter->drawText(gross_amount.x, gross_amount.y, QString::number(data_.gross_amount));

    const auto& page_info { field_settings_.value("page_info") };
    painter->drawText(page_info.x, page_info.y, QString::asprintf("%d/%d", page_num, total_pages));
}

QString PrintManager::GetColumnText(int col, const TransShadow* trans_shadow)
{
    switch (col) {
    case 0:
        return product_->Path(*trans_shadow->rhs_node);
    case 1:
        return stakeholder_->Path(*trans_shadow->support_id);
    case 2:
        return *trans_shadow->description;
    case 3:
        return QString::number(*trans_shadow->lhs_debit);
    case 4:
        return QString::number(*trans_shadow->lhs_credit);
    case 5:
        return QString::number(*trans_shadow->lhs_ratio);
    case 6:
        return QString::number(*trans_shadow->rhs_debit);
    default:
        return QString();
    }
}

void PrintManager::ApplyConfig(QPrinter* printer)
{
    QPageLayout layout = printer->pageLayout();

    const QString orientation { page_settings_.value("orientation").toString().toLower() };
    const QString paper_size { page_settings_.value("paper_size").toString().toLower() };

    layout.setOrientation(orientation == "landscape" ? QPageLayout::Landscape : QPageLayout::Portrait);
    layout.setPageSize(QPageSize(paper_size == "a4" ? QPageSize::A4 : QPageSize::A5));

    printer->setPageLayout(layout);
}

void PrintManager::ReadFieldPosition(QSettings& settings, const QString& section, const QString& prefix)
{
    settings.beginGroup(section);

    // Check if the key exists in settings
    if (!settings.contains(prefix)) {
        qWarning() << "Position setting not found for section:" << section << "field:" << prefix;
        return;
    }

    // Read the position value from the settings
    const auto position { settings.value(prefix).value<QVariantList>() };
    if (position.size() != 2) {
        qWarning() << "Non valid position value for section:" << section << "field:" << prefix;
        return;
    }

    // Try to convert the position parts to integers
    bool x_ok {};
    bool y_ok {};
    const int x { position[0].toInt(&x_ok) };
    const int y { position[1].toInt(&y_ok) };

    // Only store if both x and y are valid integers
    if (x_ok && y_ok) {
        field_settings_[prefix] = FieldSettings(x, y);
    } else {
        qWarning() << "Invalid position coordinates for section:" << section << "field:" << prefix << "value:" << position;
    }

    settings.endGroup();
}
