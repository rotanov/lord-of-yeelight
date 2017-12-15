QT += core gui widgets sql network

TARGET = LordOfYeelight
TEMPLATE = app

SOURCES += main.cpp\
    main_window.cpp \
    storage.cpp \
    bulb.cpp \
    bulb_model.cpp \
    header_view_with_checkbox.cpp \

HEADERS += \
    storage.hpp \
    bulb.hpp \
    main_window.hpp \
    bulb_model.hpp \
    header_view_with_checkbox.hpp \

FORMS += main_window.ui

RC_FILE = app.rc
