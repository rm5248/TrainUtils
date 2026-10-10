/* SPDX-License-Identifier: GPL-2.0 */
#ifndef ADDRESSSPACEWRITEREPLY_H
#define ADDRESSSPACEWRITEREPLY_H

#include <QObject>
#include <QVector>

/**
 * The result of writing a single block(up to 64 bytes) of memory to a node.
 */
class AddressSpaceWriteReply : public QObject
{
    Q_OBJECT
public:
    explicit AddressSpaceWriteReply(uint16_t alias, uint8_t space, uint32_t starting_address, const QVector<uint8_t>& data, QObject *parent = nullptr);

    bool isFinished();
    bool isError();
    uint16_t alias();
    uint8_t space();
    uint32_t startingAddress();
    /**
     * The data that is being written
     */
    QVector<uint8_t> data();
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
    uint8_t m_space;
    uint32_t m_startingAddress;
    QVector<uint8_t> m_data;
    QString m_errorMessage;
    uint16_t m_errorCode = 0;

    friend class LCCConnection;
};

#endif // ADDRESSSPACEWRITEREPLY_H
