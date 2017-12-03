set(LIB_SOURCES
    BufferStream.cpp
    Character.cpp
    Fragments.cpp
    Geometry.cpp
    LinearMath.cpp
    Log.cpp
    Message.cpp
    MessageDecoders.cpp
    MessageEncoders.cpp
    PFSArchive.cpp
    Platform.cpp
    Skeleton.cpp
    SoundTrigger.cpp
    StreamReader.cpp
    Table.cpp
    WLDData.cpp
    World.cpp
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/CryptoAuth.cpp")
    set(LIB_SOURCES ${LIB_SOURCES} CryptoAuth.cpp)
else()
    set(LIB_SOURCES ${LIB_SOURCES} PlaintextAuth.cpp)
endif()

set(LIB_HEADERS
    ../../include/EQuilibre/Core/Authentication.h
    ../../include/EQuilibre/Core/BufferStream.h
    ../../include/EQuilibre/Core/Character.h
    ../../include/EQuilibre/Core/Fragments.h
    ../../include/EQuilibre/Core/Geometry.h
    ../../include/EQuilibre/Core/LinearMath.h
    ../../include/EQuilibre/Core/Log.h
    ../../include/EQuilibre/Core/Message.h
    ../../include/EQuilibre/Core/MessageDecoders.def
    ../../include/EQuilibre/Core/MessageEncoders.def
    ../../include/EQuilibre/Core/MessageStructs.h
    ../../include/EQuilibre/Core/PFSArchive.h
    ../../include/EQuilibre/Core/Platform.h
    ../../include/EQuilibre/Core/Skeleton.h
    ../../include/EQuilibre/Core/SoundTrigger.h
    ../../include/EQuilibre/Core/StreamReader.h
    ../../include/EQuilibre/Core/Table.h
    ../../include/EQuilibre/Core/WLDData.h
    ../../include/EQuilibre/Core/World.h
)

QT4_WRAP_CPP(LIB_MOC_SOURCES ${LIB_HEADERS})

source_group(moc FILES ${LIB_MOC_SOURCES})

add_definitions(-DBUILD_CORE_DLL)

include_directories(
    ${Zlib_INCLUDE_DIRS}
    ${TomCrypt_INCLUDE_DIRS}
)

add_library(EQuilibreCore SHARED
    ${LIB_SOURCES}
    ${LIB_MOC_SOURCES}
    ${LIB_RESOURCES}
    ${LIB_RESOURCE_FILES}
    ${LIB_HEADERS}
)

add_dependencies(EQuilibreCore EQuilibreTables)

target_link_libraries(EQuilibreCore
    ${QT_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${TomCrypt_LIBRARIES}
    ${SYSTEM_LIBRARIES}
)
