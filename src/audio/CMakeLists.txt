cmake_minimum_required(VERSION 3.28)

target_sources(${MODULE_NAME} PRIVATE
    "AlsaPlayer.cpp"
    "AudioBankMappingFrame.cpp"
    "MusicEditorFrame.cpp"
    "SampleEditorFrame.cpp"
    "SfxEditorFrame.cpp"
    "SoundEventEditDialog.cpp"
    "SoundEventFormat.cpp"
    "SoundEventListColumn.cpp"
    "SoundEventYaml.cpp"
)
