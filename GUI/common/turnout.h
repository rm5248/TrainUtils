#ifndef TURNOUT_H
#define TURNOUT_H

#include <QObject>
#include <QMetaEnum>

/**
 * Represents a turnout.  Specific connections will make concrete subclasses of this type.
 */
class Turnout : public QObject
{
    Q_OBJECT
public:
    enum class TurnoutState {
        Unknown,
        Thrown,
        Closed,
    };
    Q_ENUM(TurnoutState)

    Q_PROPERTY(TurnoutState state READ getState WRITE setState NOTIFY stateChanged)

    explicit Turnout(QObject *parent = nullptr);

    TurnoutState getState();
    void setState(TurnoutState ts);

Q_SIGNALS:
    void stateChanged();

public Q_SLOTS:
    void throwTurnout();
    void closeTurnout();

protected:
    /**
     * Implemented by subclasses in order to send the command to put the turnout
     * into the specified state over the bus.
     */
    virtual void sendCommandOverBus() = 0;

protected:
    TurnoutState m_state;
};

#endif // TURNOUT_H
