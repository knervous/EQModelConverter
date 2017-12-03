set(LIB_SOURCES
    CharacterActor.cpp
    Game.cpp
    GameClient.cpp
    GamePacks.cpp
    WLDActor.cpp
    WLDMaterial.cpp
    WLDModel.cpp
    Zone.cpp
    ZoneActors.cpp
    ZoneObjects.cpp
    ZoneTerrain.cpp
)

set(LIB_HEADERS
    ../../include/EQuilibre/Game/CharacterActor.h
    ../../include/EQuilibre/Game/Game.h
    ../../include/EQuilibre/Game/GameClient.h
    ../../include/EQuilibre/Game/GamePacks.h
    ../../include/EQuilibre/Game/WLDMaterial.h
    ../../include/EQuilibre/Game/WLDModel.h
    ../../include/EQuilibre/Game/WLDActor.h
    ../../include/EQuilibre/Game/Zone.h
    ../../include/EQuilibre/Game/ZoneActors.h
    ../../include/EQuilibre/Game/ZoneObjects.h
    ../../include/EQuilibre/Game/ZoneTerrain.h
)

QT4_WRAP_CPP(LIB_MOC_SOURCES ${LIB_HEADERS})

source_group(moc FILES ${LIB_MOC_SOURCES})

add_definitions(-DBUILD_GAME_DLL)

include_directories(${Newton_INCLUDE_DIRS})

add_library(EQuilibreGame SHARED
    ${LIB_SOURCES}
    ${LIB_MOC_SOURCES}
    ${LIB_RESOURCES}
    ${LIB_RESOURCE_FILES}
    ${LIB_HEADERS}
)

target_link_libraries(EQuilibreGame
    EQuilibreCore
    EQuilibreNetwork
    EQuilibreRender
    ${QT_LIBRARIES}
    ${Newton_LIBRARIES}
    ${SYSTEM_LIBRARIES}
)
