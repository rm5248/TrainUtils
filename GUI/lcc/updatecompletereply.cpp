/* SPDX-License-Identifier: GPL-2.0 */
#include "updatecompletereply.h"

UpdateCompleteReply::UpdateCompleteReply(uint16_t alias, QObject *parent)
    : QObject{parent},
    m_alias(alias)
{
}

bool UpdateCompleteReply::isFinished(){
    return m_done;
}

bool UpdateCompleteReply::isError(){
    return m_error;
}

uint16_t UpdateCompleteReply::alias(){
    return m_alias;
}

uint16_t UpdateCompleteReply::errorCode(){
    return m_errorCode;
}

QString UpdateCompleteReply::errorMessage(){
    return m_errorMessage;
}

void UpdateCompleteReply::setInvalidResponse(uint16_t error_code, const QString& message){
    m_done = true;
    m_error = true;
    m_errorCode = error_code;
    m_errorMessage = message;
    Q_EMIT finished();
}

void UpdateCompleteReply::setValidResponse(){
    m_done = true;
    Q_EMIT finished();
}
