#ifndef SORTFILTERPROXYMODEL_H
#define SORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QStandardItemModel>

class SortFilterProxyModel : public QSortFilterProxyModel {
public:
    explicit SortFilterProxyModel(int leaf_id, QObject* parent = nullptr)
        : QSortFilterProxyModel { parent }
        , leaf_id_ { leaf_id }
    {
    }

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& /*source_parent*/) const override
    {
        assert(dynamic_cast<QStandardItemModel*>(sourceModel()) && "sourceModel() is not QStandardItemModel");
        auto* item { static_cast<QStandardItemModel*>(sourceModel())->item(source_row) };
        if (!item)
            return false;

        return item->data(Qt::UserRole).toInt() != leaf_id_;
    }

private:
    const int leaf_id_ {};
};

#endif // SORTFILTERPROXYMODEL_H
