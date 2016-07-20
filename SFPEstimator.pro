
QT += core gui
QT += widgets
QT += printsupport


TARGET   = SFPEstimator
TEMPLATE = app
CONFIG  += O3
CONFIG  += c++11

HEADERS += \
    src/roi.h \
    src/qtfiles.h \
    src/estimator.h \
    src/ImageStack/img_stack.hpp \
    src/imagedrawer.h \
    src/imagerender.h \
    src/lokalizationthread.h \
    src/stackoverview.h \
    src/threadsavequeue.h

SOURCES += \
    src/roi.cpp \
    src/main.cpp \
    src/estimator.cpp \
    src/ImageStack/img_stack.cpp \
    src/imagedrawer.cpp \
    src/imagerender.cpp \
    src/lokalizationthread.cpp \
    src/stackoverview.cpp \
    src/threadsavequeue.cpp


win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../../GnuWin32/lib/ -llibtiff
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../../GnuWin32/lib/ -llibtiff
else:unix: LIBS += -L$$PWD/../../../../../GnuWin32/lib/ -llibtiff

INCLUDEPATH += $$PWD/../../../../../GnuWin32/include
DEPENDPATH += $$PWD/../../../../../GnuWin32/include
