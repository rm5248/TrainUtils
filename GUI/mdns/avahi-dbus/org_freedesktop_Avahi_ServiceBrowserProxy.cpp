#include "org_freedesktop_Avahi_ServiceBrowserProxy.h"

using Avahi::org_freedesktop_Avahi_ServiceBrowserProxy;

org_freedesktop_Avahi_ServiceBrowserProxy::org_freedesktop_Avahi_ServiceBrowserProxy(std::string name ) : DBus::InterfaceProxy( name ){
m_method_Free = this->create_method<void()>("Free");
m_method_Start = this->create_method<void()>("Start");
m_signalproxy_ItemNew = this->create_signal<void(int32_t,int32_t,std::string,std::string,std::string,uint32_t)>( "ItemNew" );
m_signalproxy_ItemRemove = this->create_signal<void(int32_t,int32_t,std::string,std::string,std::string,uint32_t)>( "ItemRemove" );
m_signalproxy_Failure = this->create_signal<void(std::string)>( "Failure" );
m_signalproxy_AllForNow = this->create_signal<void()>( "AllForNow" );
m_signalproxy_CacheExhausted = this->create_signal<void()>( "CacheExhausted" );

}
std::shared_ptr<Avahi::org_freedesktop_Avahi_ServiceBrowserProxy> org_freedesktop_Avahi_ServiceBrowserProxy::create(std::string name ){
return std::shared_ptr<Avahi::org_freedesktop_Avahi_ServiceBrowserProxy>( new Avahi::org_freedesktop_Avahi_ServiceBrowserProxy( name ));

}
void org_freedesktop_Avahi_ServiceBrowserProxy::Free( ){
(*m_method_Free)();

}
void org_freedesktop_Avahi_ServiceBrowserProxy::Start( ){
(*m_method_Start)();

}
std::shared_ptr<DBus::SignalProxy<void(int32_t,int32_t,std::string,std::string,std::string,uint32_t)>> org_freedesktop_Avahi_ServiceBrowserProxy::signal_ItemNew( ){
return m_signalproxy_ItemNew;

}
std::shared_ptr<DBus::SignalProxy<void(int32_t,int32_t,std::string,std::string,std::string,uint32_t)>> org_freedesktop_Avahi_ServiceBrowserProxy::signal_ItemRemove( ){
return m_signalproxy_ItemRemove;

}
std::shared_ptr<DBus::SignalProxy<void(std::string)>> org_freedesktop_Avahi_ServiceBrowserProxy::signal_Failure( ){
return m_signalproxy_Failure;

}
std::shared_ptr<DBus::SignalProxy<void()>> org_freedesktop_Avahi_ServiceBrowserProxy::signal_AllForNow( ){
return m_signalproxy_AllForNow;

}
std::shared_ptr<DBus::SignalProxy<void()>> org_freedesktop_Avahi_ServiceBrowserProxy::signal_CacheExhausted( ){
return m_signalproxy_CacheExhausted;

}
