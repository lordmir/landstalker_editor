CMAKE_MINIMUM_REQUIRED(VERSION 3.13.4)

INCLUDE(FindPkgConfig)
IF(NOT PKG_CONFIG_FOUND)
    MESSAGE(FATAL_ERROR "pkg-config not found")
ENDIF()

CONFIGURE_FILE(landstalker_gfx.h.in landstalker_gfx.h)

ADD_EXECUTABLE(${PROJECT_NAME})

FIND_PACKAGE(wxWidgets COMPONENTS xrc propgrid aui adv core base REQUIRED)
IF(wxWidgets_FOUND)
    INCLUDE(${wxWidgets_USE_FILE})
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${wxWidgets_LIBRARIES})
ENDIF()

FIND_PACKAGE(PNG REQUIRED)
IF(PNG_FOUND)
    INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIR})
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${PNG_LIBRARY})
ENDIF()

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
