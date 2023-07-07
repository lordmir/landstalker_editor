#include "PaletteListFrame.h"
#include <cstdint>

enum MENU_IDS
{
    ID_FILE_EXPORT = 21000,
    ID_FILE_IMPORT
};

PaletteListFrame::PaletteListFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst),
	  m_title(""),
	  m_mode(Mode::ROOM)
{
	m_mgr.SetManagedWindow(this);
	m_list = new wxDataViewCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER | wxDV_VARIABLE_LINE_HEIGHT | wxWANTS_CHARS);

    wxDataViewTextRenderer* tr = new wxDataViewTextRenderer("string", wxDATAVIEW_CELL_INERT);
    m_renderer = new DataViewCtrlPaletteRenderer(this, wxDATAVIEW_CELL_ACTIVATABLE);

    wxDataViewColumn* column0 = new wxDataViewColumn("Name", tr, 0, 200, wxALIGN_LEFT,
            wxDATAVIEW_COL_RESIZABLE);
    wxDataViewColumn* column1 = new wxDataViewColumn("Palette", m_renderer, 1, 30, wxALIGN_LEFT,
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
    m_list->GetMainWindow()->Disconnect(wxEVT_LEAVE_WINDOW, wxMouseEventHandler(PaletteListFrame::OnMouseLeave), nullptr, this);
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

bool PaletteListFrame::ExportAllPalettes(const filesystem::path& filename)
{
    bool retval = false;
    std::ofstream fs(filename.str());

    if (fs.good())
    {
        for (const auto& pal : m_gd->GetAllPalettes())
        {
            fs << pal.first;
            for (int i = 0; i < pal.second->GetData()->GetSize(); ++i)
            {
                fs << ", " << StrPrintf("#%06X", pal.second->GetData()->GetNthUnlockedColour(i).GetRGB(false));
            }
            fs << std::endl;
        }
        retval = true;
    }

    return retval;
}

bool PaletteListFrame::ImportPalettes(const filesystem::path& filename)
{
    bool retval = true;
    std::ifstream fs(filename.str(), std::ios::in);
    std::ostringstream errorss;

    if (fs.good())
    {
        std::string line;
        std::string value;
        std::string name;
        std::vector<std::string> colours;
        while (std::getline(fs, line))
        {
            std::istringstream iss(line);
            while (std::getline(iss, value, ','))
            {
                if (name.empty())
                {
                    name = RemoveQuotes(value);
                }
                else
                {
                    auto c = RemoveQuotes(value);
                    if (!c.empty())
                    {
                        colours.push_back(c);
                    }
                }
            }
            auto pal = m_gd->GetPalette(name);
            if (pal == nullptr)
            {
                retval = false;
                errorss << "Palette with name \"" << name << "\" does not exist!" << std::endl;
            }
            else if(pal->GetData()->GetSize() != colours.size())
            {
                retval = false;
                errorss <<  name << ": Bad palette size! Expected " << pal->GetData()->GetSize()
                        << " entries, got " << colours.size() << " entries." << std::endl;
            }
            else
            {
                Palette::Colour colour;
                uint32_t buf;
                int i = 0;
                for (const auto& col : colours)
                {
                    if (!StrToHex(col, buf) || buf > 0xFFFFFF)
                    {
                        errorss << name << ": Bad Hex colour value \"" << col << "\"" << std::endl;
                        retval = false;
                        break;
                    }
                    else
                    {
                        colour.FromRGB(buf);
                        pal->GetData()->SetNthUnlockedGenesisColour(i++, colour.GetGenesis());
                    }
                }
            }
            name = "";
            colours.clear();
        }
    }
    
    if (!retval)
    {
        wxMessageBox("Errors during import!\n" + errorss.str(), "Errors during import", wxICON_ERROR);
    }

    return retval;
}

void PaletteListFrame::UpdateStatusBar(wxStatusBar& status, wxCommandEvent& evt) const
{
    int sel_colour = m_renderer->GetCursorPosition();
    auto sel_item = m_list->GetSelection();
    if(sel_item != m_prev_itm || sel_colour != m_prev_colour)
    {
        if(sel_item.IsOk() && sel_colour >= 0)
        {
            int row = reinterpret_cast<intptr_t>(sel_item.GetID()) - 1;
            wxVariant data;
            m_model->GetValueByRow(data, row, 1);
            PaletteEntry* p = static_cast<PaletteEntry*>(data.GetVoidPtr());
            if(sel_colour < p->GetData()->GetSize())
            {
                const auto& c = p->GetData()->GetNthUnlockedColour(sel_colour);
                status.SetStatusText(StrPrintf("%s, Index %d - Genesis: 0x%04X, RGB: #%06X",
                    p->GetName().c_str(), p->GetData()->GetNthUnlockedIndex(sel_colour), c.GetGenesis(), c.GetRGB(false)), 0);
            }
        }
        else
        {
            status.SetStatusText("");
        }
        m_prev_itm = sel_item;
        m_prev_colour = sel_colour;
    }
}

void PaletteListFrame::InitMenu(wxMenuBar& menu, ImageList& ilist) const
{
    auto* parent = m_mgr.GetManagedWindow();

    ClearMenu(menu);
    auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
    AddMenuItem(fileMenu, 0, ID_FILE_EXPORT, "Export All Palettes...");
    AddMenuItem(fileMenu, 1, ID_FILE_IMPORT, "Import Palettes...");
    UpdateUI();
    m_mgr.Update();
}

void PaletteListFrame::OnMenuClick(wxMenuEvent& evt)
{
    const auto id = evt.GetId();
    if ((id >= 21000) && (id < 31000))
    {
        switch (id)
        {
        case ID_FILE_EXPORT:
            OnMenuExport();
            break;
        case ID_FILE_IMPORT:
            OnMenuImport();
            break;
        default:
            wxMessageBox(wxString::Format("Unrecognised Event %d", evt.GetId()));
        }
        UpdateUI();
    }
}

void PaletteListFrame::OnMouseMove(wxMouseEvent& evt)
{
    // The HitTest() function uses this way to convert the coordinates
    wxPoint pos = m_list->ScreenToClient(m_list->GetMainWindow()->ClientToScreen(evt.GetPosition()));
    wxDataViewItem itm;
    wxDataViewColumn* col;
    m_list->HitTest(pos, itm, col);

    if (itm.IsOk())
    {
        int colour = 0;
        if (col->GetModelColumn() == 1)
        {
            auto rect = m_list->GetItemRect(itm, col);
            colour = m_renderer->HitColour({evt.GetX() - rect.GetX(), evt.GetY()}, false);
        }
        if (itm != m_prev_itm)
        {
            m_list->Select(itm);
        }
        if (colour != m_prev_colour)
        {
            m_renderer->SetCursorPosition(colour);
            m_model->ValueChanged(itm, 1);
        }
        FireEvent(EVT_STATUSBAR_UPDATE);
    }
    else
    {
        FireEvent(EVT_STATUSBAR_UPDATE);
    }
    evt.Skip();
}

void PaletteListFrame::OnMouseLeave(wxMouseEvent& evt)
{
    FireEvent(EVT_STATUSBAR_UPDATE);
    evt.Skip();
}

void PaletteListFrame::OnKeyPress(wxKeyEvent& evt)
{
    switch (evt.GetKeyCode())
    {
    case WXK_LEFT:
        m_renderer->MoveCursorLeft();
        m_list->Refresh(true);
        m_model->ValueChanged(m_list->GetSelection(), 1);
        FireEvent(EVT_STATUSBAR_UPDATE);
        evt.Skip(false);
        break;
    case WXK_RIGHT:
        m_renderer->MoveCursorRight();
        m_list->Refresh(true);
        m_model->ValueChanged(m_list->GetSelection(), 1);
        FireEvent(EVT_STATUSBAR_UPDATE);
        evt.Skip(false);
        break;
    default:
        FireEvent(EVT_STATUSBAR_UPDATE);
        evt.Skip();
    }
}

void PaletteListFrame::OnMenuImport()
{
    wxFileDialog fd(this, _("Import CSV Palette Data"), "", "", "CSV Files (*.csv)|*.csv|All Files (*.*)|*.*",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (fd.ShowModal() != wxID_CANCEL)
    {
        std::string path = fd.GetPath().ToStdString();
        ImportPalettes(path);

        Refresh();
    }
}

void PaletteListFrame::OnMenuExport()
{
    wxFileDialog fd(this, _("Export CSV Palette Data"), "", "palettes.csv", "CSV Files (*.csv)|*.csv|All Files (*.*)|*.*",
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (fd.ShowModal() != wxID_CANCEL)
    {
        std::string path = fd.GetPath().ToStdString();
        ExportAllPalettes(path);
        Update();
    }
}