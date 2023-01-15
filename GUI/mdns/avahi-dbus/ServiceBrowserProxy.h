#ifndef SERVICEBROWSERPROXY_H
#define SERVICEBROWSERPROXY_H

#include <dbus-cxx.h>
#include <memory>
#include <stdint.h>
#include <string>
#include "org_freedesktop_Avahi_ServiceBrowserProxy.h"
namespace Avahi {
class ServiceBrowserProxy
 : public DBus::ObjectProxy {
public:
ServiceBrowserProxy(std::shared_ptr<DBus::Connection> conn, std::string dest, std::string path, DBus::ThreadForCalling signalCallingThread = DBus::ThreadForCalling::DispatcherThread );
public:
    static std::shared_ptr<ServiceBrowserProxy> create(std::shared_ptr<DBus::Connection> conn, std::string dest, std::string path, DBus::ThreadForCalling signalCallingThread = DBus::ThreadForCalling::DispatcherThread );
    std::shared_ptr<Avahi::org_freedesktop_Avahi_ServiceBrowserProxy> getorg_freedesktop_Avahi_ServiceBrowserInterface( );
protected:
    std::shared_ptr<Avahi::org_freedesktop_Avahi_ServiceBrowserProxy> m_org_freedesktop_Avahi_ServiceBrowserProxy;
};
} /* namespace Avahi */
#endif /* SERVICEBROWSERPROXY_H */
