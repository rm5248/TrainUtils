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

}

void LoconetTurnout::setConnection(LoconetConnection* lnConn){
    m_conn = lnConn;
}
