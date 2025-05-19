#include "preferences.h"

#include <QCompleter>
#include <QDir>
#include <QFileDialog>
#include <QPrinterInfo>
#include <QStandardPaths>
#include <QTimer>

#include "component/constvalue.h"
#include "component/signalblocker.h"
#include "ui_preferences.h"

Preferences::Preferences(CInfo& info, CNodeModel* model, AppConfig app, FileConfig file, SectionConfig section, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::Preferences)
    , app_ { app }
    , file_ { file }
    , section_ { section }
    , model_ { model }

{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    leaf_path_branch_path_model_ = new QStandardItemModel(this);
    model_->LeafPathBranchPathModel(leaf_path_branch_path_model_);
    leaf_path_branch_path_model_->sort(0);

    IniStringList();
    IniDialog(info.unit_model);
    IniConnect();

    QTimer::singleShot(100, this, [this, info]() { IniData(); });
    IniText(info.section);
}

Preferences::~Preferences() { delete ui; }

void Preferences::IniDialog(QStandardItemModel* unit_model)
{
    ui->listWidget->setCurrentRow(0);
    ui->stackedWidget->setCurrentIndex(0);
    ui->pBtnOk->setDefault(true);
    this->setWindowTitle(tr("Preferences"));

    IniCombo(ui->comboDateFormat, date_format_list_);
    IniCombo(ui->comboLanguage, language_list_);
    IniCombo(ui->comboSeparator, separator_list_);
    IniCombo(ui->comboTheme, theme_list_);

    ui->comboDefaultUnit->setModel(unit_model);

    ui->comboStatic->setModel(leaf_path_branch_path_model_);
    ui->comboDynamicLhs->setModel(leaf_path_branch_path_model_);
    ui->comboDynamicRhs->setModel(leaf_path_branch_path_model_);

    IniCombo(ui->comboOperation, operation_list_);

    ui->comboPrinter->addItem(QString());
    const auto& printers { QPrinterInfo::availablePrinters() };

    for (const QPrinterInfo& printer : printers) {
        ui->comboPrinter->addItem(printer.printerName());
    }
}

void Preferences::IniCombo(QComboBox* combo, CStringList& list) { combo->addItems(list); }

void Preferences::IniConnect() { connect(ui->pBtnOk, &QPushButton::clicked, this, &Preferences::on_pBtnApply_clicked); }

void Preferences::IniData()
{
    IniDataCombo(ui->comboTheme, app_.theme);
    IniDataCombo(ui->comboLanguage, app_.language);
    IniDataCombo(ui->comboSeparator, app_.separator);
    IniDataCombo(ui->comboPrinter, app_.printer);

    ui->lineCompanyName->setText(file_.company_name);

    IniDataCombo(ui->comboDateFormat, section_.date_format);
    IniDataCombo(ui->comboDefaultUnit, section_.default_unit);
    ui->pBtnDocumentDir->setText(section_.document_path);
    ui->spinAmountDecimal->setValue(section_.amount_decimal);
    ui->spinCommonDecimal->setValue(section_.common_decimal);

    ui->lineStatic->setText(section_.static_label);
    IniDataCombo(ui->comboStatic, section_.static_node);
    ui->lineDynamic->setText(section_.dynamic_label);
    IniDataCombo(ui->comboDynamicLhs, section_.dynamic_node_lhs);
    IniDataCombo(ui->comboOperation, section_.operation);
    IniDataCombo(ui->comboDynamicRhs, section_.dynamic_node_rhs);

    ResizeLine(ui->lineStatic, section_.static_label);
    ResizeLine(ui->lineDynamic, section_.dynamic_label);
}

void Preferences::IniDataCombo(QComboBox* combo, int value)
{
    int item_index { combo->findData(value) };
    combo->setCurrentIndex(item_index);
}

void Preferences::IniDataCombo(QComboBox* combo, CString& string)
{
    int item_index { combo->findText(string) };
    combo->setCurrentIndex(item_index);
}

void Preferences::IniStringList()
{
    language_list_.emplaceBack(kEnUS);
    language_list_.emplaceBack(kZhCN);

    separator_list_.emplaceBack(kDash);
    separator_list_.emplaceBack(kColon);
    separator_list_.emplaceBack(kSlash);

    theme_list_.emplaceBack(kSolarizedDark);

    operation_list_.emplaceBack(kPlus);
    operation_list_.emplaceBack(kMinux);

    date_format_list_.emplaceBack(kDateTimeFST);
    date_format_list_.emplaceBack(kDateFST);
}

void Preferences::on_pBtnApply_clicked() { emit SUpdateSettings(app_, file_, section_); }

void Preferences::on_pBtnDocumentDir_clicked()
{
    const QString base_path { QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) };
    const QString current_relative { ui->pBtnDocumentDir->text() };
    const QString current_absolute { QDir(base_path).filePath(current_relative) };

    QDir().mkpath(current_absolute);

    const QString selected_path { QFileDialog::getExistingDirectory(this, tr("Select Directory"), current_absolute) };
    if (!selected_path.isEmpty()) {
        const QString relative_path { (selected_path == base_path) ? QString() : QDir(base_path).relativeFilePath(selected_path) };
        section_.document_path = relative_path;
        ui->pBtnDocumentDir->setText(relative_path);
    }
}

void Preferences::on_pBtnResetDocumentDir_clicked()
{
    section_.document_path = QString();
    ui->pBtnDocumentDir->setText(QString());
}

void Preferences::ResizeLine(QLineEdit* line, CString& text) { line->setMinimumWidth(QFontMetrics(line->font()).horizontalAdvance(text) + 8); }

void Preferences::IniText(Section section)
{
    switch (section) {
    case Section::kFinance:
        ui->labelCommonDecimal->setText(tr("FXRate Decimal"));
        ui->labelDefaultUnit->setText(tr("Local Currency"));
        break;
    case Section::kStakeholder:
        ui->labelCommonDecimal->setText(tr("Placeholder"));
        break;
    case Section::kTask:
    case Section::kProduct:
    case Section::kSales:
    case Section::kPurchase:
        ui->labelCommonDecimal->setText(tr("Quantity Decimal"));
        break;
    default:
        break;
    }
}

void Preferences::on_comboDefaultUnit_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    section_.default_unit = ui->comboDefaultUnit->currentData().toInt();
}

void Preferences::on_comboStatic_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    section_.static_node = ui->comboStatic->currentData().toInt();
}

void Preferences::on_comboDynamicLhs_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    section_.dynamic_node_lhs = ui->comboDynamicLhs->currentData().toInt();
}

void Preferences::on_comboDynamicRhs_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    section_.dynamic_node_rhs = ui->comboDynamicRhs->currentData().toInt();
}

void Preferences::on_spinAmountDecimal_editingFinished() { section_.amount_decimal = ui->spinAmountDecimal->value(); }

void Preferences::on_lineStatic_editingFinished()
{
    section_.static_label = ui->lineStatic->text();
    ResizeLine(ui->lineStatic, section_.static_label);
}

void Preferences::on_lineDynamic_editingFinished()
{
    section_.dynamic_label = ui->lineDynamic->text();
    ResizeLine(ui->lineDynamic, section_.dynamic_label);
}

void Preferences::on_spinCommonDecimal_editingFinished() { section_.common_decimal = ui->spinCommonDecimal->value(); }

void Preferences::on_comboTheme_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    app_.theme = ui->comboTheme->currentText();
}

void Preferences::on_comboLanguage_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    app_.language = ui->comboLanguage->currentText();
}

void Preferences::on_comboDateFormat_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    section_.date_format = ui->comboDateFormat->currentText();
}

void Preferences::on_comboSeparator_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    app_.separator = ui->comboSeparator->currentText();
}

void Preferences::on_comboOperation_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    section_.operation = ui->comboOperation->currentText();
}

void Preferences::on_lineCompanyName_editingFinished()
{
    file_.company_name = ui->lineCompanyName->text();
    ResizeLine(ui->lineCompanyName, file_.company_name);
}

void Preferences::on_comboPrinter_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    app_.printer = ui->comboPrinter->currentText();
}
