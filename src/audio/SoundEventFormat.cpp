#include <audio/SoundEventFormat.h>

const char* const MUSIC_NOTE_NAMES[12] = {
	"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

namespace
{
	using Landstalker::MusicData;

	wxString NoteName(uint8_t pitch)
	{
		return wxString::Format("%s%u", MUSIC_NOTE_NAMES[pitch % 12], pitch / 12);
	}

	// F8h's control byte: the top 3 bits select a loop sub-command, common to every channel type -
	// see docs/sound_driver_format.md section 5.4.
	wxString DescribeLoopControl(uint8_t control)
	{
		const uint8_t op = control & 0xE0;
		const uint8_t n = control & 0x1F;
		switch (op)
		{
		case 0x00: return "Set Marker A";
		case 0x20: return "Set Marker B";
		case 0x40: return "Play Once (A)";
		case 0x60: return "Play Once (B)";
		case 0x80: return "Marker";
		case 0xA0: return (n & 1) ? "Jump to A" : "Jump to B";
		case 0xC0: return wxString::Format("Begin Loop x%u", n + 1);
		case 0xE0: return "End Loop";
		default:   return wxString::Format("Loop %02Xh", control);
		}
	}

	wxString DescribeNoteOrRest(SoundEventChannelKind kind, const MusicData::SoundEvent& ev)
	{
		wxString text;
		if (ev.value == 0x70)
		{
			text = "Rest";
		}
		else
		{
			switch (kind)
			{
			case SoundEventChannelKind::DAC:
				text = wxString::Format("Samp %02Xh", ev.value + 1);
				break;
			case SoundEventChannelKind::PSG_NOISE:
				text = wxString::Format("Noise %Xh", ev.value & 0x07);
				break;
			case SoundEventChannelKind::PSG_TONE:
				// PSG tone uses the same note layout as FM - only the underlying frequency table
				// differs (a driver-internal detail), not the stored pitch byte's meaning.
			case SoundEventChannelKind::FM:
			default:
				text = NoteName(ev.value);
				break;
			}
		}
		if (ev.has_duration)
		{
			text += wxString::Format(" (%u)", ev.duration);
		}
		return text;
	}

	wxString DescribeCommand(SoundEventChannelKind kind, const MusicData::SoundEvent& ev)
	{
		const uint8_t op = ev.operand.empty() ? 0 : ev.operand[0];
		switch (ev.value)
		{
		case 0xFF:
			if (ev.operand.size() >= 2)
			{
				const uint8_t lo = ev.operand[0];
				const uint8_t hi = ev.operand[1];
				if (hi == 0 && lo == 0) return "End";
				if (hi == 0) return wxString::Format("Chain %02Xh", lo);
				return wxString::Format("Jump %04Xh", (static_cast<unsigned>(hi) << 8) | lo);
			}
			return "End";
		case 0xFE:
			if (kind == SoundEventChannelKind::FM) return wxString::Format("Instrument %02Xh", op);
			break;
		case 0xFD:
			if (kind == SoundEventChannelKind::FM) return wxString::Format("Volume %Xh", op & 0x0F);
			if (kind == SoundEventChannelKind::PSG_TONE || kind == SoundEventChannelKind::PSG_NOISE)
				return wxString::Format("Env %Xh Vol %Xh", (op >> 4) & 0x0F, op & 0x0F);
			break;
		case 0xFC:
			// Understood by every channel type.
			if (kind == SoundEventChannelKind::FM)
			{
				if (op == 0xFF) return "Portamento Off";
				if (op >= 0x81) return wxString::Format("Portamento Speed %Xh", op & 0x7F);
			}
			return wxString::Format("Key-off @%u%s", op & 0x7F, (op & 0x80) ? " (hold)" : "");
		case 0xFB:
			if (kind == SoundEventChannelKind::FM || kind == SoundEventChannelKind::PSG_TONE)
				return wxString::Format("Vibr %02Xh", op);
			break;
		case 0xFA:
			if (kind == SoundEventChannelKind::FM || kind == SoundEventChannelKind::DAC)
				return wxString::Format("Pan %02Xh", op);
			if (kind == SoundEventChannelKind::PSG_TONE) return wxString::Format("Tempo %02Xh", op);
			break;
		case 0xF9:
			if (kind == SoundEventChannelKind::FM || kind == SoundEventChannelKind::PSG_TONE)
				return wxString::Format("Trpose %02Xh", op);
			break;
		case 0xF8:
			// Understood by every channel type.
			return DescribeLoopControl(op);
		default:
			break;
		}
		// Not understood by this channel type - the real driver just skips it as a 2-byte no-op.
		return wxString::Format("Cmd %02Xh: %02Xh (unused here)", ev.value, op);
	}
}

SoundEventChannelKind MusicChannelKind(std::size_t channel_index)
{
	if (channel_index <= 4) return SoundEventChannelKind::FM;
	if (channel_index == 5) return SoundEventChannelKind::DAC;
	if (channel_index <= 8) return SoundEventChannelKind::PSG_TONE;
	return SoundEventChannelKind::PSG_NOISE;
}

SoundEventChannelKind SfxOverlayChannelKind(std::size_t /*channel_index*/)
{
	return SoundEventChannelKind::FM; // overlay SFX channels are always FM4/FM5/FM6
}

wxString DescribeSoundEvent(SoundEventChannelKind kind, const Landstalker::MusicData::SoundEvent& event)
{
	return event.is_command ? DescribeCommand(kind, event) : DescribeNoteOrRest(kind, event);
}

wxString SoundEventInsertKindLabel(SoundEventInsertKind kind)
{
	switch (kind)
	{
	case SoundEventInsertKind::Note:              return "Note";
	case SoundEventInsertKind::Rest:              return "Rest";
	case SoundEventInsertKind::Instrument:        return "Set Instr";
	case SoundEventInsertKind::Volume:            return "Set Volume";
	case SoundEventInsertKind::KeyOff:            return "Key-off";
	case SoundEventInsertKind::Vibrato:           return "Set Vibr";
	case SoundEventInsertKind::Pan:               return "Set PanTpo";
	case SoundEventInsertKind::Transpose:         return "Trpose";
	case SoundEventInsertKind::LoopSetMarkerA:    return "Set Marker A";
	case SoundEventInsertKind::LoopSetMarkerB:    return "Set Marker B";
	case SoundEventInsertKind::LoopPlayOnceA:     return "Play Once (A)";
	case SoundEventInsertKind::LoopPlayOnceB:     return "Play Once (B)";
	case SoundEventInsertKind::LoopMarker:        return "Marker (no-op)";
	case SoundEventInsertKind::LoopJumpToMarkerA: return "Jump to A";
	case SoundEventInsertKind::LoopJumpToMarkerB: return "Jump to B";
	case SoundEventInsertKind::LoopBegin:         return "Begin Loop";
	case SoundEventInsertKind::LoopEnd:           return "End Loop";
	case SoundEventInsertKind::End:               return "End";
	case SoundEventInsertKind::Chain:             return "Chain";
	case SoundEventInsertKind::JumpAddress:       return "Jump Addr";
	}
	return "?";
}

std::vector<SoundEventInsertKind> TopLevelInsertKinds()
{
	return {
		SoundEventInsertKind::Note,
		SoundEventInsertKind::Rest,
		SoundEventInsertKind::Instrument,
		SoundEventInsertKind::Volume,
		SoundEventInsertKind::KeyOff,
		SoundEventInsertKind::Vibrato,
		SoundEventInsertKind::Pan,
		SoundEventInsertKind::Transpose,
	};
}

std::vector<SoundEventInsertKind> LoopInsertKinds()
{
	return {
		SoundEventInsertKind::LoopSetMarkerA,
		SoundEventInsertKind::LoopSetMarkerB,
		SoundEventInsertKind::LoopPlayOnceA,
		SoundEventInsertKind::LoopPlayOnceB,
		SoundEventInsertKind::LoopMarker,
		SoundEventInsertKind::LoopJumpToMarkerA,
		SoundEventInsertKind::LoopJumpToMarkerB,
		SoundEventInsertKind::LoopBegin,
		SoundEventInsertKind::LoopEnd,
	};
}

std::vector<SoundEventInsertKind> EndInsertKinds()
{
	return {
		SoundEventInsertKind::End,
		SoundEventInsertKind::Chain,
		SoundEventInsertKind::JumpAddress,
	};
}

Landstalker::MusicData::SoundEvent MakeDefaultSoundEvent(SoundEventInsertKind kind)
{
	using SoundEvent = Landstalker::MusicData::SoundEvent;
	SoundEvent ev;
	switch (kind)
	{
	case SoundEventInsertKind::Note:
		ev.is_command = false;
		ev.value = 0;
		return ev;
	case SoundEventInsertKind::Rest:
		ev.is_command = false;
		ev.value = 0x70;
		return ev;
	case SoundEventInsertKind::Instrument: ev.is_command = true; ev.value = 0xFE; ev.operand = { 0 }; return ev;
	case SoundEventInsertKind::Volume:     ev.is_command = true; ev.value = 0xFD; ev.operand = { 0 }; return ev;
	case SoundEventInsertKind::KeyOff:     ev.is_command = true; ev.value = 0xFC; ev.operand = { 0 }; return ev;
	case SoundEventInsertKind::Vibrato:    ev.is_command = true; ev.value = 0xFB; ev.operand = { 0 }; return ev;
	case SoundEventInsertKind::Pan:        ev.is_command = true; ev.value = 0xFA; ev.operand = { 0 }; return ev;
	case SoundEventInsertKind::Transpose:  ev.is_command = true; ev.value = 0xF9; ev.operand = { 0 }; return ev;
	case SoundEventInsertKind::LoopSetMarkerA:    ev.is_command = true; ev.value = 0xF8; ev.operand = { 0x00 }; return ev;
	case SoundEventInsertKind::LoopSetMarkerB:    ev.is_command = true; ev.value = 0xF8; ev.operand = { 0x20 }; return ev;
	case SoundEventInsertKind::LoopPlayOnceA:     ev.is_command = true; ev.value = 0xF8; ev.operand = { 0x40 }; return ev;
	case SoundEventInsertKind::LoopPlayOnceB:     ev.is_command = true; ev.value = 0xF8; ev.operand = { 0x60 }; return ev;
	case SoundEventInsertKind::LoopMarker:        ev.is_command = true; ev.value = 0xF8; ev.operand = { 0x80 }; return ev;
	case SoundEventInsertKind::LoopJumpToMarkerB: ev.is_command = true; ev.value = 0xF8; ev.operand = { 0xA0 }; return ev;
	case SoundEventInsertKind::LoopJumpToMarkerA: ev.is_command = true; ev.value = 0xF8; ev.operand = { 0xA1 }; return ev;
	case SoundEventInsertKind::LoopBegin:         ev.is_command = true; ev.value = 0xF8; ev.operand = { 0xC0 }; return ev;
	case SoundEventInsertKind::LoopEnd:           ev.is_command = true; ev.value = 0xF8; ev.operand = { 0xE0 }; return ev;
	case SoundEventInsertKind::End:         ev.is_command = true; ev.value = 0xFF; ev.operand = { 0x00, 0x00 }; return ev;
	case SoundEventInsertKind::Chain:       ev.is_command = true; ev.value = 0xFF; ev.operand = { 0x01, 0x00 }; return ev;
	case SoundEventInsertKind::JumpAddress: ev.is_command = true; ev.value = 0xFF; ev.operand = { 0x00, 0x01 }; return ev;
	}
	return ev;
}

SoundEventInsertKind ClassifySoundEvent(const Landstalker::MusicData::SoundEvent& event)
{
	if (!event.is_command)
	{
		return (event.value == 0x70) ? SoundEventInsertKind::Rest : SoundEventInsertKind::Note;
	}
	const uint8_t op = event.operand.empty() ? 0 : event.operand[0];
	switch (event.value)
	{
	case 0xFE: return SoundEventInsertKind::Instrument;
	case 0xFD: return SoundEventInsertKind::Volume;
	case 0xFC: return SoundEventInsertKind::KeyOff;
	case 0xFB: return SoundEventInsertKind::Vibrato;
	case 0xFA: return SoundEventInsertKind::Pan;
	case 0xF9: return SoundEventInsertKind::Transpose;
	case 0xF8:
		switch (op & 0xE0)
		{
		case 0x00: return SoundEventInsertKind::LoopSetMarkerA;
		case 0x20: return SoundEventInsertKind::LoopSetMarkerB;
		case 0x40: return SoundEventInsertKind::LoopPlayOnceA;
		case 0x60: return SoundEventInsertKind::LoopPlayOnceB;
		case 0x80: return SoundEventInsertKind::LoopMarker;
		case 0xA0: return (op & 1) ? SoundEventInsertKind::LoopJumpToMarkerA : SoundEventInsertKind::LoopJumpToMarkerB;
		case 0xC0: return SoundEventInsertKind::LoopBegin;
		case 0xE0: default: return SoundEventInsertKind::LoopEnd;
		}
	case 0xFF:
	default:
	{
		const uint8_t lo = op;
		const uint8_t hi = (event.operand.size() >= 2) ? event.operand[1] : 0;
		if (hi != 0) return SoundEventInsertKind::JumpAddress;
		if (lo != 0) return SoundEventInsertKind::Chain;
		return SoundEventInsertKind::End;
	}
	}
}
