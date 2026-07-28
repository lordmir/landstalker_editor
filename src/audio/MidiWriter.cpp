#include <audio/MidiWriter.h>

#include <algorithm>

namespace
{
	void PushVLQ(std::vector<uint8_t>& out, uint32_t value)
	{
		uint8_t buf[5];
		int n = 0;
		buf[n++] = static_cast<uint8_t>(value & 0x7F);
		value >>= 7;
		while (value != 0)
		{
			buf[n++] = static_cast<uint8_t>(0x80 | (value & 0x7F));
			value >>= 7;
		}
		for (int i = n - 1; i >= 0; --i)
		{
			out.push_back(buf[i]);
		}
	}

	void PushU32(std::vector<uint8_t>& out, uint32_t v)
	{
		out.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
		out.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
		out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
		out.push_back(static_cast<uint8_t>(v & 0xFF));
	}

	void PushU16(std::vector<uint8_t>& out, uint16_t v)
	{
		out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
		out.push_back(static_cast<uint8_t>(v & 0xFF));
	}
}

MidiWriter::MidiWriter(uint16_t ticks_per_quarter_note)
	: m_ticks_per_quarter_note(ticks_per_quarter_note)
{
}

std::size_t MidiWriter::AddTrack(const std::string& name)
{
	m_tracks.push_back(Track{ name, {} });
	return m_tracks.size() - 1;
}

void MidiWriter::AddEvent(std::size_t track, uint32_t tick, std::vector<uint8_t> bytes)
{
	m_tracks[track].events.push_back(Event{ tick, std::move(bytes) });
}

void MidiWriter::AddNoteOn(std::size_t track, uint32_t tick, uint8_t channel, uint8_t note, uint8_t velocity)
{
	AddEvent(track, tick, {
		static_cast<uint8_t>(0x90 | (channel & 0x0F)),
		static_cast<uint8_t>(note & 0x7F),
		static_cast<uint8_t>(velocity & 0x7F) });
}

void MidiWriter::AddNoteOff(std::size_t track, uint32_t tick, uint8_t channel, uint8_t note)
{
	AddEvent(track, tick, {
		static_cast<uint8_t>(0x80 | (channel & 0x0F)),
		static_cast<uint8_t>(note & 0x7F),
		0 });
}

void MidiWriter::AddProgramChange(std::size_t track, uint32_t tick, uint8_t channel, uint8_t program)
{
	AddEvent(track, tick, {
		static_cast<uint8_t>(0xC0 | (channel & 0x0F)),
		static_cast<uint8_t>(program & 0x7F) });
}

void MidiWriter::AddControlChange(std::size_t track, uint32_t tick, uint8_t channel, uint8_t controller, uint8_t value)
{
	AddEvent(track, tick, {
		static_cast<uint8_t>(0xB0 | (channel & 0x0F)),
		static_cast<uint8_t>(controller & 0x7F),
		static_cast<uint8_t>(value & 0x7F) });
}

void MidiWriter::AddTempo(std::size_t track, uint32_t tick, uint32_t us_per_quarter_note)
{
	AddEvent(track, tick, {
		0xFF, 0x51, 0x03,
		static_cast<uint8_t>((us_per_quarter_note >> 16) & 0xFF),
		static_cast<uint8_t>((us_per_quarter_note >> 8) & 0xFF),
		static_cast<uint8_t>(us_per_quarter_note & 0xFF) });
}

std::vector<uint8_t> MidiWriter::Serialize() const
{
	std::vector<uint8_t> out;
	out.insert(out.end(), { 'M', 'T', 'h', 'd' });
	PushU32(out, 6);
	PushU16(out, 1); // format 1: multiple simultaneous tracks, first is not special-cased
	PushU16(out, static_cast<uint16_t>(m_tracks.size()));
	PushU16(out, m_ticks_per_quarter_note);

	for (const auto& track : m_tracks)
	{
		auto events = track.events;
		// stable_sort keeps same-tick events in insertion order (e.g. a note-off stays before the
		// note-on that follows it at the same tick).
		std::stable_sort(events.begin(), events.end(), [](const Event& a, const Event& b)
		{
			return a.tick < b.tick;
		});

		std::vector<uint8_t> body;
		if (!track.name.empty())
		{
			body.push_back(0x00);
			body.push_back(0xFF);
			body.push_back(0x03);
			PushVLQ(body, static_cast<uint32_t>(track.name.size()));
			body.insert(body.end(), track.name.begin(), track.name.end());
		}
		uint32_t last_tick = 0;
		for (const auto& ev : events)
		{
			PushVLQ(body, ev.tick - last_tick);
			body.insert(body.end(), ev.bytes.begin(), ev.bytes.end());
			last_tick = ev.tick;
		}
		body.push_back(0x00);
		body.push_back(0xFF);
		body.push_back(0x2F);
		body.push_back(0x00);

		out.insert(out.end(), { 'M', 'T', 'r', 'k' });
		PushU32(out, static_cast<uint32_t>(body.size()));
		out.insert(out.end(), body.begin(), body.end());
	}
	return out;
}
