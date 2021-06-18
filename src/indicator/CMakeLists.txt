
pkg_check_modules(
    QOFONO REQUIRED
    qofono-qt5
)
include_directories(${QOFONO_INCLUDE_DIRS})

include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories("${CMAKE_BINARY_DIR}/src/qdbus-stubs")
include_directories("${CMAKE_SOURCE_DIR}/src/qdbus-stubs")

# list these here to have them visible in Qt Creator
file(
        GLOB_RECURSE NETWORK_SERVICE_HEADERS
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
add_custom_target(QtCreatorHeaderHack SOURCES ${NETWORK_SERVICE_HEADERS})

set(NETWORK_SERVICE_SOURCES
    ${NETWORK_SERVICE_HEADERS}

    nmofono/connectivity-service-settings.cpp
    nmofono/hotspot-manager.cpp
    nmofono/kill-switch.cpp
    nmofono/manager.cpp
    nmofono/manager-impl.cpp
    nmofono/connection/active-connection.cpp
    nmofono/connection/active-connection-manager.cpp
    nmofono/connection/active-vpn-connection.cpp
    nmofono/wifi/access-point.cpp
    nmofono/wifi/access-point-impl.cpp
    nmofono/wifi/grouped-access-point.cpp
    nmofono/wifi/wifi-link-impl.cpp
    nmofono/wwan/modem.cpp
    nmofono/wwan/sim.cpp
    nmofono/wwan/sim-manager.cpp
    nmofono/wwan/qofono-sim-wrapper.cpp
    nmofono/vpn/openvpn-connection.cpp
    nmofono/vpn/pptp-connection.cpp
    nmofono/vpn/vpn-connection.cpp
    nmofono/vpn/vpn-manager.cpp

    factory.cpp
    indicator-menu.cpp
    icons.cpp
    menu-builder.cpp
    sim-unlock-dialog.cpp
    root-state.cpp
    vpn-status-notifier.cpp
    sections/quick-access-section.cpp
    sections/wifi-section.cpp
    sections/vpn-section.cpp
    sections/wwan-section.cpp

    connectivity-service/connectivity-service.cpp
    connectivity-service/dbus-openvpn-connection.cpp
    connectivity-service/dbus-pptp-connection.cpp
    connectivity-service/dbus-vpn-connection.cpp
    connectivity-service/dbus-modem.cpp
    connectivity-service/dbus-sim.cpp

    menuitems/access-point-item.cpp
    menuitems/switch-item.cpp
    menuitems/text-item.cpp
    menuitems/vpn-item.cpp
    menuitems/wifi-link-item.cpp
    menuitems/wwan-link-item.cpp
    menuitems/modem-info-item.cpp
)

qt5_add_dbus_adaptor(
    NETWORK_SERVICE_SOURCES
    "${DATA_DIR}/com.ubuntu.connectivity1.NetworkingStatus.xml"
    connectivity-service/connectivity-service.h
    connectivity_service::ConnectivityService
    NetworkingStatusAdaptor
)

qt5_add_dbus_adaptor(
    NETWORK_SERVICE_SOURCES
    "${DATA_DIR}/com.ubuntu.connectivity1.Modem.xml"
    connectivity-service/dbus-modem.h
    connectivity_service::DBusModem
    ModemAdaptor
)

qt5_add_dbus_adaptor(
    NETWORK_SERVICE_SOURCES
    "${DATA_DIR}/com.ubuntu.connectivity1.Sim.xml"
    connectivity-service/dbus-sim.h
    connectivity_service::DBusSim
    SimAdaptor
)

qt5_add_dbus_adaptor(
    NETWORK_SERVICE_SOURCES
    "${DATA_DIR}/com.ubuntu.connectivity1.Private.xml"
    connectivity-service/connectivity-service.h
    connectivity_service::PrivateService
    NetworkingStatusPrivateAdaptor
)

qt5_add_dbus_adaptor(
    NETWORK_SERVICE_SOURCES
    "${DATA_DIR}/com.ubuntu.connectivity1.vpn.VpnConnection.xml"
    connectivity-service/dbus-vpn-connection.h
    connectivity_service::DBusVpnConnection
    VpnConnectionAdaptor
)

qt5_add_dbus_adaptor(
    NETWORK_SERVICE_SOURCES
    "${DATA_DIR}/com.ubuntu.connectivity1.vpn.VpnConnection.OpenVpn.xml"
    connectivity-service/dbus-openvpn-connection.h
    connectivity_service::DBusOpenvpnConnection
    OpenVpnAdaptor
)

qt5_add_dbus_adaptor(
    NETWORK_SERVICE_SOURCES
    "${DATA_DIR}/com.ubuntu.connectivity1.vpn.VpnConnection.Pptp.xml"
    connectivity-service/dbus-pptp-connection.h
    connectivity_service::DBusPptpConnection
    PptpAdaptor
)

add_library(
  indicator-network-service-static
  STATIC
  ${NETWORK_SERVICE_SOURCES}
)

target_link_libraries(
    indicator-network-service-static
    menumodel_cpp
    notify_cpp
    url_dispatcher_cpp
    qdbus-stubs
    qpowerd
    util
    ${GLIB_LDFLAGS}
    ${QOFONO_LDFLAGS}
    Qt5::Core
    Qt5::DBus
    ${LIBNM_LIBRARIES}
)

###########################
# Executables
###########################

add_executable(
  indicator-network-service
  main.cpp
)

target_link_libraries(
    indicator-network-service
    indicator-network-service-static
    Qt5::Core
    Qt5::DBus
    ${LIBNM_LIBRARIES}
)

###########################
# Installation
###########################

install(
  TARGETS
    indicator-network-service
  RUNTIME DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}/indicator-network/"
)
