set(CMAKE_AUTOMOC ON)

add_executable(tst_board tst_board.cpp)
target_link_libraries(tst_board PRIVATE battleship_core Qt5::Test)
add_test(NAME tst_board COMMAND tst_board)

add_executable(tst_gamesession tst_gamesession.cpp)
target_link_libraries(tst_gamesession PRIVATE battleship_core Qt5::Test)
add_test(NAME tst_gamesession COMMAND tst_gamesession)

add_executable(tst_ui tst_ui.cpp ${CMAKE_SOURCE_DIR}/resources/res.qrc)
target_link_libraries(tst_ui PRIVATE battleship_ui Qt5::Test)
add_test(NAME tst_ui COMMAND tst_ui -platform offscreen)
set_tests_properties(tst_ui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
