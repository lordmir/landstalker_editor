#ifndef _SOUND_EVENT_FORMAT_H_
#define _SOUND_EVENT_FORMAT_H_

#include <cstddef>
#include <vector>

#include <wx/string.h>

#include <landstalker/main/MusicData.h>

// Human-readable formatting for MusicData::SoundEvent, per docs/sound_driver_format.md section 5.
// A command byte's meaning (and whether a channel understands it at all) depends on the channel's
// type, hence ChannelKind - see the format doc's per-channel command table.
enum class SoundEventChannelKind
{
    FM,
    DAC,
    PSG_TONE,
    PSG_NOISE
};

// The twelve semitone names, C first - a pitch byte is octave * 12 + semitone (see
// docs/sound_driver_format.md section 5.1). Shared by every place a pitch is shown or parsed.
extern const char* const MUSIC_NOTE_NAMES[12];

// The channel kind for one of MusicData's ten fixed music-track channel slots (FM1-5, DAC, PSG1-3,
// PSG noise, in that order) or a full-type (10-channel) SFX entry, which uses the same order.
SoundEventChannelKind MusicChannelKind(std::size_t channel_index);

// The channel kind for one of an overlay-type (3-channel) SFX entry's channels - FM4, FM5, FM6.
SoundEventChannelKind SfxOverlayChannelKind(std::size_t channel_index);

// A short description of one event - a note/rest name and duration, or a command's name and
// operand, interpreted for the given channel kind (e.g. FDh is "Volume" on an FM channel but
// "Envelope/Volume" on a PSG channel). Commands a channel doesn't actually understand (skipped as
// a 2-byte no-op by the real driver) are still shown, labelled generically.
wxString DescribeSoundEvent(SoundEventChannelKind kind, const Landstalker::MusicData::SoundEvent& event);

// The insertable/editable "shape" of an event - what parameters it has, independent of channel
// kind (channel kind only affects how those parameters are labelled/laid out in the UI). Loop and
// end/jump each cover several F8h/FFh control-byte sub-ranges - see
// docs/sound_driver_format.md sections 5.3/5.4.
enum class SoundEventInsertKind
{
    Note,
    Rest,
    Instrument,       // FEh
    Volume,           // FDh
    KeyOff,           // FCh
    Vibrato,          // FBh
    Pan,              // FAh
    Transpose,        // F9h
    LoopSetMarkerA,   // F8h 00h-1Fh
    LoopSetMarkerB,   // F8h 20h-3Fh
    LoopPlayOnceA,    // F8h 40h-5Fh
    LoopPlayOnceB,    // F8h 60h-7Fh
    LoopMarker,       // F8h 80h-9Fh
    LoopJumpToMarkerB,// F8h A0h
    LoopJumpToMarkerA,// F8h A1h
    LoopBegin,        // F8h C0h-DFh
    LoopEnd,          // F8h E0h-FFh
    End,              // FFh 00 00
    Chain,            // FFh 00 LL
    JumpAddress,      // FFh lo hi (hi != 0)
};

// A short label for an insert-menu entry, e.g. "Note", "Begin Loop".
wxString SoundEventInsertKindLabel(SoundEventInsertKind kind);
// The non-loop, non-end/jump items, in menu order.
std::vector<SoundEventInsertKind> TopLevelInsertKinds();
// The F8h loop sub-commands, in menu order (shown as a "Loop" submenu).
std::vector<SoundEventInsertKind> LoopInsertKinds();
// The FFh end/chain/jump variants, in menu order (shown as an "End / Jump" submenu).
std::vector<SoundEventInsertKind> EndInsertKinds();

// A sensible default event for a freshly-inserted item of this kind - e.g. Rest with no explicit
// duration, or "Begin Loop" repeating once (count 1). Edit the result via SoundEventEditDialog.
Landstalker::MusicData::SoundEvent MakeDefaultSoundEvent(SoundEventInsertKind kind);

// Which insert-kind an existing event corresponds to - the inverse of MakeDefaultSoundEvent, used
// to decide which fields SoundEventEditDialog shows when editing (an event's fundamental kind
// isn't itself editable - delete and re-insert with the wanted kind instead).
SoundEventInsertKind ClassifySoundEvent(const Landstalker::MusicData::SoundEvent& event);

#endif // _SOUND_EVENT_FORMAT_H_
