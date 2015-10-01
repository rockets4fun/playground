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
    # Multi-processor Compilation
    QMAKE_CXXFLAGS += /MP

    # Define warning level
    QMAKE_CXXFLAGS_WARN_ON = /W4
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
    QMAKE_CXXFLAGS_WARN_ON = -Wall -Wno-unused-parameter -Wno-null-dereference
}

THIRDPARTY = ../Thirdparty

# Profiling using "Brofiler" on Windows
win32 {
    INCLUDEPATH += $${THIRDPARTY}/Brofiler
    LIBS += $${THIRDPARTY}/Brofiler/ProfilerCore64.lib
}

# SDL
win32 {
    INCLUDEPATH += $${THIRDPARTY}/sdl/include
    win32:contains(QMAKE_HOST.arch, x86_64) {
        LIBS += -L$${THIRDPARTY}/sdl/lib/x64
    }
    else {
        LIBS += -L$${THIRDPARTY}/sdl/lib/x86
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
    # Clone branch "0.9.6" from "https://github.com/g-truc/glm.git"
    INCLUDEPATH += $${THIRDPARTY}/glm
}

# Bullet
win32 {
    # Check "USE_MSVC_RUNTIME_LIBRARY_DLL" before pressing "Generate"
    INCLUDEPATH += $${THIRDPARTY}/bullet/src
    win32:contains(QMAKE_HOST.arch, x86_64) {
        CONFIG(debug, debug|release) {
            LIBS += $${THIRDPARTY}/bullet/lib/BulletCollision_Debug.lib
            LIBS += $${THIRDPARTY}/bullet/lib/BulletDynamics_Debug.lib
            LIBS += $${THIRDPARTY}/bullet/lib/LinearMath_Debug.lib
        }
        CONFIG(release, debug|release) {
            LIBS += $${THIRDPARTY}/bullet/lib/BulletCollision.lib
            LIBS += $${THIRDPARTY}/bullet/lib/BulletDynamics.lib
            LIBS += $${THIRDPARTY}/bullet/lib/LinearMath.lib
        }
    }
    else {
    }
}
macx {
    # Clone branch "master" from "https://github.com/bulletphysics/bullet3.git"
    # Use CMake to generate "Unix Makefiles" with "Use default native compilers"
    # "Configure", "Generate" and "make" from command line
    INCLUDEPATH += $${THIRDPARTY}/bullet3/src
    LIBS += -L$${THIRDPARTY}/bullet3/src/BulletCollision
    LIBS += -L$${THIRDPARTY}/bullet3/src/BulletDynamics
    LIBS += -L$${THIRDPARTY}/bullet3/src/LinearMath
    LIBS += -lBulletCollision -lBulletDynamics -lLinearMath
}

# Assimp
win32 {
    INCLUDEPATH += $${THIRDPARTY}/assimp/include
    win32:contains(QMAKE_HOST.arch, x86_64) {
        CONFIG(debug, debug|release) {
            LIBS += $${THIRDPARTY}/assimp/lib/assimp-vc120-mtd.lib
            LIBS += $${THIRDPARTY}/assimp/lib/zlibstaticd.lib
        }
        CONFIG(release, debug|release) {
            LIBS += $${THIRDPARTY}/assimp/lib/assimp-vc120-mt.lib
            LIBS += $${THIRDPARTY}/assimp/lib/zlibstatic.lib
        }
    }
}
macx {
    # Clone branch "master" from "https://github.com/assimp/assimp.git"
    # Use CMake to generate "Unix Makefiles" with "Use default native compilers"
    # "Configure", uncheck "BUILD_SHARED_LIBS", "Generate" and "make" from command line
    INCLUDEPATH += $${THIRDPARTY}/assimp/include
    LIBS += -L$${THIRDPARTY}/assimp/lib
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
