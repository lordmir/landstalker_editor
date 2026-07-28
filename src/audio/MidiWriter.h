#ifndef _MIDI_WRITER_H_
#define _MIDI_WRITER_H_

#include <cstdint>
#include <string>
#include <vector>

// A minimal Standard MIDI File (format 1) writer - just enough event types for
// SoundEventMidi's export (note on/off, program change, two control changes, tempo, track
// name). Hand-rolled rather than pulling in a MIDI library: the SMF format is a short,
// well-documented binary structure (one header chunk plus one MTrk chunk per track, each a
// stream of <delta-time, event bytes> pairs) and we only ever need to write, never parse.
class MidiWriter
{
public:
    explicit MidiWriter(uint16_t ticks_per_quarter_note = 480);

    // Returns the new track's index, for use with the Add* calls below.
    std::size_t AddTrack(const std::string& name);

    void AddNoteOn(std::size_t track, uint32_t tick, uint8_t channel, uint8_t note, uint8_t velocity);
    void AddNoteOff(std::size_t track, uint32_t tick, uint8_t channel, uint8_t note);
    void AddProgramChange(std::size_t track, uint32_t tick, uint8_t channel, uint8_t program);
    void AddControlChange(std::size_t track, uint32_t tick, uint8_t channel, uint8_t controller, uint8_t value);
    // us_per_quarter_note: MIDI's tempo meta-event unit directly - see MusicData::GetTempoHz
    // for how a driver tempo byte's frame rate converts to this.
    void AddTempo(std::size_t track, uint32_t tick, uint32_t us_per_quarter_note);

    // The whole file (header chunk + every track chunk), ready to write to disk as-is.
    std::vector<uint8_t> Serialize() const;

private:
    struct Event
    {
        uint32_t tick;
        std::size_t seq; // insertion order - keeps same-tick events in a deterministic order
                          // (e.g. a note-off always sorts before the note-on that follows it)
        std::vector<uint8_t> bytes;
    };
    struct Track
    {
        std::string name;
        std::vector<Event> events;
    };

    void AddEvent(std::size_t track, uint32_t tick, std::vector<uint8_t> bytes);

    uint16_t m_ticks_per_quarter_note;
    std::vector<Track> m_tracks;
    std::size_t m_next_seq = 0;
};

#endif // _MIDI_WRITER_H_
