#include <misc/DamageConstantsFrame.h>

#include <array>
#include <cmath>
#include <string>

#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/statbox.h>
#include <wx/stattext.h>

#include <landstalker/main/SpriteData.h>
#include <misc/SpinCtrlSize.h>

namespace
{
	// The damage.inc constants in ROM order, with a friendly label and which group (charged-sword
	// boost vs armour defence) each belongs to. The names match the equ labels the game data is
	// keyed by (see SpriteData); the split governs only how they are grouped in the UI.
	struct ConstantEntry { const char* name; const char* label; bool sword; };
	const std::array<ConstantEntry, 9> DAMAGE_CONSTANTS = {{
		{ "MAGIC_SWORD_BOOST",      "Magic Sword",    true  },
		{ "ICE_SWORD_BOOST",        "Ice Sword",      true  },
		{ "THUNDER_SWORD_BOOST",    "Thunder Sword",  true  },
		{ "GAIA_SWORD_BOOST",       "Gaia Sword",     true  },
		{ "LEATHER_BREAST_DEFENCE", "Leather Breast", false },
		{ "STEEL_BREAST_DEFENCE",   "Steel Breast",   false },
		{ "CHROME_BREAST_DEFENCE",  "Chrome Breast",  false },
		{ "SHELL_BREAST_DEFENCE",   "Shell Breast",   false },
		{ "HYPER_BREAST_DEFENCE",   "Hyper Breast",   false },
	}};

	constexpr double FIXED_POINT_SCALE = 256.0; // raw value = fraction * 256
	constexpr int RAW_MAX = 0xFFFF;             // constants are stored as 16-bit words
	// Percentage the spin controls display and step in. One raw unit is 1/256, i.e. 100/256 %.
	constexpr double PERCENT_MAX = RAW_MAX * 100.0 / FIXED_POINT_SCALE;
	constexpr double PERCENT_STEP = 100.0 / FIXED_POINT_SCALE;

	double RawToPercent(uint16_t raw)
	{
		return raw * 100.0 / FIXED_POINT_SCALE;
	}

	uint16_t PercentToRaw(double percent)
	{
		const long raw = std::lround(percent * FIXED_POINT_SCALE / 100.0);
		return static_cast<uint16_t>(std::min<long>(RAW_MAX, std::max<long>(0, raw)));
	}
}

DamageConstantsFrame::DamageConstantsFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst)
{
	m_mgr.SetManagedWindow(this);

	m_panel = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxVSCROLL | wxHSCROLL | wxTAB_TRAVERSAL);
	m_panel->SetScrollRate(16, 16);

	m_mgr.AddPane(m_panel, wxAuiPaneInfo().CenterPane());
	m_mgr.Update();

	BuildUI();
}

DamageConstantsFrame::~DamageConstantsFrame()
{
	m_mgr.UnInit();
}

void DamageConstantsFrame::BuildUI()
{
	m_panel->Freeze();
	m_spins.assign(DAMAGE_CONSTANTS.size(), nullptr);

	auto* top = new wxBoxSizer(wxVERTICAL);
	top->Add(new wxStaticText(m_panel, wxID_ANY,
		"Damage modifiers, as a percentage of the base value. Charged-hit sword attacks are "
		"multiplied by their boost; armour multiplies incoming damage by its defence."),
		0, wxALL, 8);

	// One labelled group per constant category, each a two-column (name, percentage) grid.
	auto add_group = [this](wxWindow* parent, const wxString& title, bool sword)
	{
		auto* box = new wxStaticBoxSizer(wxVERTICAL, parent, title);
		auto* grid = new wxFlexGridSizer(3, wxSize(8, 4));
		for (std::size_t i = 0; i < DAMAGE_CONSTANTS.size(); ++i)
		{
			if (DAMAGE_CONSTANTS[i].sword != sword)
			{
				continue;
			}
			grid->Add(new wxStaticText(box->GetStaticBox(), wxID_ANY, DAMAGE_CONSTANTS[i].label),
				0, wxALIGN_CENTER_VERTICAL);
			auto* spin = new wxSpinCtrlDouble(box->GetStaticBox(), wxID_ANY, wxEmptyString,
				wxDefaultPosition, SpinCtrlSize(110), wxSP_ARROW_KEYS, 0.0, PERCENT_MAX, 0.0, PERCENT_STEP);
			spin->SetDigits(2);
			grid->Add(spin, 0, wxALIGN_CENTER_VERTICAL);
			grid->Add(new wxStaticText(box->GetStaticBox(), wxID_ANY, "%"), 0, wxALIGN_CENTER_VERTICAL);

			// Commit on a settled value (spin buttons) or focus loss, not per keystroke; there is no
			// rebuild here, so this just avoids writing transient half-typed values into the data.
			spin->Bind(wxEVT_SPINCTRLDOUBLE, [this, i](wxSpinDoubleEvent&) { CommitSpin(i); });
			spin->Bind(wxEVT_KILL_FOCUS, [this, i](wxFocusEvent& e) { e.Skip(); CommitSpin(i); });
			m_spins[i] = spin;
		}
		box->Add(grid, 0, wxALL, 6);
		return box;
	};

	top->Add(add_group(m_panel, "Magic Sword Attacks", true), 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 8);
	top->Add(add_group(m_panel, "Armour", false), 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 8);

	m_panel->SetSizer(top);
	m_panel->FitInside();
	m_panel->Layout();
	m_panel->Thaw();
}

bool DamageConstantsFrame::Open()
{
	if (!m_gd)
	{
		return false;
	}
	LoadValues();
	return true;
}

void DamageConstantsFrame::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	m_gd = gd;
	LoadValues();
}

void DamageConstantsFrame::ClearGameData()
{
	m_gd.reset();
	for (auto* spin : m_spins)
	{
		if (spin)
		{
			spin->Enable(false);
		}
	}
}

void DamageConstantsFrame::LoadValues()
{
	const bool have_data = static_cast<bool>(m_gd);
	for (std::size_t i = 0; i < m_spins.size(); ++i)
	{
		if (!m_spins[i])
		{
			continue;
		}
		m_spins[i]->Enable(have_data);
		if (have_data)
		{
			const uint16_t raw = m_gd->GetSpriteData()->GetDamageConstant(DAMAGE_CONSTANTS[i].name);
			m_spins[i]->SetValue(RawToPercent(raw));
		}
	}
}

void DamageConstantsFrame::CommitSpin(std::size_t index)
{
	if (!m_gd || index >= m_spins.size() || !m_spins[index])
	{
		return;
	}
	const uint16_t raw = PercentToRaw(m_spins[index]->GetValue());
	auto sd = m_gd->GetSpriteData();
	if (sd->GetDamageConstant(DAMAGE_CONSTANTS[index].name) != raw)
	{
		sd->SetDamageConstant(DAMAGE_CONSTANTS[index].name, raw);
	}
}

void DamageConstantsFrame::CommitPendingEdits()
{
	for (std::size_t i = 0; i < m_spins.size(); ++i)
	{
		CommitSpin(i);
	}
}
