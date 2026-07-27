cmake_minimum_required(VERSION 3.28)
set(MODULE_NAME "landstalker_editor")
project(${MODULE_NAME} VERSION 0.4.0 DESCRIPTION "Landstalker Editor Application")
set(CMAKE_CXX_STANDARD 20)

include(GNUInstallDirs)

option(LANDSTALKER_BUILD_SHARED "Build liblandstalker as a shared library" OFF)
option(INSTALL_DEPS "Automatically download and build dependencies" ON)
option(LANDSTALKER_BUILD_TESTING "Build the test suites" OFF)

include(dependencies.cmake)

UpdateSubmodules()
InstallDependencies()

find_package(OpenGL REQUIRED)

add_subdirectory(modules/liblandstalker EXCLUDE_FROM_ALL)

add_executable(${CMAKE_PROJECT_NAME} WIN32)

add_subdirectory("src")

if(LANDSTALKER_BUILD_TESTING)
    enable_testing()
    add_subdirectory("tests")
    # modules/liblandstalker is added with EXCLUDE_FROM_ALL, which also excludes its test
    # executables - without this, a default build never produces them and ctest is left
    # running gtest_discover_tests' "<target>_NOT_BUILT" placeholders.
    set_target_properties(lz77_tests tilemap3d_tests script_tests asm_file_tests labels_tests PROPERTIES EXCLUDE_FROM_ALL FALSE)
endif()

if(WIN32)
    add_custom_command (
        TARGET "${CMAKE_PROJECT_NAME}" POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy -t "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>"
                "$<TARGET_RUNTIME_DLLS:${CMAKE_PROJECT_NAME}>" USES_TERMINAL COMMAND_EXPAND_LISTS
    )
endif()

if(${LANDSTALKER_BUILD_SHARED})
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE png_shared ZLIB::ZLIB)
else()
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE png_static ZLIB::ZLIB)
endif()

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    wx::base
    wx::core
    wx::gl
    wx::adv
    wx::xrc
    wx::propgrid
    wx::aui
    wx::xml
    wx::stc
    OpenGL::GL
    GLEW::GLEW
    yaml-cpp::yaml-cpp
    pugixml::static
    landstalker
)

# On the GTK port some controls need to reach beneath wx to the native GtkTreeView (see
# ScriptEditorCtrl.cpp's hyperlink handling) - wx::core doesn't propagate the toolkit's own
# include directories, so pull them in explicitly.
if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE PkgConfig::GTK3)
endif()

install(
    TARGETS ${CMAKE_PROJECT_NAME}
    EXPORT ${CMAKE_PROJECT_NAME}Targets
    CONFIGURATIONS Release Debug
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES $<TARGET_RUNTIME_DLLS:${CMAKE_PROJECT_NAME}> TYPE BIN)

# Desktop integration for Linux: a .desktop launcher plus the application icon installed into the
# freedesktop hicolor theme, so the editor shows up (with its icon) in application menus/launchers.
if(UNIX AND NOT APPLE)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/landstalker_editor.desktop.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.desktop"
        @ONLY
    )
    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.desktop"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications"
    )
    foreach(icon_size 16 32 48 64 128)
        install(
            FILES "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/icons/${icon_size}x${icon_size}/${CMAKE_PROJECT_NAME}.png"
            DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/${icon_size}x${icon_size}/apps"
        )
    endforeach()
endif()
