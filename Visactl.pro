QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# 跨平台：用 QTcpSocket 直连 SCPI Raw Socket (5025) 替代 NI-VISA，
# 不再依赖任何外部库 / 平台特定路径。
SOURCES += \
    main.cpp \
    mainwindow.cpp \
    scpiclient.cpp

HEADERS += \
    mainwindow.h \
    scpiclient.h

FORMS += \
    mainwindow.ui
