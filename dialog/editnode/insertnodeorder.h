#ifndef INSERTNODEORDER_H
#define INSERTNODEORDER_H

#include <QComboBox>
#include <QDialog>

#include "component/info.h"
#include "component/settings.h"
#include "tree/model/abstracttreemodel.h"

namespace Ui {
class InsertNodeOrder;
}

class InsertNodeOrder final : public QDialog {
    Q_OBJECT

public:
    InsertNodeOrder(Node* node, CSectionRule& section_rule, AbstractTreeModel* order_model, AbstractTreeModel* stakeholder_model,
        const AbstractTreeModel& product_model, CInfo& info, QWidget* parent = nullptr);
    ~InsertNodeOrder();

public slots:
    void accept() override;
    void reject() override;
    void RUpdateStakeholder();

private slots:
    void on_chkBoxBranch_toggled(bool checked);
    void on_comboParty_currentTextChanged(const QString& arg1);
    void on_comboParty_currentIndexChanged(int index);
    void on_chkBoxRefund_toggled(bool checked);
    void on_comboEmployee_currentIndexChanged(int index);
    void on_rBtnCash_toggled(bool checked);
    void on_rBtnMonthly_toggled(bool checked);
    void on_rBtnPending_toggled(bool checked);
    void on_pBtnInsertParty_clicked();
    void on_dateTimeEdit_dateTimeChanged(const QDateTime& date_time);
    void on_pBtnPostOrder_toggled(bool checked);

    void on_dSpinDiscount_editingFinished();

private:
    void IniDialog();
    void IniCombo(QComboBox* combo);
    void IniCombo(QComboBox* combo, int mark);
    void IniConnect();
    void SetData();
    void SetWidgetsEnabled(bool enabled);
    void SetWidgetsEnabledBranch(bool enabled);
    void SetWidgetsEnabledPost(bool enabled);
    void ZeroSettlement();
    void EnabledPost(bool enabled);
    void Save();

private:
    Ui::InsertNodeOrder* ui;

    Node* node_ {};
    CInfo& info_ {};
    CSectionRule& section_rule_ {};
    AbstractTreeModel* stakeholder_model_ {};
    AbstractTreeModel* order_model_ {};
    const AbstractTreeModel& product_model_;
    bool is_saved_ { false };
    bool is_modified_ { false };
};

#endif // INSERTNODEORDER_H
