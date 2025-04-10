#include "printmanager.h"

#include <QFile>
#include <QPrintPreviewDialog>
#include <QRegularExpression>

PrintManager::PrintManager(NodeModel* product, NodeModel* stakeholder)
    : product_ { product }
    , stakeholder_ { stakeholder }
{
}

bool PrintManager::LoadHtml(const QString& file_path)
{
    QFile file(file_path);

    // Check if the template file exists
    if (!file.exists()) {
        qWarning() << "Template file does not exist:" << file_path;
        return false;
    }

    // Try to open the file in read-only text mode
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open template file:" << file_path;
        return false;
    }

    // Read all content
    QTextStream in(&file);
    html_ = in.readAll();

    file.close();
    ReadConfig();
    return true;
}

void PrintManager::SetData(const PrintData& data, QList<TransShadow*> trans_shadow)
{
    trans_shadow_ = trans_shadow;

    const long long total_pages { (trans_shadow_.size() + rows_per_page_ - 1) / rows_per_page_ };

    html_.replace("{{party}}", data.party);
    html_.replace("{{date_time}}", data.date_time);
    html_.replace("{{employee}}", data.employee);
    html_.replace("{{gross_amount}}", QString::number(data.gross_amount));
    html_.replace("{{unit}}", data.unit);
    html_.replace("{{total_pages}}", QString::number(total_pages));
}

void PrintManager::Preview()
{
    QPrinter printer { QPrinter::ScreenResolution };
    ApplyConfig(&printer);

    QPrintPreviewDialog preview(&printer);

    QObject::connect(&preview, &QPrintPreviewDialog::paintRequested, &preview, [this](QPrinter* printer) { this->RenderAllPages(printer); });
    preview.exec();
}

void PrintManager::Print()
{
    const long long total_pages { (trans_shadow_.size() + rows_per_page_ - 1) / rows_per_page_ };
    QPrinter printer { QPrinter::ScreenResolution };
    ApplyConfig(&printer);

    for (long long page = 0; page != total_pages; ++page) {
        long long start { page * rows_per_page_ };
        long long end { std::min(start + rows_per_page_, trans_shadow_.size()) };

        QString rows_html {};
        for (long long i = start; i != end; ++i) {
            rows_html += BuildProductRow(trans_shadow_[i]);
        }

        QString page_html = html_;
        page_html.replace("{{products}}", rows_html);
        page_html.replace("{{page}}", QString::number(page + 1));

        QTextDocument document {};

        document.setHtml(page_html);
        document.print(&printer);
    }
}

QString PrintManager::BuildProductRow(TransShadow* item)
{
    static const QString row_template { QStringLiteral(R"(
        <tr>
            <td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td><td>%7</td>
        </tr>
        )") };

    return row_template.arg(product_->Name(*item->rhs_node))
        .arg(stakeholder_->Name(*item->support_id))
        .arg(*item->description)
        .arg(*item->lhs_debit)
        .arg(*item->lhs_credit)
        .arg(*item->lhs_ratio)
        .arg(*item->rhs_debit);
}

void PrintManager::RenderAllPages(QPrinter* printer)
{
    const long long total_pages { (trans_shadow_.size() + rows_per_page_ - 1) / rows_per_page_ };

    for (long long page = 0; page != total_pages; ++page) {
        long long start { page * rows_per_page_ };
        long long end { std::min(start + rows_per_page_, trans_shadow_.size()) };

        QString rows_html {};
        for (long long i = start; i != end; ++i) {
            rows_html += BuildProductRow(trans_shadow_[i]);
        }

        QString page_html { html_ };
        page_html.replace("{{products}}", rows_html);
        page_html.replace("{{page}}", QString::number(page + 1));

        QTextDocument document {};

        document.setHtml(page_html);
        document.print(printer);

        if (page + 1 != total_pages)
            printer->newPage();
    }
}

QString PrintManager::ReadHtmlConfig(const QString& html, const QString& id) const
{
    QRegularExpression re(QString(R"(<div\s+id="%1"[^>]*>\s*([^<]*?)\s*</div>)").arg(QRegularExpression::escape(id)));
    QRegularExpressionMatch match = re.match(html);
    return match.hasMatch() ? match.captured(1).trimmed() : QString {};
}

void PrintManager::ReadConfig()
{
    rows_per_page_ = ReadHtmlConfig(html_, "rows-per-page").toInt();
    paper_size_ = ReadHtmlConfig(html_, "paper-size");
    paper_orientation_ = ReadHtmlConfig(html_, "paper-orientation");
}

void PrintManager::ApplyConfig(QPrinter* printer) const
{
    QPageLayout layout = printer->pageLayout();

    if (paper_orientation_.compare("landscape", Qt::CaseInsensitive) == 0) {
        layout.setOrientation(QPageLayout::Landscape);
    } else {
        layout.setOrientation(QPageLayout::Portrait);
    }

    if (paper_size_.compare("a4", Qt::CaseInsensitive) == 0) {
        layout.setPageSize(QPageSize(QPageSize::A4));
    } else if (paper_size_.compare("a5", Qt::CaseInsensitive) == 0) {
        layout.setPageSize(QPageSize(QPageSize::A5));
    } else {
        layout.setPageSize(QPageSize(QPageSize::A5));
    }

    printer->setPageLayout(layout);
}
