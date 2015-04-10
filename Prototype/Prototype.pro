TEMPLATE = app

CONFIG -= QT
QT -= core gui

CONFIG += console debug_and_release
CONFIG -= flat

# Max OS X deployment target
macx {
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9
    QMAKE_MAC_SDK = macosx10.9
    QMAKE_LFLAGS += -F/Library/Frameworks
}

# Compiler settings
win32 {
    # Treat all source files as C++
    QMAKE_CXXFLAGS += /TP
    # Define warning level
    QMAKE_CXXFLAGS_WARN_ON = /W4 /wd4100 /wd4512 /wd4127 /wd4201 /wd4505
}
unix {
    # Treat .c files as CPP targets
    QMAKE_EXT_CPP = .cpp .c
    # Enable C++11 support
    QMAKE_CXXFLAGS += -std=c++11
}
macx {
    QMAKE_CXXFLAGS_WARN_ON = -Wall -Wno-unused-parameter -Wno-null-dereference
}

THIRDPARTY = ../Thirdparty

# SDL
win32 {
    INCLUDEPATH += $$THIRDPARTY/sdl/include
    win32:contains(QMAKE_HOST.arch, x86_64) {
        LIBS += -L$$THIRDPARTY/sdl/lib/x64
    }
    else {
        LIBS += -L$$THIRDPARTY/sdl/lib/x86
    }
    LIBS += SDL2.lib SDL2main.lib
}
macx {
    # Install framwork from "http://libsdl.org/release/SDL2-2.0.3.dmg"
    INCLUDEPATH += /Library/Frameworks/SDL2.framework/Headers
    LIBS += -framework SDL2
}

# glm
win32 | macx {
    # Clone branch "9.6.0" from "https://github.com/g-truc/glm.git"
    INCLUDEPATH += $$THIRDPARTY/glm
}

# Bullet
win32 {
    INCLUDEPATH += $$THIRDPARTY/bullet3/src
    LIBS += -L$$THIRDPARTY/bullet3/bin
    win32:contains(QMAKE_HOST.arch, x86_64) {
        CONFIG(debug, debug|release) {
            LIBS += $$THIRDPARTY/bullet3/bin/BulletCollision_vs2010_x64_debug.lib
            LIBS += $$THIRDPARTY/bullet3/bin/BulletDynamics_vs2010_x64_debug.lib
            LIBS += $$THIRDPARTY/bullet3/bin/LinearMath_vs2010_x64_debug.lib
        }
        CONFIG(release, debug|release) {
            LIBS += $$THIRDPARTY/bullet3/bin/BulletCollision_vs2010_x64_release.lib
            LIBS += $$THIRDPARTY/bullet3/bin/BulletDynamics_vs2010_x64_release.lib
            LIBS += $$THIRDPARTY/bullet3/bin/LinearMath_vs2010_x64_release.lib
        }
    }
    else {
    }
}
macx {
    # Clone branch "master" from "https://github.com/bulletphysics/bullet3.git"
    # Use CMake to generate "Unix Makefiles" with "Use default native compilers"
    # "Configure", "Generate" and "make" from command line
    INCLUDEPATH += $$THIRDPARTY/bullet3/src
    LIBS += -L$$THIRDPARTY/bullet3/src/BulletCollision
    LIBS += -L$$THIRDPARTY/bullet3/src/BulletDynamics
    LIBS += -L$$THIRDPARTY/bullet3/src/LinearMath
    LIBS += -lBulletCollision -lBulletDynamics -lLinearMath
}

# Assimp
win32 {
    INCLUDEPATH += $$THIRDPARTY/assimp/include
    win32:contains(QMAKE_HOST.arch, x86_64) {
        CONFIG(debug, debug|release) {
            LIBS += $$THIRDPARTY/assimp/lib/Debug/assimpd.lib
        }
        CONFIG(release, debug|release) {
            LIBS += $$THIRDPARTY/assimp/lib/Release/assimp.lib
        }
    }
}
macx {
    # Clone branch "master" from "https://github.com/assimp/assimp.git"
    # Use CMake to generate "Unix Makefiles" with "Use default native compilers"
    # "Configure", uncheck "BUILD_SHARED_LIBS", "Generate" and "make" from command line
    INCLUDEPATH += $$THIRDPARTY/assimp/include
    LIBS += -L$$THIRDPARTY/assimp/lib
    LIBS += -lassimp -lz
}

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
    Math.hpp \
    Renderer.hpp \
    RocketScience.hpp \
    StateDb.hpp

SOURCES += \
    Assets.cpp \
    Physics.cpp \
    Platform.cpp \
    Math.cpp \
    Renderer.cpp \
    RocketScience.cpp \
    StateDb.cpp
