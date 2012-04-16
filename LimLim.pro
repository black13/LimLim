# -------------------------------------------------
# Project created by QtCreator 2011-10-03T23:07:41
# -------------------------------------------------
QT += core \
    gui
TARGET = LimLim
TEMPLATE = app
SOURCES += src/main.cpp \
    src/interpreter.cpp \
    src/luacontrol.cpp \
    src/debugger.cpp \
    src/console.cpp \
    src/source.cpp \
    src/editor.cpp \
    src/breakpoint.cpp \
    src/watcher.cpp \
    src/hideeventwatcher.cpp \
    src/stack.cpp
HEADERS += src/interpreter.h \
    src/luacontrol.h \
    src/debugger.h \
    src/console.h \
    src/source.h \
    src/editor.h \
    src/breakpoint.h \
    src/watcher.h \
    src/global.h \
    src/hideeventwatcher.h \
    src/stack.h
LIBS += -lqscintilla2
RESOURCES += icons.qrc
OTHER_FILES += limdebug/controller.lua \
    limdebug/engine.lua
OBJECTS_DIR = build/
MOC_DIR = build/
RCC_DIR = build/
FORMS += forms/interpreter.ui