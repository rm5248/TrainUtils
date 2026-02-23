#include "loconetturnout.h"
#include "loconetconnection.h"

LoconetTurnout::LoconetTurnout(QObject *parent)
    : Turnout{parent}
{}

void LoconetTurnout::incomingTurnoutCommand(enum loconet_turnout_status state){
    TurnoutState newState;

    switch(state){
    case LOCONET_TURNOUT_UNKNOWN:
        newState = TurnoutState::Unknown;
        break;
    case  LOCONET_TURNOUT_THROWN:
        newState = TurnoutState::Thrown;
        break;
    case LOCONET_TURNOUT_CLOSED:
        newState = TurnoutState::Closed;
        break;
    }

    if(m_state != newState){
        m_state = newState;
        Q_EMIT stateChanged();
    }
}

void LoconetTurnout::sendCommandOverBus(){
    assert(m_conn);

    if(m_state == TurnoutState::Closed){
        m_conn->closeTurnout(m_number);
    }else if(m_state == TurnoutState::Thrown){
        m_conn->throwTurnout(m_number);
    }
}

void LoconetTurnout::setNumber(int number){
    m_number = number;
}

void LoconetTurnout::setConnection(LoconetConnection* lnConn){
    m_conn = lnConn;
}

int LoconetTurnout::number(){
    return m_number;
}
