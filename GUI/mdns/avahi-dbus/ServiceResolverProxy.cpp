#include "ServiceResolverProxy.h"

using Avahi::ServiceResolverProxy;

ServiceResolverProxy::ServiceResolverProxy(std::shared_ptr<DBus::Connection> conn, std::string dest, std::string path, DBus::ThreadForCalling signalCallingThread ) : DBus::ObjectProxy( conn, dest, path ){
m_org_freedesktop_Avahi_ServiceResolverProxy = Avahi::org_freedesktop_Avahi_ServiceResolverProxy::create( "org.freedesktop.Avahi.ServiceResolver" );
this->add_interface( m_org_freedesktop_Avahi_ServiceResolverProxy );

}
std::shared_ptr<ServiceResolverProxy> ServiceResolverProxy::create(std::shared_ptr<DBus::Connection> conn, std::string dest, std::string path, DBus::ThreadForCalling signalCallingThread ){
std::shared_ptr<ServiceResolverProxy> created = std::shared_ptr<ServiceResolverProxy>( new ServiceResolverProxy( conn, dest, path, signalCallingThread ) );
conn->register_object_proxy( created );
return created;

}
std::shared_ptr<Avahi::org_freedesktop_Avahi_ServiceResolverProxy> ServiceResolverProxy::getorg_freedesktop_Avahi_ServiceResolverInterface( ){
return m_org_freedesktop_Avahi_ServiceResolverProxy;

}
