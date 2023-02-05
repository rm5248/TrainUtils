set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include( GNUInstallDirs )
include( FindPkgConfig )

add_compile_definitions(QT_NO_KEYWORDS)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets SerialPort Network REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets SerialPort Network REQUIRED)
find_package(fmt)

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
		
	pkg_check_modules( train-required-dbus REQUIRED
		dbus-cxx-qt-2.0
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

#pkg_check_modules( train-required REQUIRED
#    liblog4cxx-qt>=1.0.0
#)
find_package(log4cxx CONFIG REQUIRED)
find_package(log4cxx-qt CONFIG REQUIRED)
find_package(qtadvanceddocking CONFIG REQUIRED)

target_link_libraries(train-gui PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::SerialPort
    Qt${QT_VERSION_MAJOR}::Network
    ads::qtadvanceddocking
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
    /usr/local/include/qtads
    ${train-required-dbus_INCLUDE_DIRS}
    ../LCC
    ../LocoNetCommunications
    ../LoconetTCPUtil
)

install(TARGETS train-gui
        RUNTIME DESTINATION bin
)
