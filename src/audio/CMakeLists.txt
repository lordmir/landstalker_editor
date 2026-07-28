cmake_minimum_required(VERSION 3.28)

target_sources(${MODULE_NAME} PRIVATE
    "AlsaPlayer.cpp"
    "AudioBankMappingFrame.cpp"
    "AudioTablesYaml.cpp"
    "InstrumentParamsFrame.cpp"
    "InstrumentParamsYaml.cpp"
    "MidiWriter.cpp"
    "MusicEditorFrame.cpp"
    "SampleEditorFrame.cpp"
    "SfxEditorFrame.cpp"
    "SoundEventEditDialog.cpp"
    "SoundEventFormat.cpp"
    "SoundEventListColumn.cpp"
    "SoundEventMidi.cpp"
    "SoundEventYaml.cpp"
    "YamlIo.cpp"
    "YmInstrumentEditorFrame.cpp"
    "YmInstrumentYaml.cpp"
)
