# -------------------------------------------------
# Project created by QtCreator 2011-10-03T23:07:41
# -------------------------------------------------
QT += core \
    gui
TARGET = LuaIDE
TEMPLATE = app
SOURCES += src/main.cpp \
    src/interpreter.cpp \
    src/luacontrol.cpp \
    src/debugger.cpp \
    src/console.cpp \
    src/source.cpp \
    src/editor.cpp \
    src/breakpoint.cpp \
    src/watches/treeitem.cpp \
    src/watches/treemodel.cpp \
    src/watches/treeview.cpp \
    src/watchmodel.cpp
HEADERS += src/interpreter.h \
    src/luacontrol.h \
    src/debugger.h \
    src/console.h \
    src/source.h \
    src/editor.h \
    src/breakpoint.h \
    src/watches/treeitem.h \
    src/watches/treemodel.h \
    src/watches/treeview.h \
    src/watchmodel.h
LIBS += -lqscintilla2
RESOURCES += icons.qrc
OTHER_FILES += controller.lua
OBJECTS_DIR = build/
MOC_DIR = build/
RCC_DIR = build/
