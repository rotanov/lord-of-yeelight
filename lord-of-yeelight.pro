QT += core gui widgets sql network

TARGET = YeelightWifiBulbLanCtrl
TEMPLATE = app

SOURCES += main.cpp\
  mainwindow.cpp \
    storage.cpp \
    bulb.cpp \
    bulb_model.cpp

HEADERS += \
    storage.hpp \
    bulb.hpp \
    mainwindow.hpp \
    bulb_model.hpp

FORMS += mainwindow.ui

RC_FILE = app.rc
