#include "tablemodelutils.h"

#include <QtConcurrent>

void TableModelUtils::AccumulateSubtotal(QMutex& mutex, QList<TransShadow*>& trans_shadow_list, int start, bool rule)
{
    if (start <= -1 || start >= trans_shadow_list.size() || trans_shadow_list.isEmpty())
        return;

    auto future = QtConcurrent::run([&, start, rule]() {
        QMutexLocker locker(&mutex);
        double previous_subtotal { start >= 1 ? trans_shadow_list.at(start - 1)->subtotal : 0.0 };

        std::accumulate(trans_shadow_list.begin() + start, trans_shadow_list.end(), previous_subtotal, [&](double current_subtotal, TransShadow* trans_shadow) {
            trans_shadow->subtotal = Balance(rule, *trans_shadow->lhs_debit, *trans_shadow->lhs_credit) + current_subtotal;
            return trans_shadow->subtotal;
        });
    });
}

bool TableModelUtils::UpdateRhsNode(TransShadow* trans_shadow, int value)
{
    if (*trans_shadow->rhs_node == value)
        return false;

    *trans_shadow->rhs_node = value;
    return true;
}
