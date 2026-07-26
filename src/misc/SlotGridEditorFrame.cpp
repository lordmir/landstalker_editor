#include <misc/SlotGridEditorFrame.h>

#include <algorithm>
#include <iterator>

#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include <misc/LookupChoiceControl.h>

SlotGridEditorFrame::SlotGridEditorFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst)
{
	m_mgr.SetManagedWindow(this);

	m_panel = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxVSCROLL | wxHSCROLL | wxTAB_TRAVERSAL);
	// Non-zero on both axes so a grid wider or taller than the pane gets scrollbars rather than
	// being clipped at the window edge.
	m_panel->SetScrollRate(16, 16);

	m_mgr.AddPane(m_panel, wxAuiPaneInfo().CenterPane());
	m_mgr.Update();
}

SlotGridEditorFrame::~SlotGridEditorFrame()
{
	m_mgr.UnInit();
}

bool SlotGridEditorFrame::Open()
{
	if (!m_gd)
	{
		return false;
	}
	RebuildGrid();
	return true;
}

void SlotGridEditorFrame::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	m_gd = gd;
	if (m_gd)
	{
		RebuildGrid();
	}
}

void SlotGridEditorFrame::ClearGameData()
{
	m_gd.reset();
	m_slots.clear();
	m_panel->DestroyChildren();
	m_panel->SetSizer(nullptr);
}

void SlotGridEditorFrame::RebuildGrid()
{
	const std::size_t rows = GetRows();
	const std::size_t cols = GetCols();

	m_panel->Freeze();
	m_slots.clear();
	m_panel->DestroyChildren();

	const wxArrayString choices = BuildChoices();

	auto* top = new wxBoxSizer(wxVERTICAL);
	top->Add(new wxStaticText(m_panel, wxID_ANY, GetIntroText()), 0, wxALL, 8);

	auto* grid = new wxFlexGridSizer(static_cast<int>(cols) + 1, wxSize(8, 4));
	grid->Add(new wxStaticText(m_panel, wxID_ANY, wxEmptyString), 0, wxALIGN_CENTER_VERTICAL);
	for (std::size_t c = 0; c < cols; ++c)
	{
		grid->Add(new wxStaticText(m_panel, wxID_ANY,
			wxString::Format("Column %u", static_cast<unsigned>(c + 1))), 0, wxALIGN_CENTER);
	}

	m_slots.resize(rows * cols, nullptr);
	for (std::size_t r = 0; r < rows; ++r)
	{
		grid->Add(new wxStaticText(m_panel, wxID_ANY,
			wxString::Format("Row %u", static_cast<unsigned>(r + 1))), 0, wxALIGN_CENTER_VERTICAL);
		for (std::size_t c = 0; c < cols; ++c)
		{
			const std::size_t slot = r * cols + c;
			auto* ctrl = new LookupChoiceControl(m_panel, wxID_ANY, wxEmptyString, choices,
				wxDefaultPosition, wxSize(240, -1));
			ctrl->Bind(wxEVT_CHOICE, &SlotGridEditorFrame::OnSlotChanged, this);
			m_slots[slot] = ctrl;
			grid->Add(ctrl, 0, wxEXPAND);
		}
	}

	top->Add(grid, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
	m_panel->SetSizer(top);

	LoadValues();

	m_panel->FitInside();
	m_panel->Layout();
	m_panel->Thaw();
}

void SlotGridEditorFrame::LoadValues()
{
	if (!m_gd)
	{
		return;
	}
	const std::vector<uint8_t> bytes = GetTableBytes();
	for (std::size_t slot = 0; slot < m_slots.size(); ++slot)
	{
		if (m_slots[slot] == nullptr)
		{
			continue;
		}
		// A slot the ROM table does not cover, or a byte no choice represents, is left blank in
		// the UI but untouched in the data unless the user edits it.
		int selection = wxNOT_FOUND;
		if (slot < bytes.size())
		{
			selection = ByteToSelection(bytes[slot]);
		}
		m_slots[slot]->SetSelection(selection);
	}
}

void SlotGridEditorFrame::OnSlotChanged(wxCommandEvent& evt)
{
	if (!m_gd)
	{
		return;
	}
	auto* ctrl = dynamic_cast<LookupChoiceControl*>(evt.GetEventObject());
	if (ctrl == nullptr)
	{
		return;
	}
	const auto it = std::find(m_slots.cbegin(), m_slots.cend(), ctrl);
	if (it == m_slots.cend())
	{
		return;
	}
	const std::size_t slot = static_cast<std::size_t>(std::distance(m_slots.cbegin(), it));

	const int selection = ctrl->GetSelection();
	if (selection == wxNOT_FOUND)
	{
		return;
	}
	const uint8_t value = SelectionToByte(selection);

	std::vector<uint8_t> bytes = GetTableBytes();
	if (slot < bytes.size() && bytes[slot] != value)
	{
		bytes[slot] = value;
		SetTableBytes(bytes);
	}
}
