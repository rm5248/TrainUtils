/* SPDX-License-Identifier: GPL-2.0 */
#include "addressspacewritereply.h"

AddressSpaceWriteReply::AddressSpaceWriteReply(uint16_t alias, uint8_t space, uint32_t starting_address, const QVector<uint8_t>& data, QObject *parent)
    : QObject{parent},
    m_alias(alias),
    m_space(space),
    m_startingAddress(starting_address),
    m_data(data)
{
}

bool AddressSpaceWriteReply::isFinished(){
    return m_done;
}

bool AddressSpaceWriteReply::isError(){
    return m_error;
}

uint16_t AddressSpaceWriteReply::alias(){
    return m_alias;
}

uint8_t AddressSpaceWriteReply::space(){
    return m_space;
}

uint32_t AddressSpaceWriteReply::startingAddress(){
    return m_startingAddress;
}

QVector<uint8_t> AddressSpaceWriteReply::data(){
    return m_data;
}

uint16_t AddressSpaceWriteReply::errorCode(){
    return m_errorCode;
}

QString AddressSpaceWriteReply::errorMessage(){
    return m_errorMessage;
}

void AddressSpaceWriteReply::setInvalidResponse(uint16_t error_code, const QString& message){
    m_done = true;
    m_error = true;
    m_errorCode = error_code;
    m_errorMessage = message;
    Q_EMIT finished();
}

void AddressSpaceWriteReply::setValidResponse(){
    m_done = true;
    Q_EMIT finished();
}
