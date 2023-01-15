#include "ServiceBrowserProxy.h"

using Avahi::ServiceBrowserProxy;

ServiceBrowserProxy::ServiceBrowserProxy(std::shared_ptr<DBus::Connection> conn, std::string dest, std::string path, DBus::ThreadForCalling signalCallingThread ) : DBus::ObjectProxy( conn, dest, path ){
m_org_freedesktop_Avahi_ServiceBrowserProxy = Avahi::org_freedesktop_Avahi_ServiceBrowserProxy::create( "org.freedesktop.Avahi.ServiceBrowser" );
this->add_interface( m_org_freedesktop_Avahi_ServiceBrowserProxy );

}
std::shared_ptr<ServiceBrowserProxy> ServiceBrowserProxy::create(std::shared_ptr<DBus::Connection> conn, std::string dest, std::string path, DBus::ThreadForCalling signalCallingThread ){
std::shared_ptr<ServiceBrowserProxy> created = std::shared_ptr<ServiceBrowserProxy>( new ServiceBrowserProxy( conn, dest, path, signalCallingThread ) );
conn->register_object_proxy( created );
return created;

}
std::shared_ptr<Avahi::org_freedesktop_Avahi_ServiceBrowserProxy> ServiceBrowserProxy::getorg_freedesktop_Avahi_ServiceBrowserInterface( ){
return m_org_freedesktop_Avahi_ServiceBrowserProxy;

}
