#include <audio/AudioBankMappingFrame.h>

#include <wx/choice.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>

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

	BuildMusicSection(top, "Music Bank 4 (ids 00h-1Fh)", 0, MUSIC_BANK_SLOT_COUNT, m_bank4_rows);
	BuildMusicSection(top, "Music Bank 3 (ids 20h-3Fh)", MUSIC_BANK_SLOT_COUNT, MUSIC_BANK_SLOT_COUNT, m_bank3_rows);
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
	std::size_t count, std::vector<SlotRow>& rows)
{
	auto* box = new wxStaticBoxSizer(wxVERTICAL, m_panel, title);
	auto* grid = new wxFlexGridSizer(GRID_COLUMNS * 2, wxSize(10, 4));

	rows.resize(count);
	for (std::size_t i = 0; i < count; ++i)
	{
		const std::size_t slot = first_slot + i;
		grid->Add(new wxStaticText(box->GetStaticBox(), wxID_ANY, wxString::Format("%02Xh", static_cast<unsigned>(slot))),
			0, wxALIGN_CENTER_VERTICAL);
		rows[i].choice = new wxChoice(box->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxSize(CHOICE_WIDTH, -1));
		rows[i].choice->Bind(wxEVT_CHOICE, [this, slot](wxCommandEvent&) { OnMusicSlotChanged(slot); });
		grid->Add(rows[i].choice, 0);
	}
	box->Add(grid, 0, wxALL, 6);
	sizer->Add(box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
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
	RefreshMusicChoices(m_bank4_rows, 0);
	RefreshMusicChoices(m_bank3_rows, MUSIC_BANK_SLOT_COUNT);
	RefreshSfxChoices();
	m_populating = false;
}

void AudioBankMappingFrame::OnMusicSlotChanged(std::size_t slot)
{
	if (m_populating || !m_gd)
	{
		return;
	}
	auto& rows = (slot < MUSIC_BANK_SLOT_COUNT) ? m_bank4_rows : m_bank3_rows;
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
