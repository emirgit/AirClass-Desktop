QT       += core gui printsupport widgets pdf pdfwidgets websockets network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# Add websockets module with version check
greaterThan(QT_MAJOR_VERSION, 4): QT += websockets

# Add Qt include paths
INCLUDEPATH += $$[QT_INSTALL_HEADERS]
INCLUDEPATH += $$[QT_INSTALL_HEADERS]/QtCore
INCLUDEPATH += $$[QT_INSTALL_HEADERS]/QtGui
INCLUDEPATH += $$[QT_INSTALL_HEADERS]/QtWidgets
INCLUDEPATH += $$[QT_INSTALL_HEADERS]/QtWebSockets
DEPENDPATH += $$[QT_INSTALL_HEADERS]/QtWebSockets

CONFIG += c++17 c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    attendancemanager.cpp \
    gestureprocessor.cpp \
    main.cpp \
    mainwindow.cpp \
    presentationmanager.cpp \
    student.cpp \
    uicontroller.cpp \
    websocketclient.cpp \
    logindialog.cpp

HEADERS += \
    attendancemanager.h \
    gestureprocessor.h \
    mainwindow.h \
    presentationmanager.h \
    student.h \
    ui_mainwindow.h \
    uicontroller.h \
    websocketclient.h \
    logindialog.h

FORMS += \
    mainwindow.ui


TRANSLATIONS += \
    AirClassMay2_en_US.ts \
    AirClassMay2_tr_TR.ts

CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
