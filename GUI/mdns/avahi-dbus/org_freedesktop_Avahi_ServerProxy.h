#ifndef ORG_FREEDESKTOP_AVAHI_SERVERPROXY_H
#define ORG_FREEDESKTOP_AVAHI_SERVERPROXY_H

#include <dbus-cxx.h>
#include <memory>
#include <stdint.h>
#include <string>
namespace Avahi {
class org_freedesktop_Avahi_ServerProxy
 : public DBus::InterfaceProxy {
protected:
org_freedesktop_Avahi_ServerProxy(std::string name );
public:
    static std::shared_ptr<Avahi::org_freedesktop_Avahi_ServerProxy> create(std::string name = "org.freedesktop.Avahi.Server" );
    std::string GetVersionString( );
    uint32_t GetAPIVersion( );
    std::string GetHostName( );
    void SetHostName(std::string name );
    std::string GetHostNameFqdn( );
    std::string GetDomainName( );
    bool IsNSSSupportAvailable( );
    int32_t GetState( );
    uint32_t GetLocalServiceCookie( );
    std::string GetAlternativeHostName(std::string name );
    std::string GetAlternativeServiceName(std::string name );
    std::string GetNetworkInterfaceNameByIndex(int32_t index );
    int32_t GetNetworkInterfaceIndexByName(std::string name );
    uint32_t ResolveHostName(int32_t _interface, int32_t protocol, std::string name, int32_t aprotocol, uint32_t flags );
    uint32_t ResolveAddress(int32_t _interface, int32_t protocol, std::string address, uint32_t flags );
    uint32_t ResolveService(int32_t _interface, int32_t protocol, std::string name, std::string type, std::string domain, int32_t aprotocol, uint32_t flags );
    DBus::Path EntryGroupNew( );
    DBus::Path DomainBrowserNew(int32_t _interface, int32_t protocol, std::string domain, int32_t btype, uint32_t flags );
    DBus::Path ServiceTypeBrowserNew(int32_t _interface, int32_t protocol, std::string domain, uint32_t flags );
    DBus::Path ServiceBrowserNew(int32_t _interface, int32_t protocol, std::string type, std::string domain, uint32_t flags );
    DBus::Path ServiceResolverNew(int32_t _interface, int32_t protocol, std::string name, std::string type, std::string domain, int32_t aprotocol, uint32_t flags );
    DBus::Path HostNameResolverNew(int32_t _interface, int32_t protocol, std::string name, int32_t aprotocol, uint32_t flags );
    DBus::Path AddressResolverNew(int32_t _interface, int32_t protocol, std::string address, uint32_t flags );
    DBus::Path RecordBrowserNew(int32_t _interface, int32_t protocol, std::string name, uint16_t clazz, uint16_t type, uint32_t flags );
    std::shared_ptr<DBus::SignalProxy<void(int32_t,std::string)>> signal_StateChanged( );
protected:
    std::shared_ptr<DBus::MethodProxy<std::string()>>  m_method_GetVersionString;
    std::shared_ptr<DBus::MethodProxy<uint32_t()>>  m_method_GetAPIVersion;
    std::shared_ptr<DBus::MethodProxy<std::string()>>  m_method_GetHostName;
    std::shared_ptr<DBus::MethodProxy<void(std::string)>>  m_method_SetHostName;
    std::shared_ptr<DBus::MethodProxy<std::string()>>  m_method_GetHostNameFqdn;
    std::shared_ptr<DBus::MethodProxy<std::string()>>  m_method_GetDomainName;
    std::shared_ptr<DBus::MethodProxy<bool()>>  m_method_IsNSSSupportAvailable;
    std::shared_ptr<DBus::MethodProxy<int32_t()>>  m_method_GetState;
    std::shared_ptr<DBus::MethodProxy<uint32_t()>>  m_method_GetLocalServiceCookie;
    std::shared_ptr<DBus::MethodProxy<std::string(std::string)>>  m_method_GetAlternativeHostName;
    std::shared_ptr<DBus::MethodProxy<std::string(std::string)>>  m_method_GetAlternativeServiceName;
    std::shared_ptr<DBus::MethodProxy<std::string(int32_t)>>  m_method_GetNetworkInterfaceNameByIndex;
    std::shared_ptr<DBus::MethodProxy<int32_t(std::string)>>  m_method_GetNetworkInterfaceIndexByName;
    std::shared_ptr<DBus::MethodProxy<uint32_t(int32_t,int32_t,std::string,int32_t,uint32_t)>>  m_method_ResolveHostName;
    std::shared_ptr<DBus::MethodProxy<uint32_t(int32_t,int32_t,std::string,uint32_t)>>  m_method_ResolveAddress;
    std::shared_ptr<DBus::MethodProxy<uint32_t(int32_t,int32_t,std::string,std::string,std::string,int32_t,uint32_t)>>  m_method_ResolveService;
    std::shared_ptr<DBus::MethodProxy<DBus::Path()>>  m_method_EntryGroupNew;
    std::shared_ptr<DBus::MethodProxy<DBus::Path(int32_t,int32_t,std::string,int32_t,uint32_t)>>  m_method_DomainBrowserNew;
    std::shared_ptr<DBus::MethodProxy<DBus::Path(int32_t,int32_t,std::string,uint32_t)>>  m_method_ServiceTypeBrowserNew;
    std::shared_ptr<DBus::MethodProxy<DBus::Path(int32_t,int32_t,std::string,std::string,uint32_t)>>  m_method_ServiceBrowserNew;
    std::shared_ptr<DBus::MethodProxy<DBus::Path(int32_t,int32_t,std::string,std::string,std::string,int32_t,uint32_t)>>  m_method_ServiceResolverNew;
    std::shared_ptr<DBus::MethodProxy<DBus::Path(int32_t,int32_t,std::string,int32_t,uint32_t)>>  m_method_HostNameResolverNew;
    std::shared_ptr<DBus::MethodProxy<DBus::Path(int32_t,int32_t,std::string,uint32_t)>>  m_method_AddressResolverNew;
    std::shared_ptr<DBus::MethodProxy<DBus::Path(int32_t,int32_t,std::string,uint16_t,uint16_t,uint32_t)>>  m_method_RecordBrowserNew;
    std::shared_ptr<DBus::SignalProxy<void(int32_t,std::string)>> m_signalproxy_StateChanged;
};
} /* namespace Avahi */
#endif /* ORG_FREEDESKTOP_AVAHI_SERVERPROXY_H */
