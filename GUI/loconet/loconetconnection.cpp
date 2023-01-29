/* SPDX-License-Identifier: GPL-2.0 */
#include "loconetconnection.h"

#include <log4cxx/logger.h>
#include <fmt/format.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.loconet.LoconetConnection" );

static void LoconetConnectionIncomingMessage(struct loconet_context* ctx, struct loconet_message* msg){
    struct loconet_message new_message = *msg;
    LoconetConnection* conn = static_cast<LoconetConnection*>(loconet_context_user_data(ctx));

    QByteArray ba;
    ba.push_back(new_message.opcode);
    for(int x = 0; x < loconet_message_length(&new_message) - 1; x++){
        ba.push_back(new_message.data[x]);
    }

    Q_EMIT conn->incomingLoconetMessage(new_message);
    Q_EMIT conn->incomingRawPacket(ba);
}

LoconetConnection::LoconetConnection(QObject *parent) : SystemConnection(parent)
{
    m_locoContext = loconet_context_new_interlocked(LoconetConnection::writeCB);
    loconet_context_set_user_data(m_locoContext, this);
    loconet_context_set_message_callback(m_locoContext, LoconetConnectionIncomingMessage);

    m_sendTimer.start(50);

    connect( &m_sendTimer, &QTimer::timeout,
             this, &LoconetConnection::sendNextMessage );
}

LoconetConnection::~LoconetConnection(){
    loconet_context_free(m_locoContext);
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
