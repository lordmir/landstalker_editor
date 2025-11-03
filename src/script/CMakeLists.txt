cmake_minimum_required(VERSION 3.16)

target_sources(${MODULE_NAME} PRIVATE
    "CharacterSfxCtrl.cpp"
    "CharacterSfxDataViewModel.cpp"
    "CharacterSfxFrame.cpp"
    "DataViewScriptActionEditorControl.cpp"
    "DataViewScriptActionRenderer.cpp"
    "ProgressFlagsCtrl.cpp"
    "ProgressFlagsDataViewModel.cpp"
    "ProgressFlagsFrame.cpp"
    "ScriptDataViewEditorControl.cpp"
    "ScriptDataViewModel.cpp"
    "ScriptDataViewRenderer.cpp"
    "ScriptEditorCtrl.cpp"
    "ScriptEditorFrame.cpp"
    "ScriptTableDataViewModel.cpp"
    "ScriptTableEditorCtrl.cpp"
    "ScriptTableEditorFrame.cpp"
)
