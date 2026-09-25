/* SPDX-License-Identifier: GPL-2.0 */
#include "addressspacereadreply.h"

AddressSpaceReadReply::AddressSpaceReadReply(uint16_t alias, uint8_t space, uint32_t starting_address, QObject *parent)
    : QObject{parent},
    m_alias(alias),
    m_space(space),
    m_startingAddress(starting_address)
{
    m_data.reserve(64);
}

bool AddressSpaceReadReply::isFinished(){
    return m_done;
}

bool AddressSpaceReadReply::isError(){
    return m_error;
}

uint16_t AddressSpaceReadReply::alias(){
    return m_alias;
}

uint8_t AddressSpaceReadReply::space(){
    return m_space;
}

uint32_t AddressSpaceReadReply::startingAddress(){
    return m_startingAddress;
}

QVector<uint8_t> AddressSpaceReadReply::data(){
    return m_data;
}

void AddressSpaceReadReply::setInvalidResponse(uint16_t error_code, const char* message){
    m_error = true;
    m_done = true;
    m_errorMessage = QString::fromUtf8(message);
    Q_EMIT errorOccured(error_code, m_errorMessage);
    Q_EMIT finished();
}

void AddressSpaceReadReply::setValidResponse(uint32_t starting_address, void* data, int len){
    m_done = true;

    uint8_t* u8_data = static_cast<uint8_t*>(data);
    m_data.clear();
    for(int x = 0; x < len; x++){
        m_data.push_back(u8_data[x]);
    }

    Q_EMIT finished();
}
