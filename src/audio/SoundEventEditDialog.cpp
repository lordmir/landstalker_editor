#include <audio/SoundEventEditDialog.h>

#include <algorithm>

#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>

namespace
{
	using Landstalker::MusicData;
	using SoundEvent = MusicData::SoundEvent;

	constexpr const char* NOTE_NAMES[12] = {
		"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
	};

	void AddRow(wxFlexGridSizer* grid, wxWindow* parent, const wxString& label, wxWindow* control)
	{
		grid->Add(new wxStaticText(parent, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
		grid->Add(control, 0);
	}

	void AddRow(wxFlexGridSizer* grid, wxWindow* parent, const wxString& label, wxSizer* control)
	{
		grid->Add(new wxStaticText(parent, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
		grid->Add(control, 0);
	}

	uint8_t OperandByte(const SoundEvent& ev, std::size_t index)
	{
		return (index < ev.operand.size()) ? ev.operand[index] : 0;
	}
}

SoundEventEditDialog::SoundEventEditDialog(wxWindow* parent, SoundEventChannelKind channel_kind,
	const Landstalker::MusicData::SoundEvent& event)
	: wxDialog(parent, wxID_ANY, "Edit Event", wxDefaultPosition, wxDefaultSize,
		wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
	, m_channel_kind(channel_kind)
	, m_kind(ClassifySoundEvent(event))
	, m_event(event)
{
	BuildUI();
}

wxSizer* SoundEventEditDialog::BuildPitchFields(wxWindow* parent)
{
	auto* sizer = new wxBoxSizer(wxHORIZONTAL);
	switch (m_channel_kind)
	{
	case SoundEventChannelKind::DAC:
		m_pitch_spin = new wxSpinCtrl(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
			wxSP_ARROW_KEYS, 1, 128, m_event.value + 1);
		sizer->Add(new wxStaticText(parent, wxID_ANY, "Sample "), 0, wxALIGN_CENTER_VERTICAL);
		sizer->Add(m_pitch_spin, 0);
		break;
	case SoundEventChannelKind::PSG_NOISE:
		m_pitch_spin = new wxSpinCtrl(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
			wxSP_ARROW_KEYS, 0, 7, m_event.value & 0x07);
		sizer->Add(new wxStaticText(parent, wxID_ANY, "Noise mode "), 0, wxALIGN_CENTER_VERTICAL);
		sizer->Add(m_pitch_spin, 0);
		break;
	case SoundEventChannelKind::FM:
	case SoundEventChannelKind::PSG_TONE:
	default:
	{
		const uint8_t pitch = std::min<uint8_t>(m_event.value, 0x6F);
		m_note_choice = new wxChoice(parent, wxID_ANY);
		for (const char* name : NOTE_NAMES)
		{
			m_note_choice->Append(name);
		}
		m_note_choice->SetSelection(pitch % 12);
		m_octave_spin = new wxSpinCtrl(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(60, -1),
			wxSP_ARROW_KEYS, 0, 9, pitch / 12);
		sizer->Add(m_note_choice, 0, wxRIGHT, 6);
		sizer->Add(m_octave_spin, 0);
		break;
	}
	}
	return sizer;
}

wxSizer* SoundEventEditDialog::BuildDurationFields(wxWindow* parent)
{
	auto* sizer = new wxBoxSizer(wxHORIZONTAL);
	m_duration_check = new wxCheckBox(parent, wxID_ANY, "Explicit duration");
	m_duration_check->SetValue(m_event.has_duration);
	sizer->Add(m_duration_check, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
	m_duration_spin = new wxSpinCtrl(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		wxSP_ARROW_KEYS, 0, 255, m_event.duration);
	m_duration_spin->Enable(m_event.has_duration);
	m_duration_check->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { m_duration_spin->Enable(m_duration_check->GetValue()); });
	sizer->Add(m_duration_spin, 0);
	return sizer;
}

void SoundEventEditDialog::BuildUI()
{
	auto* top = new wxBoxSizer(wxVERTICAL);
	auto* grid = new wxFlexGridSizer(2, wxSize(10, 8));

	switch (m_kind)
	{
	case SoundEventInsertKind::Note:
		AddRow(grid, this, "Pitch", BuildPitchFields(this));
		AddRow(grid, this, "Duration", BuildDurationFields(this));
		break;
	case SoundEventInsertKind::Rest:
		AddRow(grid, this, "Duration", BuildDurationFields(this));
		break;
	case SoundEventInsertKind::Instrument:
		m_byte_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
			wxSP_ARROW_KEYS, 0, 255, OperandByte(m_event, 0));
		AddRow(grid, this, "Instrument ID", m_byte_spin);
		break;
	case SoundEventInsertKind::Volume:
		if (m_channel_kind == SoundEventChannelKind::PSG_TONE || m_channel_kind == SoundEventChannelKind::PSG_NOISE)
		{
			m_byte_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
				wxSP_ARROW_KEYS, 0, 15, (OperandByte(m_event, 0) >> 4) & 0x0F);
			AddRow(grid, this, "Envelope", m_byte_spin);
			m_byte_spin2 = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
				wxSP_ARROW_KEYS, 0, 15, OperandByte(m_event, 0) & 0x0F);
			AddRow(grid, this, "Volume", m_byte_spin2);
		}
		else if (m_channel_kind == SoundEventChannelKind::FM)
		{
			m_byte_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
				wxSP_ARROW_KEYS, 0, 15, OperandByte(m_event, 0) & 0x0F);
			AddRow(grid, this, "Volume (0 = loudest)", m_byte_spin);
		}
		else
		{
			m_byte_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
				wxSP_ARROW_KEYS, 0, 255, OperandByte(m_event, 0));
			AddRow(grid, this, "Operand", m_byte_spin);
		}
		break;
	case SoundEventInsertKind::KeyOff:
		if (m_channel_kind == SoundEventChannelKind::FM)
		{
			const uint8_t op = OperandByte(m_event, 0);
			m_mode_choice = new wxChoice(this, wxID_ANY);
			m_mode_choice->Append("Key-off timing");
			m_mode_choice->Append("Portamento speed");
			m_mode_choice->Append("Portamento off");
			const int mode = (op == 0xFF) ? 2 : ((op >= 0x81) ? 1 : 0);
			m_mode_choice->SetSelection(mode);
			AddRow(grid, this, "Mode", m_mode_choice);
			m_byte_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
				wxSP_ARROW_KEYS, 0, 127, op & 0x7F);
			AddRow(grid, this, "Release point / speed", m_byte_spin);
			m_check1 = new wxCheckBox(this, wxID_ANY, "Hold (never auto key-off)");
			m_check1->SetValue((op & 0x80) != 0 && op != 0xFF);
			grid->Add(new wxStaticText(this, wxID_ANY, wxEmptyString));
			grid->Add(m_check1, 0);
		}
		else
		{
			const uint8_t op = OperandByte(m_event, 0);
			m_byte_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
				wxSP_ARROW_KEYS, 0, 127, op & 0x7F);
			AddRow(grid, this, "Release point", m_byte_spin);
			m_check1 = new wxCheckBox(this, wxID_ANY, "Hold (never auto key-off)");
			m_check1->SetValue((op & 0x80) != 0);
			grid->Add(new wxStaticText(this, wxID_ANY, wxEmptyString));
			grid->Add(m_check1, 0);
		}
		break;
	case SoundEventInsertKind::Vibrato:
		m_byte_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
			wxSP_ARROW_KEYS, 0, 255, OperandByte(m_event, 0));
		AddRow(grid, this, "Pitch effect table ID", m_byte_spin);
		break;
	case SoundEventInsertKind::Pan:
		if (m_channel_kind == SoundEventChannelKind::FM || m_channel_kind == SoundEventChannelKind::DAC)
		{
			const uint8_t op = OperandByte(m_event, 0);
			m_check1 = new wxCheckBox(this, wxID_ANY, "Left");
			m_check1->SetValue((op & 0x80) != 0);
			m_check2 = new wxCheckBox(this, wxID_ANY, "Right");
			m_check2->SetValue((op & 0x40) != 0);
			auto* pan_sizer = new wxBoxSizer(wxHORIZONTAL);
			pan_sizer->Add(m_check1, 0, wxRIGHT, 8);
			pan_sizer->Add(m_check2, 0);
			AddRow(grid, this, "Pan", pan_sizer);
		}
		else if (m_channel_kind == SoundEventChannelKind::PSG_TONE)
		{
			m_byte_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
				wxSP_ARROW_KEYS, 0, 255, OperandByte(m_event, 0));
			AddRow(grid, this, "Tempo", m_byte_spin);
		}
		else
		{
			m_byte_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
				wxSP_ARROW_KEYS, 0, 255, OperandByte(m_event, 0));
			AddRow(grid, this, "Operand", m_byte_spin);
		}
		break;
	case SoundEventInsertKind::Transpose:
		m_byte_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
			wxSP_ARROW_KEYS, 0, 255, OperandByte(m_event, 0));
		AddRow(grid, this, "Transpose (raw byte)", m_byte_spin);
		top->Add(new wxStaticText(this, wxID_ANY,
			"Low nibble: signed semitone shift. Bits 4-6: fine detune."), 0, wxLEFT | wxTOP, 6);
		break;
	case SoundEventInsertKind::LoopBegin:
		m_count_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
			wxSP_ARROW_KEYS, 1, 32, (OperandByte(m_event, 0) & 0x1F) + 1);
		AddRow(grid, this, "Repeat count", m_count_spin);
		break;
	case SoundEventInsertKind::Chain:
		m_byte_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
			wxSP_ARROW_KEYS, 1, 255, std::max<uint8_t>(1, OperandByte(m_event, 0)));
		AddRow(grid, this, "New operation id", m_byte_spin);
		break;
	case SoundEventInsertKind::JumpAddress:
		m_address_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(100, -1),
			wxSP_ARROW_KEYS, 0, 65535,
			(static_cast<int>(OperandByte(m_event, 1)) << 8) | OperandByte(m_event, 0));
		AddRow(grid, this, "Target address", m_address_spin);
		break;
	case SoundEventInsertKind::LoopSetMarkerA:
	case SoundEventInsertKind::LoopSetMarkerB:
	case SoundEventInsertKind::LoopPlayOnceA:
	case SoundEventInsertKind::LoopPlayOnceB:
	case SoundEventInsertKind::LoopMarker:
	case SoundEventInsertKind::LoopJumpToMarkerA:
	case SoundEventInsertKind::LoopJumpToMarkerB:
	case SoundEventInsertKind::LoopEnd:
	case SoundEventInsertKind::End:
		// No parameters to edit - just confirms/keeps this event.
		top->Add(new wxStaticText(this, wxID_ANY, SoundEventInsertKindLabel(m_kind) + " has no parameters."),
			0, wxALL, 6);
		break;
	}

	if (grid->GetItemCount() > 0)
	{
		top->Add(grid, 0, wxALL, 10);
	}
	top->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 8);

	Bind(wxEVT_BUTTON, &SoundEventEditDialog::OnOk, this, wxID_OK);

	SetSizerAndFit(top);
	CentreOnParent();
}

void SoundEventEditDialog::OnOk(wxCommandEvent& evt)
{
	switch (m_kind)
	{
	case SoundEventInsertKind::Note:
	{
		uint8_t pitch = 0;
		switch (m_channel_kind)
		{
		case SoundEventChannelKind::DAC:
			pitch = static_cast<uint8_t>(m_pitch_spin->GetValue() - 1);
			break;
		case SoundEventChannelKind::PSG_NOISE:
			pitch = static_cast<uint8_t>(m_pitch_spin->GetValue() & 0x07);
			break;
		case SoundEventChannelKind::FM:
		case SoundEventChannelKind::PSG_TONE:
		default:
			pitch = static_cast<uint8_t>(std::min(0x6F, m_octave_spin->GetValue() * 12 + m_note_choice->GetSelection()));
			break;
		}
		m_event.value = pitch;
		m_event.has_duration = m_duration_check->GetValue();
		m_event.duration = static_cast<uint8_t>(m_duration_spin->GetValue());
		break;
	}
	case SoundEventInsertKind::Rest:
		m_event.value = 0x70;
		m_event.has_duration = m_duration_check->GetValue();
		m_event.duration = static_cast<uint8_t>(m_duration_spin->GetValue());
		break;
	case SoundEventInsertKind::Instrument:
		m_event.operand = { static_cast<uint8_t>(m_byte_spin->GetValue()) };
		break;
	case SoundEventInsertKind::Volume:
		if (m_channel_kind == SoundEventChannelKind::PSG_TONE || m_channel_kind == SoundEventChannelKind::PSG_NOISE)
		{
			m_event.operand = { static_cast<uint8_t>(((m_byte_spin->GetValue() & 0x0F) << 4) | (m_byte_spin2->GetValue() & 0x0F)) };
		}
		else
		{
			m_event.operand = { static_cast<uint8_t>(m_byte_spin->GetValue() & 0x0F) };
		}
		break;
	case SoundEventInsertKind::KeyOff:
		if (m_channel_kind == SoundEventChannelKind::FM)
		{
			switch (m_mode_choice->GetSelection())
			{
			case 2: m_event.operand = { 0xFF }; break; // portamento off
			case 1: m_event.operand = { static_cast<uint8_t>(0x80 | (m_byte_spin->GetValue() & 0x7F)) }; break; // portamento speed
			default:
				m_event.operand = { static_cast<uint8_t>((m_byte_spin->GetValue() & 0x7F) | (m_check1->GetValue() ? 0x80 : 0)) };
				break;
			}
		}
		else
		{
			m_event.operand = { static_cast<uint8_t>((m_byte_spin->GetValue() & 0x7F) | (m_check1->GetValue() ? 0x80 : 0)) };
		}
		break;
	case SoundEventInsertKind::Vibrato:
		m_event.operand = { static_cast<uint8_t>(m_byte_spin->GetValue()) };
		break;
	case SoundEventInsertKind::Pan:
		if (m_channel_kind == SoundEventChannelKind::FM || m_channel_kind == SoundEventChannelKind::DAC)
		{
			m_event.operand = { static_cast<uint8_t>((m_check1->GetValue() ? 0x80 : 0) | (m_check2->GetValue() ? 0x40 : 0)) };
		}
		else
		{
			m_event.operand = { static_cast<uint8_t>(m_byte_spin->GetValue()) };
		}
		break;
	case SoundEventInsertKind::Transpose:
		m_event.operand = { static_cast<uint8_t>(m_byte_spin->GetValue()) };
		break;
	case SoundEventInsertKind::LoopBegin:
		m_event.operand = { static_cast<uint8_t>(0xC0 | ((m_count_spin->GetValue() - 1) & 0x1F)) };
		break;
	case SoundEventInsertKind::Chain:
		m_event.operand = { static_cast<uint8_t>(m_byte_spin->GetValue()), 0x00 };
		break;
	case SoundEventInsertKind::JumpAddress:
	{
		const int addr = m_address_spin->GetValue();
		m_event.operand = { static_cast<uint8_t>(addr & 0xFF), static_cast<uint8_t>((addr >> 8) & 0xFF) };
		break;
	}
	case SoundEventInsertKind::LoopSetMarkerA:
	case SoundEventInsertKind::LoopSetMarkerB:
	case SoundEventInsertKind::LoopPlayOnceA:
	case SoundEventInsertKind::LoopPlayOnceB:
	case SoundEventInsertKind::LoopMarker:
	case SoundEventInsertKind::LoopJumpToMarkerA:
	case SoundEventInsertKind::LoopJumpToMarkerB:
	case SoundEventInsertKind::LoopEnd:
	case SoundEventInsertKind::End:
		break; // nothing to read back - operand is already correct from construction
	}
	evt.Skip();
}

Landstalker::MusicData::SoundEvent SoundEventEditDialog::GetResult() const
{
	return m_event;
}