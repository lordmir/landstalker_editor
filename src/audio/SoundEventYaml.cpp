#include <audio/SoundEventYaml.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <stdexcept>

using Landstalker::MusicData;

namespace
{
	using SoundEvent = MusicData::SoundEvent;
	using EventStream = MusicData::EventStream;

	std::string NoteNameStd(uint8_t pitch)
	{
		return std::string(MUSIC_NOTE_NAMES[pitch % 12]) + std::to_string(pitch / 12);
	}

	// Parses "C4", "C#4", etc. back to a 0-0x6F pitch byte; throws on anything else.
	uint8_t ParseNoteName(const std::string& text)
	{
		std::size_t i = 0;
		while (i < text.size() && (std::isalpha(static_cast<unsigned char>(text[i])) || text[i] == '#'))
		{
			++i;
		}
		const std::string name = text.substr(0, i);
		const std::string octave_str = text.substr(i);
		if (octave_str.empty())
		{
			throw std::runtime_error("Invalid note name '" + text + "' (expected e.g. 'C4', 'F#3')");
		}
		int semitone = -1;
		for (int n = 0; n < 12; ++n)
		{
			if (name == MUSIC_NOTE_NAMES[n])
			{
				semitone = n;
				break;
			}
		}
		if (semitone < 0)
		{
			throw std::runtime_error("Invalid note name '" + text + "' (expected e.g. 'C4', 'F#3')");
		}
		int octave = 0;
		try
		{
			octave = std::stoi(octave_str);
		}
		catch (const std::exception&)
		{
			throw std::runtime_error("Invalid note name '" + text + "' (expected e.g. 'C4', 'F#3')");
		}
		return static_cast<uint8_t>(std::clamp(octave * 12 + semitone, 0, 0x6F));
	}

	// Channel key names, in channel-index order, matching the driver's fixed 10-channel layout
	// (music tracks, and full-type SFX which uses the same order).
	constexpr std::array<const char*, MusicData::MUSIC_CHANNEL_COUNT> TEN_CHANNEL_KEYS = {
		"fm1", "fm2", "fm3", "fm4", "fm5", "dac", "psg1", "psg2", "psg3", "psg_noise"
	};
	constexpr std::array<const char*, MusicData::SFX_OVERLAY_CHANNEL_COUNT> OVERLAY_CHANNEL_KEYS = {
		"fm4", "fm5", "fm6"
	};

	std::string CmdName(SoundEventInsertKind kind)
	{
		switch (kind)
		{
		case SoundEventInsertKind::Instrument:        return "instrument";
		case SoundEventInsertKind::Volume:            return "volume";
		case SoundEventInsertKind::KeyOff:            return "key_off";
		case SoundEventInsertKind::Vibrato:           return "vibrato";
		case SoundEventInsertKind::Pan:               return "pan";
		case SoundEventInsertKind::Transpose:         return "transpose";
		case SoundEventInsertKind::LoopSetMarkerA:    return "set_marker_a";
		case SoundEventInsertKind::LoopSetMarkerB:    return "set_marker_b";
		case SoundEventInsertKind::LoopPlayOnceA:     return "play_once_a";
		case SoundEventInsertKind::LoopPlayOnceB:     return "play_once_b";
		case SoundEventInsertKind::LoopMarker:        return "marker";
		case SoundEventInsertKind::LoopJumpToMarkerA: return "jump_to_marker_a";
		case SoundEventInsertKind::LoopJumpToMarkerB: return "jump_to_marker_b";
		case SoundEventInsertKind::LoopBegin:         return "begin_loop";
		case SoundEventInsertKind::LoopEnd:           return "end_loop";
		case SoundEventInsertKind::End:               return "end";
		case SoundEventInsertKind::Chain:             return "chain";
		case SoundEventInsertKind::JumpAddress:       return "jump";
		case SoundEventInsertKind::Note:
		case SoundEventInsertKind::Rest:
			break;
		}
		return "end";
	}

	SoundEventInsertKind InsertKindFromCmdName(const std::string& name)
	{
		static const std::array<std::pair<const char*, SoundEventInsertKind>, 17> MAP = {{
			{ "instrument", SoundEventInsertKind::Instrument },
			{ "volume", SoundEventInsertKind::Volume },
			{ "key_off", SoundEventInsertKind::KeyOff },
			{ "vibrato", SoundEventInsertKind::Vibrato },
			{ "pan", SoundEventInsertKind::Pan },
			{ "transpose", SoundEventInsertKind::Transpose },
			{ "set_marker_a", SoundEventInsertKind::LoopSetMarkerA },
			{ "set_marker_b", SoundEventInsertKind::LoopSetMarkerB },
			{ "play_once_a", SoundEventInsertKind::LoopPlayOnceA },
			{ "play_once_b", SoundEventInsertKind::LoopPlayOnceB },
			{ "marker", SoundEventInsertKind::LoopMarker },
			{ "jump_to_marker_a", SoundEventInsertKind::LoopJumpToMarkerA },
			{ "jump_to_marker_b", SoundEventInsertKind::LoopJumpToMarkerB },
			{ "begin_loop", SoundEventInsertKind::LoopBegin },
			{ "end_loop", SoundEventInsertKind::LoopEnd },
			{ "end", SoundEventInsertKind::End },
			{ "chain", SoundEventInsertKind::Chain },
		}};
		for (const auto& entry : MAP)
		{
			if (name == entry.first)
			{
				return entry.second;
			}
		}
		if (name == "jump")
		{
			return SoundEventInsertKind::JumpAddress;
		}
		throw std::runtime_error("Unknown event command '" + name + "'");
	}

	int ReadInt(const YAML::Node& node, const std::string& field, int def, const std::string& context)
	{
		if (!node[field].IsDefined())
		{
			return def;
		}
		try
		{
			return node[field].as<int>();
		}
		catch (const YAML::Exception& e)
		{
			throw std::runtime_error("Invalid " + context + "." + field + ": " + e.what());
		}
	}

	bool ReadBool(const YAML::Node& node, const std::string& field, bool def, const std::string& context)
	{
		if (!node[field].IsDefined())
		{
			return def;
		}
		try
		{
			return node[field].as<bool>();
		}
		catch (const YAML::Exception& e)
		{
			throw std::runtime_error("Invalid " + context + "." + field + ": " + e.what());
		}
	}

	std::string ReadString(const YAML::Node& node, const std::string& field, const std::string& def,
		const std::string& context)
	{
		if (!node[field].IsDefined())
		{
			return def;
		}
		try
		{
			return node[field].as<std::string>();
		}
		catch (const YAML::Exception& e)
		{
			throw std::runtime_error("Invalid " + context + "." + field + ": " + e.what());
		}
	}

	void EmitSoundEvent(YAML::Emitter& out, SoundEventChannelKind kind, const SoundEvent& ev)
	{
		out << YAML::BeginMap;
		if (!ev.is_command)
		{
			if (ev.value == 0x70)
			{
				out << YAML::Key << "rest" << YAML::Value << true;
			}
			else
			{
				switch (kind)
				{
				case SoundEventChannelKind::DAC:
					out << YAML::Key << "sample" << YAML::Value << (static_cast<int>(ev.value) + 1);
					break;
				case SoundEventChannelKind::PSG_NOISE:
					out << YAML::Key << "noise" << YAML::Value << (ev.value & 0x07);
					break;
				case SoundEventChannelKind::FM:
				case SoundEventChannelKind::PSG_TONE:
				default:
					out << YAML::Key << "note" << YAML::Value << NoteNameStd(ev.value);
					break;
				}
			}
			if (ev.has_duration)
			{
				out << YAML::Key << "length" << YAML::Value << static_cast<int>(ev.duration);
			}
			out << YAML::EndMap;
			return;
		}

		const auto insert_kind = ClassifySoundEvent(ev);
		const uint8_t op = ev.operand.empty() ? 0 : ev.operand[0];
		out << YAML::Key << "cmd" << YAML::Value << CmdName(insert_kind);
		switch (insert_kind)
		{
		case SoundEventInsertKind::Instrument:
		case SoundEventInsertKind::Vibrato:
			out << YAML::Key << "id" << YAML::Value << YAML::Hex << static_cast<int>(op);
			break;
		case SoundEventInsertKind::Volume:
			if (kind == SoundEventChannelKind::PSG_TONE || kind == SoundEventChannelKind::PSG_NOISE)
			{
				out << YAML::Key << "envelope" << YAML::Value << ((op >> 4) & 0x0F);
				out << YAML::Key << "volume" << YAML::Value << (op & 0x0F);
			}
			else
			{
				out << YAML::Key << "volume" << YAML::Value << (op & 0x0F);
			}
			break;
		case SoundEventInsertKind::KeyOff:
			if (kind == SoundEventChannelKind::FM && op == 0xFF)
			{
				out << YAML::Key << "portamento_off" << YAML::Value << true;
			}
			else if (kind == SoundEventChannelKind::FM && op >= 0x81)
			{
				out << YAML::Key << "portamento_speed" << YAML::Value << (op & 0x7F);
			}
			else
			{
				out << YAML::Key << "release" << YAML::Value << (op & 0x7F);
				out << YAML::Key << "hold" << YAML::Value << ((op & 0x80) != 0);
			}
			break;
		case SoundEventInsertKind::Pan:
			if (kind == SoundEventChannelKind::FM || kind == SoundEventChannelKind::DAC)
			{
				out << YAML::Key << "left" << YAML::Value << ((op & 0x80) != 0);
				out << YAML::Key << "right" << YAML::Value << ((op & 0x40) != 0);
			}
			else
			{
				out << YAML::Key << "tempo" << YAML::Value << YAML::Hex << static_cast<int>(op);
			}
			break;
		case SoundEventInsertKind::Transpose:
			out << YAML::Key << "raw" << YAML::Value << YAML::Hex << static_cast<int>(op);
			break;
		case SoundEventInsertKind::LoopBegin:
			out << YAML::Key << "count" << YAML::Value << ((op & 0x1F) + 1);
			break;
		case SoundEventInsertKind::Chain:
			out << YAML::Key << "operation" << YAML::Value << static_cast<int>(op);
			break;
		case SoundEventInsertKind::JumpAddress:
		{
			const uint8_t hi = (ev.operand.size() >= 2) ? ev.operand[1] : 0;
			out << YAML::Key << "address" << YAML::Value << YAML::Hex << ((static_cast<int>(hi) << 8) | op);
			break;
		}
		default:
			// The marker/play-once/jump/end-loop commands have no dedicated fields, but their
			// control byte's low bits are driver-visible parameters all the same (e.g. F8h E8h is
			// still "end loop" - the low bits are just unused there). Emit the byte whenever it
			// isn't the canonical default, so a round trip is exact for real data like music09's
			// F8h E8h rather than silently normalising it.
			if (op != (MakeDefaultSoundEvent(insert_kind).operand.empty() ? 0 : MakeDefaultSoundEvent(insert_kind).operand[0]))
			{
				out << YAML::Key << "raw" << YAML::Value << YAML::Hex << static_cast<int>(op);
			}
			break;
		}
		out << YAML::EndMap;
	}

	SoundEvent SoundEventFromYamlNode(SoundEventChannelKind kind, const YAML::Node& node, const std::string& context)
	{
		if (!node.IsMap())
		{
			throw std::runtime_error("Invalid " + context + ": expected a map");
		}
		if (node["rest"].IsDefined())
		{
			SoundEvent ev;
			ev.is_command = false;
			ev.value = 0x70;
			ev.has_duration = node["length"].IsDefined();
			ev.duration = static_cast<uint8_t>(ReadInt(node, "length", 0, context));
			return ev;
		}
		if (node["note"].IsDefined())
		{
			SoundEvent ev;
			ev.is_command = false;
			ev.value = ParseNoteName(ReadString(node, "note", "C0", context));
			ev.has_duration = node["length"].IsDefined();
			ev.duration = static_cast<uint8_t>(ReadInt(node, "length", 0, context));
			return ev;
		}
		if (node["sample"].IsDefined())
		{
			SoundEvent ev;
			ev.is_command = false;
			ev.value = static_cast<uint8_t>(std::clamp(ReadInt(node, "sample", 1, context) - 1, 0, 0x6F));
			ev.has_duration = node["length"].IsDefined();
			ev.duration = static_cast<uint8_t>(ReadInt(node, "length", 0, context));
			return ev;
		}
		if (node["noise"].IsDefined())
		{
			SoundEvent ev;
			ev.is_command = false;
			ev.value = static_cast<uint8_t>(ReadInt(node, "noise", 0, context) & 0x07);
			ev.has_duration = node["length"].IsDefined();
			ev.duration = static_cast<uint8_t>(ReadInt(node, "length", 0, context));
			return ev;
		}
		if (!node["cmd"].IsDefined())
		{
			throw std::runtime_error("Invalid " + context + ": expected one of rest/note/sample/noise/cmd");
		}
		const auto insert_kind = InsertKindFromCmdName(ReadString(node, "cmd", "end", context));
		SoundEvent ev = MakeDefaultSoundEvent(insert_kind);
		switch (insert_kind)
		{
		case SoundEventInsertKind::Instrument:
		case SoundEventInsertKind::Vibrato:
			ev.operand = { static_cast<uint8_t>(ReadInt(node, "id", 0, context)) };
			break;
		case SoundEventInsertKind::Volume:
			if (kind == SoundEventChannelKind::PSG_TONE || kind == SoundEventChannelKind::PSG_NOISE)
			{
				ev.operand = { static_cast<uint8_t>(((ReadInt(node, "envelope", 0, context) & 0x0F) << 4)
					| (ReadInt(node, "volume", 0, context) & 0x0F)) };
			}
			else
			{
				ev.operand = { static_cast<uint8_t>(ReadInt(node, "volume", 0, context) & 0x0F) };
			}
			break;
		case SoundEventInsertKind::KeyOff:
			if (kind == SoundEventChannelKind::FM && ReadBool(node, "portamento_off", false, context))
			{
				ev.operand = { 0xFF };
			}
			else if (kind == SoundEventChannelKind::FM && node["portamento_speed"].IsDefined())
			{
				// Speeds 0 and 127 would encode to 80h (a key-off hold) and FFh (portamento off)
				// respectively - different commands entirely - so the usable range is 1-126.
				ev.operand = { static_cast<uint8_t>(0x80 |
					std::clamp(ReadInt(node, "portamento_speed", 1, context), 1, 126)) };
			}
			else
			{
				ev.operand = { static_cast<uint8_t>((ReadInt(node, "release", 0, context) & 0x7F)
					| (ReadBool(node, "hold", false, context) ? 0x80 : 0)) };
			}
			break;
		case SoundEventInsertKind::Pan:
			if (kind == SoundEventChannelKind::FM || kind == SoundEventChannelKind::DAC)
			{
				ev.operand = { static_cast<uint8_t>((ReadBool(node, "left", false, context) ? 0x80 : 0)
					| (ReadBool(node, "right", false, context) ? 0x40 : 0)) };
			}
			else
			{
				ev.operand = { static_cast<uint8_t>(ReadInt(node, "tempo", 0, context)) };
			}
			break;
		case SoundEventInsertKind::Transpose:
			ev.operand = { static_cast<uint8_t>(ReadInt(node, "raw", 0, context)) };
			break;
		case SoundEventInsertKind::LoopBegin:
			ev.operand = { static_cast<uint8_t>(0xC0 | ((std::clamp(ReadInt(node, "count", 1, context), 1, 32) - 1) & 0x1F)) };
			break;
		case SoundEventInsertKind::Chain:
			ev.operand = { static_cast<uint8_t>(ReadInt(node, "operation", 1, context)), 0x00 };
			break;
		case SoundEventInsertKind::JumpAddress:
		{
			const int addr = ReadInt(node, "address", 0, context);
			if (addr < 0x100 || addr > 0xFFFF)
			{
				// A high byte of zero means end/chain to the driver, not a jump - refuse rather
				// than silently writing an event with a different meaning.
				throw std::runtime_error("Invalid " + context + ".address: must be 0x100-0xFFFF");
			}
			ev.operand = { static_cast<uint8_t>(addr & 0xFF), static_cast<uint8_t>((addr >> 8) & 0xFF) };
			break;
		}
		default:
			// See EmitSoundEvent: a non-default control byte for the field-less F8h commands round
			// trips through "raw".
			if (node["raw"].IsDefined() && ev.value == 0xF8)
			{
				ev.operand = { static_cast<uint8_t>(ReadInt(node, "raw", 0, context)) };
			}
			break;
		}
		return ev;
	}

	void EmitChannel(YAML::Emitter& out, SoundEventChannelKind kind, const EventStream& events)
	{
		out << YAML::BeginSeq;
		for (const auto& ev : events)
		{
			EmitSoundEvent(out, kind, ev);
		}
		out << YAML::EndSeq;
	}

	EventStream ChannelFromYaml(SoundEventChannelKind kind, const YAML::Node& node, const std::string& context)
	{
		EventStream events;
		if (!node.IsDefined())
		{
			return events;
		}
		if (!node.IsSequence())
		{
			throw std::runtime_error(context + " must be a list of events");
		}
		std::size_t i = 0;
		for (const auto& item : node)
		{
			events.push_back(SoundEventFromYamlNode(kind, item, context + "[" + std::to_string(i) + "]"));
			++i;
		}
		return events;
	}
}

void EmitMusicTrackYaml(YAML::Emitter& out, const Landstalker::MusicData::MusicTrackEntry& entry)
{
	out << YAML::BeginMap;
	out << YAML::Key << "name" << YAML::Value << entry.name;
	out << YAML::Key << "tempo" << YAML::Value << static_cast<int>(entry.track.tempo);
	out << YAML::Key << "autofade" << YAML::Value << static_cast<int>(entry.track.autofade_frames);
	out << YAML::Key << "channels" << YAML::Value << YAML::BeginMap;
	for (std::size_t ch = 0; ch < MusicData::MUSIC_CHANNEL_COUNT; ++ch)
	{
		out << YAML::Key << TEN_CHANNEL_KEYS[ch] << YAML::Value;
		EmitChannel(out, MusicChannelKind(ch), entry.track.channels[ch]);
	}
	out << YAML::EndMap;
	out << YAML::EndMap;
}

Landstalker::MusicData::MusicTrackEntry MusicTrackEntryFromYaml(const YAML::Node& root)
{
	if (!root.IsMap())
	{
		throw std::runtime_error("Music track YAML must be a map");
	}
	MusicData::MusicTrackEntry entry;
	entry.name = ReadString(root, "name", "Track", "track");
	entry.track.tempo = static_cast<uint8_t>(ReadInt(root, "tempo", 200, "track"));
	entry.track.autofade_frames = static_cast<uint16_t>(ReadInt(root, "autofade", 0, "track"));
	const YAML::Node channels = root["channels"];
	if (!channels.IsDefined() || !channels.IsMap())
	{
		throw std::runtime_error("Music track YAML must have a 'channels' map");
	}
	for (std::size_t ch = 0; ch < MusicData::MUSIC_CHANNEL_COUNT; ++ch)
	{
		const std::string key = TEN_CHANNEL_KEYS[ch];
		entry.track.channels[ch] = ChannelFromYaml(MusicChannelKind(ch), channels[key], "channels." + key);
		if (entry.track.channels[ch].empty())
		{
			const std::vector<uint8_t> stop = { 0xFF, 0x00, 0x00 };
			entry.track.channels[ch] = MusicData::DecodeEventStream(stop);
		}
	}
	return entry;
}

void EmitSfxEntryYaml(YAML::Emitter& out, const Landstalker::MusicData::SfxPoolEntry& entry)
{
	const bool full = entry.entry.type == 1;
	out << YAML::BeginMap;
	out << YAML::Key << "name" << YAML::Value << entry.name;
	out << YAML::Key << "type" << YAML::Value << static_cast<int>(entry.entry.type);
	out << YAML::Key << "channels" << YAML::Value << YAML::BeginMap;
	for (std::size_t ch = 0; ch < entry.entry.channels.size(); ++ch)
	{
		const std::string key = full ? TEN_CHANNEL_KEYS[ch] : OVERLAY_CHANNEL_KEYS[ch];
		const auto kind = full ? MusicChannelKind(ch) : SfxOverlayChannelKind(ch);
		out << YAML::Key << key << YAML::Value;
		EmitChannel(out, kind, entry.entry.channels[ch]);
	}
	out << YAML::EndMap;
	out << YAML::EndMap;
}

Landstalker::MusicData::SfxPoolEntry SfxPoolEntryFromYaml(const YAML::Node& root)
{
	if (!root.IsMap())
	{
		throw std::runtime_error("SFX YAML must be a map");
	}
	MusicData::SfxPoolEntry entry;
	entry.name = ReadString(root, "name", "SFX", "sfx");
	entry.entry.type = static_cast<uint8_t>(ReadInt(root, "type", 2, "sfx"));
	const bool full = entry.entry.type == 1;
	const std::size_t count = full ? MusicData::SFX_FULL_CHANNEL_COUNT : MusicData::SFX_OVERLAY_CHANNEL_COUNT;
	const YAML::Node channels = root["channels"];
	if (!channels.IsDefined() || !channels.IsMap())
	{
		throw std::runtime_error("SFX YAML must have a 'channels' map");
	}
	entry.entry.channels.resize(count);
	for (std::size_t ch = 0; ch < count; ++ch)
	{
		const std::string key = full ? TEN_CHANNEL_KEYS[ch] : OVERLAY_CHANNEL_KEYS[ch];
		const auto kind = full ? MusicChannelKind(ch) : SfxOverlayChannelKind(ch);
		entry.entry.channels[ch] = ChannelFromYaml(kind, channels[key], "channels." + key);
		if (entry.entry.channels[ch].empty())
		{
			const std::vector<uint8_t> stop = { 0xFF, 0x00, 0x00 };
			entry.entry.channels[ch] = MusicData::DecodeEventStream(stop);
		}
	}
	return entry;
}
