/* SPDX-License-Identifier: GPL-2.0 */
#ifndef UPDATECOMPLETEREPLY_H
#define UPDATECOMPLETEREPLY_H

#include <QObject>
#include <QString>

/**
 * The result of telling a node that we are done changing its configuration
 * (the 'Update Complete' command).
 */
class UpdateCompleteReply : public QObject
{
    Q_OBJECT
public:
    explicit UpdateCompleteReply(uint16_t alias, QObject *parent = nullptr);

    bool isFinished();
    bool isError();
    uint16_t alias();
    uint16_t errorCode();
    QString errorMessage();

Q_SIGNALS:
    void finished();

private:
    void setInvalidResponse(uint16_t error_code, const QString& message);
    void setValidResponse();

private:
    bool m_done = false;
    bool m_error = false;
    uint16_t m_alias;
    uint16_t m_errorCode = 0;
    QString m_errorMessage;

    friend class LCCConnection;
};

#endif // UPDATECOMPLETEREPLY_H
