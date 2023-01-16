/* SPDX-License-Identifier: GPL-2.0 */
#include "loconetconnection.h"

LoconetConnection::LoconetConnection(QObject *parent) : QObject(parent)
{
    m_locoContext = ln_context_new_interlocked(LoconetConnection::writeCB);
    ln_set_user_data(m_locoContext, this);
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
    QByteArray toSend = QByteArray::fromRawData((const char*)data, len);

    conn->writeData(toSend);
}
