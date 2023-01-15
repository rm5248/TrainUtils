#ifndef SERVERPROXY_H
#define SERVERPROXY_H

#include <dbus-cxx.h>
#include <memory>
#include <stdint.h>
#include <string>
#include "org_freedesktop_Avahi_Server2Proxy.h"
#include "org_freedesktop_Avahi_ServerProxy.h"
namespace Avahi {
class ServerProxy
 : public DBus::ObjectProxy {
public:
ServerProxy(std::shared_ptr<DBus::Connection> conn, std::string dest, std::string path, DBus::ThreadForCalling signalCallingThread = DBus::ThreadForCalling::DispatcherThread );
public:
    static std::shared_ptr<ServerProxy> create(std::shared_ptr<DBus::Connection> conn, std::string dest, std::string path, DBus::ThreadForCalling signalCallingThread = DBus::ThreadForCalling::DispatcherThread );
    std::shared_ptr<Avahi::org_freedesktop_Avahi_ServerProxy> getorg_freedesktop_Avahi_ServerInterface( );
    std::shared_ptr<Avahi::org_freedesktop_Avahi_Server2Proxy> getorg_freedesktop_Avahi_Server2Interface( );
protected:
    std::shared_ptr<Avahi::org_freedesktop_Avahi_ServerProxy> m_org_freedesktop_Avahi_ServerProxy;
    std::shared_ptr<Avahi::org_freedesktop_Avahi_Server2Proxy> m_org_freedesktop_Avahi_Server2Proxy;
};
} /* namespace Avahi */
#endif /* SERVERPROXY_H */
