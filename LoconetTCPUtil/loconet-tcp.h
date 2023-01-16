/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LOCONET_TCP_H
#define LOCONET_TCP_H

#include <stdint.h>
#include <stddef.h>
#include <vector>
#include <memory>
#include <experimental/propagate_const>
#include <functional>
#include <string>

struct loconet_context;

namespace loconet {

/**
 * Wraps a loconet_context with support for TCP.
 *
 * This does not control a TCP socket directly; that still must be done by client code.
 * Client code should call ln_read_message from the loconet context.
 */
class LoconetTCP {
public:
    LoconetTCP();
    ~LoconetTCP();

    /**
     * Call this method when data comes in from the LoconetTCP server.
     *
     * @param data The data that came from the server
     * @param len How long the data is
     */
    void incomingData(void* data, size_t len);
    void incomingData(const std::vector<uint8_t>& data);

//    void setWriteDataFunction(std::function<void(uint8_t*,int)> writeDataFun);

    struct loconet_context* loconetContext() const;

    /**
     * Set version callback that will be called when the server sends the 'version' information.
     * @param versionCb
     */
    void setVersionCallback(std::function<void(std::string)> versionCb);

    /**
     * Set receive callback that will be called when the server receives data.
     * Note that this same data will be sent to the loconet_context, so that client
     * code only needs to be worried about this if they want to do custom handling
     * for some reason.
     * @param versionCb
     */
    void setReceiveCallback(std::function<void(const std::vector<uint8_t>&)> receiveCB);

    /**
     * Set sent callback that will be sent from the server after sending data to the server.
     * The function callback has two parameters: bool for success/error, and a comment.
     * @param versionCb
     */
    void setSentCallback(std::function<void(bool, std::string)> sentCB);

    void setTimestampCallback(std::function<void(uint32_t)> timestampCB);

    void setBreakCallback(std::function<void(uint32_t)> breakCB);

    void setErrorCallback(std::function<void(std::string)> errorCB);

    void setErrorLineCallback(std::function<void(std::string)> errorLineCB);

private:
    void handleReceivedData(const std::string& data);
    static void writeInterlockFunction( struct loconet_context* ctx, uint8_t* data, int len );

private:
    struct LoconetTCPPriv;
    std::experimental::propagate_const<std::unique_ptr<LoconetTCPPriv>> m_priv;
};

}

#endif /* LOCONET_TCP_H */
