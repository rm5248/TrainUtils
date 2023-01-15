#ifndef SERVICERESOLVERPROXY_H
#define SERVICERESOLVERPROXY_H

#include <dbus-cxx.h>
#include <memory>
#include <stdint.h>
#include <string>
#include "org_freedesktop_Avahi_ServiceResolverProxy.h"
namespace Avahi {
class ServiceResolverProxy
 : public DBus::ObjectProxy {
public:
ServiceResolverProxy(std::shared_ptr<DBus::Connection> conn, std::string dest, std::string path, DBus::ThreadForCalling signalCallingThread = DBus::ThreadForCalling::DispatcherThread );
public:
    static std::shared_ptr<ServiceResolverProxy> create(std::shared_ptr<DBus::Connection> conn, std::string dest, std::string path, DBus::ThreadForCalling signalCallingThread = DBus::ThreadForCalling::DispatcherThread );
    std::shared_ptr<Avahi::org_freedesktop_Avahi_ServiceResolverProxy> getorg_freedesktop_Avahi_ServiceResolverInterface( );
protected:
    std::shared_ptr<Avahi::org_freedesktop_Avahi_ServiceResolverProxy> m_org_freedesktop_Avahi_ServiceResolverProxy;
};
} /* namespace Avahi */
#endif /* SERVICERESOLVERPROXY_H */
