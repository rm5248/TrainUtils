/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCCCONNECTION_H
#define LCCCONNECTION_H

#include <QObject>
#include <QMap>
#include <QPointer>
#include <memory>

#include "lcc.h"
#include "lcc-network-info.h"
#include "../systemconnection.h"
#include "addressspacereply.h"
#include "addressspacereadreply.h"
#include "addressspacewritereply.h"

class LCCNode;

/**
 * Represents a connection to the LCC bus.
 *
 * Requests/responses are based off the QNetworkAccessManager API.  Create
 * a request, get an object handle for the response.  A signal will be emitted
 * when the response is ready.
 */
class LCCConnection : public SystemConnection
{
    Q_OBJECT
public:
    explicit LCCConnection(QObject *parent = nullptr);

    ~LCCConnection();

    std::shared_ptr<Turnout> getDCCTurnout(int switch_num);

    void setSimpleNodeInformation(QString manufacturer,
                                  QString model,
                                  QString hwVersion,
                                  QString swVersion);

    void setSimpleNodeNameDescription(QString nodeName,
                                      QString nodeDescription);

    void sendEvent(uint64_t event_id);

    void refreshNetwork();

    struct lcc_node_info* lccNodeInfoForID(uint64_t node_id);

    /**
     * Read a single memory block(up to 64 bytes in length).  This adds the request to read to an internal
     * queue, so this may be called multiple times.
     *
     * @param alias
     * @param space
     * @param starting_address
     * @param len
     */
    AddressSpaceReadReply* readSingleMemoryBlock(int alias, int space, uint32_t starting_address, int len);

    /**
     * Query a node for information about the address space.
     *
     * @param alias
     * @param space
     */
    AddressSpaceReply* queryAddressSpaceInformation(int alias, int space);

    /**
     * Write a single memory block(up to 64 bytes in length).  Like reads, this adds
     * the request to the internal queue.  Requests are sent in the order that they are made.
     *
     * @param alias
     * @param space
     * @param starting_address
     * @param data
     * @return
     */
    AddressSpaceWriteReply* writeSingleMemoryBlock(int alias, int space, uint32_t starting_address, const QVector<uint8_t>& data);

    std::shared_ptr<LCCNode> lccNodeForID(uint64_t node_id);

    /**
     * Get a human-readable description of an LCC error code(e.g. from a datagram rejection)
     */
    static QString errorCodeDescription(uint16_t error_code);

    void setCDI(QString cdi);

Q_SIGNALS:
    void incomingRawFrame(lcc_can_frame* frame);
    /**
     * Emitted for every event that is sent on the network by another node.
     *
     * @param source_alias The alias of the node that sent the event
     * @param event_id
     */
    void incomingEvent(uint16_t source_alias, uint64_t event_id);
    void newNodeDiscovered(uint64_t node_id);
    void nodeInformationUpdated(uint64_t node_id);
    void incomingDatagram(uint16_t source_alias, QByteArray datagramData);
    void datagramReceivedOK(uint16_t source_alias, uint8_t flags);
    void datagramRejected(uint16_t source_alias, uint16_t error_code, QByteArray optional_data);
    void addressSpaceRequestFinished(AddressSpaceReply* rep);
    void addressSpaceReadRequestFinished(AddressSpaceReadReply* rep);

protected:
    QString connectionType();

private:
    static void remote_memory_informationCB(struct lcc_remote_memory_context* ctx, uint16_t alias, int exists, int readonly, uint8_t address_space, uint32_t lowest_address, uint32_t highest_address, const char* message);
    void remoteMemoryInformation(uint16_t alias, int exists, int readonly, uint8_t address_space, uint32_t lowest_address, uint32_t highest_address, const char* message);
    static void remote_memory_receivedCB(struct lcc_remote_memory_context* ctx, uint16_t alias, uint8_t address_space, uint32_t starting_address, void* memory_data, int len);
    void remoteMemoryReceived(uint16_t alias, uint8_t address_space, uint32_t starting_address, void* memory_data, int len);
    static void remote_memory_request_failCB(struct lcc_remote_memory_context* ctx, uint16_t alias, uint16_t error_code, void* optional_data, int optional_len);
    void remoteMemoryRequestFailed(uint16_t alias, uint16_t error_code);
    static void remote_memory_read_rejectCB(struct lcc_remote_memory_context* ctx, uint16_t alias, uint8_t address_space, uint32_t starting_address, uint16_t error_code, const char* message);
    void remoteMemoryReadRejected(uint16_t alias, uint8_t address_space, uint32_t starting_address, uint16_t error_code, const char* message);
    static void remote_memory_write_okCB(struct lcc_remote_memory_context* ctx, uint16_t alias, uint8_t address_space, uint32_t starting_address);
    static void remote_memory_write_rejectedCB(struct lcc_remote_memory_context* ctx, uint16_t alias, uint8_t address_space, uint32_t starting_address, uint16_t error_code, const char* message);
    void remoteMemoryWriteFinished(uint16_t alias, uint8_t address_space, bool ok, uint16_t error_code, const QString& message);
    void memoryRequestDone();

private Q_SLOTS:
    void requestNextMemoryBlock();

protected:
    struct lcc_context* m_lcc;
    struct lcc_network_info* m_lccNetwork;
    QMap<uint64_t,std::shared_ptr<LCCNode>> m_nodes;
    QByteArray m_cdi;
    QVector<AddressSpaceReply*> m_inflight_replies;
    QVector<AddressSpaceReadReply*> m_inflight_read_replies;
    QVector<AddressSpaceWriteReply*> m_inflight_write_replies;
    // All of the requests, in the order that they were made
    QVector<QPointer<QObject>> m_request_order;
    bool m_requesting_memory = false;
};

#endif // LCCCONNECTION_H
