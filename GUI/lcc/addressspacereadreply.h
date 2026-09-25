/* SPDX-License-Identifier: GPL-2.0 */
#ifndef ADDRESSSPACEREADREPLY_H
#define ADDRESSSPACEREADREPLY_H

#include <QObject>

class AddressSpaceReadReply : public QObject
{
    Q_OBJECT
public:
    explicit AddressSpaceReadReply(uint16_t alias, uint8_t space, uint32_t starting_address, QObject *parent = nullptr);

    bool isFinished();
    bool isError();
    uint16_t alias();
    uint8_t space();
    uint32_t startingAddress();
    QVector<uint8_t> data();

Q_SIGNALS:
    void errorOccured(uint16_t error_code, QString message);
    void finished();

private:
    void setInvalidResponse(uint16_t error_code, const char* message);
    void setValidResponse(uint32_t starting_address, void* data, int len);

private:
    bool m_done = false;
    bool m_error = false;
    uint16_t m_alias;
    uint8_t m_space;
    uint32_t m_startingAddress;
    QVector<uint8_t> m_data;
    QString m_errorMessage;

    friend class LCCConnection;
};

#endif // ADDRESSSPACEREADREPLY_H
