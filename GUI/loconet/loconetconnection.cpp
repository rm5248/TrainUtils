/* SPDX-License-Identifier: GPL-2.0 */
#include "loconetconnection.h"

LoconetConnection::LoconetConnection(QObject *parent) : QObject(parent)
{
    m_locoContext = ln_context_new_interlocked(LoconetConnection::writeCB);
    ln_set_user_data(m_locoContext, this);

    m_sendTimer.start(50);

    connect( &m_sendTimer, &QTimer::timeout,
             this, &LoconetConnection::sendNextMessage );
}

LoconetConnection::~LoconetConnection(){
    ln_context_free(m_locoContext);
}

void LoconetConnection::setName(QString name){
    m_name = name;
}

QString LoconetConnection::name() const{
    return m_name;
}

void LoconetConnection::writeCB( struct loconet_context* ctx, uint8_t* data, int len ){
    LoconetConnection* conn = static_cast<LoconetConnection*>(ln_user_data(ctx));

    conn->writeData(data, len);
}

void LoconetConnection::sendMessage(loconet_message msg){
    // Buffer up the messages, send them periodically
    m_sendQueue.push_back(msg);
}

void LoconetConnection::sendNextMessage(){
    if( m_sendQueue.empty() ) return;

    loconet_message msg = m_sendQueue.front();
    ln_write_message(m_locoContext, &msg);
    m_sendQueue.pop_front();
}
