QT += core gui widgets
CONFIG += c++17
TEMPLATE = app
TARGET = BattleShip

DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += $$PWD/src

SOURCES += \
    src/main.cpp \
    src/core/Types.cpp \
    src/core/Rules.cpp \
    src/core/Ship.cpp \
    src/core/Board.cpp \
    src/core/GameSession.cpp \
    src/ui/Pixmaps.cpp \
    src/ui/CellButton.cpp \
    src/ui/ShipDrag.cpp \
    src/ui/BoardWidget.cpp \
    src/ui/ShipPalette.cpp \
    src/ui/TurnBar.cpp \
    src/ui/PlacementView.cpp \
    src/ui/BattleView.cpp \
    src/ui/Theme.cpp \
    src/ui/GameControls.cpp \
    src/ui/MenuButton.cpp \
    src/ui/MenuPage.cpp \
    src/ui/RulesView.cpp \
    src/ui/MainWindow.cpp

HEADERS += \
    src/core/Types.h \
    src/core/Rules.h \
    src/core/Ship.h \
    src/core/Board.h \
    src/core/GameSession.h \
    src/ui/Pixmaps.h \
    src/ui/CellButton.h \
    src/ui/ShipDrag.h \
    src/ui/BoardWidget.h \
    src/ui/ShipPalette.h \
    src/ui/TurnBar.h \
    src/ui/PlacementView.h \
    src/ui/BattleView.h \
    src/ui/Theme.h \
    src/ui/GameControls.h \
    src/ui/MenuButton.h \
    src/ui/MenuPage.h \
    src/ui/RulesView.h \
    src/ui/MainWindow.h

RESOURCES += \
    resources/res.qrc
