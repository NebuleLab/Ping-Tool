QT       += core gui widgets network sql charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

DEFINES += NOMINMAX

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/main.cpp \
    src/MainWindow.cpp \
    src/PingWorker.cpp \
    src/PingManager.cpp \
    src/PingModel.cpp \
    src/PingLogModel.cpp \
    src/DatabaseThread.cpp \
    src/ChartWindow.cpp

HEADERS += \
    src/MainWindow.h \
    src/PingWorker.h \
    src/PingManager.h \
    src/PingModel.h \
    src/PingLogModel.h \
    src/DatabaseThread.h \
    src/ChartWindow.h

# Windows specific libraries for ICMP
win32 {
    LIBS += -lws2_32 -liphlpapi
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
