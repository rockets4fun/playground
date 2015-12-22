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
}
unix {
    # Treat .c files as CPP targets
    QMAKE_EXT_CPP = .cpp .c
    # Enable C++11 support
    QMAKE_CXXFLAGS += -std=c++11
}
macx {
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9
    QMAKE_MAC_SDK = macosx10.9
    QMAKE_LFLAGS += -F/Library/Frameworks

    QMAKE_CXXFLAGS_WARN_ON  = -Wall
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter -Wno-null-dereference
    QMAKE_CXXFLAGS_WARN_ON += -Wno-null-dereference
}

THIRDPARTY = ../Thirdparty

# Profiling using "Brofiler" on Windows
win32 {
    INCLUDEPATH += $${THIRDPARTY}/Brofiler
    LIBS += $${THIRDPARTY}/Brofiler/ProfilerCore64.lib
}

# SDL
win32 {
    INCLUDEPATH += $${THIRDPARTY}/Sdl/include
    win32:contains(QMAKE_HOST.arch, x86_64) {
        LIBS += -L$${THIRDPARTY}/Sdl/lib/x64
    }
    else {
        LIBS += -L$${THIRDPARTY}/Sdl/lib/x86
    }
    LIBS += SDL2.lib SDL2main.lib
}
macx {
    # Install framwork from "http://libsdl.org/release/SDL2-2.0.3.dmg"
    INCLUDEPATH += /Library/Frameworks/SDL2.framework/Headers
    LIBS += -framework SDL2
}

# =====  OpenGL Mathematics (GLM) = http://glm.g-truc.net =========================================
win32 | unix {
    # Clone branch "0.9.6" from "https://github.com/g-truc/glm.git"
    INCLUDEPATH += $${THIRDPARTY}/Glm
}

# =====  Bullet Physics Library = http://bulletphysics.org  =======================================
win32 {
    # Check "USE_MSVC_RUNTIME_LIBRARY_DLL" before pressing "Generate"
    INCLUDEPATH += $${THIRDPARTY}/Bullet/src
    win32:contains(QMAKE_HOST.arch, x86_64) {
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
    else {
    }
}
macx {
    # Clone branch "master" from "https://github.com/bulletphysics/bullet3.git"
    # Use CMake to generate "Unix Makefiles" with "Use default native compilers"
    # "Configure", "Generate" and "make" from command line
    INCLUDEPATH += $${THIRDPARTY}/Bullet3/src
    LIBS += -L$${THIRDPARTY}/Bullet3/src/BulletCollision
    LIBS += -L$${THIRDPARTY}/Bullet3/src/BulletDynamics
    LIBS += -L$${THIRDPARTY}/Bullet3/src/LinearMath
    LIBS += -lBulletCollision -lBulletDynamics -lLinearMath
}

# =====  Open Asset Import Library = http://assimp.sf.net =========================================
win32 {
    INCLUDEPATH += $${THIRDPARTY}/assimp/include
    win32:contains(QMAKE_HOST.arch, x86_64) {
        CONFIG(debug, debug|release) {
            LIBS += $${THIRDPARTY}/Assimp/lib/assimp-vc120-mtd.lib
            LIBS += $${THIRDPARTY}/Assimp/lib/zlibstaticd.lib
        }
        CONFIG(release, debug|release) {
            LIBS += $${THIRDPARTY}/Assimp/lib/assimp-vc120-mt.lib
            LIBS += $${THIRDPARTY}/Assimp/lib/zlibstatic.lib
        }
    }
}
macx {
    # Clone branch "master" from "https://github.com/assimp/assimp.git"
    # Use CMake to generate "Unix Makefiles" with "Use default native compilers"
    # "Configure", uncheck "BUILD_SHARED_LIBS", "Generate" and "make" from command line
    INCLUDEPATH += $${THIRDPARTY}/Assimp/include
    LIBS += -L$${THIRDPARTY}/Assimp/lib
    LIBS += -lassimp -lz
}

# =====  ImGui - Bloat-free Immediate Mode GUI = https://github.com/ocornut/imgui  ================

HEADERS += $${THIRDPARTY}/ImGui/imgui.h
SOURCES += \
    $${THIRDPARTY}/ImGui/imgui.cpp \
    $${THIRDPARTY}/ImGui/imgui_draw.cpp

# =================================================================================================

HEADERS += \
    Template.hpp \
    TemplateIf.hpp \
    Common.hpp \
    ModuleIf.hpp

SOURCES += \
    Main.cpp

HEADERS += \
    Assets.hpp \
    Physics.hpp \
    Platform.hpp \
    Profiling.hpp \
    Logging.hpp \
    Math.hpp \
    Renderer.hpp \
    RocketScience.hpp \
    StateDb.hpp

SOURCES += \
    Assets.cpp \
    Physics.cpp \
    Platform.cpp \
    Profiling.cpp \
    Logging.cpp \
    Math.cpp \
    Renderer.cpp \
    RocketScience.cpp \
    StateDb.cpp

OTHER_FILES += \
    ../Assets/Programs/Default.program \
    ../Assets/Programs/Post.program \
    ../Assets/Programs/Emission.program \
    ../Assets/Programs/EmissionPost.program \
    ../Documentation/Notes.txt \
    ../Documentation/GameNotes.txt
