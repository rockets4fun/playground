TEMPLATE = app

CONFIG -= QT
QT -= core gui

CONFIG += console debug_and_release
CONFIG -= flat

win32 {
    # Treat all source files as C++
    QMAKE_CXXFLAGS += /TP
    # Multi-processor Compilation
    QMAKE_CXXFLAGS += /MP

    # Define warning level
    QMAKE_CXXFLAGS_WARN_ON  = /W4
    # 'identifier' : unreferenced formal parameter
    QMAKE_CXXFLAGS_WARN_ON += /wd4100
    # conditional expression is constant
    QMAKE_CXXFLAGS_WARN_ON += /wd4127
    # 'identifier' : local variable is initialized but not referenced
    QMAKE_CXXFLAGS_WARN_ON += /wd4189
    # nonstandard extension used : nameless struct/union
    QMAKE_CXXFLAGS_WARN_ON += /wd4201
    # 'function' : unreferenced local function has been removed
    QMAKE_CXXFLAGS_WARN_ON += /wd4505
    # 'class' : assignment operator could not be generated
    QMAKE_CXXFLAGS_WARN_ON += /wd4512
    # 'function': This function or variable may be unsafe
    QMAKE_CXXFLAGS_WARN_ON += /wd4996

    # Mitigate VS 2017 15.8 compiler changes
    DEFINES += _DISABLE_EXTENDED_ALIGNED_STORAGE

    LIBS += Winmm.lib
    LIBS += version.lib
}
unix {
    # Treat .c files as CPP targets
    QMAKE_EXT_CPP = .cpp .c
    # Enable C++11 support
    QMAKE_CXXFLAGS += -std=c++11
}
macx {
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.13

    QMAKE_CXXFLAGS_WARN_ON  = -Wall
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter -Wno-null-dereference
    QMAKE_CXXFLAGS_WARN_ON += -Wno-null-dereference
}

THIRDPARTY = ../Thirdparty

# =====  Brofiler - C++ Profiler for Games = http://brofiler.com/ ==================================
#win32 {
#    INCLUDEPATH += $${THIRDPARTY}/Brofiler
#    LIBS += $${THIRDPARTY}/Brofiler/ProfilerCore64.lib
#}

# ===== Remotery - A realtime CPU/GPU profiler = https://github.com/Celtoys/Remotery ===============
INCLUDEPATH += $${THIRDPARTY}/Remotery/lib
HEADERS += $${THIRDPARTY}/Remotery/lib/Remotery.h
SOURCES += $${THIRDPARTY}/Remotery/lib/Remotery.c

# =====  Simple Direct-Media Layer (SDL) = http://libsdl.org/ ======================================
INCLUDEPATH += $${THIRDPARTY}/Sdl/include
win32 {
    CONFIG(debug, debug|release) {
        LIBS += $${THIRDPARTY}/Sdl/Build/SDL2d.lib
        LIBS += $${THIRDPARTY}/Sdl/Build/SDL2maind.lib
    }
    CONFIG(release, debug|release) {
        LIBS += $${THIRDPARTY}/Sdl/Build/SDL2.lib
        LIBS += $${THIRDPARTY}/Sdl/Build/SDL2main.lib
    }
    LIBS += setupapi.lib
}
unix {
    CONFIG(debug, debug|release) {
        LIBS += -L$${THIRDPARTY}/Sdl/Build
        LIBS += -lSDL2d -lSDL2maind
    }
    CONFIG(release, debug|release) {
        LIBS += -L$${THIRDPARTY}/Sdl/Build
        LIBS += -lSDL2 -lSDL2main
    }
    QMAKE_LFLAGS += -liconv
}
macx {
    QMAKE_LFLAGS += -framework AudioToolbox
    QMAKE_LFLAGS += -framework CoreAudio
    QMAKE_LFLAGS += -framework Carbon
    QMAKE_LFLAGS += -framework ForceFeedback
    QMAKE_LFLAGS += -framework IOKit
    QMAKE_LFLAGS += -framework Cocoa
    QMAKE_LFLAGS += -framework CoreVideo
}

# =====  OpenGL Mathematics (GLM) = http://glm.g-truc.net ==========================================
win32 | unix {
    INCLUDEPATH += $${THIRDPARTY}/Glm
    DEFINES += GLM_FORCE_PURE
    DEFINES += GLM_FORCE_CTOR_INIT
}

# =====  Bullet Physics Library = http://bulletphysics.org  ========================================
INCLUDEPATH += $${THIRDPARTY}/Bullet/src
win32 {
    CONFIG(debug, debug|release) {
        LIBS += $${THIRDPARTY}/Bullet/lib/BulletCollision_Debug.lib
        LIBS += $${THIRDPARTY}/Bullet/lib/BulletDynamics_Debug.lib
        LIBS += $${THIRDPARTY}/Bullet/lib/LinearMath_Debug.lib
    }
    CONFIG(release, debug|release) {
        LIBS += $${THIRDPARTY}/Bullet/lib/BulletCollision.lib
        LIBS += $${THIRDPARTY}/Bullet/lib/BulletDynamics.lib
        LIBS += $${THIRDPARTY}/Bullet/lib/LinearMath.lib
    }
}
unix {
    LIBS += -L$${THIRDPARTY}/Bullet/src/BulletCollision
    LIBS += -L$${THIRDPARTY}/Bullet/src/BulletDynamics
    LIBS += -L$${THIRDPARTY}/Bullet/src/LinearMath
    LIBS += -lBulletCollision -lBulletDynamics -lLinearMath
}

# =====  Open Asset Import Library = http://assimp.sf.net ==========================================
INCLUDEPATH += $${THIRDPARTY}/Assimp/include
win32 {
    CONFIG(debug, debug|release) {
        LIBS += $${THIRDPARTY}/Assimp/lib/assimp-mtd.lib
        LIBS += $${THIRDPARTY}/Assimp/lib/zlibstaticd.lib
        LIBS += $${THIRDPARTY}/Assimp/lib/IrrXMLd.lib
    }
    CONFIG(release, debug|release) {
        LIBS += $${THIRDPARTY}/Assimp/lib/assimp-mt.lib
        LIBS += $${THIRDPARTY}/Assimp/lib/zlibstatic.lib
        LIBS += $${THIRDPARTY}/Assimp/lib/IrrXML.lib
    }
}
unix {
    CONFIG(debug, debug|release) {
        LIBS += -L$${THIRDPARTY}/Assimp/lib
        LIBS += -lassimp-mtd -lz -lIrrXMLd
    }
    CONFIG(release, debug|release) {
        LIBS += -L$${THIRDPARTY}/Assimp/lib
        LIBS += -lassimp-mt -lz -lIrrXML
    }
}

# =====  ImGui - Bloat-free Immediate Mode GUI = https://github.com/ocornut/imgui  =================
INCLUDEPATH += $${THIRDPARTY}/ImGui
HEADERS += $${THIRDPARTY}/ImGui/imgui.h
SOURCES += \
    $${THIRDPARTY}/ImGui/imgui.cpp \
    $${THIRDPARTY}/ImGui/imgui_draw.cpp \
    $${THIRDPARTY}/ImGui/imgui_widgets.cpp
DEFINES += IMGUI_DISABLE_OBSOLETE_FUNCTIONS

# ==================================================================================================

HEADERS += \
    Template.hpp \
    TemplateIf.hpp \
    Common.hpp \
    ModuleIf.hpp

SOURCES += \
    Main.cpp

HEADERS += \
    AppShipLanding.hpp \
    AppSpaceThrusters.hpp \
    Assets.hpp \
    ImGuiEval.hpp \
    Physics.hpp \
    Platform.hpp \
    Profiler.hpp \
    Logger.hpp \
    Math.hpp \
    Parser.hpp \
    Renderer.hpp \
    StateDb.hpp \
    Str.hpp

SOURCES += \
    AppShipLanding.cpp \
    AppSpaceThrusters.cpp \
    Assets.cpp \
    ImGuiEval.cpp \
    Physics.cpp \
    Platform.cpp \
    Profiler.cpp \
    Logger.cpp \
    Math.cpp \
    Parser.cpp \
    Renderer.cpp \
    StateDb.cpp \
    Str.cpp

OTHER_FILES += \
    ../Assets/Programs/Default.program \
    ../Assets/Programs/Post.program \
    ../Assets/Programs/Emission.program \
    ../Assets/Programs/EmissionPost.program \
    ../Documentation/Notes.txt \
    ../Documentation/GameNotes.txt
