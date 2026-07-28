#ifndef _AUDIO_TABLES_YAML_H_
#define _AUDIO_TABLES_YAML_H_

#include <vector>

#include <yaml-cpp/yaml.h>

#include <landstalker/main/AudioData.h>
#include <landstalker/main/MusicData.h>

// YAML (de)serialisation for the PCM sample directory and the music/SFX bank slot mappings.
//
// The sample table uses landstalker_tools_python's samples.yaml schema (audio.py --out-samples),
// so that file imports directly:
//
//   samples:
//   - id: 1            # 1-based position, informational
//     delay: 1         # raw playback-rate byte (AudioData::PcmSample::rate)
//     bank: 0x00
//     length: 2386
//     start_addr: 0x00
//
// plus optional `reserved`/`reserved2` fields (the table's unused +1/+3 bytes), emitted only when
// non-zero so vanilla exports stay schema-identical to the Python tool's output. Import replaces
// the whole table with the file's list.
void EmitPcmSampleTableYaml(YAML::Emitter& out, const std::vector<Landstalker::AudioData::PcmSample>& table);
std::vector<Landstalker::AudioData::PcmSample> PcmSampleTableFromYaml(const YAML::Node& root);

// The bank mapping is slot id -> pool entry name (the names shown in the Music/SFX tree items):
//
//   music_slots:
//     0x00: Track 00h
//   sfx_slots:
//     0x41: SFX 41h
//
// Import applies only the slots present (either section may be omitted) and resolves each name
// against the current pools - an unknown name is an error naming the slot. Note pool entry names
// are editor-session names (they reset to generated defaults on every load), so a mapping file is
// meant for use within the same session, or after renaming tracks to stable names.
void EmitBankMappingYaml(YAML::Emitter& out, const Landstalker::MusicData& md);
void ApplyBankMappingFromYaml(const YAML::Node& root, const Landstalker::MusicData& md,
    std::vector<std::size_t>& music_slot_map, std::vector<std::size_t>& sfx_slot_map);

#endif // _AUDIO_TABLES_YAML_H_
