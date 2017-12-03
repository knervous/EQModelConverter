set(LIB_SOURCES
    CharacterScene.cpp
    CharacterViewerWindow.cpp
    LauncherWindow.cpp
    Scene.cpp
    SceneViewport.cpp
    ZoneScene.cpp
    ZoneViewerWindow.cpp
)

set(LIB_HEADERS
    ../../include/EQuilibre/UI/CharacterScene.h
    ../../include/EQuilibre/UI/CharacterViewerWindow.h
    ../../include/EQuilibre/UI/LauncherWindow.h
    ../../include/EQuilibre/UI/SceneViewport.h
    ../../include/EQuilibre/UI/Scene.h
    ../../include/EQuilibre/UI/ZoneScene.h
    ../../include/EQuilibre/UI/ZoneViewerWindow.h
)

QT4_WRAP_CPP(LIB_MOC_SOURCES ${LIB_HEADERS})

source_group(moc FILES ${LIB_MOC_SOURCES})

add_definitions(-DBUILD_UI_DLL)

include_directories(${Newton_INCLUDE_DIRS})

add_library(EQuilibreUI SHARED
    ${LIB_SOURCES}
    ${LIB_MOC_SOURCES}
    ${LIB_RESOURCES}
    ${LIB_RESOURCE_FILES}
    ${LIB_HEADERS}
)

target_link_libraries(EQuilibreUI
    EQuilibreCore
    EQuilibreRender
    EQuilibreNetwork
    EQuilibreGame
    ${QT_LIBRARIES}
    ${SYSTEM_LIBRARIES}
)
