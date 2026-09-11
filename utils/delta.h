#pragma once

#include <QtCore/qglobal.h>

namespace node {

struct Delta final {
    double initial {};
    double final {};

    double count {};
    double measure {};

    bool IsNull() const { return qFuzzyIsNull(initial) && qFuzzyIsNull(final) && qFuzzyIsNull(count) && qFuzzyIsNull(measure); }
};

}
