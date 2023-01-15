/* SPDX-License-Identifier: GPL-2.0 */
#include "lccconnection.h"

LCCConnection::LCCConnection(QObject *parent) : QObject(parent)
{
    m_lcc = lcc_context_new();

    // TODO make this configurable.  currently set to 'assigned by software at runtime'
    lcc_context_set_unique_identifer(m_lcc, 0x040000000001);
}

LCCConnection::~LCCConnection(){
    lcc_context_free(m_lcc);
}

void LCCConnection::setName(QString name){
    m_name = name;
}

QString LCCConnection::name() const{
    return m_name;
}
