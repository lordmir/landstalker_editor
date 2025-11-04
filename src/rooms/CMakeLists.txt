cmake_minimum_required(VERSION 3.28)

target_sources(${MODULE_NAME} PRIVATE
    "CharacterDialog.cpp"
    "ChestDialog.cpp"
    "DoorDataViewModel.cpp"
    "EntityControlFrame.cpp"
    "EntityPropertiesWindow.cpp"
    "FlagDataViewModel.cpp"
    "FlagDialog.cpp"
    "HeightmapEditorCtrl.cpp"
    "LayerControlFrame.cpp"
    "Map3DEditor.cpp"
    "RoomErrorDialog.cpp"
    "RoomViewerCtrl.cpp"
    "RoomViewerFrame.cpp"
    "TileSwapControlFrame.cpp"
    "TileSwapDataViewModel.cpp"
    "TileSwapDialog.cpp"
    "WarpControlFrame.cpp"
    "WarpPropertyWindow.cpp"
)
