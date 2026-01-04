QT += core gui widgets sql concurrent printsupport network

TARGET = TaskManager
TEMPLATE = app
CONFIG += c++17 lrelease embed_translations

# 如需禁用 Qt 6 之前的废弃 API，可取消下一行注释
# DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

# ---------- QXlsx 依赖（需提前编译好 QXlsx） ----------
#INCLUDEPATH += $$PWD/../QXlsx/include
#LIBS        += -L$$PWD/../QXlsx/lib -lQXlsx
# -------------------------------------------------------

SOURCES += \
    ExportManager.cpp \
    ReminderWorker.cpp \
    TaskDatabase.cpp \
    TaskModel.cpp \
    main.cpp \
    MainWindow.cpp \

HEADERS += \
    MainWindow.h \
    ReminderWorker.h \
    ExportManager.h \
    TaskDatabase.h \
    TaskModel.h

FORMS += \
    MainWindow.ui

TRANSLATIONS +=

# ---------- 默认部署规则 ----------
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
