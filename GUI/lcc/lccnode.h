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
class SegmentMemory;

/**
 * Represents a single LCC node.
 */
class LCCNode : public QObject
{
    Q_OBJECT
public:
    explicit LCCNode(lcc_node_info* inf, LCCConnection* conn, QObject *parent = nullptr);

    bool valid() const;

    /**
     * Read the entire CDI for this node.
     * Listen for the cdiRead signal to know when the CDI is available.
     */
    void readCDI();

    /**
     * The parsed CDI data.
     *
     * @return
     */
    CDI cdi() const;

    /**
     * Read all of the memory for the device.  If the CDI has not been retreived yet,
     * it will retrieve the CDI first before reading memory.
     */
    void readAllMemory();

    /**
     * Check to see if we have all memory read from this node.
     *
     * @return
     */
    bool hasAllMemory();

    /**
     * Get the raw memory for the specified segment.
     *
     * @param segment
     * @return
     */
    SegmentMemory* segmentMemory(uint8_t segment);

Q_SIGNALS:
    /**
     * Emitted when the CDI has been read.
     */
    void cdiRead();
    void cdiReadFailure(uint16_t error_code, QString error_string);

private Q_SLOTS:
    void cdiReady();

private:
    LCCConnection* m_conn;
    lcc_node_info* m_nodeInfo;
    CDI m_cdi;

    // Memory information
    QVector<SegmentMemory*> m_memories;
};

#endif // LCCNODE_H
