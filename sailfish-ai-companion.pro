# Sailfish AI Companion — Dual-Target Build
#
#   Harbour-Build:   sfdk build -- --define "harbour 1"
#   Full-Access:     sfdk build
#
# Der Trunk ist IMMER Harbour-clean. fullaccess fügt nur hinzu.

CONFIG(fullaccess) {
    TARGET   = sailfishai
    DEFINES += SFAI_FULLACCESS SFAI_TARGET_NAME=\\\"sailfishai\\\"
    message("Building FULL-ACCESS target (OpenRepos)")
} else {
    CONFIG  += harbour
    TARGET   = harbour-nemoai
    DEFINES += SFAI_HARBOUR SFAI_TARGET_NAME=\\\"harbour-nemoai\\\"
    message("Building HARBOUR target (Jolla Store)")
}

CONFIG += sailfishapp c++14
QT     += network sql dbus

SOURCES += \
    src/main.cpp \
    src/core/aiclient.cpp \
    src/core/conversationstore.cpp \
    src/core/toolregistry.cpp \
    src/core/consentgate.cpp \
    src/core/capabilities.cpp \
    src/core/keystore.cpp \
    src/core/openrouterbackend.cpp \
    src/core/localserverbackend.cpp

HEADERS += \
    src/core/aiclient.h \
    src/core/conversationstore.h \
    src/core/toolregistry.h \
    src/core/consentgate.h \
    src/core/capabilities.h \
    src/core/keystore.h \
    src/core/illmbackend.h \
    src/core/openrouterbackend.h \
    src/core/localserverbackend.h \
    src/platform/isystemprovider.h

harbour {
    SOURCES += src/platform/sandboxed/sandboxedprovider.cpp
    HEADERS += src/platform/sandboxed/sandboxedprovider.h
}
fullaccess {
    SOURCES += src/platform/full/fullprovider.cpp
    HEADERS += src/platform/full/fullprovider.h
}

DISTFILES += \
    qml/main.qml \
    qml/pages/*.qml \
    qml/components/*.qml \
    qml/cover/*.qml \
    rpm/harbour-nemoai.spec \
    rpm/sailfishai.spec \
    translations/*.ts

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

CONFIG += sailfishapp_i18n
TRANSLATIONS += translations/harbour-nemoai-de.ts
