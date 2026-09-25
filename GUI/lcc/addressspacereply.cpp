/* SPDX-License-Identifier: GPL-2.0 */
#include "addressspacereply.h"

AddressSpaceReply::AddressSpaceReply(uint16_t alias, uint8_t space, QObject *parent)
    : QObject{parent},
    m_alias(alias),
    m_space(space)
{}

uint16_t AddressSpaceReply::alias(){
    return m_alias;
}

uint8_t AddressSpaceReply::space(){
    return m_space;
}

bool AddressSpaceReply::exists(){
    return m_exists;
}

bool AddressSpaceReply::isFinished(){
    return m_done;
}

bool AddressSpaceReply::isError(){
    return m_error;
}

uint32_t AddressSpaceReply::lowAddress(){
    return m_lowest_address;
}

uint32_t AddressSpaceReply::highAddress(){
    return m_highest_address;
}

void AddressSpaceReply::setInvalidResponse(uint32_t starting_address, uint16_t error_code, const char* message){
    m_done = true;
    m_error = true;
    m_message = QString::fromUtf8(message);

    Q_EMIT errorOccured(error_code, m_message);
    Q_EMIT finished();
}

void AddressSpaceReply::setValidResponse(bool exists, bool readonly, uint32_t lowest_address, uint32_t highest_address, const char* message){
    m_done = true;
    m_message = QString::fromUtf8(message);
    m_exists = exists;
    m_readonly = readonly;
    m_lowest_address = lowest_address;
    m_highest_address = highest_address;

    Q_EMIT finished();
}
