#include "ServerProxy.h"

using Avahi::ServerProxy;

ServerProxy::ServerProxy(std::shared_ptr<DBus::Connection> conn, std::string dest, std::string path, DBus::ThreadForCalling signalCallingThread ) : DBus::ObjectProxy( conn, dest, path ){
m_org_freedesktop_Avahi_ServerProxy = Avahi::org_freedesktop_Avahi_ServerProxy::create( "org.freedesktop.Avahi.Server" );
this->add_interface( m_org_freedesktop_Avahi_ServerProxy );
m_org_freedesktop_Avahi_Server2Proxy = Avahi::org_freedesktop_Avahi_Server2Proxy::create( "org.freedesktop.Avahi.Server2" );
this->add_interface( m_org_freedesktop_Avahi_Server2Proxy );

}
std::shared_ptr<ServerProxy> ServerProxy::create(std::shared_ptr<DBus::Connection> conn, std::string dest, std::string path, DBus::ThreadForCalling signalCallingThread ){
std::shared_ptr<ServerProxy> created = std::shared_ptr<ServerProxy>( new ServerProxy( conn, dest, path, signalCallingThread ) );
conn->register_object_proxy( created );
return created;

}
std::shared_ptr<Avahi::org_freedesktop_Avahi_ServerProxy> ServerProxy::getorg_freedesktop_Avahi_ServerInterface( ){
return m_org_freedesktop_Avahi_ServerProxy;

}
std::shared_ptr<Avahi::org_freedesktop_Avahi_Server2Proxy> ServerProxy::getorg_freedesktop_Avahi_Server2Interface( ){
return m_org_freedesktop_Avahi_Server2Proxy;

}
