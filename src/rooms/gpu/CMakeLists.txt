cmake_minimum_required(VERSION 3.28)

target_sources(${MODULE_NAME} PRIVATE
    "GLCanvas.cpp"
    "GLCanvasHeightmapMode.cpp"
    "GLCanvasLayerEditMode.cpp"
    "GLCanvasRoomMode.cpp"
    "GLCanvasEntityEditor.cpp"
    "GLCanvasWarpEditor.cpp"
    "GLCanvasTileDoorEditor.cpp"
    "GLCanvasObjectCoordinator.cpp"
    "GLCanvasObjectSupport.cpp"
    "MapRenderer.cpp"
    "HeightmapRenderer.cpp"
    "SpriteRenderer.cpp"
    "GLLoader.cpp"
)
