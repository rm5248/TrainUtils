/* SPDX-License-Identifier: GPL-2.0 */
#ifndef ADDRESSSPACEREPLY_H
#define ADDRESSSPACEREPLY_H

#include <QObject>

class AddressSpaceReply : public QObject
{
    Q_OBJECT
public:
    explicit AddressSpaceReply(uint16_t alias, uint8_t space, QObject *parent = nullptr);

    uint16_t alias();
    uint8_t space();
    bool exists();
    bool readonly();
    bool isFinished();
    bool isError();
    uint32_t lowAddress();
    uint32_t highAddress();

Q_SIGNALS:
    void errorOccured(uint16_t error_code, QString message);
    void finished();

private:
    void setInvalidResponse(uint32_t starting_address, uint16_t error_code, const char* message);
    void setValidResponse(bool exists, bool readonly, uint32_t lowest_address, uint32_t highest_address, const char* message);

    //(struct lcc_remote_memory_context* ctx, int exists, int readonly, uint8_t address_space, uint32_t lowest_address, uint32_t highest_address, const char* message)

private:
    uint16_t m_alias;
    uint16_t m_space;
    bool m_done = false;
    bool m_exists = false;
    bool m_readonly = false;
    bool m_error = false;
    uint32_t m_lowest_address = 0;
    uint32_t m_highest_address = 0;
    QString m_message;

    friend class LCCConnection;
};

#endif // ADDRESSSPACEREPLY_H
