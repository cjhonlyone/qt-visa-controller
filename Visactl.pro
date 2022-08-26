QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

#CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

#INCLUDEPATH += "C:/Program Files/IVI Foundation/VISA/Win64/Include"
#LIBS += -L"C:/Program Files (x86)/IVI Foundation/VISA/WinNT/lib/msc" -lvisa32 -lnivisa64

INCLUDEPATH += "C:\Program Files (x86)\IVI Foundation\VISA\WinNT\Include"
LIBS += "C:\Program Files (x86)\IVI Foundation\VISA\WinNT\Lib_x64\msc\visa64.lib"
LIBS += "C:\Program Files (x86)\IVI Foundation\VISA\WinNT\Lib_x64\msc\nivisa64.lib"

# for msvc > 2015
#win32:msvc{
#    QMAKE_CXXFLAGS += /source-charset:utf-8 /execution-charset:utf-8
#}
