cmake_minimum_required(VERSION 3.28)

target_sources(${MODULE_NAME} PRIVATE
    "CharacterSfxCtrl.cpp"
    "CharacterSfxDataViewModel.cpp"
    "CharacterSfxFrame.cpp"
    "ProgressFlagsCtrl.cpp"
    "ProgressFlagsDataViewModel.cpp"
    "ProgressFlagsFrame.cpp"
    "ScriptDataViewEditorControl.cpp"
    "ScriptDataViewModel.cpp"
    "ScriptDataViewRenderer.cpp"
    "ScriptEditorCtrl.cpp"
    "ScriptEditorFrame.cpp"
    "ScriptTableTreeEditorCtrl.cpp"
    "ScriptTableTreeEditorFrame.cpp"
    "ScriptTreeActionEditors.cpp"
    "ScriptTreeBuilder.cpp"
    "ScriptTreeDataViewModel.cpp"
)
