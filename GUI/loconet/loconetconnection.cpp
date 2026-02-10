/* SPDX-License-Identifier: GPL-2.0 */
#include "loconetconnection.h"
#include "loconetthrottle.h"

#include <log4cxx/logger.h>
#include <fmt/format.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.loconet.LoconetConnection" );

LoconetConnection::LoconetConnection(QObject *parent) : SystemConnection(parent)
{
    m_locoContext = loconet_context_new_interlocked(LoconetConnection::writeCB);
    loconet_context_set_user_data(m_locoContext, this);
    loconet_context_set_message_callback(m_locoContext, LoconetConnection::incomingLoconetCB);

    m_switchManager = loconet_turnout_manager_new(m_locoContext);
    loconet_turnout_manager_set_userdata(m_switchManager, this);
    loconet_turnout_manager_set_turnout_state_changed_callback(m_switchManager, LoconetConnection::incomingLoconetSwitchChangedCB);

    m_sendTimer.start(50);

    connect( &m_sendTimer, &QTimer::timeout,
             this, &LoconetConnection::sendNextMessage );
}

LoconetConnection::~LoconetConnection(){
    loconet_context_free(m_locoContext);
    loconet_turnout_manager_free(m_switchManager);
}

void LoconetConnection::writeCB( struct loconet_context* ctx, uint8_t* data, int len ){
    LoconetConnection* conn = static_cast<LoconetConnection*>(loconet_context_user_data(ctx));

    conn->writeData(data, len);
}

void LoconetConnection::sendMessage(loconet_message msg){
    // Buffer up the messages, send them periodically
    LOG4CXX_DEBUG_FMT(logger, "Queue up loconet message");
    m_sendQueue.push_back(msg);
}

void LoconetConnection::sendNextMessage(){
    if( m_sendQueue.empty() ) return;

    LOG4CXX_DEBUG_FMT(logger, "Send next loconet message");
    loconet_message msg = m_sendQueue.front();
    loconet_context_write_message(m_locoContext, &msg);
    m_sendQueue.pop_front();
}

void LoconetConnection::incomingLoconetCB(struct loconet_context* ctx, struct loconet_message* msg){
    LoconetConnection* conn = static_cast<LoconetConnection*>(loconet_context_user_data(ctx));
    conn->incomingLoconet(msg);

}
void LoconetConnection::incomingLoconet(struct loconet_message *msg){
    struct loconet_message new_message = *msg;
    QByteArray ba;
    ba.push_back(new_message.opcode);
    int msgLen = loconet_message_length(&new_message);
    if(msgLen >= 2){
        for(int x = 0; x < msgLen; x++){
            ba.push_back(new_message.data[x]);
        }
    }else{
        LOG4CXX_ERROR_FMT(logger, "bad message from loconet: invalid length {} opcode: 0x{:X}", msgLen,
                          new_message.opcode );
        return;
    }

    Q_EMIT incomingLoconetMessage(new_message);
    Q_EMIT incomingRawPacket(ba);
    loconet_turnout_manager_incoming_message(m_switchManager, msg);

    for(std::shared_ptr<LoconetThrottle>& throttle : m_throttles){
        loconet_throttle_incoming_message(throttle->m_throttle, msg);
    }
}

void LoconetConnection::throwTurnout(int switch_num){
    loconet_turnout_manager_throw(m_switchManager, switch_num, 0);
}

void LoconetConnection::closeTurnout(int switch_num){
    loconet_turnout_manager_close(m_switchManager, switch_num, 0);
}

struct loconet_context* LoconetConnection::loconetContext(){
    return m_locoContext;
}

std::shared_ptr<LoconetThrottle> LoconetConnection::newThrottle(){
    std::shared_ptr<LoconetThrottle> newThrottle = std::make_shared<LoconetThrottle>(m_locoContext);

    m_throttles.push_back(newThrottle);

    return newThrottle;
}

void LoconetConnection::incomingLoconetSwitchChangedCB(struct loconet_turnout_manager* manager, int switch_num, enum loconet_turnout_status state){
    LoconetConnection* conn = static_cast<LoconetConnection*>(loconet_turnout_manager_userdata(manager));
    conn->incomingLoconetSwitchChanged(switch_num, state);
}

void LoconetConnection::incomingLoconetSwitchChanged(int switch_num, enum loconet_turnout_status state){
    LOG4CXX_DEBUG_FMT(logger, "switch {} is {}", switch_num, state == LOCONET_TURNOUT_THROWN ? "thrown" : "closed");

    if(m_turnouts.contains(switch_num)){
        std::shared_ptr<LoconetTurnout> turnout = m_turnouts[switch_num];
        turnout->incomingTurnoutCommand(state);
    }
}

std::shared_ptr<Turnout> LoconetConnection::getDCCTurnout(int switch_num){
    if(m_turnouts.contains(switch_num)){
        return m_turnouts[switch_num];
    }else{
        std::shared_ptr<LoconetTurnout> lnTurnout = std::make_shared<LoconetTurnout>();
        m_turnouts[switch_num] = lnTurnout;
        return lnTurnout;
    }
}
