set(DOC_SOURCES
    examples/example_networking_status.qml
    qml-api/NetworkingStatus.qml
    pages/mainpage.qdoc)

# add a target to generate API documentation with qdoc
FIND_PROGRAM(QDOC_EXECUTABLE qdoc)

if(QDOC_EXECUTABLE)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/connectivity-qml.qdocconf.in"
    "${CMAKE_CURRENT_BINARY_DIR}/connectivity-qml.qdocconf"
    @ONLY
)

add_custom_target(qmldoc
    ALL
    COMMAND "QT_SELECT=qt5" "${QDOC_EXECUTABLE}" "${CMAKE_CURRENT_BINARY_DIR}/connectivity-qml.qdocconf"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Generating QML API documentation with qdoc" VERBATIM
    DEPENDS ${DOC_SOURCES}
)

# copy stylesheet files into build directory for shadow builds
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/css"
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)

# copy the API files over ot the build directery as there is a
# bug in qt5.2 qdoc for handling absolute paths
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/qml-api"
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/pages"
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/examples"
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)

install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/
    DESTINATION ${INSTALL_DOCDIR}/qml/html/
)

endif(QDOC_EXECUTABLE)
