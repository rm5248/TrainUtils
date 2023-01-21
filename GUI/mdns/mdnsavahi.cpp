/* SPDX-License-Identifier: GPL-2.0 */
#include "mdnsavahi.h"

#include <log4cxx/logger.h>
#include <QTimer>
#include <fmt/format.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.mdns.MDNSAvahi" );

static void dbus_log_function(const char* logger_name,
        const struct SL_LogLocation* location,
        const enum SL_LogLevel level,
        const char* log_string ){
    log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( logger_name );
    log4cxx::spi::LocationInfo inf( location->file,
                                    log4cxx::spi::LocationInfo::calcShortFileName(location->file),
                                    location->function,
                                    location->line_number);

    switch(level){
    case SL_TRACE:
        logger->trace(log_string, inf);
        break;
    case SL_DEBUG:
        logger->debug(log_string, inf);
        break;
    case SL_INFO:
        logger->info(log_string, inf);
        break;
    case SL_WARN:
        logger->warn(log_string, inf);
        break;
    case SL_ERROR:
        logger->error(log_string, inf);
        break;
    case SL_FATAL:
        logger->fatal(log_string, inf);
        break;
    }
}

MDNSAvahi::MDNSAvahi(QObject *parent)
    : MDNSManager{parent}
{
    DBus::set_logging_function(dbus_log_function);

    m_dispatcher = DBus::Qt::QtDispatcher::create();
    m_connection = m_dispatcher->create_connection( DBus::BusType::SYSTEM );

    QTimer::singleShot( 500, this, &MDNSAvahi::initialize );
}

void MDNSAvahi::initialize(){
    m_avahiServer = Avahi::ServerProxy::create( m_connection, "org.freedesktop.Avahi", "/" );

    DBus::Path serviceBrowsePath =
            m_avahiServer->getorg_freedesktop_Avahi_ServerInterface()
                ->ServiceBrowserNew( -1, 0, "_openlcb-can._tcp", std::string(), 0 );

    LOG4CXX_DEBUG_FMT( logger, "Service browser path: {}", serviceBrowsePath );

    m_browserProxyLCC = Avahi::ServiceBrowserProxy::create( m_connection, "org.freedesktop.Avahi", serviceBrowsePath );

    std::shared_ptr<Avahi::org_freedesktop_Avahi_ServiceBrowserProxy> proxyTmp =
            m_browserProxyLCC->getorg_freedesktop_Avahi_ServiceBrowserInterface();

    proxyTmp->signal_Failure()->connect( sigc::mem_fun( *this, &MDNSAvahi::signalFailure) );
    proxyTmp->signal_AllForNow()->connect( sigc::mem_fun( *this, &MDNSAvahi::signalAllForNow ) );
    proxyTmp->signal_CacheExhausted()->connect( sigc::mem_fun( *this, &MDNSAvahi::signalCacheExhausted ) );
    proxyTmp->signal_ItemNew()->connect( sigc::mem_fun( *this, &MDNSAvahi::signalLCCNew ) );
    proxyTmp->signal_ItemRemove()->connect( sigc::mem_fun( *this, &MDNSAvahi::signalLCCRemoved ) );

    proxyTmp->Start();

    // Create a new resolver for Loconet
    DBus::Path serviceBrowseLoconetPath =
            m_avahiServer->getorg_freedesktop_Avahi_ServerInterface()
                ->ServiceBrowserNew( -1, 0, "_loconetovertcpserver._tcp", std::string(), 0 );

    m_browserProxyLoconet = Avahi::ServiceBrowserProxy::create( m_connection, "org.freedesktop.Avahi", serviceBrowseLoconetPath );

    std::shared_ptr<Avahi::org_freedesktop_Avahi_ServiceBrowserProxy> proxyTmp2 =
            m_browserProxyLoconet->getorg_freedesktop_Avahi_ServiceBrowserInterface();

    proxyTmp2->signal_Failure()->connect( sigc::mem_fun( *this, &MDNSAvahi::signalFailure) );
    proxyTmp2->signal_AllForNow()->connect( sigc::mem_fun( *this, &MDNSAvahi::signalAllForNow ) );
    proxyTmp2->signal_CacheExhausted()->connect( sigc::mem_fun( *this, &MDNSAvahi::signalCacheExhausted ) );
    proxyTmp2->signal_ItemNew()->connect( sigc::mem_fun( *this, &MDNSAvahi::signalLoconetNew ) );
    proxyTmp2->signal_ItemRemove()->connect( sigc::mem_fun( *this, &MDNSAvahi::signalLoconetRemoved ) );

    proxyTmp2->Start();
}

void MDNSAvahi::signalFailure(std::string str){
    LOG4CXX_DEBUG( logger, "Avahi failure: " << str );
}

void MDNSAvahi::signalAllForNow(){
    LOG4CXX_DEBUG( logger, "All for now" );
}

void MDNSAvahi::signalCacheExhausted(){
    LOG4CXX_DEBUG( logger, "Cache exhausted" );
}

void MDNSAvahi::signalLCCNew(int32_t interface,
                                int32_t protocol,
                                std::string name,
                                std::string type,
                                std::string domain,
                                uint32_t flags){
    LOG4CXX_DEBUG_FMT( logger, "New LCC over TCP!  name: {} type: {}", name, type );


    LOG4CXX_DEBUG_FMT( logger, "Getting new resolver for {}:{}:{}", name, type, domain );
    DBus::Path serviceResolverPath =
            m_avahiServer->getorg_freedesktop_Avahi_ServerInterface()
            ->ServiceResolverNew(-1, 0, name, type, domain, 0, 0 );

    LOG4CXX_DEBUG_FMT( logger, "Service resolver path = {}", serviceResolverPath );
    std::shared_ptr<Avahi::ServiceResolverProxy> newResolver =
            Avahi::ServiceResolverProxy::create( m_connection, "org.freedesktop.Avahi", serviceResolverPath );
    m_nameToResolver[ QString::fromStdString( name ) ] = newResolver;

    newResolver->getorg_freedesktop_Avahi_ServiceResolverInterface()
            ->signal_Found()->connect( sigc::mem_fun( *this, &MDNSAvahi::resolvedLCCFound ) );
    newResolver->getorg_freedesktop_Avahi_ServiceResolverInterface()
            ->signal_Failure()->connect( sigc::mem_fun( *this, &MDNSAvahi::resolvedLCCError ) );
    //newResolver->getorg_freedesktop_Avahi_ServiceResolverInterface()->Start();

}

void MDNSAvahi::signalLCCRemoved(int32_t interface,
                                int32_t protocol,
                                std::string name,
                                std::string type,
                                std::string domain,
                                uint32_t flags){
    LOG4CXX_DEBUG( logger, "Removed item!  name: " << name << " type: " << type );
    QString nameAsQStr = QString::fromStdString( name );

    Q_EMIT lccServerLeft(nameAsQStr);
}

void MDNSAvahi::resolvedLCCFound(int32_t interface,
                        int32_t protocol,
                        std::string name,
                        std::string type,
                        std::string domain,
                        std::string host,
                        int32_t aprotocol,
                        std::string address,
                        uint16_t port,
                        std::vector<std::vector<uint8_t> > txt,
                        uint32_t flags){
    LOG4CXX_DEBUG( logger, "Found!  name: "
                   << name
                   << " type: "
                   << type
                   << " host: "
                   << host
                   << " address: "
                   << address
                   << " port: "
                   << port );

    Q_EMIT lccServerFound(QString::fromStdString(name), QHostAddress(QString::fromStdString(address)), port);
}

void MDNSAvahi::resolvedLCCError(std::string err){
    LOG4CXX_ERROR( logger, "Resolver had error: " << err );
}

void MDNSAvahi::signalLoconetNew(int32_t interface,
                                int32_t protocol,
                                std::string name,
                                std::string type,
                                std::string domain,
                                uint32_t flags){
    LOG4CXX_DEBUG_FMT( logger, "New Loconet over TCP!  name: {} type: {}", name, type );


    LOG4CXX_DEBUG_FMT( logger, "Getting new resolver for {}:{}:{}", name, type, domain );
    DBus::Path serviceResolverPath =
            m_avahiServer->getorg_freedesktop_Avahi_ServerInterface()
            ->ServiceResolverNew(-1, 0, name, type, domain, 0, 0 );

    LOG4CXX_DEBUG_FMT( logger, "Service resolver path = {}", serviceResolverPath );
    std::shared_ptr<Avahi::ServiceResolverProxy> newResolver =
            Avahi::ServiceResolverProxy::create( m_connection, "org.freedesktop.Avahi", serviceResolverPath );
    m_nameToResolver[ QString::fromStdString( name ) ] = newResolver;

    newResolver->getorg_freedesktop_Avahi_ServiceResolverInterface()
            ->signal_Found()->connect( sigc::mem_fun( *this, &MDNSAvahi::resolvedLoconetFound ) );
    newResolver->getorg_freedesktop_Avahi_ServiceResolverInterface()
            ->signal_Failure()->connect( sigc::mem_fun( *this, &MDNSAvahi::resolvedLoconetError ) );
    //newResolver->getorg_freedesktop_Avahi_ServiceResolverInterface()->Start();

}

void MDNSAvahi::signalLoconetRemoved(int32_t interface,
                                int32_t protocol,
                                std::string name,
                                std::string type,
                                std::string domain,
                                uint32_t flags){
    LOG4CXX_DEBUG( logger, "Removed item!  name: " << name << " type: " << type );
    QString nameAsQStr = QString::fromStdString( name );

    Q_EMIT loconetServerLeft(nameAsQStr);
}

void MDNSAvahi::resolvedLoconetFound(int32_t interface,
                        int32_t protocol,
                        std::string name,
                        std::string type,
                        std::string domain,
                        std::string host,
                        int32_t aprotocol,
                        std::string address,
                        uint16_t port,
                        std::vector<std::vector<uint8_t> > txt,
                        uint32_t flags){
    LOG4CXX_DEBUG( logger, "Found!  name: "
                   << name
                   << " type: "
                   << type
                   << " host: "
                   << host
                   << " address: "
                   << address
                   << " port: "
                   << port );

    Q_EMIT loconetServerFound(QString::fromStdString(name), QHostAddress(QString::fromStdString(address)), port);
}

void MDNSAvahi::resolvedLoconetError(std::string err){
    LOG4CXX_ERROR( logger, "Resolver had error: " << err );
}
