TEMPLATE = app

CONFIG -= QT
QT -= core gui

CONFIG += console debug_and_release
CONFIG -= flat

win32 {
    # treat all source files as C++
    QMAKE_CXXFLAGS += /TP
    # define warning level
    QMAKE_CXXFLAGS_WARN_ON = /W4 /wd4100 /wd4512 /wd4127 /wd4201 /wd4505
}

unix {
    # treat .c files as CPP targets
    QMAKE_EXT_CPP = .cpp .c
    # enable C++11 support
    QMAKE_CXXFLAGS += -std=c++11
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

# glm
win32 {
    INCLUDEPATH += $$THIRDPARTY/glm
}

HEADERS += \
    Common.hpp \
    ModuleIf.hpp \
    Platform.hpp \
    Renderer.hpp \
    StateDb.hpp \
    Template.hpp \
    TemplateIf.hpp

SOURCES += \
    Main.cpp \
    Platform.cpp \
    Renderer.cpp \
    StateDb.cpp
