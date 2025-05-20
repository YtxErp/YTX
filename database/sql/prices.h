#ifndef PRICES_H
#define PRICES_H

#include <QString>

struct PriceS {
    QString date_time {};
    int lhs_node {};
    int inside_product {};
    double unit_price {};
};

#endif // PRICES_H
