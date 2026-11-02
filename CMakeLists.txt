cmake_minimum_required(VERSION 3.16)

project(BattleShip VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/src/ui)

find_package(Qt5 5.12 REQUIRED COMPONENTS Core Gui Widgets)

# 纯逻辑层：不依赖 QtWidgets，可被单元测试直接链接
add_library(battleship_core STATIC
    src/core/Types.h
    src/core/Types.cpp
    src/core/Rules.h
    src/core/Rules.cpp
    src/core/Ship.h
    src/core/Ship.cpp
    src/core/Board.h
    src/core/Board.cpp
    src/core/GameSession.h
    src/core/GameSession.cpp
)
target_include_directories(battleship_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(battleship_core PUBLIC Qt5::Core)

# 界面层：独立成库，便于被测例直接构造
add_library(battleship_ui STATIC
    src/ui/Pixmaps.h
    src/ui/Pixmaps.cpp
    src/ui/CellButton.h
    src/ui/CellButton.cpp
    src/ui/BoardWidget.h
    src/ui/BoardWidget.cpp
    src/ui/TurnBar.h
    src/ui/TurnBar.cpp
    src/ui/PlacementView.h
    src/ui/PlacementView.cpp
    src/ui/BattleView.h
    src/ui/BattleView.cpp
    src/ui/RulesView.h
    src/ui/RulesView.cpp
    src/ui/rules.ui
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
)
target_link_libraries(battleship_ui PUBLIC battleship_core Qt5::Widgets)

# 可执行文件只保留入口与资源
add_executable(BattleShip WIN32
    src/main.cpp
    resources/res.qrc
)
target_link_libraries(BattleShip PRIVATE battleship_ui)

enable_testing()

find_package(Qt5 5.12 QUIET COMPONENTS Test)
if(Qt5Test_FOUND)
    add_subdirectory(tests)
else()
    message(STATUS "未找到 Qt5Test，跳过单元测试")
endif()
