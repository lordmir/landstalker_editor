#ifndef _SOUND_EVENT_YAML_H_
#define _SOUND_EVENT_YAML_H_

#include <yaml-cpp/yaml.h>

#include <audio/SoundEventFormat.h>
#include <landstalker/main/MusicData.h>

// YAML (de)serialization for a single music track pool entry or SFX pool entry - used by the
// "Export YAML.../Import YAML..." buttons in MusicEditorFrame/SfxEditorFrame. This is a separate
// interchange format, independent of MusicData's own ASM save/load - editing YAML never touches
// the project on disk directly, only the in-memory pool entry currently being viewed.
//
// The event dict shapes (note/rest/sample/noise, cmd: <name> + fields) are deliberately close to
// the ones landstalker_tools_python/scripts/audio.py's decode_sequence/encode_sequence produce,
// though that script works at a whole sound-bank granularity (header + pointer table + every
// track's blocks in one file) while this works one track/SFX at a time - full interop with files
// it produces isn't expected, but the vocabulary should be easy to eyeball against either.

void EmitMusicTrackYaml(YAML::Emitter& out, const Landstalker::MusicData::MusicTrackEntry& entry);
// Throws std::runtime_error (with a field-path message) on any malformed input.
Landstalker::MusicData::MusicTrackEntry MusicTrackEntryFromYaml(const YAML::Node& root);

void EmitSfxEntryYaml(YAML::Emitter& out, const Landstalker::MusicData::SfxPoolEntry& entry);
Landstalker::MusicData::SfxPoolEntry SfxPoolEntryFromYaml(const YAML::Node& root);

#endif // _SOUND_EVENT_YAML_H_
