set(LIB_SOURCES
    Connection.cpp
    Datagram.cpp
    Launcher.cpp
    Packet.cpp
    PacketQueue.cpp
    Session.cpp
)

set(LIB_HEADERS
    ../../include/EQuilibre/Network/Connection.h
    ../../include/EQuilibre/Network/Datagram.h
    ../../include/EQuilibre/Network/Launcher.h
    ../../include/EQuilibre/Network/Packet.h
    ../../include/EQuilibre/Network/PacketQueue.h
    ../../include/EQuilibre/Network/Session.h
)

QT4_WRAP_CPP(LIB_MOC_SOURCES ${LIB_HEADERS})

source_group(moc FILES ${LIB_MOC_SOURCES})

add_definitions(-DBUILD_NETWORK_DLL)

include_directories(${Zlib_INCLUDE_DIRS})

add_library(EQuilibreNetwork SHARED
    ${LIB_SOURCES}
    ${LIB_MOC_SOURCES}
    ${LIB_RESOURCES}
    ${LIB_RESOURCE_FILES}
    ${LIB_HEADERS}
)

target_link_libraries(EQuilibreNetwork
    EQuilibreCore
    ${QT_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${SYSTEM_LIBRARIES}
)
