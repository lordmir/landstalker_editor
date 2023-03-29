#include "PaletteListFrame.h"

PaletteListFrame::PaletteListFrame(wxWindow* parent)
	: EditorFrame(parent, wxID_ANY),
	  m_title(""),
	  m_mode(Mode::ROOM)
{
	m_mgr.SetManagedWindow(this);
	m_list = new wxDataViewCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_VARIABLE_LINE_HEIGHT);

    wxDataViewTextRenderer* tr = new wxDataViewTextRenderer("string", wxDATAVIEW_CELL_INERT);
    m_renderer = new DataViewCtrlPaletteRenderer(this, wxDATAVIEW_CELL_ACTIVATABLE);

    wxDataViewColumn* column0 = new wxDataViewColumn("Name", tr, 0, FromDIP(200), wxALIGN_LEFT,
            wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_RESIZABLE);
    wxDataViewColumn* column1 = new wxDataViewColumn("Palette", m_renderer, 1, 47 * 35, wxALIGN_LEFT,
            wxDATAVIEW_COL_RESIZABLE);

    m_list->AppendColumn(column0);
    m_list->AppendColumn(column1);
    m_list->Connect(wxEVT_CHAR, wxKeyEventHandler(PaletteListFrame::OnKeyPress), nullptr, this);
    m_list->GetMainWindow()->Connect(wxEVT_MOTION, wxMouseEventHandler(PaletteListFrame::OnMouseMove), nullptr, this);
    m_list->GetMainWindow()->Connect(wxEVT_LEAVE_WINDOW, wxMouseEventHandler(PaletteListFrame::OnMouseLeave), nullptr, this);

	m_mgr.AddPane(m_list, wxAuiPaneInfo().CenterPane());

	// tell the manager to "commit" all the changes just made
	m_mgr.Update();
	UpdateUI();
}

PaletteListFrame::~PaletteListFrame()
{
    m_list->Disconnect(wxEVT_CHAR, wxKeyEventHandler(PaletteListFrame::OnKeyPress), nullptr, this);
    m_list->GetMainWindow()->Disconnect(wxEVT_MOTION, wxMouseEventHandler(PaletteListFrame::OnMouseMove), nullptr, this);
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
        std::vector<std::shared_ptr<PaletteEntry>> entries;
        switch (m_mode)
        {
        case Mode::ROOM:
            for (const auto& p : m_gd->GetRoomData()->GetRoomPalettes())
            {
                entries.push_back(p);
            }
            break;
        case Mode::ROOM_MISC:
            for (const auto& type : { RoomData::MiscPaletteType::LANTERN, RoomData::MiscPaletteType::LAVA,
                RoomData::MiscPaletteType::WARP })
            {
                for (const auto& p : m_gd->GetRoomData()->GetMiscPalette(type))
                {
                    entries.push_back(p);
                }
            }
            break;
        case Mode::MISC:
            for (const auto& p : m_gd->GetGraphicsData()->GetOtherPalettes())
            {
                entries.push_back(p.second);
            }
            break;
        case Mode::EQUIP:
            for (const auto& p : m_gd->GetGraphicsData()->GetSwordPalettes())
            {
                entries.push_back(p.second);
            }
            for (const auto& p : m_gd->GetGraphicsData()->GetArmourPalettes())
            {
                entries.push_back(p.second);
            }
            break;
        case Mode::SPRITE_HI:
            for (int i = 0; i < m_gd->GetSpriteData()->GetHiPaletteCount(); ++i)
            {
                entries.push_back(m_gd->GetSpriteData()->GetHiPalette(i));
            }
            break;
        case Mode::SPRITE_LO:
            for (int i = 0; i < m_gd->GetSpriteData()->GetLoPaletteCount(); ++i)
            {
                entries.push_back(m_gd->GetSpriteData()->GetLoPalette(i));
            }
            break;
        case Mode::PROJECTILE:
            for (int i = 0; i < m_gd->GetSpriteData()->GetProjectile1PaletteCount(); ++i)
            {
                entries.push_back(m_gd->GetSpriteData()->GetProjectile1Palette(i));
            }
            for (int i = 0; i < m_gd->GetSpriteData()->GetProjectile2PaletteCount(); ++i)
            {
                entries.push_back(m_gd->GetSpriteData()->GetProjectile2Palette(i));
            }
            break;
        default:
            break;
        }
        m_renderer->Reset();
        m_model = new DataViewCtrlPaletteModel(entries);
        m_list->AssociateModel(m_model);
        m_model->DecRef();
        m_list->GetColumn(1)->SetWidth(m_renderer->GetTotalWidth(m_model->GetColumnMaxElements()));
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

void PaletteListFrame::UpdateStatusBar(wxStatusBar& status) const
{
    status.SetStatusText(m_status_text, 0);
}

void PaletteListFrame::OnMouseMove(wxMouseEvent& evt)
{
    // The HitTest() function uses this way to convert the coordinates
    wxPoint pos = m_list->ScreenToClient(m_list->GetMainWindow()->ClientToScreen(evt.GetPosition()));

    bool show_tip = false;
    wxDataViewItem itm;
    wxDataViewColumn* col;
    m_list->HitTest(pos, itm, col);

    if (itm.IsOk() && col->GetModelColumn() == 1)
    {
        auto rect = m_list->GetItemRect(itm, col);
        int colour = m_renderer->HitColour({evt.GetX() - rect.GetX(), evt.GetY()}, false);
        if (itm != m_prev_itm || colour != m_prev_colour)
        {
            m_prev_itm = itm;
            m_prev_colour = colour;
            wxVariant data;
            m_model->GetValueByRow(data, (int)itm.GetID() - 1, 1);
            PaletteEntry* p = static_cast<PaletteEntry*>(data.GetVoidPtr());
            wxLogDebug("%s - %d", p->GetName(), colour);
            if (colour >= 0 && colour < p->GetData()->GetSize())
            {
                const auto& c = p->GetData()->GetNthUnlockedColour(colour);
                m_list->Select(itm);
                m_renderer->SetCursorPosition(colour);
                m_list->GetMainWindow()->SetToolTip(wxString::Format("%s, Index %d\n0x%04X, #%06X",
                    p->GetName(), p->GetData()->GetNthUnlockedIndex(colour), c.GetGenesis(), c.GetRGB(false)));
                m_status_text = StrPrintf("%s, Index %d - Genesis: 0x%04X, RGB: #%06X",
                    p->GetName().c_str(), p->GetData()->GetNthUnlockedIndex(colour), c.GetGenesis(), c.GetRGB(false));
                show_tip = true;
                m_showing_tip = true;
                FireEvent(EVT_STATUSBAR_UPDATE);
            }
            else
            {
                m_list->GetMainWindow()->SetToolTip("");
                m_status_text = "";
                FireEvent(EVT_STATUSBAR_UPDATE);
                m_showing_tip = false;
            }
        }
    }
    else
    {
        m_list->GetMainWindow()->SetToolTip("");
        m_status_text = "";
        FireEvent(EVT_STATUSBAR_UPDATE);
        m_showing_tip = false;
    }
    evt.Skip();
}

void PaletteListFrame::OnMouseLeave(wxMouseEvent& evt)
{
    m_list->GetMainWindow()->SetToolTip("");
    m_status_text = "";
    FireEvent(EVT_STATUSBAR_UPDATE);
    m_showing_tip = false;
    evt.Skip();
}

void PaletteListFrame::OnKeyPress(wxKeyEvent& evt)
{
    switch (evt.GetKeyCode())
    {
    case WXK_LEFT:
        m_renderer->MoveCursorLeft();
        m_list->Refresh();
        break;
    case WXK_RIGHT:
        m_renderer->MoveCursorRight();
        m_list->Refresh();
        break;
    }
    evt.Skip();
}

