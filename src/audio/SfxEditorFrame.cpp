#include <audio/SfxEditorFrame.h>

#include <algorithm>
#include <fstream>
#include <sstream>

#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/scrolwin.h>
#include <wx/spinctrl.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include <audio/SoundEventYaml.h>
#include <misc/SpinCtrlSize.h>

enum MENU_IDS
{
	ID_FILE_ADD_SFX = 20000,
	ID_FILE_DELETE_SFX,
	ID_FILE_EXPORT_YAML,
	ID_FILE_IMPORT_YAML
};

namespace
{
	using Landstalker::MusicData;

	constexpr std::array<const char*, MusicData::SFX_FULL_CHANNEL_COUNT> FULL_CHANNEL_LABELS = {
		"FM 1", "FM 2", "FM 3", "FM 4", "FM 5", "DAC (ch 6)", "PSG 1", "PSG 2", "PSG 3", "PSG Noise"
	};
	constexpr std::array<const char*, MusicData::SFX_OVERLAY_CHANNEL_COUNT> OVERLAY_CHANNEL_LABELS = {
		"FM 4 (overlay)", "FM 5 (overlay)", "FM 6 (overlay)"
	};

	bool IsFullType(uint8_t type) { return type == 1; }

	std::size_t ChannelCountForType(uint8_t type)
	{
		return IsFullType(type) ? MusicData::SFX_FULL_CHANNEL_COUNT : MusicData::SFX_OVERLAY_CHANNEL_COUNT;
	}

	SoundEventChannelKind KindForChannel(uint8_t type, std::size_t channel)
	{
		return IsFullType(type) ? MusicChannelKind(channel) : SfxOverlayChannelKind(channel);
	}

	MusicData::SfxEntry MakeBlankSfx()
	{
		MusicData::SfxEntry entry;
		entry.type = 2; // overlay, matching the base game's own convention for a do-nothing entry
		const std::vector<uint8_t> stop = { 0xFF, 0x00, 0x00 };
		entry.channels.assign(MusicData::SFX_OVERLAY_CHANNEL_COUNT, MusicData::DecodeEventStream(stop));
		return entry;
	}
}

SfxEditorFrame::SfxEditorFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst)
{
	BuildUI();
}

SfxEditorFrame::~SfxEditorFrame()
{
}

SfxEditorFrame::EventStream SfxEditorFrame::GetChannelEvents(std::size_t channel) const
{
	if (!m_gd || !m_have_selection)
	{
		return {};
	}
	const auto& pool = m_gd->GetMusicData()->GetSfxPool();
	if (m_index >= pool.size() || channel >= pool[m_index].entry.channels.size())
	{
		return {};
	}
	return pool[m_index].entry.channels[channel];
}

void SfxEditorFrame::SetChannelEvents(std::size_t channel, const EventStream& events)
{
	if (!m_gd || !m_have_selection)
	{
		return;
	}
	auto md = m_gd->GetMusicData();
	auto pool = md->GetSfxPool();
	if (m_index >= pool.size() || channel >= pool[m_index].entry.channels.size())
	{
		return;
	}
	pool[m_index].entry.channels[channel] = events;
	md->SetSfxPool(pool);
}

void SfxEditorFrame::BuildUI()
{
	Freeze();
	// See MusicEditorFrame for why this is a wxScrolledWindow with a manual size handler rather
	// than a plain panel or a FitInside()-based one: fills vertically, scrolls horizontally only
	// when the visible columns are wider than the window (all ten, for a full-type SFX).
	m_panel = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxHSCROLL | wxTAB_TRAVERSAL);
	m_panel->SetScrollRate(16, 0);

	auto* top = new wxBoxSizer(wxVERTICAL);

	auto* header = new wxBoxSizer(wxHORIZONTAL);
	m_index_label = new wxStaticText(m_panel, wxID_ANY, wxEmptyString);
	header->Add(m_index_label, 1, wxALIGN_CENTER_VERTICAL);
	top->Add(header, 0, wxEXPAND | wxALL, 8);

	auto* detail_box = new wxStaticBoxSizer(wxVERTICAL, m_panel, "SFX Details");
	wxStaticBox* detail_panel = detail_box->GetStaticBox();
	m_detail_box = detail_panel;
	auto* detail_grid = new wxFlexGridSizer(2, wxSize(8, 6));
	detail_grid->AddGrowableCol(1);
	detail_grid->Add(new wxStaticText(detail_panel, wxID_ANY, "Name"), 0, wxALIGN_CENTER_VERTICAL);
	m_name_ctrl = new wxTextCtrl(detail_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		wxTE_PROCESS_ENTER);
	m_name_ctrl->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { CommitDetailFields(); });
	m_name_ctrl->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { e.Skip(); CommitDetailFields(); });
	detail_grid->Add(m_name_ctrl, 1, wxEXPAND);

	detail_grid->Add(new wxStaticText(detail_panel, wxID_ANY, "Type"), 0, wxALIGN_CENTER_VERTICAL);
	auto* type_sizer = new wxBoxSizer(wxHORIZONTAL);
	m_type_ctrl = new wxSpinCtrl(detail_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, SpinCtrlSize(70),
		wxSP_ARROW_KEYS, 1, 255, 2);
	m_type_ctrl->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { CommitDetailFields(); });
	m_type_ctrl->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { e.Skip(); CommitDetailFields(); });
	type_sizer->Add(m_type_ctrl, 0, wxRIGHT, 8);
	m_type_note = new wxStaticText(detail_panel, wxID_ANY, wxEmptyString);
	type_sizer->Add(m_type_note, 0, wxALIGN_CENTER_VERTICAL);
	detail_grid->Add(type_sizer, 0);

	detail_grid->Add(new wxStaticText(detail_panel, wxID_ANY, "Used by"), 0, wxALIGN_CENTER_VERTICAL);
	m_usage_label = new wxStaticText(detail_panel, wxID_ANY, wxEmptyString);
	detail_grid->Add(m_usage_label, 0, wxALIGN_CENTER_VERTICAL);
	detail_box->Add(detail_grid, 0, wxEXPAND | wxALL, 6);
	top->Add(detail_box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

	auto* channels_box = new wxStaticBoxSizer(wxVERTICAL, m_panel, "Channel Data");
	channels_box->Add(new wxStaticText(channels_box->GetStaticBox(), wxID_ANY,
		"One event per row. Double-click (or Enter) a row to edit it; the toolbar or right-click "
		"menu inserts new events. Which id(s) play this SFX is set in Audio > Bank Mapping."),
		0, wxALL, 6);
	m_channels_sizer = new wxBoxSizer(wxHORIZONTAL);
	for (std::size_t ch = 0; ch < MusicData::SFX_FULL_CHANNEL_COUNT; ++ch)
	{
		m_columns[ch] = std::make_unique<SoundEventListColumn>(channels_box->GetStaticBox(),
			FULL_CHANNEL_LABELS[ch], MusicChannelKind(ch),
			[this, ch]() { return GetChannelEvents(ch); },
			[this, ch](const EventStream& events) { SetChannelEvents(ch, events); });
		// Proportion 0: each column keeps its own natural (narrow) width. This also means the
		// overlay type's 3 visible columns stay the same width as the full type's ten, instead of
		// stretching to fill the row - switching types doesn't reflow the visible columns, and the
		// space the other seven would occupy is simply left blank rather than redistributed.
		m_channels_sizer->Add(m_columns[ch]->GetSizer(), 0, wxEXPAND | wxALL, 4);
	}
	channels_box->Add(m_channels_sizer, 1, wxEXPAND);
	top->Add(channels_box, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

	m_panel->SetSizer(top);
	m_panel->Bind(wxEVT_SIZE, [this](wxSizeEvent& evt)
	{
		evt.Skip();
		const wxSize min = m_panel->GetSizer() ? m_panel->GetSizer()->GetMinSize() : wxSize(0, 0);
		const wxSize client = m_panel->GetClientSize();
		m_panel->SetVirtualSize(std::max(min.GetWidth(), client.GetWidth()), client.GetHeight());
	});

	auto* outer = new wxBoxSizer(wxVERTICAL);
	outer->Add(m_panel, 1, wxEXPAND);
	SetSizer(outer);

	Bind(wxEVT_CHAR_HOOK, &SfxEditorFrame::OnCharHook, this);

	LoadDetail();
	Layout();
	Thaw();
}

bool SfxEditorFrame::Open(std::size_t index)
{
	if (!m_gd)
	{
		return false;
	}
	const auto& pool = m_gd->GetMusicData()->GetSfxPool();
	m_index = index;
	m_have_selection = index < pool.size();
	LoadDetail();
	return m_have_selection;
}

void SfxEditorFrame::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	m_gd = gd;
	m_have_selection = false;
	LoadDetail();
}

void SfxEditorFrame::ClearGameData()
{
	m_gd.reset();
	m_have_selection = false;
	LoadDetail();
}

void SfxEditorFrame::RefreshUsageLabel()
{
	if (!m_have_selection || !m_gd)
	{
		m_usage_label->SetLabel(wxEmptyString);
		return;
	}
	const auto& slot_map = m_gd->GetMusicData()->GetSfxSlotMap();
	const auto usage = std::count(slot_map.begin(), slot_map.end(), m_index);
	m_usage_label->SetLabel(wxString::Format("%ld slot(s)", static_cast<long>(usage)));
}

void SfxEditorFrame::UpdateChannelColumnVisibility(uint8_t type)
{
	const std::size_t count = ChannelCountForType(type);
	const bool full = IsFullType(type);
	for (std::size_t ch = 0; ch < m_columns.size(); ++ch)
	{
		const bool visible = ch < count;
		auto& col = *m_columns[ch];
		if (full)
		{
			col.SetLabel(FULL_CHANNEL_LABELS[ch]);
			col.SetChannelKind(MusicChannelKind(ch));
		}
		else if (ch < OVERLAY_CHANNEL_LABELS.size())
		{
			col.SetLabel(OVERLAY_CHANNEL_LABELS[ch]);
			col.SetChannelKind(SfxOverlayChannelKind(ch));
		}
		col.GetBoxWindow()->Show(visible);
		m_channels_sizer->Show(col.GetSizer(), visible, true);
	}
	m_type_note->SetLabel(full ? "Full effect (10 channels)" : "Overlay (3 channels: FM4-6)");
	if (m_panel)
	{
		m_panel->Layout();
	}
}

void SfxEditorFrame::RefreshChannelLists()
{
	uint8_t type = 2;
	if (m_have_selection)
	{
		type = m_gd->GetMusicData()->GetSfxPool()[m_index].entry.type;
	}
	UpdateChannelColumnVisibility(type);
	for (auto& col : m_columns)
	{
		col->Refresh();
	}
}

void SfxEditorFrame::LoadDetail()
{
	if (!m_name_ctrl)
	{
		return;
	}
	m_populating = true;
	RefreshMenuEnable();
	m_detail_box->Enable(m_have_selection);
	if (!m_have_selection)
	{
		m_index_label->SetLabel(m_gd ? wxString("No SFX selected.") : wxString(wxEmptyString));
		m_name_ctrl->ChangeValue(wxEmptyString);
		m_type_ctrl->SetValue(2);
		m_usage_label->SetLabel(wxEmptyString);
		RefreshChannelLists();
		m_populating = false;
		return;
	}
	const auto& entry = m_gd->GetMusicData()->GetSfxPool()[m_index];
	m_index_label->SetLabel(wxString::Format("Pool entry %zu", m_index));
	m_name_ctrl->ChangeValue(entry.name);
	m_type_ctrl->SetValue(entry.entry.type);
	RefreshUsageLabel();
	RefreshChannelLists();
	m_populating = false;
}

void SfxEditorFrame::RefreshMenuEnable() const
{
	EnableMenuItem(ID_FILE_ADD_SFX, static_cast<bool>(m_gd));
	EnableMenuItem(ID_FILE_DELETE_SFX, m_have_selection);
	EnableMenuItem(ID_FILE_EXPORT_YAML, m_have_selection);
	EnableMenuItem(ID_FILE_IMPORT_YAML, m_have_selection);
}

void SfxEditorFrame::CommitDetailFields()
{
	if (m_populating || !m_gd || !m_have_selection)
	{
		return;
	}
	auto md = m_gd->GetMusicData();
	auto pool = md->GetSfxPool();
	if (m_index >= pool.size())
	{
		return;
	}
	auto& entry = pool[m_index];
	entry.name = m_name_ctrl->GetValue().ToStdString();

	const uint8_t new_type = static_cast<uint8_t>(m_type_ctrl->GetValue());
	const bool type_changed = new_type != entry.entry.type;
	entry.entry.type = new_type;
	const std::size_t count = ChannelCountForType(new_type);
	if (entry.entry.channels.size() != count)
	{
		const std::vector<uint8_t> stop = { 0xFF, 0x00, 0x00 };
		entry.entry.channels.resize(count, MusicData::DecodeEventStream(stop));
	}
	md->SetSfxPool(pool);
	if (type_changed)
	{
		RefreshChannelLists();
	}
}

void SfxEditorFrame::OnCharHook(wxKeyEvent& evt)
{
	const int code = evt.GetKeyCode();

	if (evt.GetModifiers() == wxMOD_NONE)
	{
		if (code >= '1' && code <= '9') { m_columns[code - '1']->SetFocus(); return; }
		if (code == '0') { m_columns[9]->SetFocus(); return; }
	}

	SoundEventListColumn* active = nullptr;
	for (auto& col : m_columns)
	{
		if (col->HasListFocus()) { active = col.get(); break; }
	}
	if (!active)
	{
		evt.Skip();
		return;
	}

	if (evt.GetModifiers() == wxMOD_NONE && code == WXK_DELETE) { active->DeleteSelected(); return; }
	if (evt.GetModifiers() == wxMOD_SHIFT && code == WXK_UP) { active->MoveSelected(-1); return; }
	if (evt.GetModifiers() == wxMOD_SHIFT && code == WXK_DOWN) { active->MoveSelected(1); return; }
	if (evt.GetModifiers() != wxMOD_NONE) { evt.Skip(); return; }

	switch (code)
	{
	case 'R': active->InsertDefault(SoundEventInsertKind::Rest); return;
	case 'N': active->InsertDefault(SoundEventInsertKind::Note); return;
	case 'M': active->InsertDefault(SoundEventInsertKind::LoopSetMarkerA); return;
	case 'T': active->InsertDefault(SoundEventInsertKind::Transpose); return;
	case 'B': active->InsertDefault(SoundEventInsertKind::Vibrato); return;
	case 'I': active->InsertDefault(SoundEventInsertKind::Instrument); return;
	case 'V': active->InsertDefault(SoundEventInsertKind::Volume); return;
	case 'P': active->InsertDefault(SoundEventInsertKind::Pan); return;
	case 'K': active->InsertDefault(SoundEventInsertKind::KeyOff); return;
	case 'L': active->InsertDefault(SoundEventInsertKind::LoopBegin); return;
	case 'E': active->InsertDefault(SoundEventInsertKind::LoopEnd); return;
	case 'O': active->InsertDefault(SoundEventInsertKind::LoopPlayOnceA); return;
	case 'J': active->InsertDefault(SoundEventInsertKind::LoopJumpToMarkerB); return;
	case 'S': active->InsertDefault(SoundEventInsertKind::End); return;
	default: break;
	}
	evt.Skip();
}

void SfxEditorFrame::InitMenu(wxMenuBar& menu, ImageList& /*ilist*/) const
{
	ClearMenu(menu);
	auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
	AddMenuItem(fileMenu, 0, ID_FILE_ADD_SFX, "Add New SFX");
	AddMenuItem(fileMenu, 1, ID_FILE_DELETE_SFX, "Delete This SFX");
	AddMenuItem(fileMenu, 2, ID_FILE_EXPORT_YAML, "Export SFX as YAML...");
	AddMenuItem(fileMenu, 3, ID_FILE_IMPORT_YAML, "Import SFX from YAML...");
	RefreshMenuEnable();
}

void SfxEditorFrame::OnMenuClick(wxMenuEvent& evt)
{
	switch (evt.GetId())
	{
	case ID_FILE_ADD_SFX:
		OnAddSfx();
		break;
	case ID_FILE_DELETE_SFX:
		OnDeleteSfx();
		break;
	case ID_FILE_EXPORT_YAML:
		OnExportYaml();
		break;
	case ID_FILE_IMPORT_YAML:
		OnImportYaml();
		break;
	}
}

void SfxEditorFrame::ClearMenu(wxMenuBar& menu) const
{
	EditorFrame::ClearMenu(menu);
}

void SfxEditorFrame::OnAddSfx()
{
	if (!m_gd)
	{
		return;
	}
	auto md = m_gd->GetMusicData();
	auto pool = md->GetSfxPool();
	const std::string name = wxString::Format("SFX %zu", pool.size()).ToStdString();
	pool.push_back({ name, MakeBlankSfx() });
	md->SetSfxPool(pool);
	FireEvent(EVT_REBUILD_NAV_TREE, wxString("Audio/SFX/" + name), 0);
}

void SfxEditorFrame::OnDeleteSfx()
{
	if (!m_gd || !m_have_selection)
	{
		return;
	}
	auto md = m_gd->GetMusicData();
	auto pool = md->GetSfxPool();
	if (m_index >= pool.size())
	{
		return;
	}
	auto slot_map = md->GetSfxSlotMap();
	const auto usage = std::count(slot_map.begin(), slot_map.end(), m_index);
	if (usage > 0)
	{
		wxMessageBox(wxString::Format(
			"\"%s\" is still assigned to %ld slot(s) - reassign those in Audio > Bank Mapping first.",
			pool[m_index].name.c_str(), static_cast<long>(usage)),
			"Delete SFX", wxOK | wxICON_WARNING, this);
		return;
	}
	pool.erase(pool.begin() + m_index);
	for (auto& slot : slot_map)
	{
		if (slot > m_index)
		{
			--slot;
		}
	}
	md->SetSfxPool(pool);
	md->SetSfxSlotMap(slot_map);
	if (pool.empty())
	{
		FireEvent(EVT_REBUILD_NAV_TREE, wxString("Audio/SFX"), 0);
	}
	else
	{
		const std::size_t next = std::min(m_index, pool.size() - 1);
		FireEvent(EVT_REBUILD_NAV_TREE, wxString("Audio/SFX/" + pool[next].name), 0);
	}
}

void SfxEditorFrame::OnExportYaml()
{
	if (!m_gd || !m_have_selection)
	{
		return;
	}
	const auto& entry = m_gd->GetMusicData()->GetSfxPool()[m_index];
	wxFileDialog fd(this, "Export SFX as YAML", "", entry.name + ".yaml",
		"YAML file (*.yml;*.yaml)|*.yml;*.yaml|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return;
	}
	YAML::Emitter out;
	EmitSfxEntryYaml(out, entry);
	if (!out.good())
	{
		wxMessageBox("Failed to build YAML for this SFX.", "Export YAML", wxOK | wxICON_ERROR, this);
		return;
	}
	std::ofstream ofs(fd.GetPath().ToStdString());
	if (!ofs.is_open())
	{
		wxMessageBox("Unable to write to the selected file.", "Export YAML", wxOK | wxICON_ERROR, this);
		return;
	}
	ofs << out.c_str();
}

void SfxEditorFrame::OnImportYaml()
{
	if (!m_gd || !m_have_selection)
	{
		return;
	}
	wxFileDialog fd(this, "Import SFX from YAML", "", "",
		"YAML file (*.yml;*.yaml)|*.yml;*.yaml|All Files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return;
	}
	std::ifstream ifs(fd.GetPath().ToStdString(), std::ios::binary);
	if (!ifs.is_open())
	{
		wxMessageBox("Unable to read the selected file.", "Import YAML", wxOK | wxICON_ERROR, this);
		return;
	}
	std::ostringstream contents;
	contents << ifs.rdbuf();
	try
	{
		const YAML::Node root = YAML::Load(contents.str());
		const auto imported = SfxPoolEntryFromYaml(root);
		auto md = m_gd->GetMusicData();
		auto pool = md->GetSfxPool();
		if (m_index >= pool.size())
		{
			return;
		}
		pool[m_index] = imported;
		md->SetSfxPool(pool);
		FireEvent(EVT_REBUILD_NAV_TREE, wxString("Audio/SFX/" + imported.name), 0);
	}
	catch (const std::exception& e)
	{
		wxMessageBox(std::string("Error when parsing YAML:\n") + e.what(), "Import YAML", wxOK | wxICON_ERROR, this);
	}
}

void SfxEditorFrame::CommitPendingEdits()
{
	CommitDetailFields();
}
