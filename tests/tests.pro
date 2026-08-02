# Desktop-Testlauf für src/core/ — ohne Sailfish-SDK.
#
#   nix-shell --run 'scripts/run-tests.sh'
#
# Silica und alles unter src/platform/ bleiben aussen vor; getestet wird nur,
# was per Architekturentscheid ohnehin branchneutral sein muss.

TEMPLATE = app
TARGET   = core-tests
CONFIG  += console testcase c++14
CONFIG  -= app_bundle
QT      += testlib network sql
QT      -= gui

INCLUDEPATH += $$PWD/../src

SOURCES += \
    main.cpp \
    tst_sseparser.cpp \
    tst_conversationstore.cpp \
    tst_modellist.cpp

HEADERS += \
    tst_sseparser.h \
    tst_conversationstore.h \
    tst_modellist.h

# Der gesamte Core wird mitgebaut, nicht nur das Getestete — so faellt eine
# Aenderung, die den Target-Build brechen wuerde, schon hier auf.
SOURCES += \
    ../src/core/aiclient.cpp \
    ../src/core/capabilities.cpp \
    ../src/core/consentgate.cpp \
    ../src/core/conversationstore.cpp \
    ../src/core/keystore.cpp \
    ../src/core/localserverbackend.cpp \
    ../src/core/openrouterbackend.cpp \
    ../src/core/sseparser.cpp \
    ../src/core/toolregistry.cpp

HEADERS += \
    ../src/core/aiclient.h \
    ../src/core/capabilities.h \
    ../src/core/consentgate.h \
    ../src/core/conversationstore.h \
    ../src/core/illmbackend.h \
    ../src/core/keystore.h \
    ../src/core/localserverbackend.h \
    ../src/core/openrouterbackend.h \
    ../src/core/sseparser.h \
    ../src/core/toolregistry.h \
    ../src/platform/isystemprovider.h
