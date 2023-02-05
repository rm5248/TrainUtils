/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LOCONETTHROTTLE_H
#define LOCONETTHROTTLE_H

#include <QObject>
#include <memory>

#include "../throttle.h"
#include "loconet_throttle.h"

class LoconetConnection;
struct loconet_context;

class LoconetThrottle : public Throttle
{
    Q_OBJECT
public:
    explicit LoconetThrottle(loconet_context* conn,
                             QObject *parent = nullptr);
    ~LoconetThrottle();

    virtual void selectLocomotive(int locomotiveNumber, bool isLong);

public Q_SLOTS:
    virtual void setSpeed(int speed);
    virtual void setDirection(LocomotiveDirection dir);
    virtual void estop();
    virtual void setFunction(int funcNum, bool on);
    virtual void toggleFunction(int funcNum);
    virtual void releaseLocomotive();

private:
    struct loconet_throttle* m_throttle;

    friend class LoconetConnection;
};

#endif // LOCONETTHROTTLE_H
