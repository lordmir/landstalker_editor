#ifndef _YM_INSTRUMENT_YAML_H_
#define _YM_INSTRUMENT_YAML_H_

#include <yaml-cpp/yaml.h>

#include <landstalker/main/MusicData.h>

// YAML (de)serialisation for the whole YM instrument table, using the same schema as
// landstalker_tools_python's instrument extractor (audio.py, instruments.yaml):
//
//   ym_instruments:
//     YM_INSTMT_00:
//       feedback_algo: 0x3D
//       operators:
//         op1: {det_mult: 0x20, total_level: 0x1B, ks_ar: 0x5F, am_dr: 0x03, sr: 0x00,
//               sl_rr: 0x5A, ssg_eg: 0x00}
//         op3: {...}   # storage order S1, S3, S2, S4 - op3 comes second, as in the Python tool
//         op2: {...}
//         op4: {...}
//
// so a file exported here can be diffed against (or replaced by) the Python tool's output, and an
// instruments.yaml produced by that tool imports directly (its psg_instruments section, and any
// other unknown keys, are ignored).
void EmitYmInstrumentsYaml(YAML::Emitter& out, const Landstalker::MusicData::YmInstrumentTable& table);

// Applies the instruments in `root` over `table` - only the YM_INSTMT_NN entries present in the
// file are replaced, so a hand-trimmed file holding a few patches imports cleanly. Throws
// std::runtime_error (with the offending instrument/field named) on malformed input.
void ApplyYmInstrumentsFromYaml(const YAML::Node& root, Landstalker::MusicData::YmInstrumentTable& table);

#endif // _YM_INSTRUMENT_YAML_H_
