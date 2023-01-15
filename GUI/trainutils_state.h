/* SPDX-License-Identifier: GPL-2.0 */
#ifndef TRAINUTILS_STATE_H
#define TRAINUTILS_STATE_H

#include <memory>

namespace DBus::Qt{
class QtDispatcher;
}

class LCCManager;
class MDNSManager;

struct TrainUtilsState{
    LCCManager* lccManager;
    MDNSManager* mdnsManager;
};

#endif // TRAINUTILS_STATE_H
