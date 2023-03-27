#include "PaletteListFrame.h"

PaletteListFrame::PaletteListFrame(wxWindow* parent)
	: EditorFrame(parent, wxID_ANY),
	  m_title(""),
	  m_mode(Mode::ROOM)
{
	m_mgr.SetManagedWindow(this);
	m_test = new wxListBox(this, wxID_ANY);

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
    m_test->Clear();
    if (m_gd != nullptr)
    {
        switch (m_mode)
        {
        case Mode::ROOM:
            for (const auto& p : m_gd->GetRoomData()->GetRoomPalettes())
            {
                m_test->Append(p->GetName());
            }
            break;
        case Mode::ROOM_MISC:
            for (const auto& type : { RoomData::MiscPaletteType::LANTERN, RoomData::MiscPaletteType::LAVA,
                RoomData::MiscPaletteType::WARP })
            {
                for (const auto& p : m_gd->GetRoomData()->GetMiscPalette(type))
                {
                    m_test->Append(p->GetName());
                }
            }
            break;
        case Mode::MISC:
            for (const auto& p : m_gd->GetGraphicsData()->GetOtherPalettes())
            {
                m_test->Append(p.first);
            }
            break;
        case Mode::EQUIP:
            for (const auto& p : m_gd->GetGraphicsData()->GetSwordPalettes())
            {
                m_test->Append(p.first);
            }
            for (const auto& p : m_gd->GetGraphicsData()->GetArmourPalettes())
            {
                m_test->Append(p.first);
            }
            break;
        case Mode::SPRITE_HI:
            for (int i = 0; i < m_gd->GetSpriteData()->GetHiPaletteCount(); ++i)
            {
                m_test->Append(m_gd->GetSpriteData()->GetHiPalette(i)->GetName());
            }
            break;
        case Mode::SPRITE_LO:
            for (int i = 0; i < m_gd->GetSpriteData()->GetLoPaletteCount(); ++i)
            {
                m_test->Append(m_gd->GetSpriteData()->GetLoPalette(i)->GetName());
            }
            break;
        case Mode::PROJECTILE:
            for (int i = 0; i < m_gd->GetSpriteData()->GetProjectile1PaletteCount(); ++i)
            {
                m_test->Append(m_gd->GetSpriteData()->GetProjectile1Palette(i)->GetName());
            }
            for (int i = 0; i < m_gd->GetSpriteData()->GetProjectile2PaletteCount(); ++i)
            {
                m_test->Append(m_gd->GetSpriteData()->GetProjectile2Palette(i)->GetName());
            }
            break;
        default:
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

