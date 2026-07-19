cmake_minimum_required(VERSION 3.28)

target_sources(${MODULE_NAME} PRIVATE
    "AssemblyBuilderDialog.cpp"
    "ApplicationPreferencesDialog.cpp"
    "ExecutorThread.cpp"
    "LookupChoiceControl.cpp"
    "LookupDataViewRenderer.cpp"
    "PreferencesDialog.cpp"
    "ResizeableGrid.cpp"
    "SearchableComboUtils.cpp"
    "SelectionControlFrame.cpp"
)
