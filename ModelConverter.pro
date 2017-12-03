QT += gui
QT += core
QT += widgets
QT += opengl

CONFIG += c++11 console
CONFIG -= app_bundle

DEFINES += GLEW_STATIC GLM_STATIC@

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += main.cpp \
    mainwindow.cpp \
    lib/Authentication/PlaintextAuth.cpp \
    lib/Core/BufferStream.cpp \
    lib/Core/Character.cpp \
    lib/Core/Fragments.cpp \
    lib/Core/Geometry.cpp \
    lib/Core/LinearMath.cpp \
    lib/Core/Log.cpp \
    lib/Core/PFSArchive.cpp \
    lib/Core/PlaintextAuth.cpp \
    lib/Core/Platform.cpp \
    lib/Core/Skeleton.cpp \
    lib/Core/SoundTrigger.cpp \
    lib/Core/StreamReader.cpp \
    lib/Core/Table.cpp \
    lib/Core/WLDData.cpp \
    lib/Core/World.cpp \
    lib/Game/CharacterActor.cpp \
    lib/Game/Game.cpp \
    lib/Game/GamePacks.cpp \
    lib/Game/WLDActor.cpp \
    lib/Game/WLDMaterial.cpp \
    lib/Game/WLDModel.cpp \
    lib/Game/Zone.cpp \
    lib/Game/ZoneActors.cpp \
    lib/Game/ZoneObjects.cpp \
    lib/Game/ZoneTerrain.cpp \
    lib/Render/Material.cpp \
    lib/Render/RenderContextGL2.cpp \
    lib/Render/RenderProgramGL2.cpp \
    lib/Render/Vertex.cpp \
    lib/UI/CharacterScene.cpp \
    lib/UI/CharacterViewerWindow.cpp \
    lib/UI/Scene.cpp \
    lib/UI/SceneViewport.cpp \
    lib/UI/ZoneScene.cpp \
    lib/UI/ZoneViewerWindow.cpp \
    main.cpp \
    mainwindow.cpp \
    lib/Render/dxt.c \
    lib/Render/mipmap.c \


DISTFILES += \
    lib/Render/fragment.glsl \
    lib/Render/fragment_smooth_shading.glsl \
    lib/Render/vertex.glsl \
    lib/Render/vertex_skinned_texture.glsl \
    lib/Render/vertex_smooth_shading.glsl \


HEADERS += \
    mainwindow.h \
    EQuilibre/Core/win32/inttypes.h \
    EQuilibre/Core/win32/stdint.h \
    EQuilibre/Core/BufferStream.h \
    EQuilibre/Core/Character.h \
    EQuilibre/Core/Fragments.h \
    EQuilibre/Core/Geometry.h \
    EQuilibre/Core/LinearMath.h \
    EQuilibre/Core/Log.h \
    EQuilibre/Core/PFSArchive.h \
    EQuilibre/Core/Platform.h \
    EQuilibre/Core/Skeleton.h \
    EQuilibre/Core/SoundTrigger.h \
    EQuilibre/Core/StreamReader.h \
    EQuilibre/Core/Table.h \
    EQuilibre/Core/WLDData.h \
    EQuilibre/Core/World.h \
    EQuilibre/Game/CharacterActor.h \
    EQuilibre/Game/Game.h \
    EQuilibre/Game/GamePacks.h \
    EQuilibre/Game/WLDActor.h \
    EQuilibre/Game/WLDMaterial.h \
    EQuilibre/Game/WLDModel.h \
    EQuilibre/Game/Zone.h \
    EQuilibre/Game/ZoneActors.h \
    EQuilibre/Game/ZoneObjects.h \
    EQuilibre/Game/ZoneTerrain.h \
    EQuilibre/Render/dds.h \
    EQuilibre/Render/dxt.h \
    EQuilibre/Render/dxt_tables.h \
    EQuilibre/Render/imath.h \
    EQuilibre/Render/Material.h \
    EQuilibre/Render/mipmap.h \
    EQuilibre/Render/RenderContext.h \
    EQuilibre/Render/RenderProgram.h \
    EQuilibre/Render/Vertex.h \
    EQuilibre/UI/CharacterScene.h \
    EQuilibre/UI/CharacterViewerWindow.h \
    EQuilibre/UI/Scene.h \
    EQuilibre/UI/SceneViewport.h \
    EQuilibre/UI/ZoneScene.h \
    EQuilibre/UI/ZoneViewerWindow.h \
    include/zlib/include/zconf.h \
    include/zlib/include/zlib.h \
    include/glew-1.9.0/include/GL/glew.h \
    include/glew-1.9.0/include/GL/glxew.h \
    include/glew-1.9.0/include/GL/wglew.h \
    include/zlib/include/zconf.h \
    include/zlib/include/zlib.h

RESOURCES += \
    application.qrc \
    lib/Render/resources.qrc

unix|win32: LIBS += -L$$PWD/include/zlib/lib/ -lzdll

INCLUDEPATH += $$PWD/include/zlib/include
DEPENDPATH += $$PWD/include/zlib/include

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/include/zlib/lib/zdll.lib
else:unix|win32-g++: PRE_TARGETDEPS += $$PWD/include/zlib/lib/libzdll.a

unix|win32: LIBS += -L$$PWD/include/GL/ -lOpenGL32

INCLUDEPATH += $$PWD/include/glew-1.9.0/include
DEPENDPATH += $$PWD/include/glew-1.9.0/include

#LIBS += -L$$PWD/include/glew-2.1.0/bin/Release/x64/lglew32.dll

PRE_TARGETDEPS += $$PWD/include/GL/OpenGL32.lib

#LIBS += -L$$PWD/include/glew-2.1.0/lib/Release/x64/ -lglew32
LIBS += -L$$PWD/include/glew-1.9.0/lib/ -lglew32s


INCLUDEPATH += $$PWD/''
DEPENDPATH += $$PWD/''

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/include/glew-1.9.0/lib/libglew3s.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/include/glew-1.9.0/lib/libglew32.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/include/glew-1.9.0/lib/glew32s.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/include/glew-1.9.0/lib/glew32s.lib
else:unix: PRE_TARGETDEPS += $$PWD/include/glew-1.9.0/lib/Release/x64/libglew32.a
