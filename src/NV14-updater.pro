#-------------------------------------------------
#
# Project created by QtCreator 2018-09-21T20:23:59
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = NV14-updater
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    firmwarerequest.cpp \
    remotefileinfo.cpp \
    aboutdialog.cpp \
    dfurequest.cpp

HEADERS += \
        mainwindow.h \
    firmwarerequest.h \
    remotefileinfo.h \
    aboutdialog.h \
    dfurequest.h \
    version.h

FORMS += \
    mainwindow.ui \
    aboutdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc

RC_FILE = resources.rc

win32:RC_ICONS += resources/icon.ico

INCLUDEPATH += $$PWD/../dfuutils/include
win32{

    INCLUDEPATH += $$PWD/../libwdi/include
    INCLUDEPATH += $$PWD/../libusb-1.0.22/include
    INCLUDEPATH += "C:/Program Files (x86)/Windows Kits/10/Include/10.0.17763.0/ucrt"
    contains(QT_ARCH, i386) {
            LIBS += -L$$PWD/../libusb-1.0.22/MS32/dll -llibusb-1.0
            LIBS += -L$$PWD/../dfuutils/MS32 -ldfuutils
            LIBS += -L$$PWD/../libwdi/MS32 -llibwdi
            LIBS += -L"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.17763.0/ucrt/x86"
            DEPENDPATH += $$PWD/../libusb-1.0.22/MS32/dll
            DEPENDPATH += $$PWD/../dfuutils/MS32
            DEPENDPATH += $$PWD/../libwdi/MS32
    } else {
            QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'requireAdministrator\' uiAccess=\'false\'\"
            LIBS += -L$$PWD/../libusb-1.0.22/MS64/dll -llibusb-1.0
            LIBS += -L$$PWD/../dfuutils/MS64 -ldfuutils
            LIBS += -L$$PWD/../libwdi/MS64 -llibwdi
            LIBS += -L"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.17763.0/ucrt/x64"
            DEPENDPATH += $$PWD/../libusb-1.0.22/MS64/dll
            DEPENDPATH += $$PWD/../dfuutils/MS64
            DEPENDPATH += $$PWD/../libwdi/MS64

    }
}
unix{
LIBS += -L/usr/local/lib -lusb-1.0
LIBS += -L$$PWD/../dfuutils/ubuntu64 -ldfuutils
DEPENDPATH += $$PWD/../dfuutils/ubuntu64
}

