add_executable(editor_script_tree_tests
    test_script_tree.cpp
    ${CMAKE_SOURCE_DIR}/src/script/ScriptTreeBuilder.cpp
    ${CMAKE_SOURCE_DIR}/src/script/ScriptTreeDataViewModel.cpp
)
target_include_directories(editor_script_tree_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/modules/liblandstalker/landstalker/include
)
target_link_libraries(editor_script_tree_tests PRIVATE
    landstalker
    yaml-cpp::yaml-cpp
    wx::base
    wx::core
    wx::adv
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(editor_script_tree_tests)
