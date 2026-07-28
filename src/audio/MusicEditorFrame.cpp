#include <audio/MusicEditorFrame.h>

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

#include <audio/SoundEventMidi.h>
#include <audio/SoundEventYaml.h>
#include <audio/YamlIo.h>
#include <misc/SpinCtrlSize.h>

enum MENU_IDS
{
	ID_FILE_ADD_TRACK = 20000,
	ID_FILE_DELETE_TRACK,
	ID_FILE_EXPORT_YAML,
	ID_FILE_IMPORT_YAML,
	ID_FILE_EXPORT_MIDI
};

namespace
{
	using Landstalker::MusicData;

	constexpr std::array<const char*, MusicData::MUSIC_CHANNEL_COUNT> CHANNEL_LABELS = {
		"FM 1", "FM 2", "FM 3", "FM 4", "FM 5", "DAC (ch 6)", "PSG 1", "PSG 2", "PSG 3", "PSG Noise"
	};

	MusicData::MusicTrack MakeBlankTrack()
	{
		MusicData::MusicTrack track;
		track.tempo = 200;
		const std::vector<uint8_t> stop = { 0xFF, 0x00, 0x00 };
		for (auto& ch : track.channels)
		{
			ch = MusicData::DecodeEventStream(stop);
		}
		return track;
	}
}

MusicEditorFrame::MusicEditorFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst)
{
	BuildUI();
}

MusicEditorFrame::~MusicEditorFrame()
{
}

MusicEditorFrame::EventStream MusicEditorFrame::GetChannelEvents(std::size_t channel) const
{
	if (!m_gd || !m_have_selection)
	{
		return {};
	}
	const auto& pool = m_gd->GetMusicData()->GetMusicTrackPool();
	if (m_index >= pool.size())
	{
		return {};
	}
	return pool[m_index].track.channels[channel];
}

void MusicEditorFrame::SetChannelEvents(std::size_t channel, const EventStream& events)
{
	if (!m_gd || !m_have_selection)
	{
		return;
	}
	auto md = m_gd->GetMusicData();
	auto pool = md->GetMusicTrackPool();
	if (m_index >= pool.size())
	{
		return;
	}
	pool[m_index].track.channels[channel] = events;
	md->SetMusicTrackPool(pool);
	RefreshSizeLabel();
}

void MusicEditorFrame::BuildUI()
{
	Freeze();
	// A scrolled window that always fills its client area vertically (the size handler below pins
	// the virtual height to the client height) and only scrolls horizontally, when the ten channel
	// columns are wider than the window - see the same trick in SfxEditorFrame.
	m_panel = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxHSCROLL | wxTAB_TRAVERSAL);
	m_panel->SetScrollRate(16, 0);

	auto* top = new wxBoxSizer(wxVERTICAL);

	auto* header = new wxBoxSizer(wxHORIZONTAL);
	m_index_label = new wxStaticText(m_panel, wxID_ANY, wxEmptyString);
	header->Add(m_index_label, 1, wxALIGN_CENTER_VERTICAL);
	top->Add(header, 0, wxEXPAND | wxALL, 8);

	auto* detail_box = new wxStaticBoxSizer(wxVERTICAL, m_panel, "Track Details");
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

	detail_grid->Add(new wxStaticText(detail_panel, wxID_ANY, "Tempo"), 0, wxALIGN_CENTER_VERTICAL);
	auto* tempo_sizer = new wxBoxSizer(wxHORIZONTAL);
	m_tempo_ctrl = new wxSpinCtrl(detail_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, SpinCtrlSize(80),
		wxSP_ARROW_KEYS, 0, 255, 200);
	m_tempo_ctrl->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { CommitDetailFields(); });
	m_tempo_ctrl->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { e.Skip(); CommitDetailFields(); });
	tempo_sizer->Add(m_tempo_ctrl, 0, wxRIGHT, 8);
	m_tempo_hz_label = new wxStaticText(detail_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80, -1));
	tempo_sizer->Add(m_tempo_hz_label, 0, wxALIGN_CENTER_VERTICAL);
	auto update_tempo_hz = [this]()
	{
		m_tempo_hz_label->SetLabel(wxString::Format("%.2f Hz", MusicData::GetTempoHz(
			static_cast<uint8_t>(m_tempo_ctrl->GetValue()))));
	};
	m_tempo_ctrl->Bind(wxEVT_TEXT, [update_tempo_hz](wxCommandEvent&) { update_tempo_hz(); });
	m_tempo_ctrl->Bind(wxEVT_SPINCTRL, [update_tempo_hz](wxSpinEvent&) { update_tempo_hz(); });
	detail_grid->Add(tempo_sizer, 0);

	detail_grid->Add(new wxStaticText(detail_panel, wxID_ANY, "Autofade (frames, 0 = never)"), 0, wxALIGN_CENTER_VERTICAL);
	m_autofade_ctrl = new wxSpinCtrl(detail_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, SpinCtrlSize(90),
		wxSP_ARROW_KEYS, 0, 65535, 0);
	m_autofade_ctrl->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { CommitDetailFields(); });
	m_autofade_ctrl->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { e.Skip(); CommitDetailFields(); });
	detail_grid->Add(m_autofade_ctrl, 0);

	detail_grid->Add(new wxStaticText(detail_panel, wxID_ANY, "Used by"), 0, wxALIGN_CENTER_VERTICAL);
	m_usage_label = new wxStaticText(detail_panel, wxID_ANY, wxEmptyString);
	detail_grid->Add(m_usage_label, 0, wxALIGN_CENTER_VERTICAL);

	detail_grid->Add(new wxStaticText(detail_panel, wxID_ANY, "Size"), 0, wxALIGN_CENTER_VERTICAL);
	m_size_label = new wxStaticText(detail_panel, wxID_ANY, wxEmptyString);
	detail_grid->Add(m_size_label, 0, wxALIGN_CENTER_VERTICAL);
	detail_box->Add(detail_grid, 0, wxEXPAND | wxALL, 6);
	top->Add(detail_box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

	auto* channels_box = new wxStaticBoxSizer(wxVERTICAL, m_panel, "Channel Data");
	channels_box->Add(new wxStaticText(channels_box->GetStaticBox(), wxID_ANY,
		"One event per row. Double-click (or Enter) a row to edit it; the toolbar or right-click "
		"menu inserts new events. Which id(s) play this track is set in Audio > Bank Mapping."),
		0, wxALL, 6);
	auto* channels_sizer = new wxBoxSizer(wxHORIZONTAL);
	for (std::size_t ch = 0; ch < MusicData::MUSIC_CHANNEL_COUNT; ++ch)
	{
		m_columns[ch] = std::make_unique<SoundEventListColumn>(channels_box->GetStaticBox(), CHANNEL_LABELS[ch],
			MusicChannelKind(ch),
			[this, ch]() { return GetChannelEvents(ch); },
			[this, ch](const EventStream& events) { SetChannelEvents(ch, events); });
		// Proportion 0: each column keeps its own natural (narrow) width rather than stretching
		// to fill the row - ten of them then comfortably fit a normal window width. wxEXPAND still
		// applies on the sizer's cross axis, so columns still stretch to fill the row's height.
		channels_sizer->Add(m_columns[ch]->GetSizer(), 0, wxEXPAND | wxALL, 4);
	}
	channels_box->Add(channels_sizer, 1, wxEXPAND);
	top->Add(channels_box, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

	m_panel->SetSizer(top);
	// wxScrolledWindow's usual FitInside() pins the virtual size to the sizer's minimum in both
	// directions, which would stop the channel columns from growing to fill any extra vertical
	// space. Pin the virtual height to the client height instead (so there's never vertical
	// scroll - the column lists handle their own internal scroll), and let the virtual width grow
	// to the sizer's real minimum only when it exceeds the client width (so horizontal scroll
	// appears only for the ten-column row, not a fixed one-page assumption).
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

	Bind(wxEVT_CHAR_HOOK, &MusicEditorFrame::OnCharHook, this);

	LoadDetail();
	Layout();
	Thaw();
}

bool MusicEditorFrame::Open(std::size_t index)
{
	if (!m_gd)
	{
		return false;
	}
	const auto& pool = m_gd->GetMusicData()->GetMusicTrackPool();
	m_index = index;
	m_have_selection = index < pool.size();
	LoadDetail();
	return m_have_selection;
}

void MusicEditorFrame::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	m_gd = gd;
	m_have_selection = false;
	LoadDetail();
}

void MusicEditorFrame::ClearGameData()
{
	m_gd.reset();
	m_have_selection = false;
	LoadDetail();
}

void MusicEditorFrame::RefreshUsageLabel()
{
	if (!m_have_selection || !m_gd)
	{
		m_usage_label->SetLabel(wxEmptyString);
		return;
	}
	const auto& slot_map = m_gd->GetMusicData()->GetMusicSlotMap();
	const auto usage = std::count(slot_map.begin(), slot_map.end(), m_index);
	m_usage_label->SetLabel(wxString::Format("%ld slot(s)", static_cast<long>(usage)));
}

void MusicEditorFrame::RefreshSizeLabel()
{
	if (!m_have_selection || !m_gd)
	{
		m_size_label->SetLabel(wxEmptyString);
		return;
	}
	const auto& entry = m_gd->GetMusicData()->GetMusicTrackPool()[m_index];
	const std::size_t size = MusicData::GetMusicTrackSize(entry.track);
	m_size_label->SetLabel(wxString::Format("%zu bytes (%04zXh), including the 24-byte header", size, size));
}

void MusicEditorFrame::RefreshChannelLists()
{
	for (auto& col : m_columns)
	{
		col->Refresh();
	}
}

void MusicEditorFrame::LoadDetail()
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
		m_index_label->SetLabel(m_gd ? wxString("No track selected.") : wxString(wxEmptyString));
		m_name_ctrl->ChangeValue(wxEmptyString);
		m_tempo_ctrl->SetValue(0);
		m_tempo_hz_label->SetLabel(wxEmptyString);
		m_autofade_ctrl->SetValue(0);
		m_usage_label->SetLabel(wxEmptyString);
		m_size_label->SetLabel(wxEmptyString);
		RefreshChannelLists();
		m_populating = false;
		return;
	}
	const auto& entry = m_gd->GetMusicData()->GetMusicTrackPool()[m_index];
	m_index_label->SetLabel(wxString::Format("Pool entry %zu", m_index));
	m_name_ctrl->ChangeValue(entry.name);
	m_tempo_ctrl->SetValue(entry.track.tempo);
	m_tempo_hz_label->SetLabel(wxString::Format("%.2f Hz", MusicData::GetTempoHz(entry.track.tempo)));
	m_autofade_ctrl->SetValue(entry.track.autofade_frames);
	RefreshUsageLabel();
	RefreshSizeLabel();
	RefreshChannelLists();
	m_populating = false;
}

void MusicEditorFrame::RefreshMenuEnable() const
{
	EnableMenuItem(ID_FILE_ADD_TRACK, static_cast<bool>(m_gd));
	EnableMenuItem(ID_FILE_DELETE_TRACK, m_have_selection);
	EnableMenuItem(ID_FILE_EXPORT_YAML, m_have_selection);
	EnableMenuItem(ID_FILE_IMPORT_YAML, m_have_selection);
	EnableMenuItem(ID_FILE_EXPORT_MIDI, m_have_selection);
}

void MusicEditorFrame::CommitDetailFields()
{
	if (m_populating || !m_gd || !m_have_selection)
	{
		return;
	}
	auto md = m_gd->GetMusicData();
	auto pool = md->GetMusicTrackPool();
	if (m_index >= pool.size())
	{
		return;
	}
	auto& entry = pool[m_index];
	entry.name = m_name_ctrl->GetValue().ToStdString();
	entry.track.tempo = static_cast<uint8_t>(m_tempo_ctrl->GetValue());
	entry.track.autofade_frames = static_cast<uint16_t>(m_autofade_ctrl->GetValue());
	md->SetMusicTrackPool(pool);
}

void MusicEditorFrame::OnCharHook(wxKeyEvent& evt)
{
	const int code = evt.GetKeyCode();

	// Digit keys switch which column has focus, regardless of what currently has it.
	if (evt.GetModifiers() == wxMOD_NONE)
	{
		if (code >= '1' && code <= '9') { m_columns[code - '1']->SetFocus(); return; }
		if (code == '0') { m_columns[9]->SetFocus(); return; }
	}

	// Everything else only applies while a channel column's list has keyboard focus - otherwise
	// these keys need to reach the Name field, spin controls, etc. normally.
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

void MusicEditorFrame::InitMenu(wxMenuBar& menu, ImageList& /*ilist*/) const
{
	ClearMenu(menu);
	auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
	AddMenuItem(fileMenu, 0, ID_FILE_ADD_TRACK, "Add New Track");
	AddMenuItem(fileMenu, 1, ID_FILE_DELETE_TRACK, "Delete This Track");
	AddMenuItem(fileMenu, 2, ID_FILE_EXPORT_YAML, "Export Track as YAML...");
	AddMenuItem(fileMenu, 3, ID_FILE_IMPORT_YAML, "Import Track from YAML...");
	AddMenuItem(fileMenu, 4, ID_FILE_EXPORT_MIDI, "Export Track as MIDI...");
	RefreshMenuEnable();
}

void MusicEditorFrame::OnMenuClick(wxMenuEvent& evt)
{
	switch (evt.GetId())
	{
	case ID_FILE_ADD_TRACK:
		OnAddTrack();
		break;
	case ID_FILE_DELETE_TRACK:
		OnDeleteTrack();
		break;
	case ID_FILE_EXPORT_YAML:
		OnExportYaml();
		break;
	case ID_FILE_IMPORT_YAML:
		OnImportYaml();
		break;
	case ID_FILE_EXPORT_MIDI:
		OnExportMidi();
		break;
	}
}

void MusicEditorFrame::ClearMenu(wxMenuBar& menu) const
{
	EditorFrame::ClearMenu(menu);
}

void MusicEditorFrame::OnAddTrack()
{
	if (!m_gd)
	{
		return;
	}
	auto md = m_gd->GetMusicData();
	auto pool = md->GetMusicTrackPool();
	const std::string name = wxString::Format("Track %zu", pool.size()).ToStdString();
	pool.push_back({ name, MakeBlankTrack() });
	md->SetMusicTrackPool(pool);
	// Rebuilds the tree (a new leaf now exists for this entry) and navigates to it.
	FireEvent(EVT_REBUILD_NAV_TREE, wxString("Audio/Music/" + name), 0);
}

void MusicEditorFrame::OnDeleteTrack()
{
	if (!m_gd || !m_have_selection)
	{
		return;
	}
	auto md = m_gd->GetMusicData();
	auto pool = md->GetMusicTrackPool();
	if (m_index >= pool.size())
	{
		return;
	}
	auto slot_map = md->GetMusicSlotMap();
	const auto usage = std::count(slot_map.begin(), slot_map.end(), m_index);
	if (usage > 0)
	{
		wxMessageBox(wxString::Format(
			"\"%s\" is still assigned to %ld slot(s) - reassign those in Audio > Bank Mapping first.",
			pool[m_index].name.c_str(), static_cast<long>(usage)),
			"Delete Track", wxOK | wxICON_WARNING, this);
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
	md->SetMusicTrackPool(pool);
	md->SetMusicSlotMap(slot_map);
	if (pool.empty())
	{
		FireEvent(EVT_REBUILD_NAV_TREE, wxString("Audio/Music"), 0);
	}
	else
	{
		const std::size_t next = std::min(m_index, pool.size() - 1);
		FireEvent(EVT_REBUILD_NAV_TREE, wxString("Audio/Music/" + pool[next].name), 0);
	}
}

void MusicEditorFrame::OnExportYaml()
{
	if (!m_gd || !m_have_selection)
	{
		return;
	}
	const auto& entry = m_gd->GetMusicData()->GetMusicTrackPool()[m_index];
	ExportYamlWithDialog(this, "Export Track as YAML", entry.name + ".yaml",
		[&](YAML::Emitter& out) { EmitMusicTrackYaml(out, entry); });
}

void MusicEditorFrame::OnImportYaml()
{
	if (!m_gd || !m_have_selection)
	{
		return;
	}
	ImportYamlWithDialog(this, "Import Track from YAML", [&](const YAML::Node& root)
	{
		auto imported = MusicTrackEntryFromYaml(root);
		auto md = m_gd->GetMusicData();
		auto pool = md->GetMusicTrackPool();
		if (m_index >= pool.size())
		{
			return;
		}
		// A name clashing with another entry would fold two leaves onto one navigation-tree path
		// (and make name-based lookups like the bank-mapping YAML ambiguous) - uniquify it.
		const auto name_taken = [&](const std::string& name)
		{
			for (std::size_t i = 0; i < pool.size(); ++i)
			{
				if (i != m_index && pool[i].name == name)
				{
					return true;
				}
			}
			return false;
		};
		const std::string base_name = imported.name;
		for (int suffix = 2; name_taken(imported.name); ++suffix)
		{
			imported.name = base_name + " (" + std::to_string(suffix) + ")";
		}
		pool[m_index] = imported;
		md->SetMusicTrackPool(pool);
		FireEvent(EVT_REBUILD_NAV_TREE, wxString("Audio/Music/" + imported.name), 0);
	});
}

void MusicEditorFrame::OnExportMidi()
{
	if (!m_gd || !m_have_selection)
	{
		return;
	}
	const auto& entry = m_gd->GetMusicData()->GetMusicTrackPool()[m_index];
	wxFileDialog fd(this, "Export Track as MIDI", "", entry.name + ".mid",
		"MIDI file (*.mid)|*.mid|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return;
	}
	const auto bytes = ExportMusicTrackAsMidi(entry);
	std::ofstream ofs(fd.GetPath().ToStdString(), std::ios::binary);
	if (!ofs.is_open())
	{
		wxMessageBox("Unable to write to the selected file.", "Export MIDI", wxOK | wxICON_ERROR, this);
		return;
	}
	ofs.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

void MusicEditorFrame::CommitPendingEdits()
{
	CommitDetailFields();
}
