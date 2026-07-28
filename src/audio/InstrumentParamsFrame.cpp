#include <audio/InstrumentParamsFrame.h>

#include <audio/SoundEventFormat.h>

#include <fstream>
#include <sstream>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/choice.h>
#include <wx/listbox.h>
#include <wx/listctrl.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/statbox.h>
#include <wx/stattext.h>

#include <audio/InstrumentParamsYaml.h>
#include <audio/YamlIo.h>
#include <misc/SpinCtrlSize.h>

enum MENU_IDS
{
	ID_FILE_EXPORT_YAML = 20000,
	ID_FILE_IMPORT_YAML
};

namespace
{
	using Landstalker::MusicData;

	constexpr uint8_t EFFECT_LOOP = 0x80;
	constexpr uint8_t EFFECT_HOLD = 0x81;

	enum EffectKind { KIND_DELTA = 0, KIND_LOOP, KIND_HOLD };

}

template <typename Fn>
void InstrumentParamsFrame::ModifyParams(Fn&& fn)
{
	if (!m_gd)
	{
		return;
	}
	auto md = m_gd->GetMusicData();
	auto params = md->GetInstrumentParams();
	fn(params);
	md->SetInstrumentParams(params);
}

InstrumentParamsFrame::InstrumentParamsFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst)
{
	BuildUI();
}

InstrumentParamsFrame::~InstrumentParamsFrame()
{
}

std::vector<uint8_t>* InstrumentParamsFrame::GroupBytes(StepGroup& group, InstrumentParams& params) const
{
	auto& lists = group.is_envelope ? params.psg_envelopes : params.pitch_effects;
	if (group.selected_entry >= lists.size())
	{
		return nullptr;
	}
	return &lists[group.selected_entry];
}

wxString InstrumentParamsFrame::DescribeStep(const StepGroup& group, uint8_t byte) const
{
	if (group.is_envelope)
	{
		// Bit 7 marks a sustain/release-end step; the rest is the amplitude.
		return wxString::Format("%s%u", (byte & 0x80) ? "Sustain @" : "", byte & 0x7F);
	}
	if (byte == EFFECT_LOOP)
	{
		return "Loop (80h)";
	}
	if (byte == EFFECT_HOLD)
	{
		return "Hold (81h)";
	}
	return wxString::Format("%+d", static_cast<int8_t>(byte));
}

wxSizer* InstrumentParamsFrame::BuildStepGroup(StepGroup& group, const wxString& title, bool is_envelope)
{
	group.is_envelope = is_envelope;
	auto* box = new wxStaticBoxSizer(wxHORIZONTAL, m_panel, title);
	wxWindow* parent = box->GetStaticBox();

	group.picker = new wxListBox(parent, wxID_ANY, wxDefaultPosition, wxSize(60, 240), 0, nullptr, wxLB_SINGLE);
	for (std::size_t i = 0; i < 16; ++i)
	{
		group.picker->Append(wxString::Format("%zu", i));
	}
	group.picker->Bind(wxEVT_LISTBOX, [this, &group](wxCommandEvent&) { OnGroupEntrySelected(group); });
	box->Add(group.picker, 0, wxEXPAND | wxALL, 3);

	// See SoundEventListColumn for the wrapper panel: a wxListCtrl parented directly under a
	// static box computes an inflated minimum size on wx/GTK.
	auto* wrap = new wxPanel(parent);
	auto* wrap_sizer = new wxBoxSizer(wxVERTICAL);
	group.steps = new wxListView(wrap, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
	group.steps->AppendColumn("#", wxLIST_FORMAT_RIGHT, 40);
	group.steps->AppendColumn("Step", wxLIST_FORMAT_LEFT, 95);
	group.steps->SetMinSize(wxSize(40 + 95 + 24, 240));
	group.steps->Bind(wxEVT_LIST_ITEM_SELECTED, [this, &group](wxListEvent&) { LoadStepControls(group); });
	wrap_sizer->Add(group.steps, 1, wxEXPAND);
	wrap->SetSizer(wrap_sizer);
	box->Add(wrap, 0, wxEXPAND | wxALL, 3);

	auto* controls = new wxBoxSizer(wxVERTICAL);
	auto* buttons = new wxGridSizer(0, 2, 3, 3);
	group.add_btn = new wxButton(parent, wxID_ANY, "Add");
	group.delete_btn = new wxButton(parent, wxID_ANY, "Delete");
	group.up_btn = new wxButton(parent, wxID_ANY, "Move Up");
	group.down_btn = new wxButton(parent, wxID_ANY, "Move Down");
	group.add_btn->Bind(wxEVT_BUTTON, [this, &group](wxCommandEvent&) { OnGroupAdd(group); });
	group.delete_btn->Bind(wxEVT_BUTTON, [this, &group](wxCommandEvent&) { OnGroupDelete(group); });
	group.up_btn->Bind(wxEVT_BUTTON, [this, &group](wxCommandEvent&) { OnGroupMove(group, -1); });
	group.down_btn->Bind(wxEVT_BUTTON, [this, &group](wxCommandEvent&) { OnGroupMove(group, 1); });
	for (auto* b : { group.add_btn, group.delete_btn, group.up_btn, group.down_btn })
	{
		buttons->Add(b, 0, wxEXPAND);
	}
	controls->Add(buttons, 0, wxEXPAND | wxBOTTOM, 8);

	auto* fields = new wxFlexGridSizer(2, wxSize(6, 4));
	if (is_envelope)
	{
		fields->Add(new wxStaticText(parent, wxID_ANY, "Amplitude"), 0, wxALIGN_CENTER_VERTICAL);
		group.amplitude = new wxSpinCtrl(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, SpinCtrlSize(90),
			wxSP_ARROW_KEYS, 0, 127, 0);
		group.amplitude->Bind(wxEVT_SPINCTRL, [this, &group](wxSpinEvent&) { CommitStepControls(group); });
		fields->Add(group.amplitude, 0);
		fields->Add(new wxStaticText(parent, wxID_ANY, "Sustain marker"), 0, wxALIGN_CENTER_VERTICAL);
		group.sustain = new wxCheckBox(parent, wxID_ANY, wxEmptyString);
		group.sustain->Bind(wxEVT_CHECKBOX, [this, &group](wxCommandEvent&) { CommitStepControls(group); });
		fields->Add(group.sustain, 0);
	}
	else
	{
		fields->Add(new wxStaticText(parent, wxID_ANY, "Kind"), 0, wxALIGN_CENTER_VERTICAL);
		group.kind = new wxChoice(parent, wxID_ANY);
		group.kind->Append("Delta");
		group.kind->Append("Loop (80h)");
		group.kind->Append("Hold (81h)");
		group.kind->Bind(wxEVT_CHOICE, [this, &group](wxCommandEvent&) { CommitStepControls(group); });
		fields->Add(group.kind, 0);
		fields->Add(new wxStaticText(parent, wxID_ANY, "Delta"), 0, wxALIGN_CENTER_VERTICAL);
		group.delta = new wxSpinCtrl(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, SpinCtrlSize(90),
			wxSP_ARROW_KEYS, -127, 127, 0);
		group.delta->Bind(wxEVT_SPINCTRL, [this, &group](wxSpinEvent&) { CommitStepControls(group); });
		fields->Add(group.delta, 0);
	}
	controls->Add(fields, 0);
	box->Add(controls, 0, wxALL, 6);
	return box;
}

wxSizer* InstrumentParamsFrame::BuildByteTable(const wxString& title, std::vector<wxSpinCtrl*>& ctrls,
	std::size_t count, int max, std::size_t per_row)
{
	auto* box = new wxStaticBoxSizer(wxVERTICAL, m_panel, title);
	wxWindow* parent = box->GetStaticBox();
	auto* grid = new wxFlexGridSizer(static_cast<int>(per_row * 2), wxSize(6, 4));
	ctrls.resize(count);
	for (std::size_t i = 0; i < count; ++i)
	{
		grid->Add(new wxStaticText(parent, wxID_ANY, wxString::Format("%zXh", i)), 0, wxALIGN_CENTER_VERTICAL);
		ctrls[i] = new wxSpinCtrl(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, SpinCtrlSize(90),
			wxSP_ARROW_KEYS, 0, max, 0);
		ctrls[i]->SetBase(16);
		ctrls[i]->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { CommitTables(); });
		ctrls[i]->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { e.Skip(); CommitTables(); });
		grid->Add(ctrls[i], 0);
	}
	box->Add(grid, 0, wxALL, 6);
	return box;
}

wxSizer* InstrumentParamsFrame::BuildWordTable(const wxString& title, std::vector<wxSpinCtrl*>& ctrls, std::size_t count)
{
	auto* box = new wxStaticBoxSizer(wxVERTICAL, m_panel, title);
	wxWindow* parent = box->GetStaticBox();
	auto* grid = new wxFlexGridSizer(13, wxSize(6, 4));
	grid->Add(new wxStaticText(parent, wxID_ANY, wxEmptyString), 0);
	for (int n = 0; n < 12; ++n)
	{
		grid->Add(new wxStaticText(parent, wxID_ANY, MUSIC_NOTE_NAMES[n]), 0, wxALIGN_CENTER_HORIZONTAL);
	}
	ctrls.resize(count);
	for (std::size_t i = 0; i < count; ++i)
	{
		if (i % 12 == 0)
		{
			grid->Add(new wxStaticText(parent, wxID_ANY, wxString::Format("+%zu", i / 12)), 0, wxALIGN_CENTER_VERTICAL);
		}
		ctrls[i] = new wxSpinCtrl(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, SpinCtrlSize(100),
			wxSP_ARROW_KEYS, 0, 0xFFFF, 0);
		ctrls[i]->SetBase(16);
		ctrls[i]->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { CommitTables(); });
		ctrls[i]->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { e.Skip(); CommitTables(); });
		grid->Add(ctrls[i], 0);
	}
	box->Add(grid, 0, wxALL, 6);
	return box;
}

void InstrumentParamsFrame::BuildUI()
{
	Freeze();
	m_panel = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxVSCROLL | wxHSCROLL | wxTAB_TRAVERSAL);
	m_panel->SetScrollRate(16, 16);

	auto* top = new wxBoxSizer(wxVERTICAL);
	top->Add(new wxStaticText(m_panel, wxID_ANY,
		"The driver's instrument parameter tables (instrument_params.asm). PSG envelopes are the "
		"instruments a PSG channel selects (event FDh, high nibble); pitch effects are the vibrato/"
		"slide waveforms (event FBh)."), 0, wxALL, 8);

	auto* lists_row = new wxBoxSizer(wxHORIZONTAL);
	lists_row->Add(BuildStepGroup(m_envelopes, "PSG Envelopes", true), 0, wxEXPAND | wxRIGHT, 8);
	lists_row->Add(BuildStepGroup(m_effects, "Pitch Effects", false), 0, wxEXPAND);
	top->Add(lists_row, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

	top->Add(BuildByteTable("FM Volume Curve (volume 0-Fh, loudest last, to Total-Level offset)",
		m_levels, MusicData::YM_LEVEL_COUNT, 0x7F, 8), 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
	top->Add(BuildByteTable("Carrier Operator Mask per FM Algorithm",
		m_slots, MusicData::ALGO_COUNT, 0x0F, 8), 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
	top->Add(BuildWordTable("YM Note Frequencies ((block << 11) | F-number, 7 octaves from C)",
		m_ym_freqs, MusicData::YM_FREQUENCY_COUNT), 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
	top->Add(BuildWordTable("PSG Note Periods (10-bit, larger = lower; from A, offset by 15h)",
		m_psg_freqs, MusicData::PSG_FREQUENCY_COUNT), 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

	m_panel->SetSizer(top);
	m_panel->FitInside();

	auto* outer = new wxBoxSizer(wxVERTICAL);
	outer->Add(m_panel, 1, wxEXPAND);
	SetSizer(outer);
	Layout();
	Thaw();
}

void InstrumentParamsFrame::RefreshGroupSteps(StepGroup& group)
{
	group.steps->Freeze();
	group.steps->DeleteAllItems();
	if (m_gd)
	{
		auto params = m_gd->GetMusicData()->GetInstrumentParams();
		const auto* bytes = GroupBytes(group, params);
		if (bytes)
		{
			for (std::size_t i = 0; i < bytes->size(); ++i)
			{
				const long row = group.steps->InsertItem(static_cast<long>(i), wxString::Format("%zu", i));
				group.steps->SetItem(row, 1, DescribeStep(group, (*bytes)[i]));
			}
		}
	}
	group.steps->Thaw();
}

void InstrumentParamsFrame::LoadStepControls(StepGroup& group)
{
	if (!m_gd)
	{
		return;
	}
	const long sel = group.steps->GetFirstSelected();
	if (sel < 0)
	{
		return;
	}
	auto params = m_gd->GetMusicData()->GetInstrumentParams();
	const auto* bytes = GroupBytes(group, params);
	if (!bytes || static_cast<std::size_t>(sel) >= bytes->size())
	{
		return;
	}
	m_populating = true;
	const uint8_t byte = (*bytes)[sel];
	if (group.is_envelope)
	{
		group.amplitude->SetValue(byte & 0x7F);
		group.sustain->SetValue((byte & 0x80) != 0);
	}
	else if (byte == EFFECT_LOOP || byte == EFFECT_HOLD)
	{
		group.kind->SetSelection(byte == EFFECT_LOOP ? KIND_LOOP : KIND_HOLD);
		group.delta->SetValue(0);
		group.delta->Enable(false);
	}
	else
	{
		group.kind->SetSelection(KIND_DELTA);
		group.delta->SetValue(static_cast<int8_t>(byte));
		group.delta->Enable(true);
	}
	m_populating = false;
}

void InstrumentParamsFrame::CommitStepControls(StepGroup& group)
{
	if (m_populating || !m_gd)
	{
		return;
	}
	const long sel = group.steps->GetFirstSelected();
	if (sel < 0)
	{
		return;
	}
	uint8_t byte = 0;
	if (group.is_envelope)
	{
		byte = static_cast<uint8_t>((group.amplitude->GetValue() & 0x7F) | (group.sustain->GetValue() ? 0x80 : 0));
	}
	else
	{
		switch (group.kind->GetSelection())
		{
		case KIND_LOOP: byte = EFFECT_LOOP; break;
		case KIND_HOLD: byte = EFFECT_HOLD; break;
		default:        byte = static_cast<uint8_t>(static_cast<int8_t>(group.delta->GetValue())); break;
		}
		group.delta->Enable(group.kind->GetSelection() == KIND_DELTA);
	}
	ModifyParams([&](InstrumentParams& params)
	{
		auto* bytes = GroupBytes(group, params);
		if (bytes && static_cast<std::size_t>(sel) < bytes->size())
		{
			(*bytes)[sel] = byte;
		}
	});
	group.steps->SetItem(sel, 1, DescribeStep(group, byte));
}

void InstrumentParamsFrame::OnGroupEntrySelected(StepGroup& group)
{
	const int sel = group.picker->GetSelection();
	if (sel >= 0)
	{
		group.selected_entry = static_cast<std::size_t>(sel);
	}
	RefreshGroupSteps(group);
}

void InstrumentParamsFrame::OnGroupAdd(StepGroup& group)
{
	if (!m_gd)
	{
		return;
	}
	const long sel = group.steps->GetFirstSelected();
	std::size_t pos = 0;
	ModifyParams([&](InstrumentParams& params)
	{
		auto* bytes = GroupBytes(group, params);
		if (!bytes)
		{
			return;
		}
		pos = (sel >= 0) ? static_cast<std::size_t>(sel) + 1 : bytes->size();
		// A neutral default: full amplitude for an envelope step, a flat delta for an effect step.
		bytes->insert(bytes->begin() + pos, group.is_envelope ? 0x0F : 0x00);
	});
	RefreshGroupSteps(group);
	group.steps->Select(static_cast<long>(pos));
}

void InstrumentParamsFrame::OnGroupDelete(StepGroup& group)
{
	if (!m_gd)
	{
		return;
	}
	const long sel = group.steps->GetFirstSelected();
	if (sel < 0)
	{
		return;
	}
	ModifyParams([&](InstrumentParams& params)
	{
		auto* bytes = GroupBytes(group, params);
		if (bytes && static_cast<std::size_t>(sel) < bytes->size())
		{
			bytes->erase(bytes->begin() + sel);
		}
	});
	RefreshGroupSteps(group);
	if (group.steps->GetItemCount() > 0)
	{
		group.steps->Select(std::min<long>(sel, group.steps->GetItemCount() - 1));
	}
}

void InstrumentParamsFrame::OnGroupMove(StepGroup& group, int direction)
{
	if (!m_gd)
	{
		return;
	}
	const long sel = group.steps->GetFirstSelected();
	if (sel < 0)
	{
		return;
	}
	const long target = sel + direction;
	bool moved = false;
	ModifyParams([&](InstrumentParams& params)
	{
		auto* bytes = GroupBytes(group, params);
		if (bytes && target >= 0 && static_cast<std::size_t>(target) < bytes->size()
			&& static_cast<std::size_t>(sel) < bytes->size())
		{
			std::swap((*bytes)[sel], (*bytes)[target]);
			moved = true;
		}
	});
	if (moved)
	{
		RefreshGroupSteps(group);
		group.steps->Select(target);
	}
}

void InstrumentParamsFrame::CommitTables()
{
	if (m_populating || !m_gd)
	{
		return;
	}
	ModifyParams([&](InstrumentParams& params)
	{
		for (std::size_t i = 0; i < m_levels.size(); ++i)
		{
			params.ym_levels[i] = static_cast<uint8_t>(m_levels[i]->GetValue());
		}
		for (std::size_t i = 0; i < m_slots.size(); ++i)
		{
			params.slots_per_algo[i] = static_cast<uint8_t>(m_slots[i]->GetValue());
		}
		for (std::size_t i = 0; i < m_ym_freqs.size(); ++i)
		{
			params.ym_frequencies[i] = static_cast<uint16_t>(m_ym_freqs[i]->GetValue());
		}
		for (std::size_t i = 0; i < m_psg_freqs.size(); ++i)
		{
			params.psg_frequencies[i] = static_cast<uint16_t>(m_psg_freqs[i]->GetValue());
		}
	});
}

void InstrumentParamsFrame::RefreshAll()
{
	m_populating = true;
	const bool have = static_cast<bool>(m_gd);
	for (auto* group : { &m_envelopes, &m_effects })
	{
		group->picker->Enable(have);
		if (have && group->picker->GetSelection() < 0)
		{
			group->picker->SetSelection(static_cast<int>(group->selected_entry));
		}
		RefreshGroupSteps(*group);
	}
	if (have)
	{
		const auto& params = m_gd->GetMusicData()->GetInstrumentParams();
		for (std::size_t i = 0; i < m_levels.size(); ++i)
		{
			m_levels[i]->SetValue(params.ym_levels[i]);
		}
		for (std::size_t i = 0; i < m_slots.size(); ++i)
		{
			m_slots[i]->SetValue(params.slots_per_algo[i]);
		}
		for (std::size_t i = 0; i < m_ym_freqs.size(); ++i)
		{
			m_ym_freqs[i]->SetValue(params.ym_frequencies[i]);
		}
		for (std::size_t i = 0; i < m_psg_freqs.size(); ++i)
		{
			m_psg_freqs[i]->SetValue(params.psg_frequencies[i]);
		}
	}
	for (auto* ctrls : { &m_levels, &m_slots, &m_ym_freqs, &m_psg_freqs })
	{
		for (auto* c : *ctrls)
		{
			c->Enable(have);
		}
	}
	m_populating = false;
}

void InstrumentParamsFrame::InitMenu(wxMenuBar& menu, ImageList& /*ilist*/) const
{
	ClearMenu(menu);
	auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
	AddMenuItem(fileMenu, 0, ID_FILE_EXPORT_YAML, "Export Parameters as YAML...");
	AddMenuItem(fileMenu, 1, ID_FILE_IMPORT_YAML, "Import Parameters from YAML...");
	RefreshMenuEnable();
}

void InstrumentParamsFrame::OnMenuClick(wxMenuEvent& evt)
{
	switch (evt.GetId())
	{
	case ID_FILE_EXPORT_YAML:
		OnExportYaml();
		break;
	case ID_FILE_IMPORT_YAML:
		OnImportYaml();
		break;
	}
}

void InstrumentParamsFrame::ClearMenu(wxMenuBar& menu) const
{
	EditorFrame::ClearMenu(menu);
}

void InstrumentParamsFrame::RefreshMenuEnable() const
{
	EnableMenuItem(ID_FILE_EXPORT_YAML, static_cast<bool>(m_gd));
	EnableMenuItem(ID_FILE_IMPORT_YAML, static_cast<bool>(m_gd));
}

void InstrumentParamsFrame::OnExportYaml()
{
	if (!m_gd)
	{
		return;
	}
	ExportYamlWithDialog(this, "Export Parameters as YAML", "instrument_params.yaml",
		[&](YAML::Emitter& out) { EmitInstrumentParamsYaml(out, m_gd->GetMusicData()->GetInstrumentParams()); });
}

void InstrumentParamsFrame::OnImportYaml()
{
	if (!m_gd)
	{
		return;
	}
	if (ImportYamlWithDialog(this, "Import Parameters from YAML", [&](const YAML::Node& root)
		{
			auto md = m_gd->GetMusicData();
			auto params = md->GetInstrumentParams();
			ApplyInstrumentParamsFromYaml(root, params);
			md->SetInstrumentParams(params);
		}))
	{
		RefreshAll();
	}
}

bool InstrumentParamsFrame::Open()
{
	if (!m_gd)
	{
		return false;
	}
	RefreshAll();
	return true;
}

void InstrumentParamsFrame::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	m_gd = gd;
	RefreshAll();
}

void InstrumentParamsFrame::ClearGameData()
{
	m_gd.reset();
	RefreshAll();
}
