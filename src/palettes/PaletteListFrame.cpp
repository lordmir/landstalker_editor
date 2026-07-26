#include <palettes/PaletteListFrame.h>
#include <misc/DataViewModelAssociate.h>
#include <algorithm>
#include <cstdint>
#include <optional>
#include <wx/button.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/textdlg.h>
#include <wx/msgdlg.h>
#include <wx/numdlg.h>
#include <landstalker/misc/Labels.h>

enum MENU_IDS
{
    ID_FILE_EXPORT = 21000,
    ID_FILE_IMPORT,
    ID_FILE_IMPORT_PNG_NEW,
    ID_FILE_IMPORT_PNG_OVERWRITE
};

PaletteListFrame::PaletteListFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst),
	  m_mode(Mode::ROOM),
      m_list(nullptr),
      m_model(nullptr),
      m_renderer(nullptr),
      m_prev_colour(-1),
	  m_title(""),
      m_button_panel(nullptr),
      m_add(nullptr),
      m_remove(nullptr),
      m_move_up(nullptr),
      m_move_down(nullptr),
      m_rename(nullptr)
{
	m_mgr.SetManagedWindow(this);

    // The list and its buttons share a panel so the buttons have somewhere to sit; a right-hand
    // column keeps them out of the way of the hover-to-select swatches - moving the mouse
    // sideways to a button does not sweep across other rows and change the selection.
    wxPanel* content = new wxPanel(this, wxID_ANY);
    wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
    content->SetSizer(hsizer);

	m_list = new wxDataViewCtrl(content, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER | wxDV_VARIABLE_LINE_HEIGHT | wxWANTS_CHARS);

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
    m_list->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &PaletteListFrame::OnListContextMenu, this);
    hsizer->Add(m_list, 1, wxALL | wxEXPAND, 5);

    m_button_panel = new wxPanel(content, wxID_ANY);
    wxBoxSizer* bsizer = new wxBoxSizer(wxVERTICAL);
    m_button_panel->SetSizer(bsizer);
    m_add = new wxButton(m_button_panel, wxID_ANY, "Add");
    m_remove = new wxButton(m_button_panel, wxID_ANY, "Remove");
    m_move_up = new wxButton(m_button_panel, wxID_ANY, "Move Up");
    m_move_down = new wxButton(m_button_panel, wxID_ANY, "Move Down");
    m_rename = new wxButton(m_button_panel, wxID_ANY, "Rename...");
    for (auto* b : { m_add, m_remove, m_move_up, m_move_down, m_rename })
    {
        bsizer->Add(b, 0, wxEXPAND | wxBOTTOM, 4);
    }
    m_add->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnAddPalette(); });
    m_remove->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnRemovePalette(); });
    m_move_up->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnMovePalette(-1); });
    m_move_down->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnMovePalette(1); });
    m_rename->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnRenamePalette(); });
    hsizer->Add(m_button_panel, 0, wxALL | wxEXPAND, 5);
    m_button_panel->Hide();

	m_mgr.AddPane(content, wxAuiPaneInfo().CenterPane());

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
        std::vector<std::shared_ptr<Landstalker::PaletteEntry>> entries;
        std::vector<std::wstring> labels;
        switch (m_mode)
        {
        case Mode::ROOM:
            for (std::size_t i = 0; i < m_gd->GetRoomData()->GetRoomPalettes().size(); ++i)
            {
                entries.push_back(m_gd->GetRoomData()->GetRoomPalette(i));
                labels.push_back(m_gd->GetRoomData()->GetRoomPaletteDisplayName(i));
            }
            break;
        case Mode::ROOM_MISC:
            for (const auto& type : {
                    Landstalker::RoomData::MiscPaletteType::LANTERN,
                    Landstalker::RoomData::MiscPaletteType::LAVA,
                    Landstalker::RoomData::MiscPaletteType::WARP })
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
                labels.push_back(m_gd->GetSpriteData()->GetSpriteHighPaletteDisplayName(i));
            }
            break;
        case Mode::SPRITE_LO:
            for (int i = 0; i < m_gd->GetSpriteData()->GetLoPaletteCount(); ++i)
            {
                entries.push_back(m_gd->GetSpriteData()->GetLoPalette(i));
                labels.push_back(m_gd->GetSpriteData()->GetSpriteLowPaletteDisplayName(i));
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
        m_list->UnselectAll();
        m_model = new DataViewCtrlPaletteModel(entries, labels);
        AssociateDataViewModel(m_list, m_model);
        m_model->DecRef();
        m_list->GetColumn(1)->SetWidth(m_renderer->GetTotalWidth(m_model->GetColumnMaxElements()));
    }
    if (m_button_panel)
    {
        m_button_panel->Show(m_gd != nullptr && IsEditableMode());
        m_button_panel->GetParent()->Layout();
    }
    UpdatePaletteButtons();
}

bool PaletteListFrame::IsEditableMode() const
{
    return m_mode == Mode::ROOM || m_mode == Mode::SPRITE_LO || m_mode == Mode::SPRITE_HI;
}

bool PaletteListFrame::CanAddPalette() const
{
    if (!m_gd || !IsEditableMode())
    {
        return false;
    }
    const std::size_t cap = (m_mode == Mode::ROOM)
        ? Landstalker::RoomData::MAX_ROOM_PALETTES : Landstalker::SpriteData::MAX_SPRITE_PALETTES;
    return GetPaletteCount() < cap;
}

std::size_t PaletteListFrame::GetPaletteCount() const
{
    if (!m_gd)
    {
        return 0;
    }
    switch (m_mode)
    {
    case Mode::ROOM:      return m_gd->GetRoomData()->GetRoomPalettes().size();
    case Mode::SPRITE_LO: return m_gd->GetSpriteData()->GetLoPaletteCount();
    case Mode::SPRITE_HI: return m_gd->GetSpriteData()->GetHiPaletteCount();
    default:              return 0;
    }
}

const std::wstring& PaletteListFrame::PaletteLabelCategory() const
{
    switch (m_mode)
    {
    case Mode::SPRITE_LO: return Landstalker::Labels::C_LOW_PALETTES;
    case Mode::SPRITE_HI: return Landstalker::Labels::C_HIGH_PALETTES;
    case Mode::ROOM:
    default:              return Landstalker::Labels::C_ROOM_PALETTES;
    }
}

std::wstring PaletteListFrame::PaletteDisplayName(int row) const
{
    switch (m_mode)
    {
    case Mode::SPRITE_LO: return m_gd->GetSpriteData()->GetSpriteLowPaletteDisplayName(static_cast<uint8_t>(row));
    case Mode::SPRITE_HI: return m_gd->GetSpriteData()->GetSpriteHighPaletteDisplayName(static_cast<uint8_t>(row));
    case Mode::ROOM:
    default:              return m_gd->GetRoomData()->GetRoomPaletteDisplayName(static_cast<uint8_t>(row));
    }
}

int PaletteListFrame::GetSelectedRow() const
{
    const auto sel = m_list->GetSelection();
    if (!sel.IsOk())
    {
        return -1;
    }
    const int row = static_cast<int>(reinterpret_cast<std::intptr_t>(sel.GetID())) - 1;
    return (row >= 0 && row < static_cast<int>(GetPaletteCount())) ? row : -1;
}

void PaletteListFrame::SelectRow(int row)
{
    if (row < 0 || row >= static_cast<int>(GetPaletteCount()))
    {
        return;
    }
    const auto item = wxDataViewItem(reinterpret_cast<void*>(static_cast<std::intptr_t>(row) + 1));
    m_list->Select(item);
    m_list->EnsureVisible(item);
}

void PaletteListFrame::UpdatePaletteButtons()
{
    if (!m_button_panel)
    {
        return;
    }
    const bool editable = m_gd != nullptr && IsEditableMode();
    const int count = static_cast<int>(GetPaletteCount());
    const int row = GetSelectedRow();
    const bool sel = editable && row >= 0;

    m_add->Enable(CanAddPalette());
    // A palette can only go once nothing draws it, and the last one can never go; the in-use
    // refusal is reported on click so the user learns what is blocking it.
    m_remove->Enable(sel && count > 1);
    m_move_up->Enable(sel && row > 0);
    m_move_down->Enable(sel && row < count - 1);
    m_rename->Enable(sel);
}

void PaletteListFrame::OnAddPalette()
{
    if (!m_gd || !IsEditableMode())
    {
        return;
    }
    std::optional<uint8_t> added;
    switch (m_mode)
    {
    case Mode::ROOM:      added = m_gd->GetRoomData()->AddRoomPalette(); break;
    case Mode::SPRITE_LO: added = m_gd->GetSpriteData()->AddLoPalette(); break;
    case Mode::SPRITE_HI: added = m_gd->GetSpriteData()->AddHiPalette(); break;
    default: return;
    }
    if (!added)
    {
        wxMessageBox("The palette list is full.", "Add Palette", wxOK | wxICON_ERROR, this);
        return;
    }
    Update();
    SelectRow(*added);
    UpdatePaletteButtons();
}

void PaletteListFrame::OnRemovePalette()
{
    if (!m_gd || !IsEditableMode())
    {
        return;
    }
    const int row = GetSelectedRow();
    if (row < 0)
    {
        return;
    }
    const auto name = wxString(PaletteDisplayName(row));

    // Refuse while the palette is still referenced, naming the first few users. Build the list
    // by concatenation rather than Format to avoid mixing narrow literals into %s.
    std::size_t user_count = 0;
    wxString user_list;
    const auto append = [&user_list](const wxString& item)
    {
        if (!user_list.IsEmpty()) user_list += ", ";
        user_list += item;
    };
    if (m_mode == Mode::ROOM)
    {
        const auto rooms = m_gd->GetRoomData()->GetRoomsUsingRoomPalette(static_cast<uint8_t>(row));
        user_count = rooms.size();
        for (std::size_t i = 0; i < rooms.size() && i < 3; ++i)
        {
            append(wxString(m_gd->GetRoomData()->GetRoomDisplayName(rooms[i])));
        }
    }
    else
    {
        const auto entities = (m_mode == Mode::SPRITE_LO)
            ? m_gd->GetSpriteData()->GetEntitiesUsingLoPalette(static_cast<uint8_t>(row))
            : m_gd->GetSpriteData()->GetEntitiesUsingHiPalette(static_cast<uint8_t>(row));
        user_count = entities.size();
        for (std::size_t i = 0; i < entities.size() && i < 3; ++i)
        {
            append(wxString(Landstalker::SpriteData::GetEntityDisplayName(entities[i])));
        }
    }
    if (user_count > 3)
    {
        append(wxString::Format("and %d more", static_cast<int>(user_count - 3)));
    }
    if (user_count > 0)
    {
        const wxString what = (m_mode == Mode::ROOM)
            ? (user_count == 1 ? wxString("1 room") : wxString::Format("%d rooms", static_cast<int>(user_count)))
            : (user_count == 1 ? wxString("1 entity") : wxString::Format("%d entities", static_cast<int>(user_count)));
        wxMessageBox("'" + name + "' cannot be deleted: " + what + " still use it.\n\n  " + user_list +
            "\n\nRepoint them at another palette first.", "Remove Palette", wxOK | wxICON_ERROR, this);
        return;
    }

    if (wxMessageBox(wxString::Format("Delete palette '%s'?\n\nThis cannot be undone. Palettes "
        "above it move down one slot.", name),
        "Remove Palette", wxYES_NO | wxNO_DEFAULT | wxICON_WARNING, this) != wxYES)
    {
        return;
    }

    bool ok = false;
    switch (m_mode)
    {
    case Mode::ROOM:      ok = m_gd->GetRoomData()->DeleteRoomPalette(static_cast<uint8_t>(row)); break;
    case Mode::SPRITE_LO: ok = m_gd->GetSpriteData()->DeleteLoPalette(static_cast<uint8_t>(row)); break;
    case Mode::SPRITE_HI: ok = m_gd->GetSpriteData()->DeleteHiPalette(static_cast<uint8_t>(row)); break;
    default: return;
    }
    if (!ok)
    {
        wxMessageBox("Unable to delete the palette.", "Remove Palette", wxOK | wxICON_ERROR, this);
        return;
    }
    Update();
    SelectRow(std::min(row, static_cast<int>(GetPaletteCount()) - 1));
    UpdatePaletteButtons();
}

void PaletteListFrame::OnMovePalette(int delta)
{
    if (!m_gd || !IsEditableMode())
    {
        return;
    }
    const int row = GetSelectedRow();
    const int nrow = row + delta;
    if (row < 0 || nrow < 0 || nrow >= static_cast<int>(GetPaletteCount()))
    {
        return;
    }
    bool ok = false;
    switch (m_mode)
    {
    case Mode::ROOM:      ok = m_gd->GetRoomData()->SwapRoomPalettes(static_cast<uint8_t>(row), static_cast<uint8_t>(nrow)); break;
    case Mode::SPRITE_LO: ok = m_gd->GetSpriteData()->SwapLoPalettes(static_cast<uint8_t>(row), static_cast<uint8_t>(nrow)); break;
    case Mode::SPRITE_HI: ok = m_gd->GetSpriteData()->SwapHiPalettes(static_cast<uint8_t>(row), static_cast<uint8_t>(nrow)); break;
    default: return;
    }
    if (ok)
    {
        Update();
        SelectRow(nrow);
        UpdatePaletteButtons();
    }
}

void PaletteListFrame::OnRenamePalette()
{
    if (!m_gd || !IsEditableMode())
    {
        return;
    }
    const int row = GetSelectedRow();
    if (row < 0)
    {
        return;
    }
    const auto old_name = PaletteDisplayName(row);
    wxTextEntryDialog dlg(this, "Display name for this palette (used only in the editor):",
        "Rename Palette", wxString(old_name));
    while (dlg.ShowModal() == wxID_OK)
    {
        const auto name = dlg.GetValue().ToStdWstring();
        if (name == old_name)
        {
            return;
        }
        if (!Landstalker::Labels::IsValid(name, PaletteLabelCategory(), row))
        {
            wxMessageBox("The name must not be empty, must be unique, and must not contain "
                "non-printable characters.", "Rename Palette", wxOK | wxICON_ERROR, this);
            continue;
        }
        Landstalker::Labels::Update(PaletteLabelCategory(), row, name);
        Update();
        SelectRow(row);
        return;
    }
}

void PaletteListFrame::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
    m_gd = gd;
}

void PaletteListFrame::ClearGameData()
{
    m_gd = nullptr;
}

bool PaletteListFrame::ExportAllPalettes(const std::filesystem::path& filename)
{
    bool retval = false;
    std::ofstream fs(filename.string());

    if (fs.good())
    {
        for (const auto& pal : m_gd->GetAllPalettes())
        {
            fs << pal.first;
            for (int i = 0; i < pal.second->GetData()->GetSize(); ++i)
            {
                fs << ", " << Landstalker::StrPrintf("#%06X", pal.second->GetData()->GetNthUnlockedColour(i).GetRGB(false));
            }
            fs << std::endl;
        }
        retval = true;
    }

    return retval;
}

bool PaletteListFrame::ImportPalettes(const std::filesystem::path& filename)
{
    bool retval = true;
    std::ifstream fs(filename.string(), std::ios::in);
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
                    name = Landstalker::RemoveQuotes(value);
                }
                else
                {
                    auto c = Landstalker::RemoveQuotes(value);
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
            else if(pal->GetData()->GetSize() != static_cast<int>(colours.size()))
            {
                retval = false;
                errorss <<  name << ": Bad palette size! Expected " << pal->GetData()->GetSize()
                        << " entries, got " << colours.size() << " entries." << std::endl;
            }
            else
            {
                Landstalker::Palette::Colour colour;
                uint32_t buf;
                int i = 0;
                for (const auto& col : colours)
                {
                    if (!Landstalker::StrToHex(col, buf) || buf > 0xFFFFFF)
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

void PaletteListFrame::UpdateStatusBar(wxStatusBar& status, wxCommandEvent& /*evt*/) const
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
            Landstalker::PaletteEntry* p = static_cast<Landstalker::PaletteEntry*>(data.GetVoidPtr());
            if(sel_colour < p->GetData()->GetSize())
            {
                const auto& c = p->GetData()->GetNthUnlockedColour(sel_colour);
                status.SetStatusText(Landstalker::StrPrintf("%s, Index %d - Genesis: 0x%04X, RGB: #%06X",
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

void PaletteListFrame::InitMenu(wxMenuBar& menu, ImageList& /*ilist*/) const
{
    ClearMenu(menu);
    auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
    AddMenuItem(fileMenu, 0, ID_FILE_EXPORT, "Export All Palettes...");
    AddMenuItem(fileMenu, 1, ID_FILE_IMPORT, "Import Palettes...");
    // A new entry can only be added to an editable list with room under its cap; grey the option
    // out otherwise so it matches the "Add" button and leaves "Overwrite Selected" as the way in.
    AddMenuItem(fileMenu, 2, ID_FILE_IMPORT_PNG_NEW, "Import Palette from PNG as New Entry...")
        .Enable(CanAddPalette());
    AddMenuItem(fileMenu, 3, ID_FILE_IMPORT_PNG_OVERWRITE, "Import Palette from PNG, Overwrite Selected...");
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
        case ID_FILE_IMPORT_PNG_NEW:
            OnImportPngNewEntry();
            break;
        case ID_FILE_IMPORT_PNG_OVERWRITE:
            OnImportPngOverwrite();
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
        // The buttons act on the hovered row, so keep their enabled state in step with it.
        UpdatePaletteButtons();
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

std::shared_ptr<Landstalker::PaletteEntry> PaletteListFrame::GetPaletteEntry(int row) const
{
    if (!m_gd || row < 0 || row >= static_cast<int>(GetPaletteCount()))
    {
        return nullptr;
    }
    switch (m_mode)
    {
    case Mode::ROOM:      return m_gd->GetRoomData()->GetRoomPalette(row);
    case Mode::SPRITE_LO: return m_gd->GetSpriteData()->GetLoPalette(row);
    case Mode::SPRITE_HI: return m_gd->GetSpriteData()->GetHiPalette(row);
    default:              return nullptr;
    }
}

std::shared_ptr<Landstalker::PaletteEntry> PaletteListFrame::GetSelectedPaletteEntry() const
{
    // Read straight from the data view model, so this works in every mode - including the fixed
    // ones that concatenate several palette lists and have no simple row-to-entry mapping.
    if (!m_model)
    {
        return nullptr;
    }
    const auto sel = m_list->GetSelection();
    if (!sel.IsOk())
    {
        return nullptr;
    }
    const int row = static_cast<int>(reinterpret_cast<std::intptr_t>(sel.GetID())) - 1;
    if (row < 0)
    {
        return nullptr;
    }
    return m_model->GetPaletteEntry(row);
}

bool PaletteListFrame::PickAndReadPng(Landstalker::ImageBuffer::IndexedImage& out)
{
    wxFileDialog fd(this, _("Import Palette from PNG"), "", "",
        "PNG Image (*.png)|*.png|All Files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (fd.ShowModal() == wxID_CANCEL)
    {
        return false;
    }
    out = Landstalker::ImageBuffer::ReadIndexedPNG(fd.GetPath().ToStdString());
    if (!out.ok)
    {
        wxMessageBox("The PNG image could not be decoded.", "Import Palette from PNG",
            wxOK | wxICON_ERROR, this);
        return false;
    }
    if (!out.indexed || out.palette.empty())
    {
        wxMessageBox("The PNG must be a colour-indexed (palette) image.", "Import Palette from PNG",
            wxOK | wxICON_ERROR, this);
        return false;
    }
    return true;
}

bool PaletteListFrame::ApplyPngPalette(std::shared_ptr<Landstalker::Palette> pal,
    const Landstalker::ImageBuffer::IndexedImage& img)
{
    const int png_colours = static_cast<int>(img.palette.size());
    Landstalker::Palette::Colour colour;
    if (pal->IsVarWidth())
    {
        // The width is not fixed, so let the user decide how many of the PNG's colours to take.
        const int cap = std::min(pal->GetSize(), png_colours);
        if (cap <= 0)
        {
            wxMessageBox("The PNG palette holds no colours to import.", "Import Palette from PNG",
                wxOK | wxICON_ERROR, this);
            return false;
        }
        const long count = wxGetNumberFromUser(
            wxString::Format("The PNG palette holds %d colour(s).\nHow many would you like to import?",
                png_colours),
            "Number of colours:", "Import Palette from PNG", cap, 1, cap, this);
        if (count <= 0)
        {
            return false;
        }
        for (int i = 0; i < static_cast<int>(count); ++i)
        {
            colour.FromRGB(img.palette[i]);
            pal->SetNthUnlockedGenesisColour(static_cast<uint8_t>(i), colour.GetGenesis());
        }
    }
    else
    {
        // Fixed width: take only the editable (unlocked) indices, reading each from the same index
        // in the PNG palette - so a room palette pulls colours 2-14 and leaves 0, 1 and 15 alone.
        const int highest = pal->GetNthUnlockedIndex(static_cast<uint8_t>(pal->GetSize() - 1));
        if (png_colours <= highest)
        {
            wxMessageBox(wxString::Format("The PNG palette has %d colour(s), but this palette needs "
                "colours up to index %d. Export it to see the expected layout.", png_colours, highest),
                "Import Palette from PNG", wxOK | wxICON_ERROR, this);
            return false;
        }
        for (int nth = 0; nth < pal->GetSize(); ++nth)
        {
            const uint8_t index = pal->GetNthUnlockedIndex(static_cast<uint8_t>(nth));
            colour.FromRGB(img.palette[index]);
            pal->SetNthUnlockedGenesisColour(static_cast<uint8_t>(nth), colour.GetGenesis());
        }
    }
    return true;
}

void PaletteListFrame::OnImportPngOverwrite()
{
    if (!m_gd)
    {
        return;
    }
    const auto entry = GetSelectedPaletteEntry();
    if (!entry)
    {
        wxMessageBox("Select a palette to overwrite first.", "Import Palette from PNG",
            wxOK | wxICON_INFORMATION, this);
        return;
    }
    Landstalker::ImageBuffer::IndexedImage img;
    if (!PickAndReadPng(img))
    {
        return;
    }
    if (!ApplyPngPalette(entry->GetData(), img))
    {
        return;
    }
    const int row = GetSelectedRow();
    Update();
    if (row >= 0)
    {
        SelectRow(row);
    }
}

void PaletteListFrame::OnListContextMenu(wxDataViewEvent& evt)
{
    // Act on the right-clicked row (hover-select usually already picked it, but be explicit).
    if (evt.GetItem().IsOk())
    {
        m_list->Select(evt.GetItem());
    }
    const auto entry = GetSelectedPaletteEntry();
    if (!entry || !entry->GetData()->IsVarWidth())
    {
        // Only variable-width palettes have an adjustable length; fixed ones get no menu.
        return;
    }
    wxMenu menu;
    auto* item = menu.Append(wxID_ANY, "Set Palette Length...");
    menu.Bind(wxEVT_MENU, [this](wxCommandEvent&) { OnSetPaletteLength(); }, item->GetId());
    PopupMenu(&menu);
}

void PaletteListFrame::OnSetPaletteLength()
{
    const auto entry = GetSelectedPaletteEntry();
    if (!entry)
    {
        return;
    }
    const auto pal = entry->GetData();
    if (!pal->IsVarWidth())
    {
        return;
    }
    const int current = pal->GetSize();
    // 255 is far beyond any in-game fade and stays well inside the format's 16-bit size word.
    const long count = wxGetNumberFromUser(
        "Set the number of colours in this variable-width palette.\n"
        "New colours are added as black; reducing the count drops them from the end.",
        "Colours:", "Set Palette Length", current, 1, 255, this);
    if (count < 1 || count == current)
    {
        return;
    }
    if (!pal->SetSize(static_cast<int>(count)))
    {
        return;
    }
    const int row = GetSelectedRow();
    Update();
    if (row >= 0)
    {
        SelectRow(row);
    }
    UpdatePaletteButtons();
    FireEvent(EVT_STATUSBAR_UPDATE);
}

void PaletteListFrame::OnImportPngNewEntry()
{
    if (!m_gd)
    {
        return;
    }
    if (!IsEditableMode())
    {
        wxMessageBox("This palette list cannot be expanded. Use 'Overwrite Selected' instead.",
            "Import Palette from PNG", wxOK | wxICON_INFORMATION, this);
        return;
    }
    const std::size_t cap = (m_mode == Mode::ROOM)
        ? Landstalker::RoomData::MAX_ROOM_PALETTES : Landstalker::SpriteData::MAX_SPRITE_PALETTES;
    if (GetPaletteCount() >= cap)
    {
        wxMessageBox("The palette list is full.", "Import Palette from PNG", wxOK | wxICON_ERROR, this);
        return;
    }
    Landstalker::ImageBuffer::IndexedImage img;
    if (!PickAndReadPng(img))
    {
        return;
    }

    std::optional<uint8_t> added;
    switch (m_mode)
    {
    case Mode::ROOM:      added = m_gd->GetRoomData()->AddRoomPalette(); break;
    case Mode::SPRITE_LO: added = m_gd->GetSpriteData()->AddLoPalette(); break;
    case Mode::SPRITE_HI: added = m_gd->GetSpriteData()->AddHiPalette(); break;
    default: return;
    }
    if (!added)
    {
        wxMessageBox("The palette list is full.", "Import Palette from PNG", wxOK | wxICON_ERROR, this);
        return;
    }
    if (!ApplyPngPalette(GetPaletteEntry(*added)->GetData(), img))
    {
        // Roll the freshly added palette back so a cancelled or rejected import leaves nothing behind.
        switch (m_mode)
        {
        case Mode::ROOM:      m_gd->GetRoomData()->DeleteRoomPalette(*added); break;
        case Mode::SPRITE_LO: m_gd->GetSpriteData()->DeleteLoPalette(*added); break;
        case Mode::SPRITE_HI: m_gd->GetSpriteData()->DeleteHiPalette(*added); break;
        default: break;
        }
        Update();
        return;
    }
    Update();
    SelectRow(*added);
    UpdatePaletteButtons();
}