#ifndef ORG_FREEDESKTOP_AVAHI_SERVICERESOLVERPROXY_H
#define ORG_FREEDESKTOP_AVAHI_SERVICERESOLVERPROXY_H

#include <dbus-cxx.h>
#include <memory>
#include <stdint.h>
#include <string>
namespace Avahi {
class org_freedesktop_Avahi_ServiceResolverProxy
 : public DBus::InterfaceProxy {
protected:
org_freedesktop_Avahi_ServiceResolverProxy(std::string name );
public:
    static std::shared_ptr<Avahi::org_freedesktop_Avahi_ServiceResolverProxy> create(std::string name = "org.freedesktop.Avahi.ServiceResolver" );
    void Free( );
    void Start( );
    std::shared_ptr<DBus::SignalProxy<void(int32_t,int32_t,std::string,std::string,std::string,std::string,int32_t,std::string,uint16_t,std::vector<std::vector<uint8_t>>,uint32_t)>> signal_Found( );
    std::shared_ptr<DBus::SignalProxy<void(std::string)>> signal_Failure( );
protected:
    std::shared_ptr<DBus::MethodProxy<void()>>  m_method_Free;
    std::shared_ptr<DBus::MethodProxy<void()>>  m_method_Start;
    std::shared_ptr<DBus::SignalProxy<void(int32_t,int32_t,std::string,std::string,std::string,std::string,int32_t,std::string,uint16_t,std::vector<std::vector<uint8_t>>,uint32_t)>> m_signalproxy_Found;
    std::shared_ptr<DBus::SignalProxy<void(std::string)>> m_signalproxy_Failure;
};
} /* namespace Avahi */
#endif /* ORG_FREEDESKTOP_AVAHI_SERVICERESOLVERPROXY_H */
