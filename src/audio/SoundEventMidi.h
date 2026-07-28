#ifndef _SOUND_EVENT_MIDI_H_
#define _SOUND_EVENT_MIDI_H_

#include <cstdint>
#include <vector>

#include <landstalker/main/MusicData.h>

// Renders a music track or SFX pool entry as a Standard MIDI File, as a listening/inspection
// aid - mirrors landstalker_tools_python/scripts/midi_export.py's simulate-the-driver approach
// (see that script's AudioEngineSimulator) but works directly from MusicData's already-decoded
// SoundEvent stream rather than re-parsing raw command bytes.
//
// This is lossy by design - several driver features have no MIDI equivalent and are silently
// dropped rather than approximated:
//   - instrument selection (FEh on FM, the envelope nibble of FDh on PSG) always emits Program
//     Change 0 (Grand Piano) - a Landstalker instrument id isn't a GM program number and there's
//     no built-in table between the two, matching midi_export.py's own default (that script only
//     maps real instrument ids to real GM programs via an optional, user-supplied --inst-map
//     file, which this doesn't implement)
//   - vibrato (FBh) and the fine-detune bits of F9h - no per-note pitch-LFO/microtuning in MIDI
//   - mid-track tempo changes (FAh on a PSG tone channel) - only the track's header tempo is
//     emitted, as a single tempo meta-event at the very start (matches midi_export.py, which
//     doesn't handle these either)
//   - the FFh command's "jump to an absolute ROM address" form - MusicData::DecodeEventStream
//     stops at the first FFh byte and never resolves what the address points at (it may be this
//     same channel's own start, another track entirely, or a chain target), so a channel that
//     loops purely via this form just ends its MIDI track where the FFh is. The driver's
//     documented "infinite loop" idiom - F8h markers plus jump-to-marker (see
//     docs/sound_driver_format.md section 5.4 in landstalker_disasm) - is the common case and IS
//     fully handled (unrolled a bounded number of times, since a MIDI file must be finite).
std::vector<uint8_t> ExportMusicTrackAsMidi(const Landstalker::MusicData::MusicTrackEntry& entry, int max_loop_passes = 2);
std::vector<uint8_t> ExportSfxEntryAsMidi(const Landstalker::MusicData::SfxPoolEntry& entry, int max_loop_passes = 2);

#endif // _SOUND_EVENT_MIDI_H_
