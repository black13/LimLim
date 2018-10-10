######################################################################
# Automatically generated by qmake (2.01a) ut 17. IV 10:39:58 2012
######################################################################
QT += core gui widgets
CONFIG += release
TEMPLATE = app
TARGET = limlim
DEPENDPATH += . build forms src
INCLUDEPATH += .
SOURCES += src/main.cpp \
    src/interpreter.cpp \
    src/luacontrol.cpp \
    src/debugger.cpp \
    src/console.cpp \
    src/source.cpp \
    src/editor.cpp \
    src/breakpoint.cpp \
    src/watcher.cpp \
    src/stack.cpp
HEADERS += src/interpreter.h \
    src/luacontrol.h \
    src/debugger.h \
    src/console.h \
    src/source.h \
    src/editor.h \
    src/breakpoint.h \
    src/watcher.h \
    src/stack.h
LIBS += -lqscintilla2
RESOURCES += icons.qrc
OTHER_FILES += limdebug/controller.lua \
    limdebug/engine.lua
OBJECTS_DIR = build/
MOC_DIR = build/
RCC_DIR = build/

FORMS += \
    forms/interpreter.ui \
    forms/debugger.ui

win32: DEFINES = QSCINTILLA_DLL
