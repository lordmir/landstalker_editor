CMAKE_MINIMUM_REQUIRED(VERSION 3.13.4)

FIND_PACKAGE(Git QUIET)
IF(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    OPTION(GIT_SUBMODULE "Check submodules during build" ON)
    IF(GIT_SUBMODULE)
        MESSAGE(STATUS "Submodule Update")
        EXECUTE_PROCESS(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        IF(NOT GIT_SUBMOD_RESULT EQUAL "0")
            MESSAGE(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please check")
        ENDIF()
    ENDIF()
ENDIF()

IF(NOT EXISTS "${PROJECT_SOURCE_DIR}/landstalker_gfx/third_party/wxWidgets/CMakeLists.txt")
    MESSAGE(FATAL_ERROR "The wxWidgets submodule was not downloaded!")
ENDIF()

ADD_EXECUTABLE(${PROJECT_NAME})

CONFIGURE_FILE(landstalker_gfx.h.in landstalker_gfx.h)

SET(wxBUILD_SHARED OFF)
ADD_SUBDIRECTORY(third_party/wxWidgets)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} wx::xrc wx::propgrid wx::aui wx::adv wx::core wx::base)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF
)

TARGET_SOURCES(
    ${PROJECT_NAME}
    PUBLIC src/AnimatedTileset.cpp
    PUBLIC src/AsmFile.cpp
    PUBLIC src/BitBarrel.cpp
    PUBLIC src/BitBarrelWriter.cpp
    PUBLIC src/Block.cpp
    PUBLIC src/Blockmap2D.cpp
    PUBLIC src/BlockmapIsometric.cpp
    PUBLIC src/BlocksetCmp.cpp
    PUBLIC src/EditorFrame.cpp
    PUBLIC src/EndCreditString.cpp
    PUBLIC src/HuffmanString.cpp
    PUBLIC src/HuffmanTree.cpp
    PUBLIC src/HuffmanTrees.cpp
    PUBLIC src/ImageBuffer.cpp
    PUBLIC src/ImageList.cpp
    PUBLIC src/IntroString.cpp
    PUBLIC src/LSString.cpp
    PUBLIC src/LSTilemapCmp.cpp
    PUBLIC src/LZ77.cpp
    PUBLIC src/main.cpp
    PUBLIC src/MainFrame.cpp
    PUBLIC src/Palette.cpp
    PUBLIC src/PaletteEditor.cpp
    PUBLIC src/Rom.cpp
    PUBLIC src/RomOffsets.cpp
    PUBLIC src/Sprite.cpp
    PUBLIC src/SpriteFrame.cpp
    PUBLIC src/SpriteGraphic.cpp
    PUBLIC src/Tile.cpp
    PUBLIC src/TileAttributes.cpp
    PUBLIC src/TileEditor.cpp
    PUBLIC src/Tilemap.cpp
    PUBLIC src/Tilemap2DRLE.cpp
    PUBLIC src/Tilemap3DCmp.cpp
    PUBLIC src/Tileset.cpp
    PUBLIC src/TilesetEditor.cpp
    PUBLIC src/TilesetEditorFrame.cpp
    PUBLIC src/TilesetManager.cpp
    PUBLIC src/Utils.cpp
    PUBLIC src/wxcrafter.cpp
    PUBLIC src/wxcrafter_bitmaps.cpp
)

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME}
    PUBLIC ${PROJECT_BINARY_DIR}
    PUBLIC src/include
)

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION bin)
