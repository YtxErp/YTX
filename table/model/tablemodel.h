#ifndef TABLEMODEL_H
#define TABLEMODEL_H

// default implementations are for finance.

#include <QAbstractItemModel>

#include "component/constvalue.h"
#include "component/enumclass.h"
#include "component/info.h"
#include "component/settings.h"
#include "component/using.h"
#include "database/sqlite/sqlite.h"
#include "table/transaction.h"

class TableModel : public QAbstractItemModel {
    Q_OBJECT

public:
    TableModel(SPSqlite sql, bool node_rule, const int node_id, CInfo& info, CSectionRule& section_rule, QObject* parent = nullptr);
    virtual ~TableModel();

signals:
    // send to tree model
    void SUpdateOneTotal(int node_id, double initial_debit_diff, double initial_credit_diff, double final_debit_diff, double final_credit_diff);
    void SUpdateProperty(int node_id, double first, double third, double fourth);
    void SSearch();

    // send to signal station
    void SAppendOne(Section section, const Trans* trans);
    void SRemoveOne(Section section, int node_id, int trans_id);
    void SUpdateBalance(Section section, int node_id, int trans_id);
    void SMoveMulti(Section section, int old_node_id, int new_node_id, const QList<int>& trans_id_list);

    // send to its table view
    void SResizeColumnToContents(int column);

public slots:
    // receive from table sql
    void RRemoveMulti(const QMultiHash<int, int>& node_trans);

    // receive from tree model
    void RNodeRule(int node_id, bool node_rule);

    // receive from signal station
    void RAppendOne(const Trans* trans);
    void RRetrieveOne(Trans* trans);
    void RRemoveOne(int node_id, int trans_id);
    void RUpdateBalance(int node_id, int trans_id);
    void RMoveMulti(int old_node_id, int new_node_id, const QList<int>& trans_id_list);

public:
    // virtual functions
    virtual bool RemoveTrans(int row, const QModelIndex& parent = QModelIndex());

    // implemented functions
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    void sort(int column, Qt::SortOrder order) override;

    // member functions
    bool InsertTrans(const QModelIndex& parent = QModelIndex());

    auto GetRow(int node_id) const -> int;
    auto GetIndex(int trans_id) const -> QModelIndex;
    auto GetTrans(const QModelIndex& index) const -> Trans* { return trans_list_.at(index.row()); }

    void UpdateAllState(Check state);

protected:
    // virtual functions
    virtual bool UpdateDebit(Trans* trans, double value);
    virtual bool UpdateCredit(Trans* trans, double value);
    virtual bool UpdateRatio(Trans* trans, double value);

    virtual bool RemoveMulti(const QList<int>& trans_id_list); // just remove trnas, keep related transaction
    virtual bool InsertMulti(int node_id, const QList<int>& trans_id_list);

    // member functions
    double Balance(bool node_rule, double debit, double credit) { return (node_rule ? 1 : -1) * (credit - debit); }
    void AccumulateSubtotal(int start, bool node_rule);

    bool UpdateDateTime(SPSqlite sql, Trans* trans, CString& new_value, CString& field = DATE_TIME);
    bool UpdateDescription(SPSqlite sql, Trans* trans, CString& new_value, CString& field = DESCRIPTION);
    bool UpdateCode(SPSqlite sql, Trans* trans, CString& new_value, CString& field = CODE);
    bool UpdateOneState(SPSqlite sql, Trans* trans, bool new_value, CString& field = STATE);
    bool UpdateRelatedNode(Trans* trans, int value);

protected:
    SPSqlite sql_ {};
    bool node_rule_ {};

    CInfo& info_;
    CSectionRule& section_rule_;
    const int node_id_ {};

    QList<Trans*> trans_list_ {};

private:
    template <typename T>
    bool UpdateField(SPSqlite sql, Trans* trans, T& field_value, const T& new_value, CString& field, const std::function<void()>& action = {})
    {
        if (field_value == new_value)
            return false;

        field_value = new_value;

        if (*trans->related_node != 0) {
            sql->UpdateField(info_.transaction, field, new_value, *trans->id);

            if (action)
                action();
        }

        return true;
    }
};

#endif // TABLEMODEL_H
