/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCCNODE_H
#define LCCNODE_H

#include <QObject>
#include <QByteArray>

#include "cdi/cdi.h"

struct lcc_node_info;
struct lcc_context;

class LCCConnection;
class AddressSpaceReply;
class AddressSpaceReadReply;

/**
 * Represents a single LCC node.
 */
class LCCNode : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasCDI READ hasCDI NOTIFY cdiRead)
public:
    explicit LCCNode(lcc_node_info* inf, LCCConnection* conn, QObject *parent = nullptr);

    bool valid() const;

    bool hasCDI() const;

    /**
     * Read the entire CDI for this node.
     * Listen for the cdiRead signal to know when the CDI is available.
     */
    void readCDI();

    /**
     * The raw CDI XML data of the node.
     * @return
     */
    QString rawCDI() const;

    /**
     * The parsed CDI data.
     *
     * @return
     */
    CDI cdi() const;

Q_SIGNALS:
    /**
     * Emitted when the CDI has been read.
     */
    void cdiRead();
    void cdiReadFailure(uint16_t error_code, QString error_string);

private Q_SLOTS:
    void addressSpaceFinished();
    void addressSpaceRead();

private:
    enum class CDIReadState{
        Not_Read_Yet,
        Read_Space_Info,
        Reading_CDI,
        CDI_Complete,
    };

    LCCConnection* m_conn;
    lcc_node_info* m_nodeInfo;
    bool m_hasCDI;
    QString m_rawcdi;
    int m_cdiCurrentOffset;
    int m_cdiSize;
    CDI m_cdi;
    CDIReadState m_cdiReadState = CDIReadState::Not_Read_Yet;
    AddressSpaceReply* m_reply = nullptr;
    AddressSpaceReadReply* m_readReply = nullptr;
};

#endif // LCCNODE_H
