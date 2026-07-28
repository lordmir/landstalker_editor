#include <audio/SoundEventMidi.h>

#include <algorithm>
#include <optional>
#include <string>
#include <unordered_map>

#include <audio/MidiWriter.h>
#include <audio/SoundEventFormat.h>

namespace
{
	using Landstalker::MusicData;
	using SoundEvent = MusicData::SoundEvent;
	using EventStream = MusicData::EventStream;

	// One driver frame (a note/rest duration byte's unit) = this many MIDI ticks, at the 480
	// ticks-per-quarter-note resolution used below - matches midi_export.py's ticks_per_frame, so
	// a track's note lengths land on the same tick grid as the Python tool's output.
	constexpr uint32_t TICKS_PER_FRAME = 16;
	constexpr uint16_t TICKS_PER_QUARTER_NOTE = 480;

	// FM1-5 -> MIDI channels 0-4, DAC -> 9, PSG1-3 -> 5-7, PSG noise -> 8; overlay SFX channels
	// (FM4/FM5/FM6) -> 3-5, preserving which physical FM channel each one really is. Chosen to
	// match midi_export.py's own channel assignment, so a file exported here and one exported by
	// the Python tool for the same data land on the same MIDI channels.
	uint8_t MidiChannelForColumn(std::size_t column_index, bool overlay)
	{
		if (overlay)
		{
			return static_cast<uint8_t>(3 + column_index);
		}
		if (column_index <= 4) return static_cast<uint8_t>(column_index);
		if (column_index == 5) return 9;
		if (column_index <= 8) return static_cast<uint8_t>(5 + (column_index - 6));
		return 8;
	}

	std::string ChannelLabel(std::size_t column_index, bool overlay)
	{
		if (overlay)
		{
			static constexpr const char* labels[MusicData::SFX_OVERLAY_CHANNEL_COUNT] = {
				"FM4 (overlay)", "FM5 (overlay)", "FM6 (overlay)"
			};
			return labels[column_index];
		}
		static constexpr const char* labels[MusicData::MUSIC_CHANNEL_COUNT] = {
			"FM1", "FM2", "FM3", "FM4", "FM5", "DAC", "PSG1", "PSG2", "PSG3", "PSG Noise"
		};
		return labels[column_index];
	}

	int8_t SignExtendNibble(uint8_t v)
	{
		v &= 0x0F;
		return (v & 0x08) ? static_cast<int8_t>(v - 16) : static_cast<int8_t>(v);
	}

	// Position of the next F8h event in [from, events.size()) whose control byte's top 3 bits
	// equal `sub` (e.g. 0x60 for "F8 60h-7Fh") - used by the play-once sections (40h-5Fh/60h-7Fh)
	// to skip forward past an already-played intro on a repeat pass. events.size() if none.
	std::size_t FindNextF8(const EventStream& events, std::size_t from, uint8_t sub)
	{
		for (std::size_t i = from; i < events.size(); ++i)
		{
			const auto& ev = events[i];
			if (ev.is_command && ev.value == 0xF8)
			{
				const uint8_t op = ev.operand.empty() ? 0 : ev.operand[0];
				if ((op & 0xE0) == sub)
				{
					return i;
				}
			}
		}
		return events.size();
	}

	// Mirrors the driver's own per-channel loop state (docs/sound_driver_format.md section 5.4):
	// two markers, one counted-loop return point, two one-shot flags. Only one counted loop is
	// tracked at a time, same as the real driver (it isn't a stack - nesting isn't supported).
	struct ChannelSimState
	{
		std::optional<std::size_t> marker_a;
		std::optional<std::size_t> marker_b;
		bool playonce_a_done = false;
		bool playonce_b_done = false;
		std::optional<std::size_t> loop_return_pos;
		int loop_remaining = 0;
		int8_t transpose = 0;
		uint8_t default_duration = 1;
	};

	void SimulateChannel(const EventStream& events, SoundEventChannelKind kind, std::size_t midi_track,
		uint8_t midi_channel, int max_loop_passes, MidiWriter& writer)
	{
		if (events.empty())
		{
			return;
		}
		ChannelSimState st;
		std::size_t pos = 0;
		uint32_t tick = 0;
		bool note_sounding = false;
		uint8_t sounding_note = 0;
		// Counts revisits of each F8h jump-to-marker instruction (keyed by its own position) so a
		// genuine infinite loop (the driver's documented idiom - see the file header comment)
		// still produces a finite MIDI file, unrolled max_loop_passes times.
		std::unordered_map<std::size_t, int> jump_visits;
		// Pure defensive backstop against pathological/hand-edited data that could loop forever
		// despite the pass-count limit above (e.g. two jumps bouncing between each other without
		// either one individually being revisited enough times to trip that limit).
		const std::size_t step_cap = (events.size() + 1) * static_cast<std::size_t>(max_loop_passes + 4) * 4;
		std::size_t steps = 0;

		const auto note_off = [&](uint32_t t)
		{
			if (note_sounding)
			{
				writer.AddNoteOff(midi_track, t, midi_channel, sounding_note);
				note_sounding = false;
			}
		};

		while (pos < events.size())
		{
			if (++steps > step_cap)
			{
				break;
			}
			const SoundEvent& ev = events[pos];
			if (!ev.is_command)
			{
				uint8_t duration = st.default_duration;
				if (ev.has_duration)
				{
					duration = ev.duration;
					st.default_duration = ev.duration;
				}
				if (ev.value == 0x70) // rest
				{
					note_off(tick);
				}
				else
				{
					note_off(tick);
					int note;
					switch (kind)
					{
					case SoundEventChannelKind::DAC:
						// Matches midi_export.py: an arbitrary 34 + sample id, where the sample id
						// is the stored pitch byte + 1 (sample value+1 is what the driver triggers).
						note = 34 + (ev.value + 1);
						break;
					case SoundEventChannelKind::PSG_NOISE:
						note = 60 + (ev.value & 0x07); // noise mode, not a real pitch
						break;
					case SoundEventChannelKind::PSG_TONE:
					case SoundEventChannelKind::FM:
					default:
						// Landstalker's pitch byte is octave*12+semitone starting at "C0" - MIDI
						// note numbers are the same scheme starting one octave lower, so +12.
						note = ev.value + 12 + st.transpose;
						break;
					}
					note = std::clamp(note, 0, 127);
					writer.AddNoteOn(midi_track, tick, midi_channel, static_cast<uint8_t>(note), 100);
					sounding_note = static_cast<uint8_t>(note);
					note_sounding = true;
				}
				tick += static_cast<uint32_t>(duration) * TICKS_PER_FRAME;
				++pos;
				continue;
			}

			const uint8_t op = ev.operand.empty() ? 0 : ev.operand[0];
			switch (ev.value)
			{
			case 0xFE: // set instrument - FM only
				if (kind == SoundEventChannelKind::FM)
				{
					// Landstalker's instrument id isn't a GM program number - there's no built-in
					// mapping between the two (midi_export.py only maps one via an optional,
					// user-supplied --inst-map file), so this matches that script's default:
					// always program 0 (Grand Piano) absent a real mapping, rather than
					// misinterpreting the raw id as a GM program choice.
					writer.AddProgramChange(midi_track, tick, midi_channel, 0);
				}
				++pos;
				break;
			case 0xFD: // set volume (FM) / envelope+volume (PSG) - not understood by DAC
				if (kind != SoundEventChannelKind::DAC)
				{
					if (kind == SoundEventChannelKind::PSG_TONE || kind == SoundEventChannelKind::PSG_NOISE)
					{
						// High nibble selects the PSG envelope (this channel type's equivalent of
						// an instrument) - midi_export.py treats it as its own instrument event,
						// separate from the low-nibble volume below, defaulting to program 0 for
						// the same reason as the FM case above.
						writer.AddProgramChange(midi_track, tick, midi_channel, 0);
					}
					// Both FM and PSG document 0h (loudest) .. Fh (quietest) for this nibble -
					// invert it, since MIDI CC7 is the opposite way round (127 = loudest).
					const uint8_t nibble = op & 0x0F;
					const uint8_t cc = static_cast<uint8_t>(127 - (nibble * 127 / 15));
					writer.AddControlChange(midi_track, tick, midi_channel, 7, cc);
				}
				++pos;
				break;
			case 0xFC: // key-off / slide - understood by every channel type
				note_off(tick);
				++pos;
				break;
			case 0xFB: // vibrato - no MIDI equivalent emitted, see file header comment
				++pos;
				break;
			case 0xFA: // pan (FM/DAC) - PSG tone's mid-track tempo change is unsupported (see header)
				if (kind == SoundEventChannelKind::FM || kind == SoundEventChannelKind::DAC)
				{
					const uint8_t pan = (op & 0x80) ? 127 : ((op & 0x40) ? 0 : 64);
					writer.AddControlChange(midi_track, tick, midi_channel, 10, pan);
				}
				++pos;
				break;
			case 0xF9: // transpose - FM/PSG tone only
				if (kind == SoundEventChannelKind::FM || kind == SoundEventChannelKind::PSG_TONE)
				{
					st.transpose = SignExtendNibble(op);
				}
				++pos;
				break;
			case 0xF8: // loop command - understood by every channel type
			{
				const uint8_t sub = op & 0xE0;
				const uint8_t n = op & 0x1F;
				// Markers record the position AFTER the marker event, matching the real driver
				// (which saves its parse pointer after consuming the command). Landing ON the
				// marker instead would re-execute it on every loop pass - harmless for marker A,
				// but marker B's side effect of clearing the one-shot flags would then wrongly
				// re-arm play-once sections, replaying first-time-only intros on every pass.
				if (sub == 0x00) // set marker A
				{
					st.marker_a = pos + 1;
					++pos;
				}
				else if (sub == 0x20) // set marker B (also clears both one-shot flags)
				{
					st.marker_b = pos + 1;
					st.playonce_a_done = false;
					st.playonce_b_done = false;
					++pos;
				}
				else if (sub == 0x40) // play-once section A
				{
					if (st.playonce_a_done) pos = FindNextF8(events, pos + 1, 0x60);
					else { st.playonce_a_done = true; ++pos; }
				}
				else if (sub == 0x60) // play-once section B
				{
					if (st.playonce_b_done) pos = FindNextF8(events, pos + 1, 0x80);
					else { st.playonce_b_done = true; ++pos; }
				}
				else if (sub == 0x80) // no-op marker
				{
					++pos;
				}
				else if (sub == 0xA0) // jump to marker A/B
				{
					const bool to_a = (op & 1) != 0;
					const auto target = to_a ? st.marker_a : st.marker_b;
					if (target)
					{
						int& visits = jump_visits[pos];
						if (visits < max_loop_passes)
						{
							++visits;
							pos = *target;
						}
						else
						{
							// Unrolled enough times - end the channel here (as midi_export.py's
							// simulator does), rather than falling through the jump into bytes the
							// driver would only ever reach via a play-once skip.
							pos = events.size();
						}
					}
					else
					{
						++pos; // jump to a marker that was never set - no-op
					}
				}
				else if (sub == 0xC0) // begin counted loop
				{
					st.loop_return_pos = pos + 1;
					st.loop_remaining = n + 1;
					++pos;
				}
				else // 0xE0 - end counted loop
				{
					if (st.loop_remaining > 0)
					{
						--st.loop_remaining;
					}
					if (st.loop_remaining > 0 && st.loop_return_pos)
					{
						pos = *st.loop_return_pos;
					}
					else
					{
						++pos;
					}
				}
				break;
			}
			case 0xFF:
			default:
				// End, chain, or jump-by-address - all end this channel's renderable content here
				// (see file header comment for why a jump address can't be resolved).
				pos = events.size();
				break;
			}
		}
		note_off(tick);
	}

	std::vector<uint8_t> ExportChannels(const std::vector<EventStream>& channels, bool overlay,
		double tempo_hz, int max_loop_passes)
	{
		MidiWriter writer(TICKS_PER_QUARTER_NOTE);
		for (std::size_t ch = 0; ch < channels.size(); ++ch)
		{
			writer.AddTrack(ChannelLabel(ch, overlay));
		}
		// ticks_per_beat / ticks_per_frame = 30 frames per quarter note, so a frame's real-world
		// duration (1e6/tempo_hz microseconds) scales up by that to get MIDI's microseconds-per-
		// quarter-note tempo unit.
		const double us_per_frame = 1000000.0 / std::max(tempo_hz, 1.0);
		writer.AddTempo(0, 0, static_cast<uint32_t>(30.0 * us_per_frame));
		for (std::size_t ch = 0; ch < channels.size(); ++ch)
		{
			const auto kind = overlay ? SfxOverlayChannelKind(ch) : MusicChannelKind(ch);
			const uint8_t midi_channel = MidiChannelForColumn(ch, overlay);
			SimulateChannel(channels[ch], kind, ch, midi_channel, max_loop_passes, writer);
		}
		return writer.Serialize();
	}
}

std::vector<uint8_t> ExportMusicTrackAsMidi(const MusicData::MusicTrackEntry& entry, int max_loop_passes)
{
	std::vector<EventStream> channels(entry.track.channels.begin(), entry.track.channels.end());
	return ExportChannels(channels, false, MusicData::GetTempoHz(entry.track.tempo), max_loop_passes);
}

std::vector<uint8_t> ExportSfxEntryAsMidi(const MusicData::SfxPoolEntry& entry, int max_loop_passes)
{
	const bool overlay = entry.entry.type != 1;
	// SfxEntry has no stored tempo of its own (an SFX plays at whatever the concurrently-active
	// music track's Timer B is already set to) - use the same default midi_export.py falls back
	// to when a header has no tempo byte: Timer B = 0xBA directly (~47.5 Hz). GetTempoHz takes
	// the pre-+3 header byte, hence the subtraction.
	return ExportChannels(entry.entry.channels, overlay, MusicData::GetTempoHz(0xBA - 3), max_loop_passes);
}
