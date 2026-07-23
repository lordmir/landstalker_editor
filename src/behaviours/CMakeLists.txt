cmake_minimum_required(VERSION 3.28)

target_sources(${MODULE_NAME} PRIVATE
    "BehaviourCommandDataViewRenderer.cpp"
    "BehaviourCommandEditor.cpp"
    "BehaviourScriptDataViewModel.cpp"
    "BehaviourScriptEditorCtrl.cpp"
    "BehaviourScriptEditorFrame.cpp"
)
