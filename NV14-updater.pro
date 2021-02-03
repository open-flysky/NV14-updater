QT = core gui widgets network

HEADERS = \
   src/aboutdialog.h \
   src/dfurequest.h \
   src/dfuutil.h \
   src/firmwarerequest.h \
   src/mainwindow.h \
   src/remotefileinfo.h \
   src/resources.rc \
   src/version.h \
   dfuutils/dfu-bool.h \
   dfuutils/dfu.h \
   dfuutils/intel_hex.h \
   dfuutils/stm32mem.h

SOURCES = \
   src/aboutdialog.cpp \
   src/dfurequest.cpp \
   src/dfuutil.cpp \
   src/firmwarerequest.cpp \
   src/main.cpp \
   src/mainwindow.cpp \
   src/remotefileinfo.cpp \
   dfuutils/intel_hex.c \
   dfuutils/dfu.c \
   dfuutils/stm32mem.c \

INCLUDEPATH = \
    dfuutils \
    src

LIBS += \
    -lusb-1.0

FORMS += \
    src/aboutdialog.ui \
    src/mainwindow.ui

RESOURCES += \
    src/resources.qrc

