set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include( GNUInstallDirs )
include( FindPkgConfig )

add_compile_definitions(QT_NO_KEYWORDS)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets SerialPort Network REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets SerialPort Network REQUIRED)
find_package(fmt)
if(UNIX)
    find_package(Threads)
    find_package(dbus-cxx COMPONENTS qt)
endif()

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        trainutils_state.h
        lcc/lccmanager.cpp
        lcc/lccmanager.h
        lcc/lccconnection.cpp
        lcc/lccconnection.h
        lcc/lccnetworkconnection.cpp
        lcc/lccnetworkconnection.h
        lcc/lccserialconnection.h
        lcc/lccserialconnection.cpp
        lcc/lccqioconnection.h
        lcc/lccqioconnection.cpp
        mdns/mdnsmanager.cpp
        mdns/mdnsmanager.h
        lcctrafficmonitor.cpp
        lcctrafficmonitor.h
        lcctrafficmonitor.ui
        loconet/loconetmanager.cpp
        loconet/loconetmanager.h
        loconet/loconetconnection.cpp
        loconet/loconetconnection.h
        loconet/loconetnetworkconnection.cpp
        loconet/loconetnetworkconnection.h
        loconet/loconetusage.h
        loconet/loconetusage.cpp
        loconettrafficmonitor.h
        loconettrafficmonitor.cpp
        loconettrafficmonitor.ui
        loconetslotmonitor.h
        loconetslotmonitor.cpp
        loconetslotmonitor.ui
        loconetslotmonitormodel.h
        loconetslotmonitormodel.cpp
        loconet/loconetserialconnection.h
        loconet/loconetserialconnection.cpp
        systemconnection.h
        systemconnection.cpp
        systemconnectionstatuswidget.h
        systemconnectionstatuswidget.cpp
        systemconnectionstatuswidget.ui
        lcceventtransmit.h
        lcceventtransmit.cpp
        lcceventtransmit.ui
        lccnodeinformation.h
        lccnodeinformation.cpp
        lccnodeinformation.ui
        lccnetworkview.h
        lccnetworkview.cpp
        lccnetworkview.ui
        lccnetworktablemodel.h
        lccnetworktablemodel.cpp
        loconetswitchcontrol.h
        loconetswitchcontrol.cpp
        loconetswitchcontrol.ui
        throttledisplay.h
        throttledisplay.cpp
        throttledisplay.ui
        loconet/loconetthrottle.h
        loconet/loconetthrottle.cpp
        throttle.h
        throttle.cpp
        lccmemorydisplay.cpp
        lccmemorydisplay.h
        lccmemorydisplay.ui
        lcc/lccnode.cpp
        lcc/lccnode.h
        lcc/cdi/segment.cpp
        lcc/cdi/segment.h
        lcc/cdi/cdi.cpp
        lcc/cdi/cdi.h
        lcc/cdi/identification.cpp
        lcc/cdi/identification.h
        lcc/cdi/maptype.cpp
        lcc/cdi/maptype.h
        lcc/cdi/acdi.cpp
        lcc/cdi/acdi.h
        lcc/cdi/cdivariant.cpp
        lcc/cdi/cdivariant.h
        lcc/cdi/inttype.cpp
        lcc/cdi/inttype.h
        lcc/cdi/stringtype.cpp
        lcc/cdi/stringtype.h
        lcc/cdi/eventidtype.cpp
        lcc/cdi/eventidtype.h
        lcc/cdi/grouptype.cpp
        lcc/cdi/grouptype.h
        trainutils.qrc
        panels/paneldisplay.h panels/paneldisplay.cpp
        panels/turnoutdisplay.h panels/turnoutdisplay.cpp
        panels/panellayout.h panels/panellayout.cpp
)

if(UNIX)
    set(PROJECT_SOURCES ${PROJECT_SOURCES}
        mdns/avahi-dbus/org_freedesktop_Avahi_Server2Proxy.cpp
        mdns/avahi-dbus/org_freedesktop_Avahi_ServiceBrowserProxy.h
        mdns/avahi-dbus/ServiceBrowserProxy.cpp
        mdns/avahi-dbus/org_freedesktop_Avahi_Server2Proxy.h
        mdns/avahi-dbus/org_freedesktop_Avahi_ServiceResolverProxy.cpp
        mdns/avahi-dbus/ServiceBrowserProxy.h
        mdns/avahi-dbus/org_freedesktop_Avahi_ServerProxy.cpp
        mdns/avahi-dbus/org_freedesktop_Avahi_ServiceResolverProxy.h
        mdns/avahi-dbus/ServiceResolverProxy.cpp
        mdns/avahi-dbus/org_freedesktop_Avahi_ServerProxy.h
        mdns/avahi-dbus/ServerProxy.cpp
        mdns/avahi-dbus/ServiceResolverProxy.h
        mdns/avahi-dbus/org_freedesktop_Avahi_ServiceBrowserProxy.cpp
        mdns/avahi-dbus/ServerProxy.h

        mdns/mdnsavahi.h
        mdns/mdnsavahi.cpp
        )
endif(UNIX)

if(WIN32)
    set(PROJECT_SOURCES ${PROJECT_SOURCES}
        mdns/mdnswindows.h
        mdns/mdnswindows.cpp
    )
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(train-gui
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(train-gui SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(train-gui
            ${PROJECT_SOURCES}
        )
    endif()
endif()

pkg_check_modules( train-required REQUIRED
    sigc++-3.0
)
find_package(log4cxx CONFIG REQUIRED)
find_package(log4cxx-qt CONFIG REQUIRED)
find_package(qtadvanceddocking-qt6 CONFIG REQUIRED)

target_link_libraries(train-gui PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::SerialPort
    Qt${QT_VERSION_MAJOR}::Network
    ads::qtadvanceddocking-qt6
    ${train-required_LIBRARIES}
    ${train-required-dbus_LIBRARIES}
    lcc
    loconet-tcp
    loconet_utils
    fmt::fmt
    log4cxx
    log4cxx-qt
)
target_include_directories(train-gui PRIVATE
    ${train-required_INCLUDE_DIRS}
    ../LCC
    ../LocoNetCommunications
    ../LoconetTCPUtil
)
if(UNIX)
    target_link_libraries(train-gui PRIVATE
        dbus-cxx::dbus-cxx-qt
        )
endif()

install(TARGETS train-gui
        RUNTIME DESTINATION bin
        CONFIGURATIONS Release
)

configure_file(log4cxx.xml ${CMAKE_CURRENT_BINARY_DIR}/log4cxx.xml COPYONLY)
