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
#include "AssemblyBuilderDialog.h"
#include "PreferencesDialog.h"

MainFrame::MainFrame(wxWindow* parent, const std::string& filename)
    : MainFrameBaseClass(parent),
      m_mode(MODE_NONE),
	  m_activeEditor(nullptr),
      m_g(nullptr)
{
    m_imgs = new ImageList();
    wxGridSizer* sizer = new wxGridSizer(1);
    m_editors.insert({ EditorType::TILESET, new TilesetEditorFrame(this->m_mainwin, m_imgs) });
    m_editors.insert({ EditorType::STRING, new StringEditorFrame(this->m_mainwin, m_imgs) });
    m_editors.insert({ EditorType::PALETTE, new PaletteListFrame(this->m_mainwin, m_imgs) });
    m_editors.insert({ EditorType::MAP_VIEW, new RoomViewerFrame(this->m_mainwin, m_imgs) });
    m_editors.insert({ EditorType::MAP_2D, new Map2DEditorFrame(this->m_mainwin, m_imgs) });
    m_editors.insert({ EditorType::BLOCKSET, new BlocksetEditorFrame(this->m_mainwin, m_imgs) });
    m_editors.insert({ EditorType::SPRITE, new SpriteEditorFrame(this->m_mainwin, m_imgs) });
    m_mainwin->SetBackgroundColour(*wxBLACK);
    for (const auto& editor : m_editors)
    {
        sizer->Add(editor.second, 1, wxEXPAND | wxALL);
        sizer->Hide(editor.second);
    }
    this->m_mainwin->SetSizer(sizer);
    sizer->Layout();
    SetMode(MODE_NONE);
    m_mnu_save_as_asm->Enable(false);
    m_mnu_save_to_rom->Enable(false);
    m_mnu_save->Enable(false);
    m_mnu_build_asm->Enable(false);
    m_mnu_run_emu->Enable(false);
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
    info.SetDescription(_("Github: https://github.com/lordmir/landstalker_editor \nEmail: hase@redfern.xyz"));
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
        m_asmfile = false;
        m_built_rom = path;
        InitUI();
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
        m_asmfile = true;
        m_last_asm = path;
        InitUI();
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
        static_cast<int>(StringData::Type::MAIN)));
    m_browser->AppendItem(nodeS, "Character Names", str_img, str_img, new TreeNodeData(TreeNodeData::NODE_STRING,
        static_cast<int>(StringData::Type::NAMES)));
    m_browser->AppendItem(nodeS, "Special Character Names", str_img, str_img, new TreeNodeData(TreeNodeData::NODE_STRING,
        static_cast<int>(StringData::Type::SPECIAL_NAMES)));
    m_browser->AppendItem(nodeS, "Default Character Name", str_img, str_img, new TreeNodeData(TreeNodeData::NODE_STRING,
        static_cast<int>(StringData::Type::DEFAULT_NAME)));
    m_browser->AppendItem(nodeS, "Item Names", str_img, str_img, new TreeNodeData(TreeNodeData::NODE_STRING,
        static_cast<int>(StringData::Type::ITEM_NAMES)));
    m_browser->AppendItem(nodeS, "Menu Strings", str_img, str_img, new TreeNodeData(TreeNodeData::NODE_STRING,
        static_cast<int>(StringData::Type::MENU)));
    m_browser->AppendItem(nodeS, "Intro Strings", str_img, str_img, new TreeNodeData(TreeNodeData::NODE_STRING,
        static_cast<int>(StringData::Type::INTRO)));
    m_browser->AppendItem(nodeS, "End Credit Strings", str_img, str_img, new TreeNodeData(TreeNodeData::NODE_STRING,
        static_cast<int>(StringData::Type::END_CREDITS)));
    if (m_g->GetStringData()->GetSystemStringCount() > 0)
    {
        m_browser->AppendItem(nodeS, "System Strings", str_img, str_img, new TreeNodeData(TreeNodeData::NODE_STRING,
            static_cast<int>(StringData::Type::SYSTEM)));
    }

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
                int j = 0;
                auto entity_node = m_browser->AppendItem(spr_node, StrPrintf("Entity %03d", entity), spr_img, spr_img, new TreeNodeData(TreeNodeData::NODE_SPRITE, entity));
                for (const auto& anim : anims)
                {
                    int k = 0;
                    j++;
                    auto anim_node = m_browser->AppendItem(entity_node, anim, spr_img, spr_img, new TreeNodeData(TreeNodeData::NODE_SPRITE, (j << 8) | entity));
                    const auto& frames = m_g->GetSpriteData()->GetSpriteAnimationFrames(anim);
                    for (const auto& frame : frames)
                    {
                        k++;
                        m_browser->AppendItem(anim_node, frame, spr_img, spr_img, new TreeNodeData(TreeNodeData::NODE_SPRITE, (k << 16) | (j << 8) | entity));
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
            (static_cast<int>(RoomViewerFrame::Mode::NORMAL) << 16) | room->index));
    }
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
}

void MainFrame::InitConfig()
{
    AssemblyBuilderDialog::InitConfig(APPLICATION_NAME);
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
            }
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
    catch (const std::exception& e)
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
        m_activeEditor->Show();
        this->m_mainwin->GetSizer()->Clear();
        this->m_mainwin->GetSizer()->Add(m_activeEditor, 1, wxALL | wxEXPAND);
        this->m_mainwin->GetSizer()->Layout();
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
    for (const auto& editor : m_editors)
    {
        editor.second->Hide();
        editor.second->ClearGameData();
    }
    m_browser->DeleteAllItems();
    m_browser->SetImageList(m_imgs);
    m_properties->GetGrid()->Clear();
    m_g.reset();
    SetMode(MODE_NONE);
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

void MainFrame::OnSave(wxCommandEvent& event)
{
    if (!m_last.empty())
    {
        Save();
    }
}

void MainFrame::OnBuildAsm(wxCommandEvent& event)
{
    if (m_last_asm.empty())
    {
        return;
    }
    AssemblyBuilderDialog bdlg(this, m_last_asm, m_g, AssemblyBuilderDialog::Func::BUILD);
    bdlg.ShowModal();
}

void MainFrame::OnRunEmulator(wxCommandEvent& event)
{
    if (m_built_rom.empty())
    {
        return;
    }
    AssemblyBuilderDialog bdlg(this, m_built_rom, m_g, AssemblyBuilderDialog::Func::RUN);
    bdlg.ShowModal();
}

void MainFrame::OnPreferences(wxCommandEvent& event)
{
    PreferencesDialog dlg(this);
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
    case MODE_STRING:
        // Display Strings
        GetStringEditor()->SetMode(static_cast<StringData::Type>(m_seldata));
        ShowEditor(EditorType::STRING);
        break;
    case MODE_TILESET:
        // Display tileset
        GetTilesetEditor()->Open(m_selname);
        ShowEditor(EditorType::TILESET);
        break;
    case MODE_ANIMATED_TILESET:
        // Display animated tileset
        GetTilesetEditor()->OpenAnimated(m_selname);
        ShowEditor(EditorType::TILESET);
        break;
    case MODE_BLOCKSET:
        // Display Blockset
        GetBlocksetEditor()->Open(m_selname);
        ShowEditor(EditorType::BLOCKSET);
        break;
    case MODE_PALETTE:
        // Display palettes
        GetPaletteEditor()->SetMode(static_cast<PaletteListFrame::Mode>(m_seldata));
        ShowEditor(EditorType::PALETTE);
        break;
    case MODE_ROOMMAP:
        // Display room map
        GetRoomEditor()->SetRoomNum(m_seldata & 0xFFFF);
        ShowEditor(EditorType::MAP_VIEW);
        break;
    case MODE_SPRITE:
        // Display sprite
        GetSpriteEditor()->Open(m_seldata & 0xFF, ((m_seldata >> 16) & 0xFF) - 1, ((m_seldata >> 8) & 0xFF) - 1);
        ShowEditor(EditorType::SPRITE);
        break;
    case MODE_IMAGE:
        // Display image
        GetMap2DEditor()->Open(m_selname);
        ShowEditor(EditorType::MAP_2D);
        break;
    case MODE_NONE:
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

void MainFrame::ProcessSelectedBrowserItem(const wxTreeItemId& item)
{
    m_selname = m_browser->GetItemText(item);
    TreeNodeData* item_data = static_cast<TreeNodeData*>(m_browser->GetItemData(item));
    m_seldata = item_data->GetValue();
    switch (item_data->GetNodeType())
    {
    case TreeNodeData::NODE_STRING:
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

BlocksetEditorFrame* MainFrame::GetBlocksetEditor()
{
    return static_cast<BlocksetEditorFrame*>(m_editors.at(EditorType::BLOCKSET));
}

SpriteEditorFrame* MainFrame::GetSpriteEditor()
{
    return static_cast<SpriteEditorFrame*>(m_editors.at(EditorType::SPRITE));
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
