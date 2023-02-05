/* SPDX-License-Identifier: GPL-2.0 */
#ifndef THROTTLE_H
#define THROTTLE_H

#include <QObject>

enum class LocomotiveDirection{
    Forward,
    Reverse,
};

/**
 * A generic Throttle that can be used to control a locomotive.
 */
class Throttle : public QObject
{
    Q_OBJECT
public:
    explicit Throttle(QObject *parent = nullptr);
    ~Throttle();

    virtual void selectLocomotive(int locomotiveNumber, bool isLong) = 0;

Q_SIGNALS:
    void locomotiveSelected();

public Q_SLOTS:
    /**
     * Set the speed(0-100%)
     * @param speed
     */
    virtual void setSpeed(int speed) = 0;

    virtual void setDirection(LocomotiveDirection dir) = 0;

    /**
     * Emergency stop the selected locomotive.
     */
    virtual void estop() = 0;

    /**
     * Set the specified function to be on or off.
     */
    virtual void setFunction(int funcNum, bool on) = 0;

    /**
     * Toggle the specified function.  If the function is off, it will be turned on.
     * If the function is on, it will be turned off.
     */
    virtual void toggleFunction(int funcNum) = 0;

    /**
     * Release control of this locomotive.
     */
    virtual void releaseLocomotive() = 0;

};

#endif // THROTTLE_H
