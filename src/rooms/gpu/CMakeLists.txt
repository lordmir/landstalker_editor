cmake_minimum_required(VERSION 3.28)

target_sources(${MODULE_NAME} PRIVATE
    "GLCanvas.cpp"
    "GLCanvasEditorOverlays.cpp"
    "GLCanvasHeightmapHitTest.cpp"
    "GLCanvasHeightmapMode.cpp"
    "GLCanvasLayerEditMode.cpp"
    "GLCanvasObjectCommands.cpp"
    "GLCanvasRoomInfoOverlay.cpp"
    "GLCanvasRoomMode.cpp"
    "GLCanvasTilemapEditing.cpp"
    "GLCanvasHistory.cpp"
    "GLCanvasEntityEditor.cpp"
    "GLCanvasWarpEditor.cpp"
    "GLCanvasTileDoorEditor.cpp"
    "GLCanvasTileSwapEditor.cpp"
    "GLCanvasObjectCoordinator.cpp"
    "GLCanvasObjectSupport.cpp"
    "MapRenderer.cpp"
    "HeightmapRenderer.cpp"
    "SpriteRenderer.cpp"
    "GLLoader.cpp"
)
