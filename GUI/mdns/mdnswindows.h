/* SPDX-License-Identifier: GPL-2.0 */
#ifndef MDNSWINDOWS_H
#define MDNSWINDOWS_H

#include <QObject>

#include "mdnsmanager.h"

class MDNSWindows : public MDNSManager
{
    Q_OBJECT
public:
    explicit MDNSWindows(QObject *parent = nullptr);


};

#endif // MDNSWINDOWS_H
