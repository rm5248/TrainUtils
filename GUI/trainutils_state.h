/* SPDX-License-Identifier: GPL-2.0 */
#ifndef TRAINUTILS_STATE_H
#define TRAINUTILS_STATE_H

#include <memory>

namespace DBus::Qt{
class QtDispatcher;
}

class LCCManager;

struct TrainUtilsState{
    std::shared_ptr<DBus::Qt::QtDispatcher> dispatcher;
    LCCManager* lccManager;
};

#endif // TRAINUTILS_STATE_H
