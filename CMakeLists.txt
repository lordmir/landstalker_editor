cmake_minimum_required(VERSION 3.28)
set(MODULE_NAME "landstalker_editor")
project(${MODULE_NAME} VERSION 0.4.0 DESCRIPTION "Landstalker Editor Application")
set(CMAKE_CXX_STANDARD 20)

option(LANDSTALKER_BUILD_SHARED "Build liblandstalker as a shared library" OFF)
option(INSTALL_DEPS "Automatically download and build dependencies" ON)

include(dependencies.cmake)

UpdateSubmodules()
InstallDependencies()

add_subdirectory(modules/liblandstalker EXCLUDE_FROM_ALL)

add_executable(${CMAKE_PROJECT_NAME} WIN32)

add_subdirectory("src")


if(WIN32)
    add_custom_command (
        TARGET "${CMAKE_PROJECT_NAME}" POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy -t "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>"
                "$<TARGET_RUNTIME_DLLS:${CMAKE_PROJECT_NAME}>" USES_TERMINAL COMMAND_EXPAND_LISTS
    )
endif()

if(${LANDSTALKER_BUILD_SHARED})
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE png_shared zlibshared)
else()
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE png_static zlibstatic)
endif()

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    wx::base
    wx::core
    wx::adv
    wx::xrc
    wx::propgrid
    wx::aui
    wx::xml
    yaml-cpp::yaml-cpp
    pugixml::static
    landstalker
)

install(
    TARGETS ${CMAKE_PROJECT_NAME}
    EXPORT ${CMAKE_PROJECT_NAME}Targets
    CONFIGURATIONS Release Debug
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES $<TARGET_RUNTIME_DLLS:${CMAKE_PROJECT_NAME}> TYPE BIN)
