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
)

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
    dbus-cxx-qt-2.0
    liblog4cxx-qt>=1.0.0
)

target_link_libraries(train-gui PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::SerialPort
    Qt${QT_VERSION_MAJOR}::Network
    qtadvanceddocking
    ${train-required_LIBRARIES}
    lcc
    fmt::fmt
)
target_include_directories(train-gui PRIVATE
    /usr/local/include/qtads
    ${train-required_INCLUDE_DIRS}
    ../LCC
)
