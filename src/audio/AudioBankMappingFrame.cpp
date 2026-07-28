#include <audio/AudioBankMappingFrame.h>

#include <fstream>
#include <sstream>

#include <wx/choice.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/settings.h>
#include <wx/statbox.h>
#include <wx/stattext.h>

#include <audio/AudioTablesYaml.h>
#include <audio/YamlIo.h>

enum MENU_IDS
{
	ID_FILE_EXPORT_YAML = 20000,
	ID_FILE_IMPORT_YAML
};

namespace
{
	using Landstalker::MusicData;
	constexpr std::size_t MUSIC_BANK_SLOT_COUNT = MusicData::MUSIC_SLOT_COUNT / 2; // 32 ids per bank
	constexpr int CHOICE_WIDTH = 130;
}

AudioBankMappingFrame::AudioBankMappingFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst)
{
	BuildUI();
}

AudioBankMappingFrame::~AudioBankMappingFrame()
{
}

void AudioBankMappingFrame::BuildUI()
{
	Freeze();
	m_panel = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxVSCROLL | wxHSCROLL | wxTAB_TRAVERSAL);
	m_panel->SetScrollRate(16, 16);

	auto* top = new wxBoxSizer(wxVERTICAL);
	top->Add(new wxStaticText(m_panel, wxID_ANY,
		"Which pool entry (see the Music/SFX tree items) plays for each id."), 0, wxALL, 8);

	BuildMusicSection(top, "Music Bank 4 (ids 00h-1Fh)", 0, MUSIC_BANK_SLOT_COUNT, m_bank4);
	BuildMusicSection(top, "Music Bank 3 (ids 20h-3Fh)", MUSIC_BANK_SLOT_COUNT, MUSIC_BANK_SLOT_COUNT, m_bank3);
	BuildSfxSection(top);

	m_panel->SetSizer(top);
	m_panel->FitInside();

	auto* outer = new wxBoxSizer(wxVERTICAL);
	outer->Add(m_panel, 1, wxEXPAND);
	SetSizer(outer);
	Layout();
	Thaw();
}

void AudioBankMappingFrame::BuildMusicSection(wxSizer* sizer, const wxString& title, std::size_t first_slot,
	std::size_t count, BankSection& section)
{
	auto* box = new wxStaticBoxSizer(wxVERTICAL, m_panel, title);
	section.usage_label = new wxStaticText(box->GetStaticBox(), wxID_ANY, wxEmptyString);
	box->Add(section.usage_label, 0, wxLEFT | wxTOP, 6);
	auto* grid = new wxFlexGridSizer(GRID_COLUMNS * 2, wxSize(10, 4));

	section.rows.resize(count);
	for (std::size_t i = 0; i < count; ++i)
	{
		const std::size_t slot = first_slot + i;
		grid->Add(new wxStaticText(box->GetStaticBox(), wxID_ANY, wxString::Format("%02Xh", static_cast<unsigned>(slot))),
			0, wxALIGN_CENTER_VERTICAL);
		section.rows[i].choice = new wxChoice(box->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxSize(CHOICE_WIDTH, -1));
		section.rows[i].choice->Bind(wxEVT_CHOICE, [this, slot](wxCommandEvent&) { OnMusicSlotChanged(slot); });
		grid->Add(section.rows[i].choice, 0);
	}
	box->Add(grid, 0, wxALL, 6);
	sizer->Add(box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
}

void AudioBankMappingFrame::RefreshBankUsage()
{
	const auto refresh = [&](BankSection& section, std::size_t bank, const char* extra)
	{
		if (!section.usage_label)
		{
			return;
		}
		if (!m_gd)
		{
			section.usage_label->SetLabel(wxEmptyString);
			return;
		}
		const auto [used, capacity] = m_gd->GetMusicData()->GetMusicBankUsage(bank);
		wxString text = wxString::Format("Track data + tables%s: %zu of %zu bytes used", extra, used, capacity);
		if (used > capacity)
		{
			text += wxString::Format("  -  OVER CAPACITY by %zu bytes! The bank will not build.", used - capacity);
			section.usage_label->SetForegroundColour(*wxRED);
		}
		else
		{
			text += wxString::Format(" (%zu free)", capacity - used);
			section.usage_label->SetForegroundColour(
				wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
		}
		section.usage_label->SetLabel(text);
	};
	refresh(m_bank4, 0, " + YM instruments");
	refresh(m_bank3, 1, "");
	if (m_panel)
	{
		m_panel->Layout();
	}
}

void AudioBankMappingFrame::BuildSfxSection(wxSizer* sizer)
{
	auto* box = new wxStaticBoxSizer(wxVERTICAL, m_panel, "SFX (ids 41h-7Ah)");
	auto* grid = new wxFlexGridSizer(GRID_COLUMNS * 2, wxSize(10, 4));

	m_sfx_rows.resize(MusicData::SFX_SLOT_COUNT);
	for (std::size_t slot = 0; slot < MusicData::SFX_SLOT_COUNT; ++slot)
	{
		grid->Add(new wxStaticText(box->GetStaticBox(), wxID_ANY, wxString::Format("%02Xh", static_cast<unsigned>(slot + 0x41))),
			0, wxALIGN_CENTER_VERTICAL);
		m_sfx_rows[slot].choice = new wxChoice(box->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxSize(CHOICE_WIDTH, -1));
		m_sfx_rows[slot].choice->Bind(wxEVT_CHOICE, [this, slot](wxCommandEvent&) { OnSfxSlotChanged(slot); });
		grid->Add(m_sfx_rows[slot].choice, 0);
	}
	box->Add(grid, 0, wxALL, 6);
	sizer->Add(box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
}

void AudioBankMappingFrame::InitMenu(wxMenuBar& menu, ImageList& /*ilist*/) const
{
	ClearMenu(menu);
	auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
	AddMenuItem(fileMenu, 0, ID_FILE_EXPORT_YAML, "Export Bank Mapping as YAML...");
	AddMenuItem(fileMenu, 1, ID_FILE_IMPORT_YAML, "Import Bank Mapping from YAML...");
	RefreshMenuEnable();
}

void AudioBankMappingFrame::OnMenuClick(wxMenuEvent& evt)
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

void AudioBankMappingFrame::ClearMenu(wxMenuBar& menu) const
{
	EditorFrame::ClearMenu(menu);
}

void AudioBankMappingFrame::RefreshMenuEnable() const
{
	EnableMenuItem(ID_FILE_EXPORT_YAML, static_cast<bool>(m_gd));
	EnableMenuItem(ID_FILE_IMPORT_YAML, static_cast<bool>(m_gd));
}

void AudioBankMappingFrame::OnExportYaml()
{
	if (!m_gd)
	{
		return;
	}
	ExportYamlWithDialog(this, "Export Bank Mapping as YAML", "bank_mapping.yaml",
		[&](YAML::Emitter& out) { EmitBankMappingYaml(out, *m_gd->GetMusicData()); });
}

void AudioBankMappingFrame::OnImportYaml()
{
	if (!m_gd)
	{
		return;
	}
	if (ImportYamlWithDialog(this, "Import Bank Mapping from YAML", [&](const YAML::Node& root)
		{
			auto md = m_gd->GetMusicData();
			auto music_map = md->GetMusicSlotMap();
			auto sfx_map = md->GetSfxSlotMap();
			ApplyBankMappingFromYaml(root, *md, music_map, sfx_map);
			md->SetMusicSlotMap(music_map);
			md->SetSfxSlotMap(sfx_map);
		}))
	{
		LoadValues();
	}
}

bool AudioBankMappingFrame::Open()
{
	if (!m_gd)
	{
		return false;
	}
	LoadValues();
	return true;
}

void AudioBankMappingFrame::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	m_gd = gd;
	LoadValues();
}

void AudioBankMappingFrame::ClearGameData()
{
	m_gd.reset();
	LoadValues();
}

void AudioBankMappingFrame::RefreshMusicChoices(std::vector<SlotRow>& rows, std::size_t first_slot)
{
	if (!m_gd)
	{
		for (auto& row : rows)
		{
			row.choice->Clear();
			row.choice->Enable(false);
		}
		return;
	}
	auto md = m_gd->GetMusicData();
	const auto& pool = md->GetMusicTrackPool();
	const auto& slot_map = md->GetMusicSlotMap();
	for (std::size_t i = 0; i < rows.size(); ++i)
	{
		const std::size_t slot = first_slot + i;
		rows[i].choice->Enable(true);
		rows[i].choice->Clear();
		for (const auto& entry : pool)
		{
			rows[i].choice->Append(entry.name.empty() ? wxString("(unnamed)") : wxString(entry.name));
		}
		if (slot < slot_map.size() && slot_map[slot] < pool.size())
		{
			rows[i].choice->SetSelection(static_cast<int>(slot_map[slot]));
		}
	}
}

void AudioBankMappingFrame::RefreshSfxChoices()
{
	if (!m_gd)
	{
		for (auto& row : m_sfx_rows)
		{
			row.choice->Clear();
			row.choice->Enable(false);
		}
		return;
	}
	auto md = m_gd->GetMusicData();
	const auto& pool = md->GetSfxPool();
	const auto& slot_map = md->GetSfxSlotMap();
	for (std::size_t slot = 0; slot < m_sfx_rows.size(); ++slot)
	{
		m_sfx_rows[slot].choice->Enable(true);
		m_sfx_rows[slot].choice->Clear();
		for (const auto& entry : pool)
		{
			m_sfx_rows[slot].choice->Append(entry.name.empty() ? wxString("(unnamed)") : wxString(entry.name));
		}
		if (slot < slot_map.size() && slot_map[slot] < pool.size())
		{
			m_sfx_rows[slot].choice->SetSelection(static_cast<int>(slot_map[slot]));
		}
	}
}

void AudioBankMappingFrame::LoadValues()
{
	m_populating = true;
	RefreshMusicChoices(m_bank4.rows, 0);
	RefreshMusicChoices(m_bank3.rows, MUSIC_BANK_SLOT_COUNT);
	RefreshSfxChoices();
	RefreshBankUsage();
	m_populating = false;
}

void AudioBankMappingFrame::OnMusicSlotChanged(std::size_t slot)
{
	if (m_populating || !m_gd)
	{
		return;
	}
	auto& rows = (slot < MUSIC_BANK_SLOT_COUNT) ? m_bank4.rows : m_bank3.rows;
	const std::size_t i = (slot < MUSIC_BANK_SLOT_COUNT) ? slot : (slot - MUSIC_BANK_SLOT_COUNT);
	const int selection = rows[i].choice->GetSelection();
	if (selection < 0)
	{
		return;
	}
	auto md = m_gd->GetMusicData();
	auto slot_map = md->GetMusicSlotMap();
	if (slot >= slot_map.size())
	{
		return;
	}
	slot_map[slot] = static_cast<std::size_t>(selection);
	md->SetMusicSlotMap(slot_map);
	RefreshBankUsage();
}

void AudioBankMappingFrame::OnSfxSlotChanged(std::size_t slot)
{
	if (m_populating || !m_gd || slot >= m_sfx_rows.size())
	{
		return;
	}
	const int selection = m_sfx_rows[slot].choice->GetSelection();
	if (selection < 0)
	{
		return;
	}
	auto md = m_gd->GetMusicData();
	auto slot_map = md->GetSfxSlotMap();
	if (slot >= slot_map.size())
	{
		return;
	}
	slot_map[slot] = static_cast<std::size_t>(selection);
	md->SetSfxSlotMap(slot_map);
}
