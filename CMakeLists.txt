project(indicator-network C CXX)
cmake_minimum_required(VERSION 2.8.9)

string(TOLOWER "${CMAKE_BUILD_TYPE}" build_type_lower)

if("${build_type_lower}" STREQUAL debug)
  set(TRACE_DEFAULT TRUE)
else()
  set(TRACE_DEFAULT FALSE)
endif()

option(trace_messages "Print debug trace messages." ${TRACE_DEFAULT})
option(REMOTE_BUILD "Remote build (skip docs, translations, tests)." FALSE)
option(ENABLE_TESTS "Enable tests" TRUE)

if(${trace_messages})
  add_definitions(-DINDICATOR_NETWORK_TRACE_MESSAGES)
endif()

add_definitions(
  -DQT_NO_KEYWORDS=1
)

find_package(PkgConfig REQUIRED)
include(EnableCoverageReport)
include(GNUInstallDirs)
include(UseGSettings)

set(PROJECT_VERSION "16.10.0")
set(PACKAGE ${CMAKE_PROJECT_NAME})
set(GETTEXT_PACKAGE indicator-network)
add_definitions (
    -DGETTEXT_PACKAGE="${GETTEXT_PACKAGE}"
    -DGNOMELOCALEDIR="${CMAKE_INSTALL_FULL_LOCALEDIR}"
)

# Workaround for libexecdir on debian
if (EXISTS "/etc/debian_version")
  set(CMAKE_INSTALL_LIBEXECDIR ${CMAKE_INSTALL_LIBDIR})
  set(CMAKE_INSTALL_FULL_LIBEXECDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBEXECDIR}")
endif()

set(DATA_DIR "${CMAKE_SOURCE_DIR}/data")

set(GLIB_REQUIRED_VERSION 2.26)

pkg_check_modules(
  GLIB REQUIRED
  glib-2.0>=${GLIB_REQUIRED_VERSION}
  gio-2.0>=${GLIB_REQUIRED_VERSION}
)
include_directories(${GLIB_INCLUDE_DIRS})

pkg_check_modules(
  UNITY_API REQUIRED
  libunity-api
)
include_directories(${UNITY_API_INCLUDE_DIRS})

set(OFONO_REQUIRED_VERSION 1.12)
pkg_check_modules(
  OFONO REQUIRED
  ofono>=${OFONO_REQUIRED_VERSION}
)
include_directories(${OFONO_INCLUDE_DIRS})

pkg_check_modules(
    NETWORKMANAGER REQUIRED
    NetworkManager
)
include_directories(${NETWORKMANAGER_INCLUDE_DIRS})

pkg_check_modules(
  LIBSECRET REQUIRED
  libsecret-1
)
include_directories(${LIBSECRET_INCLUDE_DIRS})

find_package(Qt5Core REQUIRED)
include_directories(${Qt5Core_INCLUDE_DIRS})

find_package(Qt5DBus COMPONENTS Qt5DBusMacros REQUIRED)
include_directories(${Qt5DBus_INCLUDE_DIRS})

pkg_check_modules(NM REQUIRED NetworkManager REQUIRED)
include_directories(${NM_INCLUDE_DIRS})

pkg_check_modules(URL_DISPATCHER REQUIRED url-dispatcher-1)
include_directories(${URL_DISPATCHER_INCLUDE_DIRS})

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(${CMAKE_BINARY_DIR})

set(COMMON_FLAGS "-Wall -Wextra -Wpedantic -fno-permissive -fPIC -fvisibility=hidden -pthread")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)
if(CMAKE_COMPILER_IS_GNUCC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS}")
endif()

configure_file(
  "config.h.in"
  "config.h"
)

set(CONNECTIVITY_QT_VERSION_MAJOR 1)
set(CONNECTIVITY_QT_VERSION_MINOR 0)
set(CONNECTIVITY_QT_LIB_TARGET connectivity-qt${CONNECTIVITY_QT_VERSION_MAJOR})

add_subdirectory(data)
add_subdirectory(src)

if(NOT REMOTE_BUILD)

add_subdirectory(po)
add_subdirectory(doc)

if(ENABLE_TESTS)
enable_testing()
add_subdirectory(tests)

ADD_CUSTOM_TARGET(
        check
        ${CMAKE_CTEST_COMMAND} --force-new-ctest-process --output-on-failure
)

enable_coverage_report(
  TARGETS
    indicator-network-service
    menumodel_cpp
    notify_cpp
    qdbus-stubs
    i-n-sniffer
    url_dispatcher_cpp
  TESTS
    unit-tests
    integration-tests
    secret-agent-test-bin
  FILTER
    ${CMAKE_SOURCE_DIR}/tests/*
    ${CMAKE_BINARY_DIR}/*
)

endif()

endif()

add_subdirectory(scripts)
