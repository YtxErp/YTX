#include "printmanager.h"

#include <QFile>
#include <QFont>
#include <QPrintPreviewDialog>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextTableCell>
#include <QVariant>

PrintManager::PrintManager(NodeModel* product, NodeModel* stakeholder)
    : product_ { product }
    , stakeholder_ { stakeholder }
{
}

void PrintManager::SetData(const PrintData& data, QList<TransShadow*> trans_shadow)
{
    data_ = data;
    trans_shadow_ = trans_shadow;
}

void PrintManager::Preview()
{
    QPrinter printer { QPrinter::ScreenResolution };
    ApplyConfig(&printer);

    QPrintPreviewDialog preview(&printer);

    QObject::connect(&preview, &QPrintPreviewDialog::paintRequested, &preview, [this](QPrinter* printer) { this->RenderAllPages(printer); });
    preview.exec();
}

void PrintManager::Print() { }

bool PrintManager::LoadIni(const QString& file_path)
{
    // Page settings
    QSettings settings(file_path, QSettings::IniFormat);

    page_settings_["paper_size"] = settings.value("page_settings/paper_size", "A5");
    page_settings_["orientation"] = settings.value("page_settings/orientation", "landscape");
    page_settings_["rows_per_page"] = settings.value("page_settings/rows_per_page", "7").toInt();
    page_settings_["font_size"] = settings.value("page_settings/font_size", "12").toInt();
    page_settings_["row"] = settings.value("page_settings/row", "11").toInt();
    page_settings_["column"] = settings.value("page_settings/column", "7").toInt();

    const QList<QString> header_fields { "party", "date_time" };
    const QList<QString> content_fields { "content" };
    const QList<QString> footer_fields { "employee", "gross_amount", "page_info" };

    // Read fields for header section
    for (const QString& field : header_fields) {
        ReadFieldPosition(settings, field_position_, "header", field);
    }

    // Read fields for content section
    for (const QString& field : content_fields) {
        ReadFieldPosition(settings, field_position_, "content", field);
    }

    // Read fields for footer section
    for (const QString& field : footer_fields) {
        ReadFieldPosition(settings, field_position_, "footer", field);
    }

    return true;
}

void PrintManager::RenderAllPages(QPrinter* printer)
{
    // Set font size from configuration
    QFont font {};
    font.setPointSize(page_settings_["font_size"].toInt());

    // Fetch configuration values for rows and columns
    const int rows_per_page { page_settings_["rows_per_page"].toInt() }; // Number of rows per page
    const int total_rows { page_settings_["row"].toInt() }; // Total rows of data
    const int total_columns { page_settings_["column"].toInt() }; // Total columns

    // Get the starting position for content
    const QPoint content_position { field_position_["content"].row, field_position_["content"].column };

    // Calculate total pages required based on the total rows and rows per page
    const long long total_pages { (trans_shadow_.size() + rows_per_page - 1) / rows_per_page }; // Ceiling division to determine total pages

    // Create QTextDocument for rendering the content
    QTextDocument document {};
    QTextCursor cursor(&document);

    // Insert a table with the total rows and total columns
    QTextTable* table = cursor.insertTable(total_rows, total_columns);

    for (int row = 0; row < total_rows; ++row) {
        for (int col = 0; col < total_columns; ++col) {
            cursor = table->cellAt(row, col).firstCursorPosition();
            cursor.insertText(QString("Row %1, Col %2").arg(row + 1).arg(col + 1)); // Example content
        }
    }

    // Use QPainter to render the table from QTextDocument
    document.print(printer);
}

void PrintManager::ApplyConfig(QPrinter* printer)
{
    QPageLayout layout = printer->pageLayout();

    const QString orientation { page_settings_.value("orientation").toString().toLower() };
    const QString paper_size { page_settings_.value("paper_size").toString().toLower() };

    layout.setPageSize(QPageSize(paper_size == "a4" ? QPageSize::A4 : QPageSize::A5));
    layout.setOrientation(orientation == "landscape" ? QPageLayout::Landscape : QPageLayout::Portrait);

    printer->setPageLayout(layout);
}

void PrintManager::ReadFieldPosition(QSettings& settings, QHash<QString, FieldPosition>& field_position, const QString& section, const QString& prefix)
{
    settings.beginGroup(section);

    // Construct the complete key for the settings value
    const QString full_key { prefix + "_position" };

    // Check if the key exists in settings
    if (!settings.contains(full_key)) {
        qWarning() << "Position setting not found for section:" << section << "field:" << prefix;
        return;
    }

    // Read the position value from the settings
    const auto position { settings.value(full_key).value<QVariantList>() };
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
        field_position[prefix] = FieldPosition(x, y);
    } else {
        qWarning() << "Invalid position coordinates for section:" << section << "field:" << prefix << "value:" << position;
    }

    settings.endGroup();
}
