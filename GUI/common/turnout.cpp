#include "turnout.h"

Turnout::Turnout(QObject *parent)
    : QObject{parent},
    m_state(TurnoutState::Unknown)
{}

Turnout::TurnoutState Turnout::getState(){
    return m_state;
}

void Turnout::setState(TurnoutState ts){
    if(m_state == ts){
        return;
    }

    m_state = ts;
    sendCommandOverBus();
    Q_EMIT stateChanged();
}

void Turnout::throwTurnout(){
    if(m_state == TurnoutState::Thrown){
        return;
    }

    m_state = TurnoutState::Thrown;
    sendCommandOverBus();
    Q_EMIT stateChanged();
}

void Turnout::closeTurnout(){
    if(m_state == TurnoutState::Closed){
        return;
    }

    m_state = TurnoutState::Closed;
    sendCommandOverBus();
    Q_EMIT stateChanged();
}

void Turnout::toggleTurnout(){
    if(m_state == TurnoutState::Closed){
        m_state = TurnoutState::Thrown;
    }else if(m_state == TurnoutState::Thrown ||
               m_state == TurnoutState::Unknown){
        m_state = TurnoutState::Closed;
    }

    sendCommandOverBus();
    Q_EMIT stateChanged();
}
