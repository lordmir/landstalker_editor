#include "MainFrame.h"

#include <fstream>
#include <sstream>
#include <cstring>
#include <iomanip>
#include <set>
#include <codecvt>
#include <locale>
#include <algorithm>
#include <filesystem>

#include <wx/wx.h>
#include <wx/aboutdlg.h>
#include <wx/dcclient.h>
#include <wx/msgdlg.h>
#include <wx/richmsgdlg.h>
#include <wx/colour.h>
#include <wx/graphics.h>

#include "Rom.h"
#include "Blockmap2D.h"
#include "ImageBuffer.h"

MainFrame::MainFrame(wxWindow* parent, const std::string& filename)
    : MainFrameBaseClass(parent),
      m_scale(1),
      m_roomnum(0),
      m_mode(MODE_NONE),
      m_layer_controls_enabled(false),
	  m_activeEditor(nullptr),
      m_g(nullptr)
{
    m_imgs = new ImageList();
    wxGridSizer* sizer = new wxGridSizer(1);
    m_tilesetEditor = new TilesetEditorFrame(this->m_scrollwindow);
    m_stringEditor = new StringEditorFrame(this->m_scrollwindow);
    m_paletteEditor = new PaletteListFrame(this->m_scrollwindow);
    m_canvas = new wxScrolledCanvas(this->m_scrollwindow);
    m_editors.insert({ EditorType::TILESET, m_tilesetEditor });
    m_editors.insert({ EditorType::STRING, m_stringEditor});
    m_editors.insert({ EditorType::PALETTE, m_paletteEditor });
    m_scrollwindow->SetBackgroundColour(*wxBLACK);
    for (const auto& editor : m_editors)
    {
        sizer->Add(editor.second, 1, wxEXPAND | wxALL);
        sizer->Hide(editor.second);
    }
    sizer->Add(m_canvas, 1, wxEXPAND | wxALL);
    sizer->Hide(m_canvas);
    this->m_scrollwindow->SetSizer(sizer);
    sizer->Layout();
    SetMode(MODE_NONE);
    m_mnu_save_as_asm->Enable(false);
    m_mnu_save_to_rom->Enable(false);
    m_mnu_export->Enable(false);
    if (!filename.empty())
    {
        OpenFile(filename.c_str());
    }
	this->GetToolBar()->Hide();
	this->Connect(EVT_STATUSBAR_INIT, wxCommandEventHandler(MainFrame::OnStatusBarInit), nullptr, this);
	this->Connect(EVT_STATUSBAR_UPDATE, wxCommandEventHandler(MainFrame::OnStatusBarUpdate), nullptr, this);
	this->Connect(EVT_STATUSBAR_CLEAR, wxCommandEventHandler(MainFrame::OnStatusBarClear), nullptr, this);
	this->Connect(EVT_PROPERTIES_INIT, wxCommandEventHandler(MainFrame::OnPropertiesInit), nullptr, this);
	this->Connect(EVT_PROPERTIES_UPDATE, wxCommandEventHandler(MainFrame::OnPropertiesUpdate), nullptr, this);
	this->Connect(EVT_PROPERTIES_CLEAR, wxCommandEventHandler(MainFrame::OnPropertiesClear), nullptr, this);
	this->Connect(wxEVT_PG_CHANGED, wxPropertyGridEventHandler(MainFrame::OnPropertyChange), nullptr, this);
	this->Connect(EVT_MENU_INIT, wxCommandEventHandler(MainFrame::OnMenuInit), nullptr, this);
	this->Connect(EVT_MENU_CLEAR, wxCommandEventHandler(MainFrame::OnMenuClear), nullptr, this);
	this->Connect(wxEVT_COMMAND_MENU_SELECTED, wxMenuEventHandler(MainFrame::OnMenuClick), nullptr, this);
	this->Connect(wxEVT_AUI_PANE_CLOSE, wxAuiManagerEventHandler(MainFrame::OnPaneClose), nullptr, this);
    m_canvas->Connect(wxEVT_PAINT, wxPaintEventHandler(MainFrame::OnScrollWindowPaint), NULL, this);
    m_canvas->Connect(wxEVT_MOUSEWHEEL, wxMouseEventHandler(MainFrame::OnScrollWindowMousewheel), NULL, this);
    m_canvas->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(MainFrame::OnScrollWindowLeftDown), NULL, this);
    m_canvas->Connect(wxEVT_LEFT_UP, wxMouseEventHandler(MainFrame::OnScrollWindowLeftUp), NULL, this);
    m_canvas->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(MainFrame::OnScrollWindowRightDown), NULL, this);
    m_canvas->Connect(wxEVT_RIGHT_UP, wxMouseEventHandler(MainFrame::OnScrollWindowRightUp), NULL, this);
    m_canvas->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainFrame::OnScrollWindowKeyDown), NULL, this);
    m_canvas->Connect(wxEVT_KEY_UP, wxKeyEventHandler(MainFrame::OnScrollWindowKeyUp), NULL, this);
    m_canvas->Connect(wxEVT_MOTION, wxMouseEventHandler(MainFrame::OnScrollWindowMouseMove), NULL, this);
    m_canvas->Connect(wxEVT_SIZE, wxSizeEventHandler(MainFrame::OnScrollWindowResize), NULL, this);
}

MainFrame::~MainFrame()
{
    this->Disconnect(EVT_STATUSBAR_INIT, wxCommandEventHandler(MainFrame::OnStatusBarInit), nullptr, this);
    this->Disconnect(EVT_STATUSBAR_UPDATE, wxCommandEventHandler(MainFrame::OnStatusBarUpdate), nullptr, this);
    this->Disconnect(EVT_STATUSBAR_CLEAR, wxCommandEventHandler(MainFrame::OnStatusBarClear), nullptr, this);
    this->Disconnect(EVT_PROPERTIES_INIT, wxCommandEventHandler(MainFrame::OnPropertiesInit), nullptr, this);
    this->Disconnect(EVT_PROPERTIES_UPDATE, wxCommandEventHandler(MainFrame::OnPropertiesUpdate), nullptr, this);
    this->Disconnect(EVT_PROPERTIES_CLEAR, wxCommandEventHandler(MainFrame::OnPropertiesClear), nullptr, this);
    this->Disconnect(wxEVT_PG_CHANGED, wxPropertyGridEventHandler(MainFrame::OnPropertyChange), nullptr, this);
    this->Disconnect(EVT_MENU_INIT, wxCommandEventHandler(MainFrame::OnMenuInit), nullptr, this);
    this->Disconnect(EVT_MENU_CLEAR, wxCommandEventHandler(MainFrame::OnMenuClear), nullptr, this);
    this->Disconnect(wxEVT_COMMAND_MENU_SELECTED, wxMenuEventHandler(MainFrame::OnMenuClick), nullptr, this);
    this->Disconnect(wxEVT_AUI_PANE_CLOSE, wxAuiManagerEventHandler(MainFrame::OnPaneClose), nullptr, this);
    m_canvas->Disconnect(wxEVT_PAINT, wxPaintEventHandler(MainFrame::OnScrollWindowPaint), NULL, this);
    m_canvas->Disconnect(wxEVT_MOUSEWHEEL, wxMouseEventHandler(MainFrame::OnScrollWindowMousewheel), NULL, this);
    m_canvas->Disconnect(wxEVT_LEFT_DOWN, wxMouseEventHandler(MainFrame::OnScrollWindowLeftDown), NULL, this);
    m_canvas->Disconnect(wxEVT_LEFT_UP, wxMouseEventHandler(MainFrame::OnScrollWindowLeftUp), NULL, this);
    m_canvas->Disconnect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(MainFrame::OnScrollWindowRightDown), NULL, this);
    m_canvas->Disconnect(wxEVT_RIGHT_UP, wxMouseEventHandler(MainFrame::OnScrollWindowRightUp), NULL, this);
    m_canvas->Disconnect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainFrame::OnScrollWindowKeyDown), NULL, this);
    m_canvas->Disconnect(wxEVT_KEY_UP, wxKeyEventHandler(MainFrame::OnScrollWindowKeyUp), NULL, this);
    m_canvas->Disconnect(wxEVT_MOTION, wxMouseEventHandler(MainFrame::OnScrollWindowMouseMove), NULL, this);
    m_canvas->Disconnect(wxEVT_SIZE, wxSizeEventHandler(MainFrame::OnScrollWindowResize), NULL, this);

    delete m_imgs;
}

void MainFrame::OnExit(wxCommandEvent& event)
{
    Close();
    event.Skip();
}

void MainFrame::OnAbout(wxCommandEvent& event)
{
    wxUnusedVar(event);
    wxAboutDialogInfo info;
    info.SetCopyright(_("Landstalker Graphics Viewer"));
    info.SetLicence(_("GPL v2 or later"));
    info.SetDescription(_("Github: www.github.com/lordmir/landstalker_gfx\nEmail: hase@redfern.xyz"));
    ::wxAboutBox(info);
    event.Skip();
}

void MainFrame::OpenRomFile(const wxString& path)
{
    try
    {
        if (CloseFiles() != ReturnCode::OK)
        {
            return;
        }
        m_rom.load_from_file(static_cast<std::string>(path));
        m_g = std::make_shared<GameData>(m_rom);
        this->SetLabel("Landstalker Graphics Viewer - " + m_rom.get_description());
        InitUI();
        m_asmfile = false;
    }
    catch (const std::runtime_error& e)
    {
        CloseFiles(true);
        wxMessageBox(e.what());
    }
    SetMode(MODE_NONE);
}

void MainFrame::OpenAsmFile(const wxString& path)
{
    try
    {
        if (CloseFiles() != ReturnCode::OK)
        {
            return;
        }
        m_g = std::make_shared<GameData>(path.ToStdString());
        this->SetLabel("Landstalker Graphics Viewer - " + path);
        InitUI();
        m_asmfile = true;
    }
    catch (const std::runtime_error& e)
    {
        CloseFiles(true);
        wxMessageBox(e.what());
    }
}

void MainFrame::InitUI()
{
    m_browser->DeleteAllItems();
    m_browser->SetImageList(m_imgs);
    m_properties->GetGrid()->Clear();
    for (const auto& editor : m_editors)
    {
        editor.second->SetGameData(m_g);
    }
    SetMode(MODE_NONE);

    const int str_img = m_imgs->GetIdx("string");
    const int img_img = m_imgs->GetIdx("image");
    const int ts_img = m_imgs->GetIdx("tileset");
    const int ats_img = m_imgs->GetIdx("ats");
    const int fonts_img = m_imgs->GetIdx("fonts");
    const int bs_img = m_imgs->GetIdx("big_tiles");
    const int pal_img = m_imgs->GetIdx("palette");
    const int rm_img = m_imgs->GetIdx("room");
    const int spr_img = m_imgs->GetIdx("sprite");
    wxTreeItemId nodeRoot = m_browser->AddRoot("");
    wxTreeItemId nodeS = m_browser->AppendItem(nodeRoot, "Strings", str_img, str_img, new TreeNodeData());
    wxTreeItemId nodeTs = m_browser->AppendItem(nodeRoot, "Tilesets", ts_img, ts_img, new TreeNodeData());
    wxTreeItemId nodeG = m_browser->AppendItem(nodeRoot, "Graphics", img_img, img_img, new TreeNodeData());
    wxTreeItemId nodeGF = m_browser->AppendItem(nodeG, "Fonts", fonts_img, fonts_img, new TreeNodeData());
    wxTreeItemId nodeGU = m_browser->AppendItem(nodeG, "User Interface", ts_img, ts_img, new TreeNodeData());
    wxTreeItemId nodeGS = m_browser->AppendItem(nodeG, "Status Effects", ts_img, ts_img, new TreeNodeData());
    wxTreeItemId nodeGW = m_browser->AppendItem(nodeG, "Sword Effects", ts_img, ts_img, new TreeNodeData());
    wxTreeItemId nodeGE = m_browser->AppendItem(nodeG, "End Credits", img_img, img_img, new TreeNodeData());
    wxTreeItemId nodeGI = m_browser->AppendItem(nodeG, "Island Map", img_img, img_img, new TreeNodeData());
    wxTreeItemId nodeGL = m_browser->AppendItem(nodeG, "Lithograph", img_img, img_img, new TreeNodeData());
    wxTreeItemId nodeGT = m_browser->AppendItem(nodeG, "Title Screen", img_img, img_img, new TreeNodeData());
    wxTreeItemId nodeGSe = m_browser->AppendItem(nodeG, "Sega Logo", img_img, img_img, new TreeNodeData());
    wxTreeItemId nodeGC = m_browser->AppendItem(nodeG, "Climax Logo", img_img, img_img, new TreeNodeData());
    wxTreeItemId nodeGLo = m_browser->AppendItem(nodeG, "Load Game", img_img, img_img, new TreeNodeData());
    wxTreeItemId nodeBs = m_browser->AppendItem(nodeRoot, "Blocksets", bs_img, bs_img, new TreeNodeData());
    wxTreeItemId nodeP = m_browser->AppendItem(nodeRoot, "Palettes", pal_img, pal_img, new TreeNodeData());
    wxTreeItemId nodeRm = m_browser->AppendItem(nodeRoot, "Rooms", rm_img, rm_img, new TreeNodeData());
    wxTreeItemId nodeSprites = m_browser->AppendItem(nodeRoot, "Sprites", spr_img, spr_img, new TreeNodeData());

    m_browser->AppendItem(nodeS, "Compressed Strings", str_img, str_img, new TreeNodeData(TreeNodeData::NODE_STRING,
        static_cast<int>(StringEditorFrame::Mode::MODE_MAIN)));
    m_browser->AppendItem(nodeS, "Character Names", str_img, str_img, new TreeNodeData(TreeNodeData::NODE_STRING,
        static_cast<int>(StringEditorFrame::Mode::MODE_CHARS)));
    m_browser->AppendItem(nodeS, "Special Character Names", str_img, str_img, new TreeNodeData(TreeNodeData::NODE_STRING,
        static_cast<int>(StringEditorFrame::Mode::MODE_SPECIAL_CHARS)));
    m_browser->AppendItem(nodeS, "Default Character Name", str_img, str_img, new TreeNodeData(TreeNodeData::NODE_STRING,
        static_cast<int>(StringEditorFrame::Mode::MODE_DEFAULT_CHAR)));
    m_browser->AppendItem(nodeS, "Item Names", str_img, str_img, new TreeNodeData(TreeNodeData::NODE_STRING,
        static_cast<int>(StringEditorFrame::Mode::MODE_ITEMS)));
    m_browser->AppendItem(nodeS, "Menu Strings", str_img, str_img, new TreeNodeData(TreeNodeData::NODE_STRING,
        static_cast<int>(StringEditorFrame::Mode::MODE_MENU)));
    m_browser->AppendItem(nodeS, "Intro Strings", str_img, str_img, new TreeNodeData(TreeNodeData::NODE_STRING,
        static_cast<int>(StringEditorFrame::Mode::MODE_INTRO)));
    m_browser->AppendItem(nodeS, "End Credit Strings", str_img, str_img, new TreeNodeData(TreeNodeData::NODE_STRING,
        static_cast<int>(StringEditorFrame::Mode::MODE_END_CREDITS)));
    m_browser->AppendItem(nodeS, "System Strings", str_img, str_img, new TreeNodeData(TreeNodeData::NODE_STRING,
        static_cast<int>(StringEditorFrame::Mode::MODE_SYSTEM)));

    m_browser->AppendItem(nodeP, "Room Palettes", pal_img, pal_img, new TreeNodeData(TreeNodeData::NODE_PALETTE,
        static_cast<int>(PaletteListFrame::Mode::ROOM)));
    m_browser->AppendItem(nodeP, "Misc Room Palettes", pal_img, pal_img, new TreeNodeData(TreeNodeData::NODE_PALETTE,
        static_cast<int>(PaletteListFrame::Mode::ROOM_MISC)));
    m_browser->AppendItem(nodeP, "Sprite Low Palettes", pal_img, pal_img, new TreeNodeData(TreeNodeData::NODE_PALETTE,
        static_cast<int>(PaletteListFrame::Mode::SPRITE_LO)));
    m_browser->AppendItem(nodeP, "Sprite High Palettes", pal_img, pal_img, new TreeNodeData(TreeNodeData::NODE_PALETTE,
        static_cast<int>(PaletteListFrame::Mode::SPRITE_HI)));
    m_browser->AppendItem(nodeP, "Sprite Projectile Palettes", pal_img, pal_img, new TreeNodeData(TreeNodeData::NODE_PALETTE,
        static_cast<int>(PaletteListFrame::Mode::PROJECTILE)));
    m_browser->AppendItem(nodeP, "Equipment Palettes", pal_img, pal_img, new TreeNodeData(TreeNodeData::NODE_PALETTE,
        static_cast<int>(PaletteListFrame::Mode::EQUIP)));
    m_browser->AppendItem(nodeP, "Misc Palettes", pal_img, pal_img, new TreeNodeData(TreeNodeData::NODE_PALETTE,
        static_cast<int>(PaletteListFrame::Mode::MISC)));

    for (int i = 0; i < 255; ++i)
    {
        if (!m_g->GetSpriteData()->IsSprite(i))
        {
            continue;
        }
        auto spr_name = m_g->GetSpriteData()->GetSpriteName(i);
        const auto& entities = m_g->GetSpriteData()->GetEntitiesFromSprite(i);
        const auto& anims = m_g->GetSpriteData()->GetSpriteAnimations(i);
        wxTreeItemId spr_node;
        if (entities.size() > 0)
        {
            spr_node = m_browser->AppendItem(nodeSprites, spr_name, spr_img, spr_img, new TreeNodeData(TreeNodeData::NODE_SPRITE, entities[0]));
            for (const auto& entity : entities)
            {
                auto entity_node = m_browser->AppendItem(spr_node, StrPrintf("Entity %03d", entity), spr_img, spr_img, new TreeNodeData(TreeNodeData::NODE_SPRITE, entity));
                for (const auto& anim : anims)
                {
                    auto anim_node = m_browser->AppendItem(entity_node, anim, spr_img, spr_img, new TreeNodeData(TreeNodeData::NODE_SPRITE, (1 << 8) | entity));
                    const auto& frames = m_g->GetSpriteData()->GetSpriteAnimationFrames(anim);
                    for (const auto& frame : frames)
                    {
                        m_browser->AppendItem(anim_node, frame, spr_img, spr_img, new TreeNodeData(TreeNodeData::NODE_SPRITE, (2 << 8) | entity));
                    }
                }
            }
        }
        else
        {
            spr_node = m_browser->AppendItem(nodeSprites, spr_name, spr_img, spr_img, new TreeNodeData(TreeNodeData::NODE_BASE));
        }
    }

    for (const auto& t : m_g->GetRoomData()->GetTilesets())
    {
        auto ts_node = m_browser->AppendItem(nodeTs, t->GetName(), ts_img, ts_img, new TreeNodeData(TreeNodeData::NODE_TILESET));
        for (const auto& at : m_g->GetRoomData()->GetAnimatedTilesets(t->GetName()))
        {
            m_browser->AppendItem(ts_node, at->GetName(), ats_img, ats_img, new TreeNodeData(TreeNodeData::NODE_ANIM_TILESET));
        }
        auto pri = m_browser->AppendItem(nodeBs, t->GetName(), bs_img, bs_img, new TreeNodeData(TreeNodeData::NODE_BASE));
        for (const auto& bs : m_g->GetRoomData()->GetBlocksetList(t->GetName()))
        {
            m_browser->AppendItem(pri, bs->GetName(), bs_img, bs_img, new TreeNodeData(TreeNodeData::NODE_BLOCKSET, (bs->GetIndex().first << 8) | bs->GetIndex().second));
        }
    }

    for (const auto& map : m_g->GetGraphicsData()->GetUIMaps())
    {
        m_browser->AppendItem(nodeGU, map->GetName(), img_img, img_img, new TreeNodeData(TreeNodeData::NODE_IMAGE));
    }
    for (const auto& map : m_g->GetStringData()->GetTextboxMaps())
    {
        m_browser->AppendItem(nodeGU, map->GetName(), img_img, img_img, new TreeNodeData(TreeNodeData::NODE_IMAGE));
    }
    m_browser->AppendItem(nodeGE, m_g->GetGraphicsData()->GetEndCreditLogosMaps()->GetName(), img_img, img_img, new TreeNodeData(TreeNodeData::NODE_IMAGE));
    for (const auto& map : m_g->GetGraphicsData()->GetIslandMapMaps())
    {
        m_browser->AppendItem(nodeGI, map->GetName(), img_img, img_img, new TreeNodeData(TreeNodeData::NODE_IMAGE));
    }
    m_browser->AppendItem(nodeGL, m_g->GetGraphicsData()->GetLithographMap()->GetName(), img_img, img_img, new TreeNodeData(TreeNodeData::NODE_IMAGE));
    for (const auto& map : m_g->GetGraphicsData()->GetTitleScreenMap())
    {
        m_browser->AppendItem(nodeGT, map->GetName(), img_img, img_img, new TreeNodeData(TreeNodeData::NODE_IMAGE));
    }
    m_browser->AppendItem(nodeGC, m_g->GetGraphicsData()->GetClimaxLogoMap()->GetName(), img_img, img_img, new TreeNodeData(TreeNodeData::NODE_IMAGE));
    m_browser->AppendItem(nodeGLo, m_g->GetGraphicsData()->GetGameLoadScreenMap()->GetName(), img_img, img_img, new TreeNodeData(TreeNodeData::NODE_IMAGE));

    m_browser->AppendItem(nodeGF, m_g->GetRoomData()->GetIntroFont()->GetName(), fonts_img, fonts_img, new TreeNodeData(TreeNodeData::NODE_TILESET));
    for (const auto& ts : m_g->GetStringData()->GetFonts())
    {
        m_browser->AppendItem(nodeGF, ts->GetName(), fonts_img, fonts_img, new TreeNodeData(TreeNodeData::NODE_TILESET));
    }
    for (const auto& ts : m_g->GetGraphicsData()->GetFonts())
    {
        m_browser->AppendItem(nodeGF, ts->GetName(), fonts_img, fonts_img, new TreeNodeData(TreeNodeData::NODE_TILESET));
    }
    for (const auto& ts : m_g->GetGraphicsData()->GetUIGraphics())
    {
        m_browser->AppendItem(nodeGU, ts->GetName(), ts_img, ts_img, new TreeNodeData(TreeNodeData::NODE_TILESET));
    }
    for (const auto& ts : m_g->GetGraphicsData()->GetStatusEffects())
    {
        m_browser->AppendItem(nodeGS, ts->GetName(), ts_img, ts_img, new TreeNodeData(TreeNodeData::NODE_TILESET));
    }
    for (const auto& ts : m_g->GetGraphicsData()->GetSwordEffects())
    {
        m_browser->AppendItem(nodeGW, ts->GetName(), ts_img, ts_img, new TreeNodeData(TreeNodeData::NODE_TILESET));
    }
    m_browser->AppendItem(nodeGE, m_g->GetGraphicsData()->GetEndCreditLogosTiles()->GetName(), ts_img, ts_img, new TreeNodeData(TreeNodeData::NODE_TILESET));
    for (const auto& ts : m_g->GetGraphicsData()->GetIslandMapTiles())
    {
        m_browser->AppendItem(nodeGI, ts->GetName(), ts_img, ts_img, new TreeNodeData(TreeNodeData::NODE_TILESET));
    }
    m_browser->AppendItem(nodeGL, m_g->GetGraphicsData()->GetLithographTiles()->GetName(), ts_img, ts_img, new TreeNodeData(TreeNodeData::NODE_TILESET));
    for (const auto& ts : m_g->GetGraphicsData()->GetTitleScreenTiles())
    {
        m_browser->AppendItem(nodeGT, ts->GetName(), ts_img, ts_img, new TreeNodeData(TreeNodeData::NODE_TILESET));
    }
    m_browser->AppendItem(nodeGSe, m_g->GetGraphicsData()->GetSegaLogoTiles()->GetName(), ts_img, ts_img, new TreeNodeData(TreeNodeData::NODE_TILESET));
    m_browser->AppendItem(nodeGC, m_g->GetGraphicsData()->GetClimaxLogoTiles()->GetName(), ts_img, ts_img, new TreeNodeData(TreeNodeData::NODE_TILESET));
    for (const auto& ts : m_g->GetGraphicsData()->GetGameLoadScreenTiles())
    {
        m_browser->AppendItem(nodeGLo, ts->GetName(), ts_img, ts_img, new TreeNodeData(TreeNodeData::NODE_TILESET));
    }

    for (const auto& room : m_g->GetRoomData()->GetRoomlist())
    {
        wxTreeItemId cRm = m_browser->AppendItem(nodeRm, room->name, rm_img, rm_img, new TreeNodeData(TreeNodeData::NODE_ROOM, room->index));
        m_browser->AppendItem(cRm, "Heightmap", rm_img, rm_img, new TreeNodeData(TreeNodeData::NODE_ROOM_HEIGHTMAP, room->index));
        m_browser->AppendItem(cRm, "Warps", rm_img, rm_img, new TreeNodeData(TreeNodeData::NODE_ROOM_WARPS, room->index));
    }
    m_mnu_save_as_asm->Enable(true);
    m_mnu_save_to_rom->Enable(true);
    m_mnu_export->Enable(true);
}

MainFrame::ReturnCode MainFrame::Save()
{
    if (m_asmfile == true)
    {
        return SaveAsAsm();
    }
    else
    {
        return SaveToRom();
    }
    return ReturnCode::ERR;
}

MainFrame::ReturnCode MainFrame::SaveAsAsm(std::string path)
{
    try
    {
        if (path.empty())
        {
            wxDirDialog dlg(this, "Select Output Directory to Write Assembly...", "", wxDD_DEFAULT_STYLE);
            if (dlg.ShowModal() != wxID_OK)
            {
                return ReturnCode::CANCELLED;
            }
            path = dlg.GetPath().ToStdString();
        }
        if (m_g)
        {
            m_g->Save(path);
        }
        return ReturnCode::OK;
    }
    catch (const std::exception& e)
    {
        throw;
    }
    return ReturnCode::ERR;
}

MainFrame::ReturnCode MainFrame::SaveToRom(std::string path)
{
    try
    {
        if (m_rom.size() == 0)
        {
            // No ROM loaded, load one in now
            wxFileDialog fdlog(this, "Open existing ROM", "", "",
                "ROM files (*.bin; *.md)|*.bin;*.md|"
                "All files (*.*)|*.*",
                wxFD_OPEN | wxFD_FILE_MUST_EXIST);
            if (fdlog.ShowModal() == wxID_OK)
            {
                m_rom.load_from_file(fdlog.GetPath().ToStdString());
            }
            else
            {
                return ReturnCode::CANCELLED;
            }
        }
        if (path.empty())
        {
            wxFileDialog fdlog(this, "Save new ROM as", "", "",
                "ROM files (*.bin; *.md)|*.bin;*.md|"
                "All files (*.*)|*.*",
                wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
            if (fdlog.ShowModal() != wxID_OK)
            {
                return ReturnCode::CANCELLED;
            }
            path = fdlog.GetPath().ToStdString();
        }
        if (m_g)
        {
            std::ostringstream message, details;
            m_g->RefreshPendingWrites(m_rom);
            auto result = m_g->GetPendingWrites();
            bool warning = false;
            if (!m_g->WillFitInRom(m_rom))
            {
                message << "Warning: Data will not fit in ROM without overwriting existing structures!\n";
                message << "To avoid this issue, it is recommended to use a disassembly source.\n\n";
                warning = true;
            }
            else
            {
                message << "Success: Data will fit into ROM without overwriting existing structures.\n\n";
            }
            for (const auto& w : result)
            {
                uint32_t addr = 0;
                uint32_t size = 0;
                if (m_rom.section_exists(w.first))
                {
                    auto sec = m_rom.get_section(w.first);
                    addr = sec.begin;
                    size = sec.size();
                }
                else if (m_rom.address_exists(w.first))
                {
                    addr = m_rom.get_address(w.first);
                    size = sizeof(uint32_t);
                }
                details << w.first << " @ " << Hex(addr) << ": write " << w.second->size() << " bytes, available "
                    << size << " bytes: " << ((w.second->size() <= size) ? "OK" : "BAD") << std::endl;
            }
            message << "\nProceed?";
            auto msgbox = wxRichMessageDialog(this, message.str(), "Inject into ROM", wxYES_NO | (warning ? wxICON_EXCLAMATION : wxICON_INFORMATION));
            msgbox.ShowDetailedText(details.str());
            int answer = msgbox.ShowModal();
            if (answer != wxID_YES)
            {
                m_g->AbandomRomInjection();
                return ReturnCode::CANCELLED;
            }
            else
            {
                Rom output(m_rom);
                m_g->InjectIntoRom(output);
                output.writeFile(path);
                return ReturnCode::OK;
            }
        }
    }
    catch (const std::exception& e)
    {
        throw;
    }
    return ReturnCode::ERR;
}

void MainFrame::DrawBlocks(const std::string& name, std::size_t row_width, std::size_t scale, uint8_t pal)
{
    auto bs = m_g->GetRoomData()->GetBlockset(name);
    auto ts = m_g->GetRoomData()->GetTileset(bs->GetTileset())->GetData();
    auto blockset = bs->GetData();
    auto palette = m_g->GetRoomData()->GetDefaultTilesetPalette(bs->GetTileset())->GetData();

    m_imgbuf.Clear();
    if (blockset->size() > 0)
    {
        const std::size_t ROW_WIDTH = std::min<std::size_t>(16U, blockset->size());
        const std::size_t ROW_HEIGHT = std::min<std::size_t>(128U, blockset->size() / ROW_WIDTH + (blockset->size() % ROW_WIDTH != 0));
        Blockmap2D map(ROW_WIDTH, ROW_HEIGHT, ts->GetTileWidth(), ts->GetTileHeight(), 0, 0, 0);
        m_imgbuf.Resize(map.GetBitmapWidth(), map.GetBitmapHeight());
        map.SetTileset(ts);
        map.SetBlockset(blockset);
        map.Fill(0, 1, blockset->size());
        map.Draw(m_imgbuf);
    }
    m_scale = scale;
    bmp = m_imgbuf.MakeBitmap({ palette });
    ForceRepaint();
}

void DrawTile(wxGraphicsContext& gc, int x, int y, int z, int width, int height, int restrictions, int classification)
{
    wxPoint2DDouble tile_points[] = {
        wxPoint2DDouble(x + width / 2, y             ),
        wxPoint2DDouble(x + width    , y + height / 2),
        wxPoint2DDouble(x + width / 2, y + height    ),
        wxPoint2DDouble(x            , y + height / 2),
        wxPoint2DDouble(x + width / 2, y             )
    };
    wxPoint2DDouble left_wall[] = {
        wxPoint2DDouble(x            , y + height / 2             ),
        wxPoint2DDouble(x + width / 2, y + height                 ),
        wxPoint2DDouble(x + width / 2, y + height     + z * height),
        wxPoint2DDouble(x            , y + height / 2 + z * height),
        wxPoint2DDouble(x            , y + height / 2             )
    };
    wxPoint2DDouble right_wall[] = {
        wxPoint2DDouble(x + width / 2, y + height                 ),
        wxPoint2DDouble(x + width    , y + height / 2             ),
        wxPoint2DDouble(x + width    , y + height / 2 + z * height),
        wxPoint2DDouble(x + width / 2, y + height     + z * height),
        wxPoint2DDouble(x + width / 2, y + height                 )
    };
    wxColor bg;
    if (restrictions == 0)
    {
        bg.Set(z * 3, 48 + z * 8, z*3);
    }
    else if (restrictions == 0x4)
    {
        bg.Set(48 + z * 8, z*3, z*3);
    }
    else if (restrictions == 0x2)
    {
        bg.Set(32 + z * 3, 32 + z * 3, 48 + z * 12);
    }
    else if (restrictions == 0x6)
    {
        bg.Set(48 + z * 8, 32 + z * 3, 48 + z * 8);
    }
    else
    {
        bg.Set(48 + z * 8, 48 + z * 8, z * 3);
    }
    gc.SetBrush(wxBrush(bg));
    gc.SetPen(*wxTRANSPARENT_PEN);
    //gc.SetTextForeground(*wxWHITE);
    gc.DrawLines(sizeof(tile_points) / sizeof(tile_points[0]), tile_points);
    bg = bg.ChangeLightness(70);
    gc.SetBrush(wxBrush(bg));
    gc.DrawLines(sizeof(left_wall) / sizeof(left_wall[0]), left_wall);
    bg = bg.ChangeLightness(70);
    gc.SetBrush(wxBrush(bg));
    gc.DrawLines(sizeof(right_wall) / sizeof(right_wall[0]), right_wall);
    if (classification != 0)
    {
        // Set font and transparent colour
        gc.SetFont(wxFont(height/2, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
           wxFONTWEIGHT_NORMAL, false),
           wxColour(255, 255, 255, 255));
        std::ostringstream ss;
        ss << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << classification;
        double textwidth, textheight;
        gc.GetTextExtent(ss.str(), &textwidth, &textheight);
        gc.DrawText(ss.str(), x + (width - textwidth)/2, y + (height - textheight)/2 );
    }
}

void MainFrame::DrawWarp(wxGraphicsContext& gc, const WarpList::Warp& warp, std::shared_ptr<Tilemap3D> map, int tile_width, int tile_height)
{
    int x = 0;
    int y = 0;
    if (warp.room1 == m_roomnum)
    {
        x = warp.x1;
        y = warp.y1;
    }
    else if (warp.room2 == m_roomnum)
    {
        x = warp.x2;
        y = warp.y2;
    }
    int z = map->GetHeight({ x - 12,y - 12 });
    auto xy = map->Iso3DToPixel({ x,y,z });
    int width = tile_width;
    int height = tile_height;
    std::vector<wxPoint2DDouble> tile_points = {
        wxPoint2DDouble(xy.x + tile_width / 2, xy.y),
        wxPoint2DDouble(xy.x + (warp.x_size + 1) * tile_width / 2, xy.y + warp.x_size * tile_height / 2),
        wxPoint2DDouble(xy.x + (warp.x_size - warp.y_size + 1) * tile_width / 2, xy.y + (warp.x_size + warp.y_size) * tile_height / 2),
        wxPoint2DDouble(xy.x + (1 - warp.y_size) * tile_width / 2, xy.y + warp.y_size * tile_height / 2),
        wxPoint2DDouble(xy.x + tile_width / 2, xy.y)
    };
    switch (warp.type)
    {
    case WarpList::Warp::Type::NORMAL:
        gc.SetPen(*wxYELLOW_PEN);
        break;
    case WarpList::Warp::Type::STAIR_SE:
        gc.SetPen(*wxGREEN_PEN);
        break;
    case WarpList::Warp::Type::STAIR_SW:
        gc.SetPen(*wxCYAN_PEN);
        break;
    default:
        gc.SetPen(*wxRED_PEN);
        break;
    }
    gc.SetBrush(*wxTRANSPARENT_BRUSH);
    gc.DrawLines(tile_points.size(), &tile_points[0]);
    m_warp_poly.push_back({ warp, tile_points });
}

void SetOpacity(wxImage& image, uint8_t opacity)
{
    uint8_t* alpha = image.GetAlpha();
    for (int i = 0; i < (image.GetHeight() * image.GetWidth()); i++)
    {
        *alpha = (*alpha < opacity) ? *alpha : opacity;
        alpha++;
    }
}

void MainFrame::DrawTilemap(uint16_t room, std::size_t scale)
{
    auto map = m_g->GetRoomData()->GetMapForRoom(room)->GetData();
    auto blocksets = m_g->GetRoomData()->GetBlocksetsForRoom(room);
    auto palette = m_g->GetRoomData()->GetPaletteForRoom(room)->GetData();
    auto tileset = m_g->GetRoomData()->GetTilesetForRoom(room)->GetData();
    auto blockset = m_g->GetRoomData()->GetCombinedBlocksetForRoom(room);

    m_warp_poly.clear();
    m_link_poly.clear();

    const std::size_t TILE_WIDTH = 32;
    const std::size_t TILE_HEIGHT = 16;

    uint8_t bg_opacity  = m_checkBgVisible->GetValue() ? m_sliderBgOpacity->GetValue() : 0;
    uint8_t fg1_opacity = m_checkFg1Visible->GetValue() ? m_sliderFg1Opacity->GetValue() : 0;
    uint8_t fg2_opacity = m_checkFg2Visible->GetValue() ? m_sliderFg2Opacity->GetValue() : 0;
    uint8_t hm_opacity  = m_checkHeightmapVisible->GetValue() ? m_sliderHeightmapOpacity->GetValue() : 0;
    uint8_t spr_opacity = m_checkSpritesVisible->GetValue() ? m_sliderSpritesOpacity->GetValue() : 0;

    m_imgbuf.Resize(map->GetPixelWidth(), map->GetPixelHeight());
    ImageBuffer fg(map->GetPixelWidth(), map->GetPixelHeight());
    m_imgbuf.Insert3DMapLayer(0, 0, 0, Tilemap3D::Layer::BG, map, tileset, blockset);
    fg.Insert3DMapLayer(0, 0, 0, Tilemap3D::Layer::FG, map, tileset, blockset);
    m_scale = scale;
    std::shared_ptr<wxBitmap> bg_bmp(m_imgbuf.MakeBitmap({ palette }, true, bg_opacity));
    std::shared_ptr<wxBitmap> fg_bmp(fg.MakeBitmap({ palette }, true, fg1_opacity, fg2_opacity));
    wxImage hm_img(fg.GetWidth(), fg.GetHeight());
    wxImage disp_img(fg.GetWidth(), fg.GetHeight());
    hm_img.InitAlpha();
    SetOpacity(hm_img, 0x00);
    wxGraphicsContext* hm_gc = wxGraphicsContext::Create(hm_img);
    hm_gc->SetPen(*wxWHITE_PEN);
    hm_gc->SetBrush(*wxBLACK_BRUSH);
    for (int y = 0; y < map->GetHeightmapHeight(); ++y)
        for (int x = 0; x < map->GetHeightmapWidth(); ++x)
        {
            // Only display cells that are not completely restricted
            if ((map->GetHeight({x, y}) > 0 || (map->GetCellProps({x, y}) != 0x04)))
            {
                int z = map->GetHeight({x, y});
                auto xy(map->Iso3DToPixel({ x + 12, y + 12, z }));
                DrawTile(*hm_gc, xy.x, xy.y, z, TILE_WIDTH, TILE_HEIGHT, map->GetCellProps({x,y}), map->GetCellType({x,y}));
            }
        }
    delete hm_gc;
    SetOpacity(hm_img, hm_opacity);
    wxBitmap hm_bmp(hm_img);
    wxGraphicsContext* disp_gc = wxGraphicsContext::Create(disp_img);
    disp_gc->DrawBitmap(*bg_bmp, 0, 0, bg_bmp->GetWidth(), bg_bmp->GetHeight());
    disp_gc->DrawBitmap(*fg_bmp, 0, 0, fg_bmp->GetWidth(), fg_bmp->GetHeight());
    disp_gc->DrawBitmap(hm_bmp, 0, 0, hm_bmp.GetWidth(), hm_bmp.GetHeight());
    delete disp_gc;
    bmp = std::make_shared<wxBitmap>(disp_img);
    memDc.SelectObject(*bmp);
    ForceRepaint();
}

void MainFrame::DrawHeightmap(uint16_t room, std::size_t scale)
{
    auto tilemap = m_g->GetRoomData()->GetMapForRoom(room);
    m_warp_poly.clear();
    m_link_poly.clear();
    auto map = tilemap->GetData();

    const std::size_t TILE_WIDTH = 32;
    const std::size_t TILE_HEIGHT = 32;
    const std::size_t ROW_WIDTH = map->GetHeightmapWidth();
    const std::size_t ROW_HEIGHT = map->GetHeightmapHeight();
    const std::size_t BMP_WIDTH = ROW_WIDTH * TILE_WIDTH + 1;
    const std::size_t BMP_HEIGHT = ROW_HEIGHT * TILE_WIDTH + 1;
    //std::size_t x = 0;
    //std::size_t y = 0;
    bmp = std::make_shared<wxBitmap>(BMP_WIDTH, BMP_HEIGHT);
    memDc.SelectObject(*bmp);
    memDc.SetBackground(*wxBLACK_BRUSH);
    memDc.Clear();
    memDc.SetPen(*wxWHITE_PEN);
    memDc.SetBrush(*wxBLACK_BRUSH);
    memDc.SetTextBackground(*wxBLACK);
    memDc.SetTextForeground(*wxWHITE);
    for(int y = 0; y < ROW_HEIGHT; ++y)
        for(int x = 0; x < ROW_WIDTH; ++x)
        {
            // Only display cells that are not completely restricted
            if ((map->GetHeight({ x, y }) > 0 || (map->GetCellProps({ x, y }) != 0x04)))
            {
                memDc.DrawRectangle(x * TILE_WIDTH, y*TILE_HEIGHT, TILE_WIDTH+1, TILE_HEIGHT+1);
                std::stringstream ss;
                ss << std::hex << std::uppercase << std::setfill('0') << std::setw(1) << static_cast<unsigned>(map->GetHeight({x,y})) << ","
                << std::setfill('0') << std::setw(1) << static_cast<unsigned>(map->GetCellProps({x,y})) << "\n"
                << std::setfill('0') << std::setw(2) << static_cast<unsigned>(map->GetCellType({x,y}));
                memDc.DrawText(ss.str(),x*TILE_WIDTH+2, y*TILE_HEIGHT + 1);
            }
        }
    memDc.SetBrush(*wxTRANSPARENT_BRUSH);
    memDc.SelectObject(wxNullBitmap);

    m_scale = scale;
    m_canvas->SetScrollbars(scale,scale,BMP_WIDTH,BMP_HEIGHT,0,0);
    wxClientDC dc(m_canvas);
    dc.SetBackground(*wxBLACK_BRUSH);
    dc.Clear();
    PaintNow(dc, scale);
}

void MainFrame::AddRoomLink(wxGraphicsContext* gc, const std::string& label, uint16_t room, int x, int y)
{
    wxFont font(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    double w, h;
    gc->SetFont(font, *wxWHITE);
    gc->GetTextExtent(label, &w, &h);
    const double LINK_X = x + std::max<double>(145, w + 20);
    const double LINK_Y = y;
    gc->DrawText(label, x, y);
    gc->SetFont(font, *wxYELLOW);
    auto roomlabel = StrPrintf("Room %03d", room);
    gc->DrawText(roomlabel, LINK_X, LINK_Y);
    gc->GetTextExtent(roomlabel, &w, & h);
    m_link_poly.push_back({ room, {
        {LINK_X, LINK_Y},
        {LINK_X + w, LINK_Y},
        {LINK_X + w, LINK_Y + h},
        {LINK_X, LINK_Y + h},
        {LINK_X, LINK_Y}
    } });
}

void MainFrame::DrawWarps(uint16_t room, std::size_t scale)
{
    if (m_g == nullptr)
    {
        return;
    }
    auto map = m_g->GetRoomData()->GetMapForRoom(room)->GetData();
    auto tileset = m_g->GetRoomData()->GetTilesetForRoom(room)->GetData();
    auto palette = m_g->GetRoomData()->GetPaletteForRoom(room)->GetData();
    auto blockset = m_g->GetRoomData()->GetCombinedBlocksetForRoom(room);

    const std::size_t TILE_WIDTH = 32;
    const std::size_t TILE_HEIGHT = 16;

    m_imgbuf.Resize(map->GetPixelWidth(), map->GetPixelHeight());
    m_imgbuf.Insert3DMapLayer(0, 0, 0, Tilemap3D::Layer::BG, map, tileset, blockset);
    m_imgbuf.Insert3DMapLayer(0, 0, 0, Tilemap3D::Layer::FG, map, tileset, blockset);
    m_scale = scale;
    std::shared_ptr<wxBitmap> bg_bmp(m_imgbuf.MakeBitmap({ palette }));
    wxImage hm_img(m_imgbuf.GetWidth(), m_imgbuf.GetHeight());
    wxImage disp_img(m_imgbuf.GetWidth(), m_imgbuf.GetHeight());
    hm_img.InitAlpha();
    SetOpacity(hm_img, 0x00);
    wxGraphicsContext* hm_gc = wxGraphicsContext::Create(hm_img);
    hm_gc->SetPen(*wxWHITE_PEN);
    hm_gc->SetBrush(*wxBLACK_BRUSH);
    m_warp_poly.clear();
    m_link_poly.clear();
    auto warps = m_g->GetRoomData()->GetWarpsForRoom(room);
    for (const auto& warp : warps)
    {
        DrawWarp(*hm_gc, warp, map, TILE_WIDTH, TILE_HEIGHT);
    }
    wxColour bkColor(*wxBLACK);
    wxColour textColor(*wxWHITE);
    int line = 0;
    if (m_g->GetRoomData()->HasClimbDestination(room))
    {
        AddRoomLink(hm_gc, "Climb Destination:", m_g->GetRoomData()->GetClimbDestination(m_roomnum), 5, 5 + line * 16);
        line++;
    }
    if (m_g->GetRoomData()->HasFallDestination(room))
    {
        AddRoomLink(hm_gc, "Fall Destination:", m_g->GetRoomData()->GetFallDestination(m_roomnum), 5, 5 + line * 16);
        line++;
    }
    auto txns = m_g->GetRoomData()->GetTransitions(room);
    for (const auto t : txns)
    {
        std::string label = StrPrintf("Transition when flag %04d is %s:", t.second, (t.first.first == room) ? "SET" : "CLEAR");
        uint16_t dest = (t.first.first == room) ? t.first.second : t.first.first;
        AddRoomLink(hm_gc, label, dest, 5, 5 + line * 16);
        line++;
    }
    delete hm_gc;
    wxBitmap hm_bmp(hm_img);
    wxGraphicsContext* disp_gc = wxGraphicsContext::Create(disp_img);
    disp_gc->DrawBitmap(*bg_bmp, 0, 0, bg_bmp->GetWidth(), bg_bmp->GetHeight());
    disp_gc->DrawBitmap(hm_bmp, 0, 0, hm_bmp.GetWidth(), hm_bmp.GetHeight());
    delete disp_gc;
    bmp = std::make_shared<wxBitmap>(disp_img);
    memDc.SelectObject(*bmp);
    ForceRepaint();
}

void MainFrame::OnStatusBarInit(wxCommandEvent& event)
{
	EditorFrame* frame = static_cast<EditorFrame*>(event.GetClientData());
	frame->InitStatusBar(*this->m_statusbar);
	event.Skip();
}

void MainFrame::OnStatusBarUpdate(wxCommandEvent& event)
{
	EditorFrame* frame = static_cast<EditorFrame*>(event.GetClientData());
	frame->UpdateStatusBar(*this->m_statusbar);
	event.Skip();
}

void MainFrame::OnStatusBarClear(wxCommandEvent& event)
{
	EditorFrame* frame = static_cast<EditorFrame*>(event.GetClientData());
	frame->ClearStatusBar(*this->m_statusbar);
	event.Skip();
}

void MainFrame::OnPropertiesInit(wxCommandEvent& event)
{
	EditorFrame* frame = static_cast<EditorFrame*>(event.GetClientData());
	frame->InitProperties(*this->m_properties);
	event.Skip();
}

void MainFrame::OnPropertiesUpdate(wxCommandEvent& event)
{
	EditorFrame* frame = static_cast<EditorFrame*>(event.GetClientData());
	frame->UpdateProperties(*this->m_properties);
	event.Skip();
}

void MainFrame::OnPropertiesClear(wxCommandEvent& event)
{
	EditorFrame* frame = static_cast<EditorFrame*>(event.GetClientData());
	frame->ClearProperties(*this->m_properties);
	event.Skip();
}

void MainFrame::OnPropertyChange(wxPropertyGridEvent& event)
{
	if (m_activeEditor != nullptr)
	{
		m_activeEditor->OnPropertyChange(event);
	}
	event.Skip();
}

void MainFrame::OnMenuInit(wxCommandEvent& event)
{
	EditorFrame* frame = static_cast<EditorFrame*>(event.GetClientData());
	frame->InitMenu(*this->m_menubar, *m_imgs);
	event.Skip();
}

void MainFrame::OnMenuClear(wxCommandEvent& event)
{
	EditorFrame* frame = static_cast<EditorFrame*>(event.GetClientData());
	frame->ClearMenu(*this->m_menubar);
	event.Skip();
}

void MainFrame::OnMenuClick(wxMenuEvent& event)
{
	if (m_activeEditor != nullptr)
	{
		m_activeEditor->OnMenuClick(event);
	}
	event.Skip();
}

void MainFrame::OnPaneClose(wxAuiManagerEvent& event)
{
	if (m_activeEditor != nullptr)
	{
		auto* pane = event.GetPane();
		pane->Hide();
		m_activeEditor->UpdateUI();
	}
	event.Skip();
}

void MainFrame::DrawSprite(const std::string& name, int data, std::size_t scale)
{
    uint8_t entity = data & 0xFF;
    uint8_t mode = (data >> 8);
    auto pal = m_g->GetSpriteData()->GetSpritePalette(entity);
    uint8_t spr = m_g->GetSpriteData()->GetSpriteFromEntity(entity);
    std::shared_ptr<SpriteFrameEntry> frame;
    if (mode == 0)
    {
        frame = m_g->GetSpriteData()->GetDefaultEntityFrame(entity);
    }
    else if (mode == 1)
    {
        frame = m_g->GetSpriteData()->GetSpriteFrame(name, 0);
    }
    else
    {
        frame = m_g->GetSpriteData()->GetSpriteFrame(name);
    }
    m_scale = scale;
	m_imgbuf.Resize(160, 160);
    m_imgbuf.InsertSprite(80, 80, 0, *frame->GetData());
    bmp = m_imgbuf.MakeBitmap({pal});
    ForceRepaint();
}

void MainFrame::DrawImage(const std::string& image, std::size_t scale)
{
    m_scale = scale;
    auto img = m_g->GetTilemap(image);
    if (img)
    {
        const auto map = img->GetData();
        const auto ts_entry = m_g->GetTileset(img->GetTileset());
        const auto ts = ts_entry->GetData();
        const auto pal = m_g->GetPalette(ts_entry->GetDefaultPalette())->GetData();
        m_imgbuf.Resize(map->GetWidth() * ts->GetTileWidth(), map->GetHeight() * ts->GetTileHeight());
        m_imgbuf.InsertMap(0, 0, 0, *map, *ts);
        bmp = m_imgbuf.MakeBitmap({pal});
        ForceRepaint();
    }
}

void MainFrame::ShowStrings()
{
    if (!m_stringEditor->IsShown())
    {
        m_activeEditor = m_stringEditor;
        m_tilesetEditor->Hide();
        m_canvas->Hide();
        m_stringEditor->Show();
        this->m_scrollwindow->GetSizer()->Clear();
        this->m_scrollwindow->GetSizer()->Add(m_stringEditor, 1, wxALL | wxEXPAND);
        this->m_scrollwindow->GetSizer()->Layout();
    }
}

void MainFrame::ShowEditor(EditorType editor)
{
    if (!m_editors.at(editor)->IsShown())
    {
        m_activeEditor = m_editors.at(editor);
        for (const auto& editor : m_editors)
        {
            if (editor.second != m_activeEditor)
            {
                editor.second->Hide();
            }
        }
        m_canvas->Hide();
        m_activeEditor->Show();
        this->m_scrollwindow->GetSizer()->Clear();
        this->m_scrollwindow->GetSizer()->Add(m_activeEditor, 1, wxALL | wxEXPAND);
        this->m_scrollwindow->GetSizer()->Layout();
    }
}

void MainFrame::HideAllEditors()
{
    m_activeEditor = nullptr;
    for (const auto& editor : m_editors)
    {
        editor.second->Hide();
    }
}

void MainFrame::ShowTileset()
{
    ShowEditor(EditorType::TILESET);
}

void MainFrame::ShowBitmap()
{
    if (!m_canvas->IsShown())
    {
        m_activeEditor = nullptr;
        HideAllEditors();
        m_canvas->Show();
        this->m_scrollwindow->GetSizer()->Clear();
        this->m_scrollwindow->GetSizer()->Add(m_canvas, 1, wxALL | wxEXPAND);
        this->m_scrollwindow->GetSizer()->Layout();
        this->m_scrollwindow->Refresh(true);
    }
}

void MainFrame::ForceRepaint()
{
    m_canvas->SetScrollRate(1, 1);
    m_canvas->SetVirtualSize(m_imgbuf.GetWidth(), m_imgbuf.GetHeight());
    wxClientDC dc(m_canvas);
    dc.SetBackground(*wxBLACK_BRUSH);
    dc.Clear();
    PaintNow(dc, m_scale);
}

void MainFrame::ClearScreen()
{
    bmp = std::make_shared<wxBitmap>(1, 1);
    memDc.SelectObject(*bmp);
    memDc.SetBackground(*wxBLACK_BRUSH);
    memDc.Clear();
    memDc.SelectObject(wxNullBitmap);
    ForceRepaint();
}

void MainFrame::OnPaint(wxPaintEvent& event)
{
    event.Skip();
}

void MainFrame::PaintNow(wxDC& dc, std::size_t scale)
{
    int x, y, w, h;
    m_canvas->GetViewStart(&x, &y);
    m_canvas->GetClientSize(&w, &h);
    double dscale = static_cast<double>(scale);
    memDc.SelectObject(wxNullBitmap);

    if (bmp != nullptr)
    {
        memDc.SelectObject(*bmp);
    }
    else
    {
        bmp = std::make_shared<wxBitmap>(1, 1);
        memDc.SelectObject(*bmp);
        memDc.SetBackground(*wxBLACK_BRUSH);
        memDc.Clear();
    }

    dc.SetUserScale(dscale, dscale);
    dc.Blit(0, 0, w/dscale+1, h/dscale+1, &memDc, x, y, wxCOPY, true);
    memDc.SelectObject(wxNullBitmap);
}

void MainFrame::OnScrollWindowPaint(wxPaintEvent& event)
{
    wxPaintDC dc(m_canvas);
    PaintNow(dc, m_scale);
    event.Skip();
}

MainFrame::ReturnCode MainFrame::CloseFiles(bool force)
{
    if (!force && CheckForFileChanges())
    {
        int answer = wxMessageBox("Do you want to save your changes?", "Message", wxYES_NO | wxCANCEL | wxICON_QUESTION);
        if (answer == wxCANCEL)
        {
            return ReturnCode::CANCELLED;
        }
        else if (answer == wxYES)
        {
            auto result = Save();
            if (result != ReturnCode::OK)
            {
                return result;
            }
        }
    }
    m_browser->DeleteAllItems();
    m_browser->SetImageList(m_imgs);
    m_properties->GetGrid()->Clear();
    m_g.reset();
    for (const auto& editor : m_editors)
    {
        editor.second->ClearGameData();
    }
    SetMode(MODE_NONE);
    this->SetLabel("Landstalker Graphics Viewer");
    m_mnu_save_as_asm->Enable(false);
    m_mnu_save_to_rom->Enable(false);
    m_mnu_export->Enable(false);
    return ReturnCode::OK;
}

bool MainFrame::CheckForFileChanges()
{
    if (m_g && m_g->HasBeenModified())
    {
        return true;
    }
    return false;
}

void MainFrame::OpenFile(const wxString& path)
{
    auto extension = std::filesystem::path(path.ToStdString()).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
        [](const unsigned char i) { return std::tolower(i); });
    if (extension == ".asm")
    {
        OpenAsmFile(path);
    }
    else
    {
        OpenRomFile(path);
    }
}

void MainFrame::OnOpen(wxCommandEvent& event)
{
    wxFileDialog fdlog(this, "Open file", "", "",
        "All compatible files (*.bin; *.md; landstalker*.asm)|landstalker*.asm;*.bin;*.md|"
        "ROM files (*.bin; *.md)|*.bin;*.md|"
        "Top-level assembler file (landstalker*.asm)|landstalker*.asm|"
        "All files (*.*)|*.*",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (fdlog.ShowModal() == wxID_OK)
    {
        OpenFile(fdlog.GetPath());
    }
    event.Skip();
}

void MainFrame::OnSaveAsAsm(wxCommandEvent& event)
{
    try
    {
        auto result = SaveAsAsm();
        if (result == ReturnCode::ERR)
        {
            wxMessageBox("Failed to save as assembler.");
        }
    }
    catch (const std::exception& e)
    {
        wxMessageBox(std::string("Failed to save as assembler: ") + e.what(), "Error", wxICON_ERROR);
    }
    event.Skip();
}

void MainFrame::OnSaveToRom(wxCommandEvent& event)
{
    try
    {
        auto result = SaveToRom();
        if (result == ReturnCode::ERR)
        {
            wxMessageBox("Failed to save to ROM.");
        }
    }
    catch (const std::exception& e)
    {
        wxMessageBox(std::string("Failed to save to ROM: ") + e.what(), "Error", wxICON_ERROR);
    }
    event.Skip();
}

void MainFrame::OnExport(wxCommandEvent& event)
{
    std::string filter = "";
    switch (m_mode)
    {
    case MODE_STRING:
        filter = "Text File (*.txt)|*.txt";
        break;
    case MODE_BLOCKSET:
    case MODE_IMAGE:
    case MODE_SPRITE:
        filter = "PNG Image (*.png)|*.png";
        break;
    case MODE_ROOMMAP:
        filter = "Text File (*.txt)|*.txt|PNG Image (*.png)|*.png";
    default:
        return;
    }
    wxFileDialog fdlog(this, _("Export"), "", "",
        filter, wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (fdlog.ShowModal() == wxID_OK)
    {
        std::string filename = std::string(fdlog.GetPath());
        std::string extension = "";
        if (filename.find_last_of('.') != std::string::npos)
        {
            extension = filename.substr(filename.find_last_of('.') + 1);
            std::transform(extension.begin(), extension.end(), extension.begin(),
                [](const unsigned char i) { return std::tolower(i); });
        }
        if (extension == "png")
        {
            ExportPng(filename);
        }
        else
        {
            ExportTxt(filename);
        }
    }
    event.Skip();
}

void MainFrame::InitRoom(uint16_t room)
{
    m_roomnum = room;
}

void MainFrame::PopulateRoomProperties(uint16_t room)
{
    m_properties->GetGrid()->Clear();
    std::ostringstream ss;
    ss.str(std::string());
    const auto rd = m_g->GetRoomData()->GetRoom(room);
    auto tm = m_g->GetRoomData()->GetMapForRoom(room);

    ss << "Room: " << std::dec << std::uppercase << std::setw(3) << std::setfill('0') << rd->index
        << " Tileset: 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(rd->tileset)
        << " Palette: 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(rd->room_palette)
        << " PriBlockset: 0x" << std::hex << std::uppercase << std::setw(1) << std::setfill('0') << static_cast<unsigned>(rd->pri_blockset)
        << " SecBlockset: 0x" << std::hex << std::uppercase << std::setw(1) << std::setfill('0') << static_cast<unsigned>(rd->sec_blockset)
        << " BGM: 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(rd->bgm)
        << " Map: " << rd->map;
    SetStatusText(ss.str());
    ss.str(std::string());
    ss << std::dec << std::uppercase << std::setw(3) << std::setfill('0') << room;
    m_properties->Append(new wxStringProperty("Room Number", "RN", std::to_string(rd->index)));
    m_properties->Append(new wxStringProperty("Tileset", "TS", Hex(rd->tileset)));
    m_properties->Append(new wxStringProperty("Room Palette", "RP", Hex(rd->room_palette)));
    m_properties->Append(new wxStringProperty("Primary Blockset", "PBT", std::to_string(rd->pri_blockset)));
    m_properties->Append(new wxStringProperty("Secondary Blockset", "SBT", Hex(rd->sec_blockset)));
    m_properties->Append(new wxStringProperty("BGM", "BGM", Hex(rd->bgm)));
    m_properties->Append(new wxStringProperty("Map", "M", rd->map));
    m_properties->Append(new wxStringProperty("Unknown Parameter 1", "UP1", std::to_string(rd->unknown_param1)));
    m_properties->Append(new wxStringProperty("Unknown Parameter 2", "UP2", std::to_string(rd->unknown_param2)));
    m_properties->Append(new wxStringProperty("Z Begin", "ZB", std::to_string(rd->room_z_begin)));
    m_properties->Append(new wxStringProperty("Z End", "ZE", std::to_string(rd->room_z_end)));
    m_properties->Append(new wxStringProperty("Tilemap Left Offset", "TLO", std::to_string(tm->GetData()->GetLeft())));
    m_properties->Append(new wxStringProperty("Tilemap Top Offset",  "TTO", std::to_string(tm->GetData()->GetTop())));
    m_properties->Append(new wxStringProperty("Tilemap Width", "TW", std::to_string(tm->GetData()->GetWidth())));
    m_properties->Append(new wxStringProperty("Tilemap Height", "TH", std::to_string(tm->GetData()->GetHeight())));
    m_properties->Append(new wxStringProperty("Heightmap Width", "HW", std::to_string(tm->GetData()->GetHeightmapWidth())));
    m_properties->Append(new wxStringProperty("Heightmap Height", "HH", std::to_string(tm->GetData()->GetHeightmapHeight())));
}

void MainFrame::EnableLayerControls(bool state)
{
    if (state != m_layer_controls_enabled)
    {
        m_optBgSelect->Enable(state);
        m_optFg1Select->Enable(state);
        m_optFg2Select->Enable(state);
        m_optHeightmapSelect->Enable(state);
        m_optSpritesSelect->Enable(state);
        m_checkBgVisible->Enable(state);
        m_checkFg1Visible->Enable(state);
        m_checkFg2Visible->Enable(state);
        m_checkHeightmapVisible->Enable(state);
        m_checkSpritesVisible->Enable(state);
        m_sliderBgOpacity->Enable(state);
        m_sliderFg1Opacity->Enable(state);
        m_sliderFg2Opacity->Enable(state);
        m_sliderHeightmapOpacity->Enable(state);
        m_sliderSpritesOpacity->Enable(state);
        
        if (state == true)
        {
            // Reset to default state
            m_optBgSelect->SetValue(true);
            m_checkBgVisible->SetValue(true);
            m_checkFg1Visible->SetValue(true);
            m_checkFg2Visible->SetValue(true);
            m_checkHeightmapVisible->SetValue(false);
            m_checkSpritesVisible->SetValue(true);
            m_sliderBgOpacity->SetValue(255);
            m_sliderFg1Opacity->SetValue(255);
            m_sliderFg2Opacity->SetValue(255);
            m_sliderHeightmapOpacity->SetValue(192);
            m_sliderSpritesOpacity->SetValue(255);
        }
        m_layer_controls_enabled = state;
    }
}

void MainFrame::SetMode(const Mode& mode)
{
    m_mode = mode;
    Refresh();
}

void MainFrame::Refresh()
{
    switch (m_mode)
    {
    case MODE_STRING:
    {
        ClearScreen();
        ShowEditor(EditorType::STRING);
        EnableLayerControls(false);
        break;
    }
    case MODE_TILESET:
    {
        // Display tileset
        auto ts = m_g->GetTileset(m_selname);
        m_tilesetEditor->Open(m_selname);
        ShowEditor(EditorType::TILESET);
        EnableLayerControls(false);
        break;
    }
    case MODE_ANIMATED_TILESET:
    {
        // Display animated tileset
        auto ts = m_g->GetAnimatedTileset(m_selname);
        m_tilesetEditor->OpenAnimated(m_selname);
        ShowEditor(EditorType::TILESET);
        EnableLayerControls(false);
        break;
    }
    case MODE_BLOCKSET:
        EnableLayerControls(false);
        ShowBitmap();
        DrawBlocks(m_selname, 16, 1, 0);
        break;
    case MODE_PALETTE:
        // Display palettes
        ClearScreen();
        ShowEditor(EditorType::PALETTE);
        EnableLayerControls(false);
        break;
    case MODE_ROOMMAP:
        // Display room map
        ShowBitmap();
        EnableLayerControls(true);
        InitRoom(m_seldata);
        PopulateRoomProperties(m_seldata);
        DrawTilemap(m_seldata);
        break;
    case MODE_SPRITE:
    {
        // Display sprite
        ShowBitmap();
        EnableLayerControls(false);
        DrawSprite(m_selname, m_seldata, 4);
        break;
    }
    case MODE_IMAGE:
        // Display image
        ShowBitmap();
        EnableLayerControls(false);
        DrawImage(m_selname, 2);
        break;
    case MODE_NONE:
    default:
        EnableLayerControls(false);
        ShowBitmap();
        ClearScreen();
        break;
    }
}

bool MainFrame::ExportPng(const std::string& filename)
{
    return false;
}

bool MainFrame::ExportTxt(const std::string& filename)
{
    std::ofstream ofs(filename);

    switch (m_mode)
    {
    case MODE_STRING:
        return true;
    case MODE_ROOMMAP:
        {
            auto map = m_g->GetRoomData()->GetMapForRoom(m_roomnum)->GetData();
            // Height Map
            std::string heightMapString;
            const std::size_t ROW_WIDTH = map->GetHeightmapWidth();
            const std::size_t ROW_HEIGHT = map->GetHeightmapHeight();

            ofs << "#HEIGHTMAP: X Y HEIGHT RESTRICTIONS CLASSIFICATION" << std::endl;
            for (int y = 0; y < ROW_HEIGHT; ++y)
                for (int x = 0; x < ROW_WIDTH; ++x)
                {
                    std::stringstream ss;
                    ss << static_cast<unsigned>(x) << " "
                        << static_cast<unsigned>(y) << " "
                        << std::hex << std::uppercase << std::setfill('0') << std::setw(1) << static_cast<unsigned>(map->GetHeight({x,y})) << " "
                        << std::setfill('0') << std::setw(1) << static_cast<unsigned>(map->GetCellProps({x,y})) << " "
                        << std::setfill('0') << std::setw(2) << static_cast<unsigned>(map->GetCellType({x,y}));
                    ofs << ss.str() << std::endl;
                }

            // Tile Map
            const std::size_t TILE_WIDTH = 32;
            const std::size_t TILE_HEIGHT = 16;

            ofs << "#MAP: X Y XY.X XY.Y FG BG" << std::endl;
            for (int y = 0; y < map->GetHeight(); ++y)
                for (int x = 0; x < map->GetWidth(); ++x)
                {
                    auto xy(map->ToIsometric({x, y}));
                    ofs << x << " " << y << " " << xy.x << " " << xy.y << " "
                        << static_cast<unsigned>(map->GetBlock(xy, Tilemap3D::Layer::FG)) << " "
                        << static_cast<unsigned>(map->GetBlock(xy, Tilemap3D::Layer::BG)) << std::endl;
                }
        }
        return true;
    }
    return false;
}

ImageList& MainFrame::GetImageList()
{
	return *m_imgs;
}

void MainFrame::OnBrowserSelect(wxTreeEvent& event)
{
    ProcessSelectedBrowserItem(event.GetItem());
    event.Skip();
}

void MainFrame::ProcessSelectedBrowserItem(const wxTreeItemId& item)
{
    m_selname = m_browser->GetItemText(item);
    TreeNodeData* item_data = static_cast<TreeNodeData*>(m_browser->GetItemData(item));
    m_seldata = item_data->GetValue();
    switch (item_data->GetNodeType())
    {
    case TreeNodeData::NODE_STRING:
        m_stringEditor->SetMode(static_cast<StringEditorFrame::Mode>(m_seldata));
        SetMode(MODE_STRING);
        break;
    case TreeNodeData::NODE_TILESET:
        SetMode(MODE_TILESET);
        break;
    case TreeNodeData::NODE_ANIM_TILESET:
        SetMode(MODE_ANIMATED_TILESET);
        break;
    case TreeNodeData::NODE_BLOCKSET:
    {
        SetMode(MODE_BLOCKSET);
        break;
    }
    case TreeNodeData::NODE_PALETTE:
    {
        static_cast<PaletteListFrame*>(m_editors.at(EditorType::PALETTE))->SetMode(static_cast<PaletteListFrame::Mode>(m_seldata));
        SetMode(MODE_PALETTE);
        break;
    }
    case TreeNodeData::NODE_ROOM:
        SetMode(MODE_ROOMMAP);
        break;
    case TreeNodeData::NODE_ROOM_HEIGHTMAP:
        SetMode(MODE_ROOMMAP);
        InitRoom(m_seldata);
        PopulateRoomProperties(m_seldata);
        DrawHeightmap(m_seldata);
        break;
    case TreeNodeData::NODE_ROOM_WARPS:
        SetMode(MODE_ROOMMAP);
        InitRoom(m_seldata);
        PopulateRoomProperties(m_seldata);
        DrawWarps(m_seldata);
        break;
    case TreeNodeData::NODE_SPRITE:
    {
        SetMode(MODE_SPRITE);
        break;
    }
    case TreeNodeData::NODE_IMAGE:
        SetMode(MODE_IMAGE);
        break;
    default:
        // do nothing
        break;
    }
}
void MainFrame::OnKeyDown(wxKeyEvent& event)
{
    event.Skip();
}
void MainFrame::OnKeyUp(wxKeyEvent& event)
{
    event.Skip();
}
void MainFrame::OnMousewheel(wxMouseEvent& event)
{
    event.Skip();
}
void MainFrame::OnScrollWindowKeyDown(wxKeyEvent& event)
{
    event.Skip();
}
void MainFrame::OnScrollWindowKeyUp(wxKeyEvent& event)
{
    event.Skip();
}
void MainFrame::OnScrollWindowResize(wxSizeEvent& event)
{
    event.Skip();
}
void MainFrame::OnScrollWindowLeftDown(wxMouseEvent& event)
{
    event.Skip();
}

bool pnpoly(const std::vector<wxPoint2DDouble>& poly, int x, int y)
{
    int i, j;
    bool c = false;
    for (i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
        if (((poly[i].m_y > y) != (poly[j].m_y > y)) &&
            (x < (poly[j].m_x - poly[i].m_x) * (y - poly[i].m_y) / (poly[j].m_y - poly[i].m_y) + poly[i].m_x))
            c = !c;
    }
    return c;
}

void MainFrame::GoToRoom(uint16_t room)
{
    auto r = m_browser->GetRootItem();
    wxTreeItemIdValue cookie;
    auto c = m_browser->GetFirstChild(r, cookie);
    while (c.IsOk() == true)
    {
        auto label = m_browser->GetItemText(c);
        if (label == "Rooms")
        {
            break;
        }
        c = m_browser->GetNextSibling(c);
    }
    c = m_browser->GetFirstChild(c, cookie);
    while (c.IsOk() == true)
    {
        TreeNodeData* itemData = static_cast<TreeNodeData*>(m_browser->GetItemData(c));
        if (itemData->GetValue() == room)
        {
            break;
        }
        c = m_browser->GetNextSibling(c);
    }
    c = m_browser->GetFirstChild(c, cookie);
    while (c.IsOk() == true)
    {
        TreeNodeData* itemData = static_cast<TreeNodeData*>(m_browser->GetItemData(c));
        if (itemData->GetNodeType() == TreeNodeData::NodeType::NODE_ROOM_WARPS)
        {
            break;
        }
        c = m_browser->GetNextSibling(c);
    }
    if (c.IsOk())
    {
        m_browser->SelectItem(c);
        ProcessSelectedBrowserItem(c);
    }
}

void MainFrame::OnScrollWindowLeftUp(wxMouseEvent& event)
{
    if (m_g != nullptr && m_mode == Mode::MODE_ROOMMAP)
    {
        auto map = m_g->GetRoomData()->GetMapForRoom(m_roomnum)->GetData();
        if (map)
        {
            int x, y;
            m_canvas->GetViewStart(&x, &y);
            x += event.GetX();
            y += event.GetY();
            for (const auto& wp : m_warp_poly)
            {
                if (pnpoly(wp.second, x, y))
                {
                    uint16_t room = (wp.first.room1 == m_roomnum) ? wp.first.room2 : wp.first.room1;
                    GoToRoom(room);
                    break;
                }
            }
            for (const auto& lp : m_link_poly)
            {
                if (pnpoly(lp.second, x, y))
                {
                    GoToRoom(lp.first);
                    break;
                }
            }
        }
    }
    event.Skip();
}
void MainFrame::OnScrollWindowMousewheel(wxMouseEvent& event)
{
    event.Skip();
}

void MainFrame::OnScrollWindowMouseMove(wxMouseEvent& event)
{
    if (m_g != nullptr && m_mode == Mode::MODE_ROOMMAP)
    {
        auto map = m_g->GetRoomData()->GetMapForRoom(m_roomnum)->GetData();
        if (map)
        {
            int x, y;
            m_canvas->GetViewStart(&x, &y);
            x += event.GetX();
            y += event.GetY();
            auto r = map->PixelToHMPoint({ x,y });
            auto msg = StrPrintf("(%d,%d) [%d,%d]", x, y, r.x, r.y);
            m_canvas->SetCursor(wxCURSOR_ARROW);
            for (const auto& wp : m_warp_poly)
            {
                if (pnpoly(wp.second, x, y))
                {
                    uint16_t room = (wp.first.room1 == m_roomnum) ? wp.first.room2 : wp.first.room1;
                    uint8_t wx = (wp.first.room1 == m_roomnum) ? wp.first.x2 : wp.first.x1;
                    uint8_t wy = (wp.first.room1 == m_roomnum) ? wp.first.y2 : wp.first.y1;
                    msg += StrPrintf(" - Warp to room %03d (%d,%d)", room, wx, wy);
                    m_canvas->SetCursor(wxCURSOR_HAND);
                    break;
                }
            }
            for (const auto& lp : m_link_poly)
            {
                if (pnpoly(lp.second, x, y))
                {
                    m_canvas->SetCursor(wxCURSOR_HAND);
                    break;
                }
            }
            m_statusbar->SetLabelText(msg);
        }
    }
    event.Skip();
}
void MainFrame::OnScrollWindowRightDown(wxMouseEvent& event)
{
    event.Skip();
}
void MainFrame::OnScrollWindowRightUp(wxMouseEvent& event)
{
    event.Skip();
}
void MainFrame::OnClose(wxCloseEvent& event)
{
    if(CloseFiles(!event.CanVeto()) != ReturnCode::OK)
    {
        event.Veto();
        return;
    }
    event.Skip();
}
void MainFrame::OnLayerOpacityChange(wxScrollEvent& event)
{
    DrawTilemap(m_roomnum);
    event.Skip();
}
void MainFrame::OnLayerSelect(wxCommandEvent& event)
{
    DrawTilemap(m_roomnum);
    event.Skip();
}
void MainFrame::OnLayerVisibilityChange(wxCommandEvent& event)
{
    DrawTilemap(m_roomnum);
    event.Skip();
}
