#include <user_interface/main/include/MainFrame.h>

#include <fstream>
#include <sstream>
#include <cstring>
#include <iomanip>
#include <set>
#include <codecvt>
#include <locale>
#include <algorithm>
#include <filesystem>
#include <stack>
#include <future>

#include <wx/wx.h>
#include <wx/filename.h>
#include <wx/aboutdlg.h>
#include <wx/dcclient.h>
#include <wx/msgdlg.h>
#include <wx/richmsgdlg.h>
#include <wx/colour.h>
#include <wx/graphics.h>
#include <wx/progdlg.h>

#include <landstalker/main/include/Rom.h>
#include <landstalker/2d_maps/include/Blockmap2D.h>
#include <landstalker/main/include/ImageBuffer.h>
#include <user_interface/misc/include/AssemblyBuilderDialog.h>
#include <user_interface/misc/include/PreferencesDialog.h>

MainFrame::MainFrame(wxWindow* parent, const std::string& filename)
    : MainFrameBaseClass(parent),
      m_mode(Mode::NONE),
	  m_activeEditor(nullptr),
      m_g(nullptr)
{
    Freeze();
    m_imgs = new ImageList();
    m_imgs32 = new ImageList(true);
    wxGridSizer* sizer = new wxGridSizer(1);
    m_editors.insert({ EditorType::TILESET, new TilesetEditorFrame(this->m_mainwin, m_imgs) });
    m_editors.insert({ EditorType::STRING, new StringEditorFrame(this->m_mainwin, m_imgs) });
    m_editors.insert({ EditorType::PALETTE, new PaletteListFrame(this->m_mainwin, m_imgs) });
    m_editors.insert({ EditorType::MAP_VIEW, new RoomViewerFrame(this->m_mainwin, m_imgs) });
    m_editors.insert({ EditorType::MAP_2D, new Map2DEditorFrame(this->m_mainwin, m_imgs) });
    m_editors.insert({ EditorType::BLOCKSET, new BlocksetEditorFrame(this->m_mainwin, m_imgs) });
    m_editors.insert({ EditorType::SPRITE, new SpriteEditorFrame(this->m_mainwin, m_imgs) });
    m_editors.insert({ EditorType::ENTITY, new EntityViewerFrame(this->m_mainwin, m_imgs) });
    m_editors.insert({ EditorType::BEHAVIOUR_SCRIPT, new BehaviourScriptEditorFrame(this->m_mainwin, m_imgs) });
    m_editors.insert({ EditorType::SCRIPT, new ScriptEditorFrame(this->m_mainwin, m_imgs) });
    m_editors.insert({ EditorType::SCRIPT_TABLE, new ScriptTableEditorFrame(this->m_mainwin, m_imgs) });
    m_editors.insert({ EditorType::PROGRESS_FLAGS, new ProgressFlagsEditorFrame(this->m_mainwin, m_imgs) });
    m_editors.insert({ EditorType::CHARACTER_SFX, new CharacterSfxEditorFrame(this->m_mainwin, m_imgs) });
    m_mainwin->SetBackgroundColour(*wxBLACK);
    for (const auto& editor : m_editors)
    {
        sizer->Add(editor.second, 1, wxEXPAND | wxALL);
        sizer->Hide(editor.second);
    }
    this->m_mainwin->SetSizer(sizer);
    sizer->Layout();
    SetMode(Mode::NONE);
    m_mnu_save_as_asm->Enable(false);
    m_mnu_save_to_rom->Enable(false);
    m_mnu_save->Enable(false);
    m_mnu_build_asm->Enable(false);
    m_mnu_run_emu->Enable(false);
    Thaw();
    if (!filename.empty())
    {
        OpenFile(filename.c_str());
    }
    InitConfig();
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
    this->Connect(EVT_RENAME_NAV_ITEM, wxCommandEventHandler(MainFrame::OnRenameNavItem), nullptr, this);
    this->Connect(EVT_DELETE_NAV_ITEM, wxCommandEventHandler(MainFrame::OnDeleteNavItem), nullptr, this);
    this->Connect(EVT_ADD_NAV_ITEM, wxCommandEventHandler(MainFrame::OnAddNavItem), nullptr, this);
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
    this->Disconnect(EVT_DELETE_NAV_ITEM, wxCommandEventHandler(MainFrame::OnDeleteNavItem), nullptr, this);
    this->Disconnect(EVT_ADD_NAV_ITEM, wxCommandEventHandler(MainFrame::OnAddNavItem), nullptr, this);

    delete m_imgs;
    delete m_imgs32;
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
    wxIcon icn;
    icn.CopyFromBitmap(m_imgs32->GetImage("msword"));
    info.SetIcon(icn);
    info.SetName(_("Landstalker Editor"));
    info.SetVersion(_("0.3.4"));
    info.SetCopyright(_("Copyright 2024 All Rights Reserved."));
    info.SetWebSite(_("https://github.com/lordmir/landstalker_editor"), _("GitHub"));
    info.AddDeveloper(_("LordMir <hase@redfern.xyz>"));
    info.SetDescription(_("User Guide: https://github.com/lordmir/landstalker_editor/wiki/"));
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
        OpenLabelsFile(path.ToStdString());
        m_rom.load_from_file(static_cast<std::string>(path));
        m_g = std::make_shared<GameData>();
        {
            auto future = std::async(std::launch::async, [this, &path] { return m_g->Open(m_rom); });
            auto dialog = wxProgressDialog("Opening ROM", "Reading data from ROM", 100, this);
            do
            {
                wxYield();
                auto progress = m_g->GetProgress();
                dialog.Update(static_cast<int>(5.0 + progress.second * 90.0), progress.first);

            } while (future.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready);
            if (!future.get())
            {
                throw std::runtime_error("Error opening ROM");
            }
            if (!m_g->IsReady())
            {
                throw std::runtime_error("Error opening ROM");
            }
            dialog.Update(99, "Preparing User Interface");
            this->SetLabel("Landstalker Editor - " + m_rom.get_description());
            m_asmfile = false;
            m_built_rom = path;
            InitUI();
        }
    }
    catch (const std::runtime_error& e)
    {
        CloseFiles(true);
        wxMessageBox(e.what());
    }
    SetMode(Mode::NONE);
}

void MainFrame::OpenAsmFile(const wxString& path)
{
    try
    {
        if (CloseFiles() != ReturnCode::OK)
        {
            return;
        }
        OpenLabelsFile(std::filesystem::path(path.ToStdString()).parent_path().string());
        m_g = std::make_shared<GameData>();
        {
            auto future = std::async(std::launch::async, [this, &path] { return m_g->Open(path.ToStdString()); });
            auto dialog = wxProgressDialog("Opening ASM", "Reading data from ASM", 100, this);
            do
            {
                wxYield();
                auto progress = m_g->GetProgress();
                dialog.Update(static_cast<int>(5.0 + progress.second * 90.0), progress.first);

            } while (future.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready);
            if (!future.get())
            {
                throw std::runtime_error("Error opening ASM");
            }
            if (!m_g->IsReady())
            {
                throw std::runtime_error("Error opening ASM");
            }
            dialog.Update(99, "Preparing User Interface");
            this->SetLabel("Landstalker Editor - " + path);
            wxFileName name(path);
            m_asmfile = true;
            m_last_asm = name.GetPath();
            InitUI();
        }
    }
    catch (const std::runtime_error& e)
    {
        CloseFiles(true);
        wxMessageBox(e.what());
    }
}

void MainFrame::InitUI()
{
    Freeze();
    m_browser->DeleteAllItems();
    m_browser->SetImageList(m_imgs);
    m_properties->GetGrid()->Clear();
    for (const auto& editor : m_editors)
    {
        editor.second->SetGameData(m_g);
    }
    SetMode(Mode::NONE);

    const int str_img = m_imgs->GetIdx("string");
    const int img_img = m_imgs->GetIdx("image");
    const int ts_img = m_imgs->GetIdx("tileset");
    const int ats_img = m_imgs->GetIdx("ats");
    const int fonts_img = m_imgs->GetIdx("fonts");
    const int bs_img = m_imgs->GetIdx("big_tiles");
    const int pal_img = m_imgs->GetIdx("palette");
    const int rm_img = m_imgs->GetIdx("room");
    const int spr_img = m_imgs->GetIdx("sprite");
    const int ent_img = m_imgs->GetIdx("entity");
    const int scr_img = m_imgs->GetIdx("script");
    const int bscr_img = m_imgs->GetIdx("bscript");
    const int data_img = m_imgs->GetIdx("data");
    const int dtable_img = m_imgs->GetIdx("data_table");

    wxTreeItemId nodeRoot = m_browser->AddRoot("");
    wxTreeItemId nodeS = m_browser->AppendItem(nodeRoot, "Strings", str_img, str_img, new TreeNodeData());
    wxTreeItemId nodeScript = m_browser->AppendItem(nodeRoot, "Script", scr_img, scr_img, new TreeNodeData());
    wxTreeItemId nodeData = m_browser->AppendItem(nodeRoot, "Data", data_img, data_img, new TreeNodeData());
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
    InsertNavItem(L"Rooms", rm_img);
    InsertNavItem(L"Entities", ent_img);
    InsertNavItem(L"Sprites", spr_img);

    m_browser->AppendItem(nodeScript, "Main Script", scr_img, scr_img, new TreeNodeData(TreeNodeData::Node::SCRIPT));
    if (m_g->GetScriptData()->HasTables())
    {
        auto nodeST = m_browser->AppendItem(nodeScript, "Script Tables", scr_img, scr_img, new TreeNodeData(TreeNodeData::Node::BASE));
        m_browser->AppendItem(nodeST, "Cutscene Table", scr_img, scr_img, new TreeNodeData(TreeNodeData::Node::SCRIPT_TABLE, static_cast<std::size_t>(ScriptTableDataViewModel::Mode::CUTSCENE) << 16));
        m_browser->AppendItem(nodeST, "Character Table", scr_img, scr_img, new TreeNodeData(TreeNodeData::Node::SCRIPT_TABLE, static_cast<std::size_t>(ScriptTableDataViewModel::Mode::CHARACTER) << 16));
        auto nodeSTS = m_browser->AppendItem(nodeST, "Shop Tables", scr_img, scr_img, new TreeNodeData(TreeNodeData::Node::BASE, static_cast<std::size_t>(ScriptTableDataViewModel::Mode::SHOP) << 16));
        for (std::size_t i = 0; i < m_g->GetScriptData()->GetShopTable()->size(); ++i)
        {
            m_browser->AppendItem(nodeSTS, StrPrintf("ShopTable%d", i), scr_img, scr_img, new TreeNodeData(TreeNodeData::Node::SCRIPT_TABLE, (static_cast<std::size_t>(ScriptTableDataViewModel::Mode::SHOP) << 16) | i, scr_img, false));
        }
        auto nodeSTI = m_browser->AppendItem(nodeST, "Item Tables", scr_img, scr_img, new TreeNodeData(TreeNodeData::Node::BASE, static_cast<std::size_t>(ScriptTableDataViewModel::Mode::ITEM)));
        for (std::size_t i = 0; i < m_g->GetScriptData()->GetItemTable()->size(); ++i)
        {
            m_browser->AppendItem(nodeSTI, StrPrintf("ItemTable%d", i), scr_img, scr_img, new TreeNodeData(TreeNodeData::Node::SCRIPT_TABLE, (static_cast<std::size_t>(ScriptTableDataViewModel::Mode::ITEM) << 16) | i, scr_img, false));
        }
        m_browser->AppendItem(nodeScript, "Progress Flags", dtable_img, dtable_img, new TreeNodeData(TreeNodeData::Node::PROGRESS_FLAGS));
    }
    m_browser->AppendItem(nodeScript, "Entity Scripts", bscr_img, bscr_img, new TreeNodeData(TreeNodeData::Node::BEHAVIOUR_SCRIPT));

    m_browser->AppendItem(nodeData, "Character Sound Effects", dtable_img, dtable_img, new TreeNodeData(TreeNodeData::Node::CHARACTER_SFX));

    m_browser->AppendItem(nodeS, "Compressed Strings", str_img, str_img, new TreeNodeData(TreeNodeData::Node::STRING,
        static_cast<int>(StringData::Type::MAIN)));
    m_browser->AppendItem(nodeS, "Character Names", str_img, str_img, new TreeNodeData(TreeNodeData::Node::STRING,
        static_cast<int>(StringData::Type::NAMES)));
    m_browser->AppendItem(nodeS, "Special Character Names", str_img, str_img, new TreeNodeData(TreeNodeData::Node::STRING,
        static_cast<int>(StringData::Type::SPECIAL_NAMES)));
    m_browser->AppendItem(nodeS, "Default Character Name", str_img, str_img, new TreeNodeData(TreeNodeData::Node::STRING,
        static_cast<int>(StringData::Type::DEFAULT_NAME)));
    m_browser->AppendItem(nodeS, "Item Names", str_img, str_img, new TreeNodeData(TreeNodeData::Node::STRING,
        static_cast<int>(StringData::Type::ITEM_NAMES)));
    m_browser->AppendItem(nodeS, "Menu Strings", str_img, str_img, new TreeNodeData(TreeNodeData::Node::STRING,
        static_cast<int>(StringData::Type::MENU)));
    m_browser->AppendItem(nodeS, "Intro Strings", str_img, str_img, new TreeNodeData(TreeNodeData::Node::STRING,
        static_cast<int>(StringData::Type::INTRO)));
    m_browser->AppendItem(nodeS, "End Credit Strings", str_img, str_img, new TreeNodeData(TreeNodeData::Node::STRING,
        static_cast<int>(StringData::Type::END_CREDITS)));
    if (m_g->GetStringData()->GetSystemStringCount() > 0)
    {
        m_browser->AppendItem(nodeS, "System Strings", str_img, str_img, new TreeNodeData(TreeNodeData::Node::STRING,
            static_cast<int>(StringData::Type::SYSTEM)));
    }

    m_browser->AppendItem(nodeP, "Room Palettes", pal_img, pal_img, new TreeNodeData(TreeNodeData::Node::PALETTE,
        static_cast<int>(PaletteListFrame::Mode::ROOM)));
    m_browser->AppendItem(nodeP, "Misc Room Palettes", pal_img, pal_img, new TreeNodeData(TreeNodeData::Node::PALETTE,
        static_cast<int>(PaletteListFrame::Mode::ROOM_MISC)));
    m_browser->AppendItem(nodeP, "Sprite Low Palettes", pal_img, pal_img, new TreeNodeData(TreeNodeData::Node::PALETTE,
        static_cast<int>(PaletteListFrame::Mode::SPRITE_LO)));
    m_browser->AppendItem(nodeP, "Sprite High Palettes", pal_img, pal_img, new TreeNodeData(TreeNodeData::Node::PALETTE,
        static_cast<int>(PaletteListFrame::Mode::SPRITE_HI)));
    m_browser->AppendItem(nodeP, "Sprite Projectile Palettes", pal_img, pal_img, new TreeNodeData(TreeNodeData::Node::PALETTE,
        static_cast<int>(PaletteListFrame::Mode::PROJECTILE)));
    m_browser->AppendItem(nodeP, "Equipment Palettes", pal_img, pal_img, new TreeNodeData(TreeNodeData::Node::PALETTE,
        static_cast<int>(PaletteListFrame::Mode::EQUIP)));
    m_browser->AppendItem(nodeP, "Misc Palettes", pal_img, pal_img, new TreeNodeData(TreeNodeData::Node::PALETTE,
        static_cast<int>(PaletteListFrame::Mode::MISC)));

    for (int i = 0; i < 255; ++i)
    {
        if (!m_g->GetSpriteData()->IsSprite(i))
        {
            continue;
        }
        const std::wstring spr_name = SpriteData::GetSpriteDisplayName(i);
        InsertNavItem(L"Sprites/" + spr_name, spr_img, TreeNodeData::Node::SPRITE, i, false);
    }
    for (int i = 0; i < 255; ++i)
    {
        if (!m_g->GetSpriteData()->IsEntity(i))
        {
            continue;
        }
        const std::wstring ent_name = SpriteData::GetEntityDisplayName(i);
        InsertNavItem(L"Entities/" + ent_name, ent_img, TreeNodeData::Node::ENTITY, i, false);
    }

    for (const auto& t : m_g->GetRoomData()->GetTilesets())
    {
        auto ts_node = m_browser->AppendItem(nodeTs, t->GetName(), ts_img, ts_img, new TreeNodeData(TreeNodeData::Node::TILESET));
        for (const auto& at : m_g->GetRoomData()->GetAnimatedTilesets(t->GetName()))
        {
            m_browser->AppendItem(ts_node, at->GetName(), ats_img, ats_img, new TreeNodeData(TreeNodeData::Node::ANIM_TILESET));
        }
        auto pri = m_browser->AppendItem(nodeBs, t->GetName(), bs_img, bs_img, new TreeNodeData(TreeNodeData::Node::BASE));
        for (const auto& bs : m_g->GetRoomData()->GetBlocksetList(t->GetName()))
        {
            m_browser->AppendItem(pri, bs->GetName(), bs_img, bs_img, new TreeNodeData(TreeNodeData::Node::BLOCKSET, (bs->GetIndex().first << 8) | bs->GetIndex().second));
        }
    }

    for (const auto& map : m_g->GetGraphicsData()->GetUIMaps())
    {
        m_browser->AppendItem(nodeGU, map->GetName(), img_img, img_img, new TreeNodeData(TreeNodeData::Node::IMAGE));
    }
    for (const auto& map : m_g->GetStringData()->GetTextboxMaps())
    {
        m_browser->AppendItem(nodeGU, map->GetName(), img_img, img_img, new TreeNodeData(TreeNodeData::Node::IMAGE));
    }
    m_browser->AppendItem(nodeGE, m_g->GetGraphicsData()->GetEndCreditLogosMaps()->GetName(), img_img, img_img, new TreeNodeData(TreeNodeData::Node::IMAGE));
    for (const auto& map : m_g->GetGraphicsData()->GetIslandMapMaps())
    {
        m_browser->AppendItem(nodeGI, map->GetName(), img_img, img_img, new TreeNodeData(TreeNodeData::Node::IMAGE));
    }
    m_browser->AppendItem(nodeGL, m_g->GetGraphicsData()->GetLithographMap()->GetName(), img_img, img_img, new TreeNodeData(TreeNodeData::Node::IMAGE));
    for (const auto& map : m_g->GetGraphicsData()->GetTitleScreenMap())
    {
        m_browser->AppendItem(nodeGT, map->GetName(), img_img, img_img, new TreeNodeData(TreeNodeData::Node::IMAGE));
    }
    m_browser->AppendItem(nodeGC, m_g->GetGraphicsData()->GetClimaxLogoMap()->GetName(), img_img, img_img, new TreeNodeData(TreeNodeData::Node::IMAGE));
    m_browser->AppendItem(nodeGLo, m_g->GetGraphicsData()->GetGameLoadScreenMap()->GetName(), img_img, img_img, new TreeNodeData(TreeNodeData::Node::IMAGE));

    m_browser->AppendItem(nodeGF, m_g->GetRoomData()->GetIntroFont()->GetName(), fonts_img, fonts_img, new TreeNodeData(TreeNodeData::Node::TILESET));
    for (const auto& ts : m_g->GetStringData()->GetFonts())
    {
        m_browser->AppendItem(nodeGF, ts->GetName(), fonts_img, fonts_img, new TreeNodeData(TreeNodeData::Node::TILESET));
    }
    for (const auto& ts : m_g->GetGraphicsData()->GetFonts())
    {
        m_browser->AppendItem(nodeGF, ts->GetName(), fonts_img, fonts_img, new TreeNodeData(TreeNodeData::Node::TILESET));
    }
    for (const auto& ts : m_g->GetGraphicsData()->GetUIGraphics())
    {
        m_browser->AppendItem(nodeGU, ts->GetName(), ts_img, ts_img, new TreeNodeData(TreeNodeData::Node::TILESET));
    }
    for (const auto& ts : m_g->GetGraphicsData()->GetStatusEffects())
    {
        m_browser->AppendItem(nodeGS, ts->GetName(), ts_img, ts_img, new TreeNodeData(TreeNodeData::Node::TILESET));
    }
    for (const auto& ts : m_g->GetGraphicsData()->GetSwordEffects())
    {
        m_browser->AppendItem(nodeGW, ts->GetName(), ts_img, ts_img, new TreeNodeData(TreeNodeData::Node::TILESET));
    }
    m_browser->AppendItem(nodeGE, m_g->GetGraphicsData()->GetEndCreditLogosTiles()->GetName(), ts_img, ts_img, new TreeNodeData(TreeNodeData::Node::TILESET));
    for (const auto& ts : m_g->GetGraphicsData()->GetIslandMapTiles())
    {
        m_browser->AppendItem(nodeGI, ts->GetName(), ts_img, ts_img, new TreeNodeData(TreeNodeData::Node::TILESET));
    }
    m_browser->AppendItem(nodeGL, m_g->GetGraphicsData()->GetLithographTiles()->GetName(), ts_img, ts_img, new TreeNodeData(TreeNodeData::Node::TILESET));
    for (const auto& ts : m_g->GetGraphicsData()->GetTitleScreenTiles())
    {
        m_browser->AppendItem(nodeGT, ts->GetName(), ts_img, ts_img, new TreeNodeData(TreeNodeData::Node::TILESET));
    }
    m_browser->AppendItem(nodeGSe, m_g->GetGraphicsData()->GetSegaLogoTiles()->GetName(), ts_img, ts_img, new TreeNodeData(TreeNodeData::Node::TILESET));
    m_browser->AppendItem(nodeGC, m_g->GetGraphicsData()->GetClimaxLogoTiles()->GetName(), ts_img, ts_img, new TreeNodeData(TreeNodeData::Node::TILESET));
    for (const auto& ts : m_g->GetGraphicsData()->GetGameLoadScreenTiles())
    {
        m_browser->AppendItem(nodeGLo, ts->GetName(), ts_img, ts_img, new TreeNodeData(TreeNodeData::Node::TILESET));
    }

    for (const auto& room : m_g->GetRoomData()->GetRoomlist())
    {
        InsertNavItem(L"Rooms/" + room->GetDisplayName(), rm_img, TreeNodeData::Node::ROOM, room->index, false);
    }

    SortNavItems(m_browser->GetRootItem());

    m_mnu_save_as_asm->Enable(true);
    m_mnu_save_to_rom->Enable(true);
    if (m_asmfile)
    {
        m_mnu_build_asm->Enable(true);
    }
    else
    {
        m_mnu_run_emu->Enable(true);
    }
    Thaw();
}

void MainFrame::InitConfig()
{
    AssemblyBuilderDialog::InitConfig(m_config);
}

MainFrame::ReturnCode MainFrame::Save()
{
    if (m_last_was_asm == true)
    {
        return SaveAsAsm(m_last.ToStdString());
    }
    else
    {
        return SaveToRom(m_last.ToStdString());
    }
}

MainFrame::ReturnCode MainFrame::SaveLabelsFile(std::string path)
{
    std::filesystem::path path_obj = std::filesystem::path(path);
    if (std::filesystem::is_directory(path_obj))
    {
        path_obj /= "landstalker_labels.yaml";
    }
    else
    {
        std::string prefix = path_obj.has_stem() ? path_obj.stem().string() : "landstalker";
        path_obj = path_obj.replace_filename(prefix + "_labels.yaml");
    }
    Labels::SaveData(path_obj.string());
    return ReturnCode::OK;
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
            AssemblyBuilderDialog bdlg(this, path, m_g);
            bdlg.ShowModal();
            if (bdlg.DidOperationSucceed())
            {
                m_last_asm = path;
                m_last = path;
                m_last_was_asm = true;
                m_mnu_save->Enable(true);
                m_mnu_build_asm->Enable(true);

                auto romfile = wxFileName(path, "");
                romfile.SetFullName(bdlg.GetBuiltRomName());
                if (romfile.Exists())
                {
                    m_built_rom = romfile.GetFullPath();
                    m_mnu_run_emu->Enable(true);
                }
                SaveLabelsFile(path);
            }
        }
        return ReturnCode::OK;
    }
    catch (const std::exception&)
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
            SaveLabelsFile(path);
        }
        if (m_g)
        {
            auto dlg = AssemblyBuilderDialog(this, path, m_g, AssemblyBuilderDialog::Func::INJECT, std::make_shared<Rom>(m_rom));
            dlg.ShowModal();
            if (dlg.DidOperationSucceed() && wxFileName(path).Exists())
            {
                m_last_rom = path;
                m_built_rom = path;
                m_last = path;
                m_last_was_asm = false;
                m_mnu_run_emu->Enable(true);
                m_mnu_save->Enable(true);
            }
            return ReturnCode::OK;
        }
    }
    catch (const std::exception&)
    {
        throw;
    }
    return ReturnCode::ERR;
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
	frame->UpdateStatusBar(*this->m_statusbar, event);
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
    Freeze();
	frame->InitProperties(*this->m_properties);
    Thaw();
	event.Skip();
}

void MainFrame::OnPropertiesUpdate(wxCommandEvent& event)
{
	EditorFrame* frame = static_cast<EditorFrame*>(event.GetClientData());
    if (frame && m_properties->GetGrid())
    {
        Freeze();
        frame->UpdateProperties(*this->m_properties);
        Thaw();
    }
	event.Skip();
}

void MainFrame::OnPropertiesClear(wxCommandEvent& event)
{
	EditorFrame* frame = static_cast<EditorFrame*>(event.GetClientData());
    Freeze();
	frame->ClearProperties(*this->m_properties);
    Thaw();
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
    Freeze();
	frame->InitMenu(*this->m_menubar, *m_imgs);
    Thaw();
	event.Skip();
}

void MainFrame::OnMenuClear(wxCommandEvent& event)
{
	EditorFrame* frame = static_cast<EditorFrame*>(event.GetClientData());
    Freeze();
	frame->ClearMenu(*this->m_menubar);
    Thaw();
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
        Freeze();
		auto* pane = event.GetPane();
		pane->Hide();
		m_activeEditor->UpdateUI();
        Thaw();
	}
	event.Skip();
}

void MainFrame::OnGoToNavItem(wxCommandEvent& event)
{
    GoToNavItem(std::wstring(event.GetString()), event.GetInt());
}

void MainFrame::OnRenameNavItem(wxCommandEvent& event)
{
    std::wstring combined(event.GetString().ToStdWstring());
    int pos = combined.find_first_of(L'\1');
    std::wstring new_path(combined.substr(0, pos));
    std::wstring old_path(combined.substr(pos + 1));
    
    RenameNavItem(old_path, new_path);
}

void MainFrame::OnDeleteNavItem(wxCommandEvent& event)
{
    DeleteNavItem(event.GetString().ToStdWstring());
}

void MainFrame::OnAddNavItem(wxCommandEvent& event)
{
    AddNavItem(event.GetString().ToStdWstring(), event.GetExtraLong(), static_cast<TreeNodeData::Node>(event.GetId()), event.GetInt(), false);
}

std::optional<wxTreeItemId> MainFrame::FindNavItem(const std::wstring& path)
{
    std::wistringstream ss(path);
    wxTreeItemIdValue cookie;
    std::wstring name;
    auto c = m_browser->GetRootItem();
    while (std::getline(ss, name, L'/'))
    {
        if (!c.IsOk())
        {
            return std::nullopt;
        }
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
        return c;
    }
    return std::nullopt;
}

std::optional<wxTreeItemId> MainFrame::InsertNavItem(const std::wstring& path, int img, const TreeNodeData::Node& type, int value, bool no_delete)
{
    static const int CLOSED_FOLDER_ICON = m_imgs->GetIdx("closed_folder");
    static const int OPEN_FOLDER_ICON = m_imgs->GetIdx("open_folder");

    std::wistringstream ss(path);
    wxTreeItemIdValue cookie;
    std::wstring name;
    auto parent = m_browser->GetRootItem();
    auto child = parent;
    if (FindNavItem(path))
    {
        // Already exists
        return FindNavItem(path);
    }
    while (std::getline(ss, name, L'/'))
    {
        child = m_browser->GetFirstChild(parent, cookie);
        while (child.IsOk() == true)
        {
            auto label = m_browser->GetItemText(child);
            if (label == name)
            {
                parent = child;
                break;
            }
            child = m_browser->GetNextSibling(child);
        }
        if (parent.IsOk() && !child.IsOk())
        {
            // Child not found, create one
            if (ss.eof())
            {
                // Final Node
                child = m_browser->AppendItem(parent, name, img, img, new TreeNodeData(type, value, img, no_delete));
            }
            else
            {
                // Subdirectory
                child = m_browser->AppendItem(parent, name, CLOSED_FOLDER_ICON, CLOSED_FOLDER_ICON,
                    new TreeNodeData(TreeNodeData::Node::BASE, value, CLOSED_FOLDER_ICON, no_delete));
                m_browser->SetItemImage(child, OPEN_FOLDER_ICON, wxTreeItemIcon_Expanded);
                m_browser->SetItemImage(child, OPEN_FOLDER_ICON, wxTreeItemIcon_SelectedExpanded);
            }
            parent = child;
        }
    }
    if (ss.eof() && child.IsOk())
    {
        return child;
    }
    return std::nullopt;
}

void MainFrame::SortNavItems(const wxTreeItemId& parent)
{
    if (m_browser->HasChildren(parent))
    {
        wxTreeItemIdValue cookie;
        
        m_browser->SortChildren(parent);
        auto child = m_browser->GetFirstChild(parent, cookie);
        do
        {
            if (m_browser->HasChildren(child))
            {
                SortNavItems(child);
            }
        } while ((child = m_browser->GetNextChild(parent, cookie)));
    }
}

bool MainFrame::RemoveNavItem(const std::wstring& path)
{
    std::stack<wxTreeItemId> path_elems;
    std::wistringstream ss(path);
    wxTreeItemIdValue cookie;
    std::wstring name;
    auto parent = m_browser->GetRootItem();
    auto child = parent;
    while (std::getline(ss, name, L'/'))
    {
        child = m_browser->GetFirstChild(parent, cookie);
        while (child.IsOk() == true)
        {
            auto label = m_browser->GetItemText(child);
            if (label == name)
            {
                parent = child;
                path_elems.push(child);
                break;
            }
            child = m_browser->GetNextSibling(child);
        }
        if (!child.IsOk())
        {
            // Path not found
            return false;
        }
    }
    while (!path_elems.empty() && m_browser->GetChildrenCount(path_elems.top()) == 0)
    {
        TreeNodeData* node_data = static_cast<TreeNodeData*>(m_browser->GetItemData(path_elems.top()));
        if (node_data->DoNotDelete() == true)
        {
            break;
        }
        m_browser->Delete(path_elems.top());
        path_elems.pop();
    }
    return true;
}

bool MainFrame::RenameNavItem(const std::wstring& old_path, const std::wstring& new_path)
{
    auto old_item = FindNavItem(old_path);
    auto new_item = FindNavItem(new_path);
    if (!old_item || new_item)
    {
        return false;
    }
    if (GetNavItemParent(old_path) == GetNavItemParent(new_path))
    {
        m_browser->SetItemText(*old_item, new_path.substr(new_path.find_last_of(L"/") + 1));
        return true;
    }
    TreeNodeData* node_data = static_cast<TreeNodeData*>(m_browser->GetItemData(*old_item));
    if (!node_data)
    {
        return false;
    }
    new_item = InsertNavItem(new_path, node_data->GetNodeImage(), node_data->GetNodeType(), node_data->GetValue(), node_data->DoNotDelete());
    if (!new_item)
    {
        return false;
    }
    if (!RemoveNavItem(old_path))
    {
        RemoveNavItem(new_path);
        return false;
    }
    SortNavItems(m_browser->GetRootItem());
    m_browser->SelectItem(*new_item);
    return true;
}

bool MainFrame::AddNavItem(const std::wstring& path, int image, const TreeNodeData::Node& type, int value, bool no_delete)
{
    auto item = InsertNavItem(path, image, type, value, no_delete);
    if (item)
    {
        m_browser->SelectItem(*item);
        ProcessSelectedBrowserItem(*item);
    }
    return true;
}

bool MainFrame::DeleteNavItem(const std::wstring& path)
{
    if (*FindNavItem(path) == m_browser->GetSelection())
    {
        m_browser->SelectItem(m_browser->GetNextVisible(m_browser->GetSelection()));
    }
    return RemoveNavItem(path);
}

std::wstring MainFrame::GetNavItemParent(const std::wstring& path)
{
    return path.substr(0, path.find_last_of(L'/'));
}

void MainFrame::GoToNavItem(const std::wstring& path, int data)
{
    auto item = FindNavItem(path);
    if (item)
    {
        m_browser->SelectItem(*item);
        ProcessSelectedBrowserItem(*item, data);
    }
}

void MainFrame::ShowEditor(EditorType editor)
{
    Freeze();
    if (!m_editors.at(editor)->IsShown())
    {
        m_activeEditor = m_editors.at(editor);
        for (const auto& ed : m_editors)
        {
            if (ed.second != m_activeEditor)
            {
                ed.second->Hide();
            }
        }
        m_activeEditor->Show();
        this->m_mainwin->GetSizer()->Clear();
        this->m_mainwin->GetSizer()->Add(m_activeEditor, 1, wxALL | wxEXPAND);
        this->m_mainwin->GetSizer()->Layout();
    }
    Thaw();
}

void MainFrame::HideAllEditors()
{
    m_activeEditor = nullptr;
    for (const auto& editor : m_editors)
    {
        editor.second->Hide();
    }
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
    Freeze();
    for (const auto& editor : m_editors)
    {
        editor.second->Hide();
        editor.second->ClearGameData();
        editor.second->ClearMenu(*m_menubar);
        editor.second->ClearStatusBar(*m_statusbar);
        editor.second->ClearProperties(*m_properties);
    }
    m_browser->DeleteAllItems();
    m_browser->SetImageList(m_imgs);
    m_properties->GetGrid()->Clear();
    m_g.reset();
    SetMode(Mode::NONE);
    this->SetLabel("Landstalker Editor");
    m_mnu_save_as_asm->Enable(false);
    m_mnu_save_to_rom->Enable(false);
    m_mnu_save->Enable(false);
    m_mnu_build_asm->Enable(false);
    m_mnu_run_emu->Enable(false);
    m_last.clear();
    m_last_asm.clear();
    m_last_rom.clear();
    m_built_rom.clear();
    Thaw();
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
    auto path_obj = std::filesystem::path(path.ToStdString());
    auto extension = path_obj.extension().string();
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

void MainFrame::OpenLabelsFile(std::string path)
{
    std::filesystem::path path_obj = std::filesystem::path(path);
    if (std::filesystem::is_directory(path_obj))
    {
        path_obj /= "landstalker_labels.yaml";
    }
    else
    {
        std::string prefix = path_obj.has_stem() ? path_obj.stem().string() : "landstalker";
        path_obj = path_obj.replace_filename(prefix + "_labels.yaml");
    }
    Labels::LoadData(path_obj.string());
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

void MainFrame::OnSave(wxCommandEvent& /*event*/)
{
    if (!m_last.empty())
    {
        Save();
    }
}

void MainFrame::OnBuildAsm(wxCommandEvent& /*event*/)
{
    if (m_last_asm.empty())
    {
        return;
    }
    AssemblyBuilderDialog bdlg(this, m_last_asm, m_g, AssemblyBuilderDialog::Func::BUILD);
    bdlg.ShowModal();
    if(bdlg.DidOperationSucceed())
    {
        auto romfile = wxFileName(m_last_asm, "");
        romfile.SetFullName(bdlg.GetBuiltRomName());
        if (romfile.Exists())
        {
            m_built_rom = romfile.GetFullPath();
            m_mnu_run_emu->Enable(true);
        }
    }
}

void MainFrame::OnRunEmulator(wxCommandEvent& /*event*/)
{
    if (m_built_rom.empty())
    {
        return;
    }
    AssemblyBuilderDialog bdlg(this, m_built_rom, m_g, AssemblyBuilderDialog::Func::RUN);
    bdlg.ShowModal();
}

void MainFrame::OnPreferences(wxCommandEvent& /*event*/)
{
    PreferencesDialog dlg(this, m_config);
    dlg.ShowModal();
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
    case Mode::STRING:
        // Display Strings
        GetStringEditor()->SetMode(static_cast<StringData::Type>(m_seldata));
        ShowEditor(EditorType::STRING);
        break;
    case Mode::TILESET:
        // Display tileset
        GetTilesetEditor()->Open(m_selname);
        ShowEditor(EditorType::TILESET);
        break;
    case Mode::ANIMATED_TILESET:
        // Display animated tileset
        GetTilesetEditor()->OpenAnimated(m_selname);
        ShowEditor(EditorType::TILESET);
        break;
    case Mode::BLOCKSET:
        // Display Blockset
        GetBlocksetEditor()->Open(m_selname);
        ShowEditor(EditorType::BLOCKSET);
        break;
    case Mode::PALETTE:
        // Display palettes
        GetPaletteEditor()->SetMode(static_cast<PaletteListFrame::Mode>(m_seldata));
        ShowEditor(EditorType::PALETTE);
        break;
    case Mode::ROOMMAP:
        // Display room map
        GetRoomEditor()->SetRoomNum(m_seldata & 0xFFFF);
        ShowEditor(EditorType::MAP_VIEW);
        break;
    case Mode::SPRITE:
        // Display sprite
        GetSpriteEditor()->Open(m_seldata & 0xFF);
        ShowEditor(EditorType::SPRITE);
        break;
    case Mode::IMAGE:
        // Display image
        GetMap2DEditor()->Open(m_selname);
        ShowEditor(EditorType::MAP_2D);
        break;
    case Mode::ENTITY:
        // Display entity
        GetEntityViewer()->Open(m_seldata);
        ShowEditor(EditorType::ENTITY);
        break;
    case Mode::BEHAVIOUR_SCRIPT:
        // Display entity
        GetBehaviourScriptEditor()->Open(m_extradata);
        ShowEditor(EditorType::BEHAVIOUR_SCRIPT);
        break;
    case Mode::SCRIPT:
        // Display script
        GetScriptEditor()->Open(m_extradata);
        ShowEditor(EditorType::SCRIPT);
        break;
    case Mode::SCRIPT_TABLE:
        // Display script table
        GetScriptTableEditor()->Open(static_cast<ScriptTableDataViewModel::Mode>(m_seldata >> 16), m_seldata & 0xFFFF, m_extradata);
        ShowEditor(EditorType::SCRIPT_TABLE);
        break;
    case Mode::PROGRESS_FLAGS:
        // Display flags
        GetProgressFlagsEditorFrame()->Open(m_seldata & 0xFFFF, m_extradata);
        ShowEditor(EditorType::PROGRESS_FLAGS);
        break;
    case Mode::CHARACTER_SFX:
        // Display character SFX
        GetCharacterSfxEditorFrame()->Open(m_extradata);
        ShowEditor(EditorType::CHARACTER_SFX);
        break;
    case Mode::NONE:
    default:
        HideAllEditors();
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

void MainFrame::ProcessSelectedBrowserItem(const wxTreeItemId& item, int data)
{
    m_selname = m_browser->GetItemText(item);
    TreeNodeData* item_data = static_cast<TreeNodeData*>(m_browser->GetItemData(item));
    m_seldata = item_data->GetValue();
    m_extradata = data;
    switch (item_data->GetNodeType())
    {
    case TreeNodeData::Node::STRING:
        SetMode(Mode::STRING);
        break;
    case TreeNodeData::Node::TILESET:
        SetMode(Mode::TILESET);
        break;
    case TreeNodeData::Node::ANIM_TILESET:
        SetMode(Mode::ANIMATED_TILESET);
        break;
    case TreeNodeData::Node::BLOCKSET:
        SetMode(Mode::BLOCKSET);
        break;
    case TreeNodeData::Node::PALETTE:
        SetMode(Mode::PALETTE);
        break;
    case TreeNodeData::Node::ROOM:
        SetMode(Mode::ROOMMAP);
        break;
    case TreeNodeData::Node::SPRITE:
        SetMode(Mode::SPRITE);
        break;
    case TreeNodeData::Node::IMAGE:
        SetMode(Mode::IMAGE);
        break;
    case TreeNodeData::Node::ENTITY:
        SetMode(Mode::ENTITY);
        break;
    case TreeNodeData::Node::BEHAVIOUR_SCRIPT:
        SetMode(Mode::BEHAVIOUR_SCRIPT);
        break;
    case TreeNodeData::Node::SCRIPT:
        SetMode(Mode::SCRIPT);
        break;
    case TreeNodeData::Node::SCRIPT_TABLE:
        SetMode(Mode::SCRIPT_TABLE);
        break;
    case TreeNodeData::Node::PROGRESS_FLAGS:
        SetMode(Mode::PROGRESS_FLAGS);
        break;
    case TreeNodeData::Node::CHARACTER_SFX:
        SetMode(Mode::CHARACTER_SFX);
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

BlocksetEditorFrame* MainFrame::GetBlocksetEditor()
{
    return static_cast<BlocksetEditorFrame*>(m_editors.at(EditorType::BLOCKSET));
}

SpriteEditorFrame* MainFrame::GetSpriteEditor()
{
    return static_cast<SpriteEditorFrame*>(m_editors.at(EditorType::SPRITE));
}

EntityViewerFrame* MainFrame::GetEntityViewer()
{
    return static_cast<EntityViewerFrame*>(m_editors.at(EditorType::ENTITY));
}

BehaviourScriptEditorFrame* MainFrame::GetBehaviourScriptEditor()
{
    return static_cast<BehaviourScriptEditorFrame*>(m_editors.at(EditorType::BEHAVIOUR_SCRIPT));
}

ScriptEditorFrame* MainFrame::GetScriptEditor()
{
    return static_cast<ScriptEditorFrame*>(m_editors.at(EditorType::SCRIPT));
}

ScriptTableEditorFrame* MainFrame::GetScriptTableEditor()
{
    return static_cast<ScriptTableEditorFrame*>(m_editors.at(EditorType::SCRIPT_TABLE));
}

ProgressFlagsEditorFrame* MainFrame::GetProgressFlagsEditorFrame()
{
    return static_cast<ProgressFlagsEditorFrame*>(m_editors.at(EditorType::PROGRESS_FLAGS));
}

CharacterSfxEditorFrame* MainFrame::GetCharacterSfxEditorFrame()
{
    return static_cast<CharacterSfxEditorFrame*>(m_editors.at(EditorType::CHARACTER_SFX));
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
