#include "org_freedesktop_Avahi_ServiceResolverProxy.h"

using Avahi::org_freedesktop_Avahi_ServiceResolverProxy;

org_freedesktop_Avahi_ServiceResolverProxy::org_freedesktop_Avahi_ServiceResolverProxy(std::string name ) : DBus::InterfaceProxy( name ){
m_method_Free = this->create_method<void()>("Free");
m_method_Start = this->create_method<void()>("Start");
m_signalproxy_Found = this->create_signal<void(int32_t,int32_t,std::string,std::string,std::string,std::string,int32_t,std::string,uint16_t,std::vector<std::vector<uint8_t>>,uint32_t)>( "Found" );
m_signalproxy_Failure = this->create_signal<void(std::string)>( "Failure" );

}
std::shared_ptr<Avahi::org_freedesktop_Avahi_ServiceResolverProxy> org_freedesktop_Avahi_ServiceResolverProxy::create(std::string name ){
return std::shared_ptr<Avahi::org_freedesktop_Avahi_ServiceResolverProxy>( new Avahi::org_freedesktop_Avahi_ServiceResolverProxy( name ));

}
void org_freedesktop_Avahi_ServiceResolverProxy::Free( ){
(*m_method_Free)();

}
void org_freedesktop_Avahi_ServiceResolverProxy::Start( ){
(*m_method_Start)();

}
std::shared_ptr<DBus::SignalProxy<void(int32_t,int32_t,std::string,std::string,std::string,std::string,int32_t,std::string,uint16_t,std::vector<std::vector<uint8_t>>,uint32_t)>> org_freedesktop_Avahi_ServiceResolverProxy::signal_Found( ){
return m_signalproxy_Found;

}
std::shared_ptr<DBus::SignalProxy<void(std::string)>> org_freedesktop_Avahi_ServiceResolverProxy::signal_Failure( ){
return m_signalproxy_Failure;

}
