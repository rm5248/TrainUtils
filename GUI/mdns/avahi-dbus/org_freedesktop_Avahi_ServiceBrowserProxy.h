#ifndef ORG_FREEDESKTOP_AVAHI_SERVICEBROWSERPROXY_H
#define ORG_FREEDESKTOP_AVAHI_SERVICEBROWSERPROXY_H

#include <dbus-cxx.h>
#include <memory>
#include <stdint.h>
#include <string>
namespace Avahi {
class org_freedesktop_Avahi_ServiceBrowserProxy
 : public DBus::InterfaceProxy {
protected:
org_freedesktop_Avahi_ServiceBrowserProxy(std::string name );
public:
    static std::shared_ptr<Avahi::org_freedesktop_Avahi_ServiceBrowserProxy> create(std::string name = "org.freedesktop.Avahi.ServiceBrowser" );
    void Free( );
    void Start( );
    std::shared_ptr<DBus::SignalProxy<void(int32_t,int32_t,std::string,std::string,std::string,uint32_t)>> signal_ItemNew( );
    std::shared_ptr<DBus::SignalProxy<void(int32_t,int32_t,std::string,std::string,std::string,uint32_t)>> signal_ItemRemove( );
    std::shared_ptr<DBus::SignalProxy<void(std::string)>> signal_Failure( );
    std::shared_ptr<DBus::SignalProxy<void()>> signal_AllForNow( );
    std::shared_ptr<DBus::SignalProxy<void()>> signal_CacheExhausted( );
protected:
    std::shared_ptr<DBus::MethodProxy<void()>>  m_method_Free;
    std::shared_ptr<DBus::MethodProxy<void()>>  m_method_Start;
    std::shared_ptr<DBus::SignalProxy<void(int32_t,int32_t,std::string,std::string,std::string,uint32_t)>> m_signalproxy_ItemNew;
    std::shared_ptr<DBus::SignalProxy<void(int32_t,int32_t,std::string,std::string,std::string,uint32_t)>> m_signalproxy_ItemRemove;
    std::shared_ptr<DBus::SignalProxy<void(std::string)>> m_signalproxy_Failure;
    std::shared_ptr<DBus::SignalProxy<void()>> m_signalproxy_AllForNow;
    std::shared_ptr<DBus::SignalProxy<void()>> m_signalproxy_CacheExhausted;
};
} /* namespace Avahi */
#endif /* ORG_FREEDESKTOP_AVAHI_SERVICEBROWSERPROXY_H */
