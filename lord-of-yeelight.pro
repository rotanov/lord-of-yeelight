QT += core gui widgets sql network

TARGET = YeelightWifiBulbLanCtrl
TEMPLATE = app

SOURCES += main.cpp\
  mainwindow.cpp \
    storage.cpp \
    bulb.cpp \
    bulb_model.cpp \
    header_view_with_checkbox.cpp \

HEADERS += \
    storage.hpp \
    bulb.hpp \
    mainwindow.hpp \
    bulb_model.hpp \
    header_view_with_checkbox.hpp \

FORMS += mainwindow.ui

RC_FILE = app.rc
