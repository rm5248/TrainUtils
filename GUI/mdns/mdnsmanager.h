/* SPDX-License-Identifier: GPL-2.0 */
#ifndef MDNSMANAGER_H
#define MDNSMANAGER_H

#include <QObject>
#include <QHostAddress>

#if defined(Q_OS_LINUX)
#include <dbus-cxx-qt.h>
#include "avahi-dbus/ServerProxy.h"
#include "avahi-dbus/ServiceBrowserProxy.h"
#include "avahi-dbus/ServiceResolverProxy.h"
#endif

class MDNSManager : public QObject
{
    Q_OBJECT
public:
    explicit MDNSManager(QObject *parent = nullptr);

Q_SIGNALS:
    void lccServerFound(QString serviceName, QHostAddress address, uint16_t port);
    void lccServerLeft(QString serviceName);
    void loconetServerFound(QString serviceName, QHostAddress address, uint16_t port);
    void loconetServerLeft(QString serviceName);

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
#if defined(Q_OS_LINUX)
    std::shared_ptr<DBus::Qt::QtDispatcher> m_dispatcher;
    std::shared_ptr<DBus::Connection> m_connection;
    std::shared_ptr<Avahi::ServerProxy> m_avahiServer;
    std::shared_ptr<Avahi::ServiceBrowserProxy> m_browserProxyLCC;
    std::shared_ptr<Avahi::ServiceBrowserProxy> m_browserProxyLoconet;
    QMap<QString,std::shared_ptr<Avahi::ServiceResolverProxy>> m_nameToResolver;
#elif defined(Q_OS_WIN)
#endif
};

#endif // MDNSMANAGER_H
