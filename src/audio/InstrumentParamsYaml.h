#ifndef _INSTRUMENT_PARAMS_YAML_H_
#define _INSTRUMENT_PARAMS_YAML_H_

#include <yaml-cpp/yaml.h>

#include <landstalker/main/MusicData.h>

// YAML (de)serialisation for the driver's instrument parameter tables. Two top-level keys:
//
//   psg_instruments:                     # audio.py's instruments.yaml schema, so that file
//     t_PSG_INSTRUMENT_0:                # imports directly (its ym_instruments section belongs
//       attack: [15]                     # to the YM Instruments editor and is ignored here)
//       release: [11]
//   instrument_params:                   # everything instruments.yaml doesn't cover
//     ym_levels: [0x70, ...]             # 16 entries
//     slots_per_algo: [0x8, ...]         # 8
//     ym_frequencies: [0xA8A, ...]       # 84
//     psg_frequencies: [0x3EF, ...]      # 64
//     pitch_effects:
//       t_PITCH_EFFECT_1: [-16, 16, 16, -16, loop]   # signed deltas; "loop"/"hold" = 80h/81h
//
// An envelope is normally emitted attack/release style (amplitudes, with the last entry of each
// phase carrying the bit-7 marker implicitly); one whose bytes don't reconstruct exactly from
// that form (e.g. trailing padding after the final marker) is emitted as `raw: [bytes]` instead,
// keeping every export lossless. Import accepts either form.
void EmitInstrumentParamsYaml(YAML::Emitter& out, const Landstalker::MusicData::InstrumentParams& params);

// Applies whatever of the two top-level keys (and `instrument_params` sub-keys) are present -
// absent sections leave the corresponding tables untouched, so a trimmed file imports cleanly.
// Throws std::runtime_error naming the offending table/field on malformed input.
void ApplyInstrumentParamsFromYaml(const YAML::Node& root, Landstalker::MusicData::InstrumentParams& params);

#endif // _INSTRUMENT_PARAMS_YAML_H_
