#include "PaletteListFrame.h"
#include "wx/textctrl.h"

PaletteListFrame::PaletteListFrame(wxWindow* parent)
	: EditorFrame(parent, wxID_ANY),
	  m_title(""),
	  m_mode(Mode::ROOM)
{
	m_mgr.SetManagedWindow(this);
	m_test = new wxTextCtrl(this, wxID_ANY, "");

	m_mgr.AddPane(m_test, wxAuiPaneInfo().CenterPane());

	// tell the manager to "commit" all the changes just made
	m_mgr.Update();
	UpdateUI();
}

PaletteListFrame::~PaletteListFrame()
{
}

void PaletteListFrame::SetMode(Mode mode)
{
    m_mode = mode;
    Update();
}

void PaletteListFrame::Update()
{
    if (m_gd != nullptr)
    {
        switch (m_mode)
        {
        case Mode::ROOM:
            m_test->SetValue("Room Palette");
            break;
        case Mode::ROOM_MISC:
            m_test->SetValue("Misc Room Palettes");
            break;
        default:
            m_test->SetValue("???");
            break;
        }
    }
}

void PaletteListFrame::SetGameData(std::shared_ptr<GameData> gd)
{
    m_gd = gd;
}

void PaletteListFrame::ClearGameData()
{
    m_gd = nullptr;
}

