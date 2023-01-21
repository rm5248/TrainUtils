/* SPDX-License-Identifier: GPL-2.0 */
#ifndef MDNSAVAHI_H
#define MDNSAVAHI_H

#include <QObject>

#include "mdnsmanager.h"

#include <dbus-cxx-qt.h>
#include "avahi-dbus/ServerProxy.h"
#include "avahi-dbus/ServiceBrowserProxy.h"
#include "avahi-dbus/ServiceResolverProxy.h"

class MDNSAvahi : public MDNSManager
{
    Q_OBJECT
public:
    explicit MDNSAvahi(QObject *parent = nullptr);

private Q_SLOTS:
    void initialize();

private:
    void signalFailure( std::string str );
    void signalAllForNow();
    void signalCacheExhausted();
    void signalLCCNew(int32_t,int32_t,std::string,std::string,std::string,uint32_t);
    void signalLCCRemoved(int32_t,int32_t,std::string,std::string,std::string,uint32_t);
    void signalLoconetNew(int32_t,int32_t,std::string,std::string,std::string,uint32_t);
    void signalLoconetRemoved(int32_t,int32_t,std::string,std::string,std::string,uint32_t);

    void resolvedLCCFound(int32_t interface,
               int32_t protocol,
               std::string name,
               std::string type,
               std::string domain,
               std::string host,
               int32_t aprotocol,
               std::string address,
               uint16_t port,
               std::vector<std::vector<uint8_t>> txt,
               uint32_t flags);
    void resolvedLCCError( std::string err );

    void resolvedLoconetFound(int32_t interface,
               int32_t protocol,
               std::string name,
               std::string type,
               std::string domain,
               std::string host,
               int32_t aprotocol,
               std::string address,
               uint16_t port,
               std::vector<std::vector<uint8_t>> txt,
               uint32_t flags);
    void resolvedLoconetError( std::string err );

private:
    std::shared_ptr<DBus::Qt::QtDispatcher> m_dispatcher;
    std::shared_ptr<DBus::Connection> m_connection;
    std::shared_ptr<Avahi::ServerProxy> m_avahiServer;
    std::shared_ptr<Avahi::ServiceBrowserProxy> m_browserProxyLCC;
    std::shared_ptr<Avahi::ServiceBrowserProxy> m_browserProxyLoconet;
    QMap<QString,std::shared_ptr<Avahi::ServiceResolverProxy>> m_nameToResolver;
};

#endif // MDNSAVAHI_H
