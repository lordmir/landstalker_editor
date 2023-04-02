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
      m_mode(MODE_NONE),
	  m_activeEditor(nullptr),
      m_g(nullptr)
{
    m_imgs = new ImageList();
    wxGridSizer* sizer = new wxGridSizer(1);
    m_canvas = new wxScrolledCanvas(this->m_scrollwindow);
    m_editors.insert({ EditorType::TILESET, new TilesetEditorFrame(this->m_scrollwindow) });
    m_editors.insert({ EditorType::STRING, new StringEditorFrame(this->m_scrollwindow) });
    m_editors.insert({ EditorType::PALETTE, new PaletteListFrame(this->m_scrollwindow) });
    m_editors.insert({ EditorType::MAP_VIEW, new RoomViewerFrame(this->m_scrollwindow) });
    m_editors.insert({ EditorType::MAP_2D, new Map2DEditorFrame(this->m_scrollwindow) });
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
    if (!filename.empty())
    {
        OpenFile(filename.c_str());
    }
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
    this->Connect(EVT_GO_TO_NAV_ITEM, wxCommandEventHandler(MainFrame::OnGoToNavItem), nullptr, this);
    m_canvas->Connect(wxEVT_PAINT, wxPaintEventHandler(MainFrame::OnScrollWindowPaint), NULL, this);
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
    this->Disconnect(EVT_GO_TO_NAV_ITEM, wxCommandEventHandler(MainFrame::OnGoToNavItem), nullptr, this);
    m_canvas->Disconnect(wxEVT_PAINT, wxPaintEventHandler(MainFrame::OnScrollWindowPaint), NULL, this);

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
    info.SetCopyright(_("Landstalker Editor"));
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
        this->SetLabel("Landstalker Editor - " + m_rom.get_description());
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
        this->SetLabel("Landstalker Editor - " + path);
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
        wxTreeItemId cRm = m_browser->AppendItem(nodeRm, room->name, rm_img, rm_img, new TreeNodeData(TreeNodeData::NODE_ROOM,
            (static_cast<int>(RoomViewerCtrl::Mode::NORMAL) << 16) | room->index));
        m_browser->AppendItem(cRm, "Heightmap", rm_img, rm_img, new TreeNodeData(TreeNodeData::NODE_ROOM,
            (static_cast<int>(RoomViewerCtrl::Mode::HEIGHTMAP) << 16) | room->index));
        m_browser->AppendItem(cRm, "Warps", rm_img, rm_img, new TreeNodeData(TreeNodeData::NODE_ROOM,
            (static_cast<int>(RoomViewerCtrl::Mode::WARPS) << 16) | room->index));
        m_browser->AppendItem(cRm, "Entities", rm_img, rm_img, new TreeNodeData(TreeNodeData::NODE_ROOM,
            (static_cast<int>(RoomViewerCtrl::Mode::ENTITY_PLACEMENT) << 16) | room->index));
    }
    m_mnu_save_as_asm->Enable(true);
    m_mnu_save_to_rom->Enable(true);
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

void MainFrame::OnGoToNavItem(wxCommandEvent& event)
{
    GoToNavItem(std::string(event.GetString()));
}

void MainFrame::GoToNavItem(const std::string& path)
{
    std::istringstream ss(path);
    wxTreeItemIdValue cookie;
    std::string name;
    auto c = m_browser->GetRootItem();
    while (std::getline(ss, name, '/'))
    {
        c = m_browser->GetFirstChild(c, cookie);
        while (c.IsOk() == true)
        {
            auto label = m_browser->GetItemText(c);
            if (label == name)
            {
                break;
            }
            c = m_browser->GetNextSibling(c);
        }
    }
    if (ss.eof() && c.IsOk())
    {
        m_browser->SelectItem(c);
        ProcessSelectedBrowserItem(c);
    }
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
    this->SetLabel("Landstalker Editor");
    m_mnu_save_as_asm->Enable(false);
    m_mnu_save_to_rom->Enable(false);
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
    m_filehistory->AddFileToHistory(path);
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
    event.Skip();
}

void MainFrame::OnMRUFile(wxCommandEvent& event)
{
    wxString f(m_filehistory->GetHistoryFile(event.GetId() - wxID_FILE1));
    if (!f.empty())
        OpenFile(f);
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
        break;
    }
    case MODE_TILESET:
    {
        // Display tileset
        GetTilesetEditor()->Open(m_selname);
        ShowEditor(EditorType::TILESET);
        break;
    }
    case MODE_ANIMATED_TILESET:
    {
        // Display animated tileset
        auto ts = m_g->GetAnimatedTileset(m_selname);
        GetTilesetEditor()->OpenAnimated(m_selname);
        ShowEditor(EditorType::TILESET);
        break;
    }
    case MODE_BLOCKSET:
        ShowBitmap();
        DrawBlocks(m_selname, 16, 1, 0);
        break;
    case MODE_PALETTE:
        // Display palettes
        ClearScreen();
        ShowEditor(EditorType::PALETTE);
        break;
    case MODE_ROOMMAP:
        // Display room map
        ShowBitmap();
        GetRoomEditor()->SetRoomNum(m_seldata & 0xFFFF, static_cast<RoomViewerCtrl::Mode>(m_seldata >> 16));
        ShowEditor(EditorType::MAP_VIEW);
        break;
    case MODE_SPRITE:
    {
        // Display sprite
        ShowBitmap();
        DrawSprite(m_selname, m_seldata, 4);
        break;
    }
    case MODE_IMAGE:
        // Display image
        GetMap2DEditor()->Open(m_selname);
        ShowEditor(EditorType::MAP_2D);
        //DrawImage(m_selname, 2);
        break;
    case MODE_NONE:
    default:
        ShowBitmap();
        ClearScreen();
        break;
    }
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
        GetStringEditor()->SetMode(static_cast<StringEditorFrame::Mode>(m_seldata));
        SetMode(MODE_STRING);
        break;
    case TreeNodeData::NODE_TILESET:
        SetMode(MODE_TILESET);
        break;
    case TreeNodeData::NODE_ANIM_TILESET:
        SetMode(MODE_ANIMATED_TILESET);
        break;
    case TreeNodeData::NODE_BLOCKSET:
        SetMode(MODE_BLOCKSET);
        break;
    case TreeNodeData::NODE_PALETTE:
        GetPaletteEditor()->SetMode(static_cast<PaletteListFrame::Mode>(m_seldata));
        SetMode(MODE_PALETTE);
        break;
    case TreeNodeData::NODE_ROOM:
        SetMode(MODE_ROOMMAP);
        break;
    case TreeNodeData::NODE_SPRITE:
        SetMode(MODE_SPRITE);
        break;
    case TreeNodeData::NODE_IMAGE:
        SetMode(MODE_IMAGE);
        break;
    default:
        // do nothing
        break;
    }
}

TilesetEditorFrame* MainFrame::GetTilesetEditor()
{
    return static_cast<TilesetEditorFrame*>(m_editors.at(EditorType::TILESET));
}

StringEditorFrame* MainFrame::GetStringEditor()
{
    return static_cast<StringEditorFrame*>(m_editors.at(EditorType::STRING));
}

PaletteListFrame* MainFrame::GetPaletteEditor()
{
    return static_cast<PaletteListFrame*>(m_editors.at(EditorType::PALETTE));
}

RoomViewerFrame* MainFrame::GetRoomEditor()
{
    return static_cast<RoomViewerFrame*>(m_editors.at(EditorType::MAP_VIEW));
}

Map2DEditorFrame* MainFrame::GetMap2DEditor()
{
    return static_cast<Map2DEditorFrame*>(m_editors.at(EditorType::MAP_2D));
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
