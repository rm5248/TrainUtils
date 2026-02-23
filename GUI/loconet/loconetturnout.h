#ifndef LOCONETTURNOUT_H
#define LOCONETTURNOUT_H

#include <QObject>

#include "../common/turnout.h"
#include "loconet_turnout.h"

class LoconetConnection;

class LoconetTurnout : public Turnout
{
    Q_OBJECT
public:
    explicit LoconetTurnout(QObject *parent = nullptr);

    void setConnection(LoconetConnection* lnConn);
    void setNumber(int number);

    int number();

Q_SIGNALS:

protected:
    void sendCommandOverBus();

private:
    void incomingTurnoutCommand(enum loconet_turnout_status state);

private:
    LoconetConnection* m_conn = nullptr;
    int m_number = 0;

    friend class LoconetConnection;
};

#endif // LOCONETTURNOUT_H
