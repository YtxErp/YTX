#include "printmanager.h"

#include <QFile>
#include <QPainter>
#include <QPrintPreviewDialog>

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

    page_settings_["paper_size"] = settings.value("page_settings/paper_size");
    page_settings_["orientation"] = settings.value("page_settings/orientation");
    page_settings_["rows_per_page"] = settings.value("page_settings/rows_per_page").toInt();
    page_settings_["font_size"] = settings.value("page_settings/font_size").toInt();

    const QList<QString> header_fields { "party", "date_time" };
    const QList<QString> content_fields { "inside_product", "outside_product", "description", "first", "second", "unit_price", "amount" };
    const QList<QString> footer_fields { "employee", "gross_amount", "page_info" };

    // Read fields for header section
    for (const QString& field : header_fields) {
        ReadFieldGeometry(settings, field_geometry_, "header", field);
    }

    // Read fields for content section
    for (const QString& field : content_fields) {
        ReadFieldGeometry(settings, field_geometry_, "content", field);
    }

    // Read fields for footer section
    for (const QString& field : footer_fields) {
        ReadFieldGeometry(settings, field_geometry_, "footer", field);
    }

    return true;
}

void PrintManager::RenderAllPages(QPrinter* printer)
{
    QPainter painter {};
    if (!painter.begin(printer)) {
        qWarning("Failed to start painter for printing.");
        return;
    }

    QFont font {};
    font.setPointSize(page_settings_["font_size"].toInt());
    painter.setFont(font);

    int rows_per_page = page_settings_["rows_per_page"].toInt();
    int current_row = 0; // 当前绘制的行号
    int total_rows = trans_shadow_.size();

    RenderHeader(&painter);

    // 当前页面内容的 y 坐标
    int y_offset = field_geometry_["inside_product"].y;

    for (int i = 0; i != total_rows; ++i) {
        const TransShadow* item = trans_shadow_.at(i);

        // 获取每列的字段位置
        const FieldGeometry& inside_product_geom = field_geometry_["inside_product"];
        const FieldGeometry& outside_product_geom = field_geometry_["outside_product"];
        const FieldGeometry& description_geom = field_geometry_["description"];
        const FieldGeometry& first_geom = field_geometry_["first"];
        const FieldGeometry& second_geom = field_geometry_["second"];
        const FieldGeometry& unit_price_geom = field_geometry_["unit_price"];
        const FieldGeometry& amount_geom = field_geometry_["amount"];

        // 绘制每列的文本
        painter.drawText(inside_product_geom.x, y_offset, product_->Name(*item->rhs_node));
        painter.drawText(outside_product_geom.x, y_offset, stakeholder_->Name(*item->support_id));
        painter.drawText(description_geom.x, y_offset, *item->description);
        painter.drawText(first_geom.x, y_offset, QString::number(*item->lhs_debit));
        painter.drawText(second_geom.x, y_offset, QString::number(*item->lhs_credit));
        painter.drawText(unit_price_geom.x, y_offset, QString::number(*item->lhs_ratio));
        painter.drawText(amount_geom.x, y_offset, QString::number(*item->rhs_debit));

        // 更新 y 坐标，准备绘制下一行
        y_offset += inside_product_geom.y; // 假设 y 位置是固定的

        // 检查是否需要分页
        current_row++;
        if (current_row >= rows_per_page) {
            // 达到每页最大行数时，插入分页符
            painter.end(); // 结束当前页面的绘制

            printer->newPage(); // 创建新的一页

            // 重新开始新的页面绘制
            painter.begin(printer);

            // 绘制新的页眉
            RenderHeader(&painter);

            // 重置 y 坐标
            y_offset = field_geometry_["inside_product"].y;

            // 重置行计数器
            current_row = 0;
        }
    }

    // 绘制页脚
    RenderFooter(&painter);

    painter.end();
}

void PrintManager::RenderHeader(QPainter* painter) const
{
    const FieldGeometry& party_geometry { field_geometry_["party"] };
    const FieldGeometry& date_time_geometry { field_geometry_["date_time"] };

    painter->drawText(party_geometry.x, party_geometry.y, data_.party);
    painter->drawText(date_time_geometry.x, date_time_geometry.y, data_.date_time);
}

void PrintManager::RenderFooter(QPainter* painter) const
{
    const FieldGeometry& employee_geometry = field_geometry_["employee"];
    const FieldGeometry& gross_amount_geometry = field_geometry_["gross_amount"];

    painter->drawText(employee_geometry.x, employee_geometry.y, data_.employee);
    painter->drawText(gross_amount_geometry.x, gross_amount_geometry.y, QString::number(data_.gross_amount));
}

void PrintManager::ApplyConfig(QPrinter* printer) const
{
    QPageLayout layout = printer->pageLayout();

    if (page_settings_["orientation"].compare("landscape", Qt::CaseInsensitive) == 0) {
        layout.setOrientation(QPageLayout::Landscape);
    } else {
        layout.setOrientation(QPageLayout::Portrait);
    }

    if (page_settings_["paper_size"].compare("a4", Qt::CaseInsensitive) == 0) {
        layout.setPageSize(QPageSize(QPageSize::A4));
    } else if (page_settings_["paper_size"].compare("a5", Qt::CaseInsensitive) == 0) {
        layout.setPageSize(QPageSize(QPageSize::A5));
    } else {
        layout.setPageSize(QPageSize(QPageSize::A5));
    }

    printer->setPageLayout(layout);
}

void PrintManager::ReadFieldGeometry(QSettings& settings, QHash<QString, FieldGeometry>& field_geometry, const QString& section, const QString& prefix)
{
    // Read the x, y, width values from the settings for a given section and prefix
    QString field_name { prefix + "_x" };
    int x = settings.value(section + "/" + field_name).toInt();

    field_name = prefix + "_y";
    int y = settings.value(section + "/" + field_name).toInt();

    field_name = prefix + "_width";
    int width = settings.value(section + "/" + field_name).toInt();

    // Store the values in the field_geometry hash map
    field_geometry[prefix] = FieldGeometry { x, y, width };
}
