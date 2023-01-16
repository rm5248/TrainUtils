/* SPDX-License-Identifier: GPL-2.0 */
#ifndef TRAINUTILS_STATE_H
#define TRAINUTILS_STATE_H

#include <memory>

namespace DBus::Qt{
class QtDispatcher;
}

class LCCManager;
class MDNSManager;
class LoconetManager;

struct TrainUtilsState{
    LCCManager* lccManager;
    MDNSManager* mdnsManager;
    LoconetManager* loconetManager;
};

#endif // TRAINUTILS_STATE_H
