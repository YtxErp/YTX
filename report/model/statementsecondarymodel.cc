#include "statementsecondarymodel.h"

#include <QDir>
#include <QFileDialog>
#include <QtConcurrent>

#include "component/constvalue.h"
#include "component/enumclass.h"
#include "document.h"
#include "global/resourcepool.h"
#include "mainwindowutils.h"

StatementSecondaryModel::StatementSecondaryModel(
    Sql* sql, CInfo& info, int party_id, CStringHash& product_leaf, PNodeModel stakeholder, CString& company_name, QObject* parent)
    : QAbstractItemModel { parent }
    , sql_ { qobject_cast<SqlO*>(sql) }
    , info_ { info }
    , party_id_ { party_id }
    , product_leaf_ { product_leaf }
    , stakeholder_leaf_ { stakeholder->LeafPath() }
    , stakeholder_ { stakeholder }
    , company_name_ { company_name }
{
}

StatementSecondaryModel::~StatementSecondaryModel() { ResourcePool<Trans>::Instance().Recycle(trans_list_); }

QModelIndex StatementSecondaryModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex StatementSecondaryModel::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int StatementSecondaryModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return trans_list_.size();
}

int StatementSecondaryModel::columnCount(const QModelIndex& /*parent*/) const { return info_.statement_secondary_header.size(); }

QVariant StatementSecondaryModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* trans { trans_list_.at(index.row()) };
    const StatementSecondaryEnum kColumn { index.column() };

    switch (kColumn) {
    case StatementSecondaryEnum::kDateTime:
        return trans->date_time;
    case StatementSecondaryEnum::kInsideProduct:
        return trans->rhs_node;
    case StatementSecondaryEnum::kOutsideProduct:
        return trans->support_id == 0 ? QVariant() : trans->support_id;
    case StatementSecondaryEnum::kFirst:
        return trans->lhs_debit == 0 ? QVariant() : trans->lhs_debit;
    case StatementSecondaryEnum::kSecond:
        return trans->lhs_credit == 0 ? QVariant() : trans->lhs_credit;
    case StatementSecondaryEnum::kUnitPrice:
        return trans->lhs_ratio == 0 ? QVariant() : trans->lhs_ratio;
    case StatementSecondaryEnum::kDescription:
        return trans->description;
    case StatementSecondaryEnum::kGrossAmount:
        return trans->rhs_debit == 0 ? QVariant() : trans->rhs_debit;
    case StatementSecondaryEnum::kSettlement:
        return trans->rhs_credit == 0 ? QVariant() : trans->rhs_credit;
    case StatementSecondaryEnum::kState:
        return trans->state ? trans->state : QVariant();
    default:
        return QVariant();
    }
}

bool StatementSecondaryModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const StatementSecondaryEnum kColumn { index.column() };
    const int kRow { index.row() };

    auto* trans { trans_list_.at(kRow) };

    switch (kColumn) {
    case StatementSecondaryEnum::kState:
        trans->state = value.toBool();
        break;
    default:
        return false;
    }

    return true;
}

QVariant StatementSecondaryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return info_.statement_secondary_header.at(section);

    return QVariant();
}

void StatementSecondaryModel::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.statement_secondary_header.size() - 1)
        return;

    auto Compare = [column, order](const Trans* lhs, const Trans* rhs) -> bool {
        const StatementSecondaryEnum kColumn { column };

        switch (kColumn) {
        case StatementSecondaryEnum::kDateTime:
            return (order == Qt::AscendingOrder) ? (lhs->date_time < rhs->date_time) : (lhs->date_time > rhs->date_time);
        case StatementSecondaryEnum::kInsideProduct:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_node < rhs->rhs_node) : (lhs->rhs_node > rhs->rhs_node);
        case StatementSecondaryEnum::kOutsideProduct:
            return (order == Qt::AscendingOrder) ? (lhs->support_id < rhs->support_id) : (lhs->support_id > rhs->support_id);
        case StatementSecondaryEnum::kFirst:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_debit < rhs->lhs_debit) : (lhs->lhs_debit > rhs->lhs_debit);
        case StatementSecondaryEnum::kSecond:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_credit < rhs->lhs_credit) : (lhs->lhs_credit > rhs->lhs_credit);
        case StatementSecondaryEnum::kUnitPrice:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_ratio < rhs->lhs_ratio) : (lhs->lhs_ratio > rhs->lhs_ratio);
        case StatementSecondaryEnum::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case StatementSecondaryEnum::kSettlement:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_credit < rhs->rhs_credit) : (lhs->rhs_credit > rhs->rhs_credit);
        case StatementSecondaryEnum::kGrossAmount:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_debit < rhs->rhs_debit) : (lhs->rhs_debit > rhs->rhs_debit);
        case StatementSecondaryEnum::kState:
            return (order == Qt::AscendingOrder) ? (lhs->state < rhs->state) : (lhs->state > rhs->state);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(trans_list_.begin(), trans_list_.end(), Compare);
    emit layoutChanged();
}

void StatementSecondaryModel::RResetModel(int unit, const QDateTime& start, const QDateTime& end)
{
    if (party_id_ <= 0 || !start.isValid() || !end.isValid())
        return;

    beginResetModel();
    if (!trans_list_.isEmpty())
        ResourcePool<Trans>::Instance().Recycle(trans_list_);

    sql_->ReadStatementSecondary(trans_list_, party_id_, unit, start, end);
    endResetModel();
}

void StatementSecondaryModel::RExport(int unit, const QDateTime& start, const QDateTime& end)
{
    double pbalance { 0.0 };
    double cdelta { 0.0 };

    if (unit != std::to_underlying(UnitO::kIS)) {
        sql_->ReadBalance(pbalance, cdelta, party_id_, unit, start, end);
    }

    CString name { QDir::homePath() + QDir::separator() + stakeholder_->Name(party_id_) + QStringLiteral("-") + company_name_ + QStringLiteral("-")
        + end.toString(kMonthFST) };

    QString destination { QFileDialog::getSaveFileName(nullptr, tr("Export Excel"), name, QStringLiteral("*.xlsx")) };
    if (!MainWindowUtils::PrepareNewFile(destination, kDotSuffixXLSX))
        return;

    auto future = QtConcurrent::run([=, this]() {
        try {
            YXlsx::Document d(destination);

            auto book { d.GetWorkbook() };

            book->AppendSheet(tr("Statement"));
            auto sheet { book->GetCurrentWorksheet() };

            sheet->Write(1, 1, stakeholder_->Name(party_id_));
            sheet->Write(2, 1, tr("Date"));
            sheet->Write(2, 2, start.toString(kDateFST));
            sheet->Write(2, 3, end.toString(kDateFST));
            sheet->Write(3, 1, tr("Previous Balance"));
            sheet->Write(3, 2, pbalance);
            sheet->Write(4, 1, tr("Current Delta"));
            sheet->Write(4, 3, cdelta);
            sheet->Write(5, 1, tr("Current Balance"));
            sheet->Write(5, 3, pbalance + cdelta);

            const QStringList header { tr("Date"), tr("Inside"), tr("Outside"), tr("First"), tr("Second"), tr("UnitPrice"), tr("Description"),
                tr("GrossAmount") };
            sheet->WriteRow(7, 4, header);

            const qsizetype rows { trans_list_.size() };

            QList<QVariantList> list(rows);

            qsizetype row_index { 0 };
            for (const auto* trans : std::as_const(trans_list_)) {
                list[row_index] << trans->date_time << product_leaf_.value(trans->rhs_node) << stakeholder_leaf_.value(trans->support_id) << trans->lhs_debit
                                << trans->lhs_credit << trans->lhs_ratio << trans->description << trans->rhs_debit;
                ++row_index;
            }

            for (qsizetype start_row = 0; start_row != rows; ++start_row) {
                sheet->WriteRow(start_row + 8, 4, list.at(start_row));
            }

            d.Save();
            return true;
        } catch (...) {
            qWarning() << "Export failed due to an unknown exception.";
            return false;
        }
    });

    auto* watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [watcher, destination]() {
        watcher->deleteLater();

        bool success { watcher->future().result() };
        if (success) {
            MainWindowUtils::Message(QMessageBox::Information, tr("Export Completed"), tr("Export completed successfully."), kThreeThousand);
        } else {
            QFile::remove(destination);
            MainWindowUtils::Message(QMessageBox::Critical, tr("Export Failed"), tr("Export failed. The file has been deleted."), kThreeThousand);
        }
    });

    watcher->setFuture(future);
}
