#include <user_interface/sprites/include/SpriteEditorFrame.h>
#include <user_interface/main/include/MainFrame.h>

#include <wx/propgrid/advprops.h>
#include <fstream>
#include <landstalker/misc/include/Utils.h>

enum MENU_IDS
{
	ID_FILE_EXPORT_FRM = 20000,
	ID_FILE_EXPORT_TILES,
	ID_FILE_EXPORT_VDPMAP,
	ID_FILE_EXPORT_PNG,
	ID_FILE_IMPORT_FRM,
	ID_FILE_IMPORT_TILES,
	ID_FILE_IMPORT_VDPMAP,
	ID_VIEW,
	ID_VIEW_TOGGLE_GRIDLINES,
	ID_VIEW_TOGGLE_ALPHA,
	ID_VIEW_TOGGLE_HITBOX,
	ID_VIEW_SEP1,
	ID_VIEW_TOOLBAR,
	ID_VIEW_FRAMES,
	ID_VIEW_SUBSPRITES,
	ID_VIEW_ANIMATIONS,
	ID_VIEW_ANIM_FRAMES,
	ID_VIEW_EDITOR,
	ID_VIEW_PALETTE,
	ID_VIEW_PREVIEW,
	ID_TOGGLE_GRIDLINES = 30000,
	ID_TOGGLE_ALPHA,
	ID_TOGGLE_HITBOX,
	ID_COMPRESS_FRAME,
	ID_SWAP_TILES,
	ID_CUT_TILE,
	ID_COPY_TILE,
	ID_PASTE_TILE,
	ID_CLEAR_TILE,
	ID_ZOOM,
	ID_PLAY_PAUSE,
	ID_PLAY_SPEED
};

wxBEGIN_EVENT_TABLE(SpriteEditorFrame, wxWindow)
EVT_CHAR(SpriteEditorFrame::OnKeyDown)
EVT_SLIDER(ID_ZOOM, SpriteEditorFrame::OnZoomChange)
EVT_SLIDER(ID_PLAY_SPEED, SpriteEditorFrame::OnSpeedChange)
EVT_COMMAND(wxID_ANY, EVT_SPRITE_FRAME_SELECT, SpriteEditorFrame::OnTileSelected)
EVT_COMMAND(wxID_ANY, EVT_PALETTE_COLOUR_SELECT, SpriteEditorFrame::OnPaletteColourSelect)
EVT_COMMAND(wxID_ANY, EVT_PALETTE_COLOUR_HOVER, SpriteEditorFrame::OnPaletteColourHover)
EVT_COMMAND(wxID_ANY, EVT_SPRITE_FRAME_HOVER, SpriteEditorFrame::OnTileHovered)
EVT_COMMAND(wxID_ANY, EVT_TILE_PIXEL_HOVER, SpriteEditorFrame::OnTilePixelHover)
EVT_COMMAND(wxID_ANY, EVT_TILE_CHANGE, SpriteEditorFrame::OnTileChanged)
EVT_COMMAND(wxID_ANY, EVT_SPRITE_FRAME_CHANGE, SpriteEditorFrame::OnTileChanged)
EVT_COMMAND(wxID_ANY, EVT_SPRITE_FRAME_EDIT_REQUEST, SpriteEditorFrame::OnTileEditRequested)
EVT_COMMAND(wxID_ANY, EVT_FRAME_SELECT, SpriteEditorFrame::OnFrameSelect)
EVT_COMMAND(wxID_ANY, EVT_FRAME_ADD, SpriteEditorFrame::OnFrameAdd)
EVT_COMMAND(wxID_ANY, EVT_FRAME_DELETE, SpriteEditorFrame::OnFrameDelete)
EVT_COMMAND(wxID_ANY, EVT_SUBSPRITE_SELECT, SpriteEditorFrame::OnSubSpriteSelect)
EVT_COMMAND(wxID_ANY, EVT_SUBSPRITE_ADD, SpriteEditorFrame::OnSubSpriteAdd)
EVT_COMMAND(wxID_ANY, EVT_SUBSPRITE_DELETE, SpriteEditorFrame::OnSubSpriteDelete)
EVT_COMMAND(wxID_ANY, EVT_SUBSPRITE_MOVE_UP, SpriteEditorFrame::OnSubSpriteMoveUp)
EVT_COMMAND(wxID_ANY, EVT_SUBSPRITE_MOVE_DOWN, SpriteEditorFrame::OnSubSpriteMoveDown)
EVT_COMMAND(wxID_ANY, EVT_SUBSPRITE_UPDATE, SpriteEditorFrame::OnSubSpriteUpdate)
EVT_COMMAND(wxID_ANY, EVT_ANIMATION_SELECT, SpriteEditorFrame::OnAnimationSelect)
EVT_COMMAND(wxID_ANY, EVT_ANIMATION_ADD, SpriteEditorFrame::OnAnimationAdd)
EVT_COMMAND(wxID_ANY, EVT_ANIMATION_DELETE, SpriteEditorFrame::OnAnimationDelete)
EVT_COMMAND(wxID_ANY, EVT_ANIMATION_MOVE_UP, SpriteEditorFrame::OnAnimationMoveUp)
EVT_COMMAND(wxID_ANY, EVT_ANIMATION_MOVE_DOWN, SpriteEditorFrame::OnAnimationMoveDown)
EVT_COMMAND(wxID_ANY, EVT_ANIMATION_FRAME_SELECT, SpriteEditorFrame::OnAnimationFrameSelect)
EVT_COMMAND(wxID_ANY, EVT_ANIMATION_FRAME_ADD, SpriteEditorFrame::OnAnimationFrameAdd)
EVT_COMMAND(wxID_ANY, EVT_ANIMATION_FRAME_DELETE, SpriteEditorFrame::OnAnimationFrameDelete)
EVT_COMMAND(wxID_ANY, EVT_ANIMATION_FRAME_MOVE_UP, SpriteEditorFrame::OnAnimationFrameMoveUp)
EVT_COMMAND(wxID_ANY, EVT_ANIMATION_FRAME_MOVE_DOWN, SpriteEditorFrame::OnAnimationFrameMoveDown)
EVT_COMMAND(wxID_ANY, EVT_ANIMATION_FRAME_CHANGE, SpriteEditorFrame::OnAnimationFrameChange)
wxEND_EVENT_TABLE()

SpriteEditorFrame::SpriteEditorFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst)
{
	m_mgr.SetManagedWindow(this);

	m_spriteeditor = new SpriteFrameEditorCtrl(this);
	m_preview = new EntityViewerCtrl(this);
	m_paledit = new PaletteEditor(this);
	m_tileedit = new TileEditor(this);
	m_framectrl = new FrameControlFrame(this, imglst);
	m_subspritectrl = new SubspriteControlFrame(this, imglst);
	m_animctrl = new AnimationControlFrame(this, imglst);
	m_animframectrl = new AnimationFrameControlFrame(this, imglst);

	m_preview->SetPixelSize(2);

	// add the panes to the manager
	m_mgr.SetDockSizeConstraint(0.3, 0.3);
	m_mgr.AddPane(m_tileedit, wxAuiPaneInfo().Left().Layer(2).MinSize(100, 100).BestSize(450, 450).FloatingSize(450, 450).Caption("Editor"));
	m_mgr.AddPane(m_paledit, wxAuiPaneInfo().Bottom().Layer(1).MinSize(180, 40).BestSize(700, 100).FloatingSize(700, 100).Caption("Palette"));
	m_mgr.AddPane(m_framectrl, wxAuiPaneInfo().Left().Layer(2).Resizable(false).MinSize(220, 150)
		.BestSize(220, 200).FloatingSize(220, 200).Caption("Frames"));
	m_mgr.AddPane(m_subspritectrl, wxAuiPaneInfo().Left().Layer(2).Resizable(false).MinSize(220, 150)
		.BestSize(220, 200).FloatingSize(220, 200).Caption("Subsprites"));
	m_mgr.AddPane(m_preview, wxAuiPaneInfo().Right().Layer(2).Movable(true).Resizable(true).MinSize(220, 150)
		.BestSize(220, 200).FloatingSize(220, 200).Caption("Preview"));
	m_mgr.AddPane(m_animctrl, wxAuiPaneInfo().Right().Layer(2).Movable(true).Resizable(true).MinSize(220, 150)
		.BestSize(220, 200).FloatingSize(220, 200).Caption("Animations"));
	m_mgr.AddPane(m_animframectrl, wxAuiPaneInfo().Right().Layer(2).Movable(true).Resizable(true).MinSize(220, 150)
		.BestSize(220, 200).FloatingSize(220, 200).Caption("Animation Frames"));

	m_mgr.AddPane(m_spriteeditor, wxAuiPaneInfo().CenterPane());
	// tell the manager to "commit" all the changes just made
	m_mgr.Update();
	UpdateUI();

	this->Connect(wxEVT_CHAR, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_spriteeditor->Connect(wxEVT_CHAR, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_preview->Connect(wxEVT_CHAR, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_paledit->Connect(wxEVT_CHAR, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_tileedit->Connect(wxEVT_CHAR, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_framectrl->Connect(wxEVT_CHAR, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_subspritectrl->Connect(wxEVT_CHAR, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_animctrl->Connect(wxEVT_CHAR, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_animframectrl->Connect(wxEVT_CHAR, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	this->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_spriteeditor->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_preview->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_paledit->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_tileedit->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_framectrl->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_subspritectrl->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_animctrl->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_animframectrl->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
}

SpriteEditorFrame::~SpriteEditorFrame()
{

	this->Disconnect(wxEVT_CHAR, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_spriteeditor->Disconnect(wxEVT_CHAR, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_preview->Disconnect(wxEVT_CHAR, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_paledit->Disconnect(wxEVT_CHAR, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_tileedit->Disconnect(wxEVT_CHAR, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_framectrl->Disconnect(wxEVT_CHAR, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_subspritectrl->Disconnect(wxEVT_CHAR, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_animctrl->Disconnect(wxEVT_CHAR, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_animframectrl->Disconnect(wxEVT_CHAR, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	this->Disconnect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_spriteeditor->Disconnect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_preview->Disconnect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_paledit->Disconnect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_tileedit->Disconnect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_framectrl->Disconnect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_subspritectrl->Disconnect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_animctrl->Disconnect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_animframectrl->Disconnect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
}

bool SpriteEditorFrame::Open(uint8_t spr, int frame, int anim, int ent)
{
	OpenFrame(spr, frame, anim, ent);
	m_preview->Open(m_gd->GetSpriteData()->GetEntitiesFromSprite(m_sprite->GetSprite())[0], m_anim, m_palette);
	m_framectrl->SetSelected(m_frame + 1);
	m_animctrl->SetSelected(m_anim + 1);
	m_animframectrl->SetSelected(1);
	m_subspritectrl->SetSelected(0);
	m_spriteeditor->SelectSubSprite(-1);
	return true;
}

bool SpriteEditorFrame::OpenFrame(uint8_t spr, int frame, int anim, int ent)
{
	if (m_gd == nullptr)
	{
		return false;
	}
	uint8_t entity = ent == -1 ? m_gd->GetSpriteData()->GetEntitiesFromSprite(spr)[0] : ent;
	m_frame = frame;
	m_anim = anim;
	if (frame == -1 && anim == -1)
	{
		m_sprite = m_gd->GetSpriteData()->GetDefaultEntityFrame(entity);
		m_frame = m_gd->GetSpriteData()->GetDefaultAbsFrameId(entity);
		m_anim = m_gd->GetSpriteData()->GetDefaultEntityAnimationId(entity);
	}
	else if (anim == -1)
	{
		m_sprite = m_gd->GetSpriteData()->GetSpriteFrame(spr, frame);
		m_anim = m_gd->GetSpriteData()->GetDefaultEntityAnimationId(entity);
	}
	else if (frame == -1)
	{
		m_sprite = m_gd->GetSpriteData()->GetSpriteFrame(spr, anim, 0);
	}
	else
	{
		m_sprite = m_gd->GetSpriteData()->GetSpriteFrame(spr, frame);
		m_anim = anim;
	}
	m_palette = m_gd->GetSpriteData()->GetEntityPalette(entity);
	m_spriteeditor->Open(m_sprite->GetData(), m_palette, m_sprite->GetSprite());
	m_paledit->SelectPalette(m_palette);
	m_tileedit->SetActivePalette(m_palette);
	m_tileedit->SetTile(Tile(0));
	m_tileedit->SetTileset(m_spriteeditor->GetTileset());
	m_tileedit->SetTile(m_spriteeditor->GetFirstTile());
	m_spriteeditor->SelectTile(m_spriteeditor->GetFirstTile());
	m_reset_props = true;
	Update();
	return true;
}

void SpriteEditorFrame::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
	m_tileedit->SetGameData(gd);
	m_paledit->SetGameData(gd);
	m_animctrl->SetGameData(gd);
	m_spriteeditor->SetGameData(gd);
	m_preview->SetGameData(gd);
	m_framectrl->SetGameData(gd);
	m_animframectrl->SetGameData(gd);
}

void SpriteEditorFrame::ClearGameData()
{
	m_gd = nullptr;
	m_tileedit->SetGameData(nullptr);
	m_paledit->SetGameData(nullptr);
	m_animctrl->ClearGameData();
	m_preview->ClearGameData();
	m_framectrl->ClearGameData();
	m_animframectrl->ClearGameData();
	m_anim = 0;
	m_frame = 0;
	m_sprite = nullptr;
}

void SpriteEditorFrame::SetActivePalette(const std::string& name)
{
	if (m_gd != nullptr)
	{
		auto pal = m_gd->GetPalette(name);
		if (pal != nullptr)
		{
			m_palette = std::make_shared<Palette>(*pal->GetData());
			m_spriteeditor->SetActivePalette(m_palette);
			m_preview->SetActivePalette(m_palette);
			m_paledit->SelectPalette(m_palette);
			m_tileedit->SetActivePalette(m_palette);
		}
	}
}

void SpriteEditorFrame::SetActivePalette(const std::vector<std::string>& names)
{
	if (m_gd != nullptr)
	{
		std::vector<std::shared_ptr<Palette>> pals;
		std::transform(names.cbegin(), names.cend(), std::back_inserter<std::vector<std::shared_ptr<Palette>>>(pals), [this](const auto& name)
			{
				return m_gd->GetPalette(name)->GetData();
			});
		m_palette = std::make_shared<Palette>(pals);
		m_spriteeditor->SetActivePalette(m_palette);
		m_preview->SetActivePalette(m_palette);
		m_paledit->SelectPalette(m_palette);
		m_tileedit->SetActivePalette(m_palette);
	}
}

void SpriteEditorFrame::Redraw() const
{
	m_spriteeditor->UpdateSubSprites();
	m_paledit->Refresh(true);
	m_tileedit->Redraw();
}

void SpriteEditorFrame::RedrawTiles(int index) const
{
	m_spriteeditor->RedrawTiles(index);
	m_tileedit->Redraw();
	m_preview->Refresh(true);
}

void SpriteEditorFrame::Update()
{
	m_subspritectrl->SetSubsprites(m_sprite->GetData()->GetSubSprites());
	m_framectrl->SetSprite(m_sprite->GetSprite());
	m_animctrl->SetSprite(m_sprite->GetSprite());
	m_animframectrl->SetAnimation(m_sprite->GetSprite(), m_anim);
	Redraw();
	UpdateUI();
	FireEvent(EVT_PROPERTIES_UPDATE);
	FireEvent(EVT_STATUSBAR_UPDATE);
}

bool SpriteEditorFrame::Save()
{
	return m_spriteeditor->Save(m_filename, m_spriteeditor->GetCompressed());
}

bool SpriteEditorFrame::SaveAs(wxString filename, bool compressed)
{
	return m_spriteeditor->Save(filename, compressed);
}

const std::string& SpriteEditorFrame::GetFilename() const
{
	return m_filename;
}

void SpriteEditorFrame::InitMenu(wxMenuBar& menu, ImageList& ilist) const
{
	ClearMenu(menu);
	auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
	AddMenuItem(fileMenu, 0, ID_FILE_EXPORT_FRM, "Export Sprite Frame as Binary...");
	AddMenuItem(fileMenu, 1, ID_FILE_EXPORT_TILES, "Export Sprite Tileset as Binary...");
	AddMenuItem(fileMenu, 2, ID_FILE_EXPORT_VDPMAP, "Export VDP Sprite Map as CSV...");
	AddMenuItem(fileMenu, 3, ID_FILE_EXPORT_PNG, "Export Sprite as PNG...");
	AddMenuItem(fileMenu, 4, ID_FILE_IMPORT_FRM, "Import Sprite Frame from Binary...");
	AddMenuItem(fileMenu, 5, ID_FILE_IMPORT_TILES, "Import Sprite Tileset from Binary...");
	AddMenuItem(fileMenu, 6, ID_FILE_IMPORT_VDPMAP, "Import VDP Sprite Map from CSV...");
	auto& viewMenu = AddMenu(menu, 1, ID_VIEW, "View");
	AddMenuItem(viewMenu, 0, ID_VIEW_TOGGLE_GRIDLINES, "Gridlines", wxITEM_CHECK);
	AddMenuItem(viewMenu, 1, ID_VIEW_TOGGLE_ALPHA, "Show Alpha as Black", wxITEM_CHECK);
	AddMenuItem(viewMenu, 2, ID_VIEW_TOGGLE_HITBOX, "Hitbox", wxITEM_CHECK);
	AddMenuItem(viewMenu, 3, ID_VIEW_SEP1, "", wxITEM_SEPARATOR);
	AddMenuItem(viewMenu, 4, ID_VIEW_TOOLBAR, "Toolbar", wxITEM_CHECK);
	AddMenuItem(viewMenu, 5, ID_VIEW_EDITOR, "Tile Editor", wxITEM_CHECK);
	AddMenuItem(viewMenu, 6, ID_VIEW_PALETTE, "Palette", wxITEM_CHECK);
	AddMenuItem(viewMenu, 7, ID_VIEW_PREVIEW, "Preview", wxITEM_CHECK);
	AddMenuItem(viewMenu, 8, ID_VIEW_FRAMES, "Frames", wxITEM_CHECK);
	AddMenuItem(viewMenu, 9, ID_VIEW_SUBSPRITES, "Subsprites", wxITEM_CHECK);
	AddMenuItem(viewMenu, 10, ID_VIEW_ANIMATIONS, "Animations", wxITEM_CHECK);
	AddMenuItem(viewMenu, 11, ID_VIEW_ANIM_FRAMES, "Animation Frames", wxITEM_CHECK);

	auto* parent = m_mgr.GetManagedWindow();
	wxAuiToolBar* toolbar = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORIZONTAL);
	m_zoomslider = new wxSlider(toolbar, ID_ZOOM, m_zoom, 1, 8, wxDefaultPosition, wxSize(80, wxDefaultCoord), wxSL_HORIZONTAL | wxSL_INVERSE);
	m_speedslider = new wxSlider(toolbar, ID_PLAY_SPEED, m_speed, 1, 10, wxDefaultPosition, wxSize(80, wxDefaultCoord));
	toolbar->AddTool(ID_TOGGLE_GRIDLINES, "Toggle Gridlines", ilist.GetImage("gridlines"), "Toggle Gridlines", wxITEM_CHECK);
	toolbar->AddTool(ID_TOGGLE_ALPHA, "Toggle Alpha", ilist.GetImage("alpha"), "Toggle Alpha", wxITEM_CHECK);
	toolbar->AddTool(ID_TOGGLE_HITBOX, "Toggle Hitbox", ilist.GetImage("ehitbox"), "Toggle Hitbox", wxITEM_CHECK);
	toolbar->AddSeparator();
	toolbar->AddTool(ID_COMPRESS_FRAME, "Compress Frame", ilist.GetImage("compress"), "Compress Frame", wxITEM_CHECK);
	toolbar->AddSeparator();
	toolbar->AddTool(ID_CUT_TILE, "Cut", ilist.GetImage("cut"), "Cut");
	toolbar->AddTool(ID_COPY_TILE, "Copy", ilist.GetImage("copy"), "Copy");
	toolbar->AddTool(ID_PASTE_TILE, "Paste", ilist.GetImage("paste"), "Paste");
	toolbar->AddTool(ID_SWAP_TILES, "Swap", ilist.GetImage("swap"), "Swap");
	toolbar->AddTool(ID_CLEAR_TILE, "Clear", ilist.GetImage("delete"), "Clear");
	toolbar->AddSeparator();
	toolbar->AddLabel(wxID_ANY, "Zoom:");
	toolbar->AddControl(m_zoomslider, "Zoom");
	toolbar->AddSeparator();
	toolbar->AddTool(ID_PLAY_PAUSE, "Play/Pause", ilist.GetImage("play"), "Play/Pause", wxITEM_CHECK);
	toolbar->AddSeparator();
	toolbar->AddLabel(wxID_ANY, "Speed:");
	toolbar->AddControl(m_speedslider, "Play Speed");

	AddToolbar(m_mgr, *toolbar, "Sprite", "Sprite Tools", wxAuiPaneInfo().ToolbarPane().Top().Row(1).Position(1));


	UpdateUI();

	m_mgr.Update();
}

void SpriteEditorFrame::ClearMenu(wxMenuBar& menu) const
{
	EditorFrame::ClearMenu(menu);
	m_zoomslider = nullptr;
	m_speedslider = nullptr;
}

void SpriteEditorFrame::OnMenuClick(wxMenuEvent& evt)
{
	ProcessEvent(evt.GetId());
	evt.Skip();
}

void SpriteEditorFrame::ProcessEvent(int id)
{
	switch (id)
	{
	case ID_FILE_EXPORT_FRM:
		OnExportFrm();
		break;
	case ID_FILE_EXPORT_TILES:
		OnExportTiles();
		break;
	case ID_FILE_EXPORT_VDPMAP:
		OnExportVdpSpritemap();
		break;
	case ID_FILE_EXPORT_PNG:
		OnExportPng();
		break;
	case ID_FILE_IMPORT_FRM:
		OnImportFrm();
		break;
	case ID_FILE_IMPORT_TILES:
		OnImportTiles();
		break;
	case ID_FILE_IMPORT_VDPMAP:
		OnImportVdpSpritemap();
		break;
	case ID_VIEW_TOGGLE_GRIDLINES:
	case ID_TOGGLE_GRIDLINES:
		m_spriteeditor->SetBordersEnabled(!m_spriteeditor->GetBordersEnabled());
		break;
	case ID_VIEW_TOGGLE_ALPHA:
	case ID_TOGGLE_ALPHA:
		m_spriteeditor->SetAlphaEnabled(!m_spriteeditor->GetAlphaEnabled());
		break;
	case ID_VIEW_TOGGLE_HITBOX:
	case ID_TOGGLE_HITBOX:
		m_spriteeditor->SetHitboxEnabled(!m_spriteeditor->GetHitboxEnabled());
		break;
	case ID_COMPRESS_FRAME:
		m_sprite->GetData()->SetCompressed(!m_sprite->GetData()->GetCompressed());
		break;
	case ID_CUT_TILE:
		if (m_spriteeditor->IsSelectionValid())
		{
			m_spriteeditor->CutCell();
		}
		break;
	case ID_COPY_TILE:
		if (m_spriteeditor->IsSelectionValid())
		{
			m_spriteeditor->CopyCell();
		}
		break;
	case ID_PASTE_TILE:
		if (m_spriteeditor->IsSelectionValid())
		{
			m_spriteeditor->PasteCell();
		}
		break;
	case ID_SWAP_TILES:
		if (m_spriteeditor->IsSelectionValid())
		{
			m_spriteeditor->SwapCell();
		}
		break;
	case ID_CLEAR_TILE:
		if (m_spriteeditor->IsSelectionValid())
		{
			m_spriteeditor->ClearCell();
		}
		break;
	case ID_PLAY_PAUSE:
		if (m_preview->IsPlaying())
		{
			m_preview->Pause();
		}
		else
		{
			m_preview->Play();
		}
		break;
	case ID_VIEW_TOOLBAR:
		SetToolbarVisibility("Sprite", !IsToolbarVisible("Sprite"));
		break;
	case ID_VIEW_EDITOR:
		SetPaneVisibility(m_tileedit, !IsPaneVisible(m_tileedit));
		break;
	case ID_VIEW_PALETTE:
		SetPaneVisibility(m_paledit, !IsPaneVisible(m_paledit));
		break;
	case ID_VIEW_PREVIEW:
		SetPaneVisibility(m_preview, !IsPaneVisible(m_preview));
		break;
	case ID_VIEW_FRAMES:
		SetPaneVisibility(m_framectrl, !IsPaneVisible(m_framectrl));
		break;
	case ID_VIEW_SUBSPRITES:
		SetPaneVisibility(m_subspritectrl, !IsPaneVisible(m_subspritectrl));
		break;
	case ID_VIEW_ANIMATIONS:
		SetPaneVisibility(m_animctrl, !IsPaneVisible(m_animctrl));
		break;
	case ID_VIEW_ANIM_FRAMES:
		SetPaneVisibility(m_animframectrl, !IsPaneVisible(m_animframectrl));
		break;
	default:
		break;
	}
	UpdateUI();
	FireEvent(EVT_STATUSBAR_UPDATE);
	FireEvent(EVT_PROPERTIES_UPDATE);
}

std::string SpriteEditorFrame::ShowFrameDialog(const std::string& prompt, const std::string& title)
{
	wxArrayString choices;
	std::vector<std::string> frames = m_gd->GetSpriteData()->GetSpriteFrames(m_sprite->GetSprite());
	for (const auto& f : frames)
	{
		choices.Add(f);
	}
	wxSingleChoiceDialog dlg(this, prompt, title, choices);
	int result = dlg.ShowModal();

	return result == wxID_OK ? dlg.GetStringSelection().ToStdString() : std::string();
}

void SpriteEditorFrame::ExportFrm(const std::string& filename) const
{
	auto bytes = m_sprite->GetData()->GetBits();
	WriteBytes(bytes, filename);
}

void SpriteEditorFrame::ExportTiles(const std::string& filename) const
{
	auto bytes = m_sprite->GetData()->GetTileset()->GetBits(false);
	WriteBytes(bytes, filename);
}

void SpriteEditorFrame::ExportVdpSpritemap(const std::string& filename) const
{
	std::ofstream fs(filename, std::ios::out | std::ios::trunc);
	fs << (m_sprite->GetData()->GetCompressed() ? "1" : "0") << std::endl;
	for (std::size_t i = 0; i < m_sprite->GetData()->GetSubSpriteCount(); ++i)
	{
		const auto& data = m_sprite->GetData()->GetSubSprite(i);
		fs << StrPrintf("% 03d,% 03d,%01d,%01d\n", data.x, data.y, data.w, data.h);
	}
}

void SpriteEditorFrame::ExportPng(const std::string& filename) const
{
	int width = m_sprite->GetData()->GetWidth();
	int height = m_sprite->GetData()->GetHeight();
	ImageBuffer buf(width, height);
	buf.InsertSprite(-m_sprite->GetData()->GetLeft(), -m_sprite->GetData()->GetTop(), 0, *m_sprite->GetData());
	buf.WritePNG(filename, { m_palette }, true);
}

void SpriteEditorFrame::ImportFrm(const std::string& filename)
{
	auto bytes = ReadBytes(filename);
	m_sprite->GetData()->SetBits(bytes);
	RedrawTiles();
	m_reset_props = true;
	Update();
}

void SpriteEditorFrame::ImportTiles(const std::string& filename)
{
	auto bytes = ReadBytes(filename);
	m_sprite->GetData()->GetTileset()->SetBits(bytes, false);
	RedrawTiles();
	m_reset_props = true;
	Update();
}

void SpriteEditorFrame::ImportVdpSpritemap(const std::string& filename)
{
	std::ifstream fs(filename, std::ios::in);
	bool compressed = false;
	std::vector<SpriteFrame::SubSprite> subs;
	std::string row;
	std::string cell;
	std::getline(fs, row);
	if (row.size() > 0)
	{
		compressed = row[0] == '1';
	}
	while (std::getline(fs, row))
	{
		uint32_t x, y, w, h;
		std::istringstream ss(row);
		std::getline(ss, cell, ',');
		StrToInt(cell, x);
		std::getline(ss, cell, ',');
		StrToInt(cell, y);
		std::getline(ss, cell, ',');
		StrToInt(cell, w);
		std::getline(ss, cell, ',');
		StrToInt(cell, h);
		subs.emplace_back(x, y, w, h);
	}
	m_sprite->GetData()->SetCompressed(compressed);
	m_sprite->GetData()->SetSubSprites(subs);

	m_reset_props = true;
	Update();
}

void SpriteEditorFrame::InitProperties(wxPropertyGridManager& props) const
{
	if (m_gd && m_sprite && ArePropsInitialised() == false)
	{
		RefreshLists();
		props.GetGrid()->Clear();
		auto sd = m_gd->GetSpriteData();
		int sprite_index = m_sprite->GetSprite();

		props.Append(new wxPropertyCategory("Main", "Main"));
		props.Append(new wxStringProperty("Name", "Name", _(m_sprite->GetName())));
		props.Append(new wxIntProperty("ID", "ID", sprite_index))->Enable(false);
		props.Append(new wxStringProperty("Start Address", "Start Address", _(StrPrintf("0x%06X", m_sprite->GetStartAddress()))))->Enable(false);
		props.Append(new wxStringProperty("End Address", "End Address", _(StrPrintf("0x%06X", m_sprite->GetEndAddress()))))->Enable(false);
		props.Append(new wxIntProperty("Size (bytes)", "Size", m_sprite->GetDataLength()))->Enable(false);
		props.Append(new wxEnumProperty("Low Palette", "Low Palette", m_lo_palettes));
		props.Append(new wxEnumProperty("High Palette", "High Palette", m_hi_palettes));
		props.Append(new wxEnumProperty("Projectile/Misc Palette 1", "Projectile/Misc Palette 1", m_misc_palettes));
		props.Append(new wxEnumProperty("Projectile/Misc Palette 2", "Projectile/Misc Palette 2", m_misc_palettes));
		props.Append(new wxPropertyCategory("Frame", "Frame"));
		auto prop_cmp = new wxBoolProperty("Compressed", "Compressed", m_sprite->GetData()->GetCompressed());
		prop_cmp->SetAttribute(wxPG_BOOL_USE_CHECKBOX, true);
		props.Append(prop_cmp);
		props.Append(new wxPropertyCategory("Animation", "Animation"));
		auto flags = sd->GetSpriteAnimationFlags(sprite_index);
		props.Append(new wxEnumProperty("Idle Animation Frame Count", "Idle Animation Frame Count", m_idle_frame_count_options));
		props.Append(new wxEnumProperty("Idle Animation Source", "Idle Animation Source", m_idle_frame_source_options));
		props.Append(new wxEnumProperty("Jump Animation Source", "Jump Animation Source", m_jump_frame_source_options));
		props.Append(new wxEnumProperty("Walk Cycle Frame Count", "Walk Cycle Frame Count", m_walk_frame_count_options));
		props.Append(new wxEnumProperty("Take Damage Animation Source", "Take Damage Animation Source", m_take_damage_frame_source_options));
		auto prop_dnr = new wxBoolProperty("Do Not Rotate", "Do Not Rotate", flags.do_not_rotate);
		prop_dnr->SetAttribute(wxPG_BOOL_USE_CHECKBOX, true);
		props.Append(prop_dnr);
		auto prop_fa = new wxBoolProperty("Full Animations", "Full Animations", flags.has_full_animations);
		prop_fa->SetAttribute(wxPG_BOOL_USE_CHECKBOX, true);
		props.Append(prop_fa);
		props.Append(new wxPropertyCategory("Hitbox", "Hitbox"));
		wxPGProperty* base_prop = new wxFloatProperty("Width/Length", "Width/Length", sd->GetSpriteHitbox(sprite_index).first / 8.0);
		base_prop->SetAttribute(wxPG_ATTR_MIN, 0.0);
		base_prop->SetAttribute(wxPG_ATTR_MAX, 31.875);
		base_prop->SetAttribute(wxPG_ATTR_SPINCTRL_STEP, 0.125);
		base_prop->SetEditor(wxPGEditor_SpinCtrl);
		props.Append(base_prop);
		wxPGProperty* height_prop = new wxFloatProperty("Height", "Height", sd->GetSpriteHitbox(sprite_index).second / 16.0);
		height_prop->SetAttribute(wxPG_ATTR_MIN, 0.0);
		height_prop->SetAttribute(wxPG_ATTR_MAX, 15.9375);
		height_prop->SetAttribute(wxPG_ATTR_SPINCTRL_STEP, 0.0625);
		height_prop->SetEditor(wxPGEditor_SpinCtrl);
		props.Append(height_prop);
		wxPGProperty* vol_prop = new wxFloatProperty("Volume", "Volume", sd->GetSpriteVolume(sprite_index) / 16.0);
		vol_prop->SetAttribute(wxPG_ATTR_MIN, 0.0);
		vol_prop->SetAttribute(wxPG_ATTR_MAX, 4095.9375);
		vol_prop->SetAttribute(wxPG_ATTR_SPINCTRL_STEP, 0.0625);
		vol_prop->SetEditor(wxPGEditor_SpinCtrl);
		props.Append(vol_prop);
		EditorFrame::InitProperties(props);
		RefreshProperties(props);
	}
}

void SpriteEditorFrame::RefreshLists() const
{
	if (m_gd && m_sprite)
	{
		auto entities = m_gd->GetSpriteData()->GetEntitiesFromSprite(m_sprite->GetSprite());
		std::set<int> hi_pals, lo_pals;
		for (const auto& e : entities)
		{
			auto epals = m_gd->GetSpriteData()->GetEntityPaletteIdxs(e);
			lo_pals.insert(epals.first);
			hi_pals.insert(epals.second);
		}
		m_lo_palettes.Clear();
		m_lo_palettes.Add("<None>");
		for (int i = 0; i < m_gd->GetSpriteData()->GetLoPaletteCount(); ++i)
		{
			m_lo_palettes.Add(_(m_gd->GetSpriteData()->GetLoPalette(i)->GetName()));
		}
		for (int i = 0; i < static_cast<int>(m_lo_palettes.GetCount()); ++i)
		{
			if (lo_pals.count(i - 1) > 0)
			{
				wxFont font = m_lo_palettes.Item(i).GetFont();
				font.SetWeight(wxFontWeight::wxFONTWEIGHT_BOLD);
				m_lo_palettes.Item(i).SetFont(font);
			}
		}
		m_hi_palettes.Clear();
		m_hi_palettes.Add("<None>");
		for (int i = 0; i < m_gd->GetSpriteData()->GetHiPaletteCount(); ++i)
		{
			m_hi_palettes.Add(_(m_gd->GetSpriteData()->GetHiPalette(i)->GetName()));
		}
		for (int i = 0; i < static_cast<int>(m_hi_palettes.GetCount()); ++i)
		{
			if (hi_pals.count(i - 1) > 0)
			{
				wxFont font = m_hi_palettes.Item(i).GetFont();
				font.SetWeight(wxFontWeight::wxFONTWEIGHT_BOLD);
				m_hi_palettes.Item(i).SetFont(font);
			}
		}
		m_misc_palettes.Clear();
		m_misc_palettes.Add("<None>");
		for (const auto& p: m_gd->GetAllPalettes())
		{
			if (m_lo_palettes.Index(p.first) == -1 && m_hi_palettes.Index(p.first) == -1)
			{
				m_misc_palettes.Add(_(p.first));
			}
		}
		m_idle_frame_count_options.Clear();
		m_idle_frame_count_options.Add("1");
		m_idle_frame_count_options.Add("2");
		m_idle_frame_source_options.Clear();
		m_idle_frame_source_options.Add("Use Walk Frames");
		m_idle_frame_source_options.Add("Dedicated");
		m_jump_frame_source_options.Clear();
		m_jump_frame_source_options.Add("Use Idle Frames");
		m_jump_frame_source_options.Add("Dedicated");
		m_walk_frame_count_options.Clear();
		m_walk_frame_count_options.Add("2");
		m_walk_frame_count_options.Add("4");
		m_take_damage_frame_source_options.Clear();
		m_take_damage_frame_source_options.Add("Use Idle Frames");
		m_take_damage_frame_source_options.Add("Dedicated");
	}
}

void SpriteEditorFrame::UpdateProperties(wxPropertyGridManager& props) const
{
	EditorFrame::UpdateProperties(props);
	if (ArePropsInitialised() == true)
	{
		if (m_reset_props)
		{
			props.GetGrid()->ClearModifiedStatus();
			m_reset_props = false;
		}
		RefreshProperties(props);
	}
}

void SpriteEditorFrame::RefreshProperties(wxPropertyGridManager& props) const
{
	if (m_gd != nullptr && m_sprite != nullptr)
	{
		RefreshLists();
		props.GetGrid()->Freeze();

		auto sd = m_gd->GetSpriteData();
		int sprite_index = m_sprite->GetSprite();
		int entity_index = sd->GetEntitiesFromSprite(sprite_index)[0];

		props.GetGrid()->SetPropertyValue("Name", _(sd->GetSpriteName(sprite_index)));
		props.GetGrid()->SetPropertyValue("ID", static_cast<int>(sprite_index));
		props.GetGrid()->SetPropertyValue("Start Address", _(StrPrintf("0x%06X", m_sprite->GetStartAddress())));
		props.GetGrid()->SetPropertyValue("End Address", _(StrPrintf("0x%06X", m_sprite->GetEndAddress())));
		props.GetGrid()->SetPropertyValue("Size", static_cast<int>(m_sprite->GetDataLength()));
		props.GetGrid()->GetProperty("Low Palette")->SetChoices(m_lo_palettes);
		props.GetGrid()->GetProperty("High Palette")->SetChoices(m_hi_palettes);
		props.GetGrid()->GetProperty("Low Palette")->SetChoiceSelection(sd->GetEntityPaletteIdxs(entity_index).first + 1);
		props.GetGrid()->GetProperty("High Palette")->SetChoiceSelection(sd->GetEntityPaletteIdxs(entity_index).second + 1);
		props.GetGrid()->GetProperty("Projectile/Misc Palette 1")->SetChoiceSelection(0);
		props.GetGrid()->GetProperty("Projectile/Misc Palette 2")->SetChoiceSelection(0);
		props.GetGrid()->SetPropertyValue("Volume", static_cast<double>(sd->GetSpriteVolume(sprite_index)) / 16.0);
		props.GetGrid()->SetPropertyValue("Compressed", m_sprite->GetData()->GetCompressed());
		auto flags = sd->GetSpriteAnimationFlags(sprite_index);
		props.GetGrid()->GetProperty("Idle Animation Frame Count")->SetChoices(m_idle_frame_count_options);
		props.GetGrid()->GetProperty("Idle Animation Source")->SetChoices(m_idle_frame_source_options);
		props.GetGrid()->GetProperty("Jump Animation Source")->SetChoices(m_jump_frame_source_options);
		props.GetGrid()->GetProperty("Walk Cycle Frame Count")->SetChoices(m_walk_frame_count_options);
		props.GetGrid()->GetProperty("Take Damage Animation Source")->SetChoices(m_take_damage_frame_source_options);
		props.GetGrid()->GetProperty("Idle Animation Frame Count")->SetChoiceSelection(1 - static_cast<int>(flags.idle_animation_frames));
		props.GetGrid()->GetProperty("Walk Cycle Frame Count")->SetChoiceSelection(1 - static_cast<int>(flags.walk_animation_frame_count));
		props.GetGrid()->GetProperty("Idle Animation Source")->SetChoiceSelection(static_cast<int>(flags.idle_animation_source));
		props.GetGrid()->GetProperty("Jump Animation Source")->SetChoiceSelection(static_cast<int>(flags.jump_animation_source));
		props.GetGrid()->GetProperty("Take Damage Animation Source")->SetChoiceSelection(static_cast<int>(flags.take_damage_animation_source));
		props.GetGrid()->SetPropertyValue("Do Not Rotate", flags.do_not_rotate);
		props.GetGrid()->SetPropertyValue("Full Animations", flags.has_full_animations);
		props.GetGrid()->SetPropertyValue("Width/Length", sd->GetSpriteHitbox(sprite_index).first / 8.0);
		props.GetGrid()->SetPropertyValue("Height", sd->GetSpriteHitbox(sprite_index).second / 16.0);
		props.GetGrid()->Thaw();
	}
}

void SpriteEditorFrame::OnPropertyChange(wxPropertyGridEvent& evt)
{
	auto* ctrl = static_cast<wxPropertyGridManager*>(evt.GetEventObject());
	wxPGProperty* property = evt.GetProperty();
	if (property == nullptr || m_gd == nullptr || m_sprite == nullptr)
	{
		return;
	}
	ctrl->GetGrid()->Freeze();
	auto sd = m_gd->GetSpriteData();
	int sprite_index = m_sprite->GetSprite();
	const wxString& name = property->GetName();
	if (name == "Width/Length")
	{
		auto hitbox = sd->GetSpriteHitbox(sprite_index);
		int value = static_cast<int>(property->GetValuePlain().GetDouble() * 8.0);
		if (value != hitbox.first)
		{
			hitbox.first = std::clamp<uint8_t>(value, 0, 255);
			sd->SetSpriteHitbox(sprite_index, hitbox.first, hitbox.second);
			FireEvent(EVT_PROPERTIES_UPDATE);
			if (m_spriteeditor->GetHitboxEnabled())
			{
				m_spriteeditor->RedrawTiles();
			}
		}
	}
	else if (name == "Height")
	{
		auto hitbox = sd->GetSpriteHitbox(sprite_index);
		int value = static_cast<int>(property->GetValuePlain().GetDouble() * 16.0);
		if (value != hitbox.second)
		{
			hitbox.second = std::clamp<uint8_t>(value, 0, 255);
			sd->SetSpriteHitbox(sprite_index, hitbox.first, hitbox.second);
			FireEvent(EVT_PROPERTIES_UPDATE);
			if (m_spriteeditor->GetHitboxEnabled())
			{
				m_spriteeditor->RedrawTiles();
			}
		}
	}
	else if (name == "Additional Value")
	{
		uint16_t value = static_cast<uint16_t>(property->GetValuePlain().GetDouble() * 16.0);
		sd->SetSpriteVolume(sprite_index, value);
	}
	else if (name == "Low Palette" || name == "High Palette" || name == "Projectile/Misc Palette 1" || name == "Projectile/Misc Palette 2")
	{
		std::vector<std::string> palettes;
		int lo_pal = ctrl->GetGrid()->GetPropertyByName("Low Palette")->GetValue().GetLong();
		int hi_pal = ctrl->GetGrid()->GetPropertyByName("High Palette")->GetValue().GetLong();
		int misc_pal1 = ctrl->GetGrid()->GetPropertyByName("Projectile/Misc Palette 1")->GetValue().GetLong();
		int misc_pal2 = ctrl->GetGrid()->GetPropertyByName("Projectile/Misc Palette 2")->GetValue().GetLong();
		if (lo_pal != 0)
		{
			palettes.push_back(m_lo_palettes.Item(lo_pal).GetText().ToStdString());
		}
		if (hi_pal != 0)
		{
			palettes.push_back(m_hi_palettes.Item(hi_pal).GetText().ToStdString());
		}
		if (misc_pal1 != 0)
		{
			palettes.push_back(m_misc_palettes.Item(misc_pal1).GetText().ToStdString());
		}
		if (misc_pal2 != 0)
		{
			palettes.push_back(m_misc_palettes.Item(misc_pal2).GetText().ToStdString());
		}
		SetActivePalette(palettes);
	}
	else if (name == "Compressed")
	{
		bool value = property->GetValuePlain().GetBool();
		if (value != m_sprite->GetData()->GetCompressed())
		{
			m_sprite->GetData()->SetCompressed(value);
			UpdateUI();
		}
	}
	else if (name == "Idle Animation Frame Count" || name == "Idle Animation Source" || name == "Jump Animation Source" || name == "Walk Cycle Frame Count" ||
		     name == "Take Damage Animation Source" || name == "Do Not Rotate" || name == "Full Animations")
	{
		SpriteData::AnimationFlags flags;
		flags.idle_animation_frames = ctrl->GetGrid()->GetPropertyByName("Idle Animation Frame Count")->GetValue().GetLong() == 0 ?
			SpriteData::AnimationFlags::IdleAnimationFrameCount::ONE_FRAME : SpriteData::AnimationFlags::IdleAnimationFrameCount::TWO_FRAMES;
		flags.idle_animation_source = ctrl->GetGrid()->GetPropertyByName("Idle Animation Source")->GetValue().GetLong() == 0 ?
			SpriteData::AnimationFlags::IdleAnimationSource::USE_WALK_FRAMES : SpriteData::AnimationFlags::IdleAnimationSource::DEDICATED;
		flags.jump_animation_source = ctrl->GetGrid()->GetPropertyByName("Jump Animation Source")->GetValue().GetLong() == 0 ?
			SpriteData::AnimationFlags::JumpAnimationSource::USE_IDLE_FRAMES : SpriteData::AnimationFlags::JumpAnimationSource::DEDICATED;
		flags.walk_animation_frame_count = ctrl->GetGrid()->GetPropertyByName("Walk Cycle Frame Count")->GetValue().GetLong() == 0 ?
			SpriteData::AnimationFlags::WalkAnimationFrameCount::TWO_FRAMES : SpriteData::AnimationFlags::WalkAnimationFrameCount::FOUR_FRAMES;
		flags.take_damage_animation_source = ctrl->GetGrid()->GetPropertyByName("Take Damage Animation Source")->GetValue().GetLong() == 0 ?
			SpriteData::AnimationFlags::TakeDamageAnimationSource::USE_IDLE_FRAMES : SpriteData::AnimationFlags::TakeDamageAnimationSource::DEDICATED;
		flags.do_not_rotate = ctrl->GetGrid()->GetPropertyByName("Do Not Rotate")->GetValue().GetBool();
		flags.has_full_animations = ctrl->GetGrid()->GetPropertyByName("Full Animations")->GetValue().GetBool();
		sd->SetSpriteAnimationFlags(m_sprite->GetSprite(), flags);
	}
	ctrl->GetGrid()->Thaw();
}

void SpriteEditorFrame::UpdateUI() const
{
	CheckMenuItem(ID_VIEW_TOOLBAR, IsToolbarVisible("Sprite"));
	CheckMenuItem(ID_VIEW_EDITOR, IsPaneVisible(m_tileedit));
	CheckMenuItem(ID_VIEW_PALETTE, IsPaneVisible(m_paledit));
	CheckMenuItem(ID_VIEW_PREVIEW, IsPaneVisible(m_preview));
	CheckMenuItem(ID_VIEW_FRAMES, IsPaneVisible(m_framectrl));
	CheckMenuItem(ID_VIEW_SUBSPRITES, IsPaneVisible(m_subspritectrl));
	CheckMenuItem(ID_VIEW_ANIMATIONS, IsPaneVisible(m_animctrl));
	CheckMenuItem(ID_VIEW_ANIM_FRAMES, IsPaneVisible(m_animframectrl));
	if (m_spriteeditor != nullptr && m_sprite != nullptr)
	{
		EnableMenuItem(ID_VIEW_TOGGLE_ALPHA, true);
		EnableToolbarItem("Sprite", ID_TOGGLE_ALPHA, true);
		CheckMenuItem(ID_VIEW_TOGGLE_ALPHA, !m_spriteeditor->GetAlphaEnabled());
		CheckToolbarItem("Sprite", ID_TOGGLE_ALPHA, !m_spriteeditor->GetAlphaEnabled());
		EnableMenuItem(ID_VIEW_TOGGLE_GRIDLINES, true);
		EnableToolbarItem("Sprite", ID_TOGGLE_GRIDLINES, true);
		CheckMenuItem(ID_VIEW_TOGGLE_GRIDLINES, m_spriteeditor->GetBordersEnabled());
		CheckToolbarItem("Sprite", ID_TOGGLE_GRIDLINES, m_spriteeditor->GetBordersEnabled());
		EnableMenuItem(ID_VIEW_TOGGLE_HITBOX, true);
		EnableToolbarItem("Sprite", ID_TOGGLE_HITBOX, true);
		CheckMenuItem(ID_VIEW_TOGGLE_HITBOX, m_spriteeditor->GetHitboxEnabled());
		CheckToolbarItem("Sprite", ID_TOGGLE_HITBOX, m_spriteeditor->GetHitboxEnabled());
		CheckToolbarItem("Sprite", ID_COMPRESS_FRAME, m_sprite->GetData()->GetCompressed());
		EnableToolbarItem("Sprite", ID_CUT_TILE, m_spriteeditor->IsSelectionValid());
		EnableToolbarItem("Sprite", ID_COPY_TILE, m_spriteeditor->IsSelectionValid());
		EnableToolbarItem("Sprite", ID_PASTE_TILE, m_spriteeditor->IsSelectionValid() && !m_spriteeditor->IsClipboardEmpty());
		EnableToolbarItem("Sprite", ID_SWAP_TILES, m_spriteeditor->IsSelectionValid());
		EnableToolbarItem("Sprite", ID_CLEAR_TILE, m_spriteeditor->IsSelectionValid());
		EnableToolbarItem("Sprite", ID_PLAY_PAUSE, true);
		CheckToolbarItem("Sprite", ID_PLAY_PAUSE, m_preview->IsPlaying());
		if (m_zoomslider != nullptr)
		{
			EnableToolbarItem("Sprite", ID_ZOOM, true);
			m_zoomslider->SetValue(m_spriteeditor->GetPixelSize());
		}
		if (m_speedslider != nullptr)
		{
			EnableToolbarItem("Sprite", ID_PLAY_SPEED, true);
			m_speedslider->SetValue(m_preview->GetAnimSpeed());
		}
	}
	else
	{
		EnableMenuItem(ID_VIEW_TOGGLE_ALPHA, false);
		EnableMenuItem(ID_VIEW_TOGGLE_GRIDLINES, false);
		EnableMenuItem(ID_VIEW_TOGGLE_HITBOX, false);
		EnableToolbarItem("Sprite", ID_TOGGLE_ALPHA, false);
		EnableToolbarItem("Sprite", ID_TOGGLE_GRIDLINES, false);
		EnableToolbarItem("Sprite", ID_TOGGLE_HITBOX, false);
		EnableToolbarItem("Sprite", ID_COMPRESS_FRAME, false);
		EnableToolbarItem("Sprite", ID_CUT_TILE, false);
		EnableToolbarItem("Sprite", ID_COPY_TILE, false);
		EnableToolbarItem("Sprite", ID_PASTE_TILE, false);
		EnableToolbarItem("Sprite", ID_SWAP_TILES, false);
		EnableToolbarItem("Sprite", ID_CLEAR_TILE, false);
		EnableToolbarItem("Sprite", ID_PLAY_PAUSE, false);
		if (m_zoomslider != nullptr)
		{
			EnableToolbarItem("Sprite", ID_ZOOM, false);
		}
		if (m_speedslider != nullptr)
		{
			EnableToolbarItem("Sprite", ID_PLAY_SPEED, false);
		}
	}
}

void SpriteEditorFrame::OnKeyDown(wxKeyEvent& evt)
{
	if (!m_spriteeditor->HandleKeyDown(evt.GetKeyCode(), evt.GetModifiers()))
	{
		evt.Skip();
	}
}

void SpriteEditorFrame::OnFrameSelect(wxCommandEvent& evt)
{
	m_sprite = m_gd->GetSpriteData()->GetSpriteFrame(evt.GetString().ToStdString());
	OpenFrame(m_sprite->GetSprite(), evt.GetInt() - 1);
}

void SpriteEditorFrame::OnFrameAdd(wxCommandEvent& /*evt*/)
{
	std::string name = "";
	auto dlg = wxTextEntryDialog(this, "Enter a unique name for the new frame", "New frame");
	do
	{
		dlg.ShowModal();
		name = dlg.GetValue().ToStdString();
	} while (m_gd->GetSpriteData()->SpriteFrameExists(name));
	m_gd->GetSpriteData()->AddSpriteFrame(m_sprite->GetSprite(), name);
	m_sprite = m_gd->GetSpriteData()->GetSpriteFrame(name);
	m_framectrl->SetSprite(m_sprite->GetSprite());
	m_animframectrl->SetAnimation(m_sprite->GetSprite(), m_anim);
	int sel = m_gd->GetSpriteData()->GetSpriteFrameId(m_sprite->GetSprite(), name);
	m_framectrl->SetSelected(sel + 1);
	OpenFrame(m_sprite->GetSprite(), sel);
}

void SpriteEditorFrame::OnFrameDelete(wxCommandEvent& evt)
{
	if (evt.GetString().IsEmpty())
	{
		return;
	}
	int response = wxMessageBox("Are you sure you want to delete the frame \"" + evt.GetString() + "\"?", "Delete frame", wxYES_NO | wxICON_EXCLAMATION);
	if (response == wxYES)
	{
		m_gd->GetSpriteData()->DeleteSpriteFrame(evt.GetString().ToStdString());
		std::string next_frame = m_gd->GetSpriteData()->GetSpriteFrames(m_sprite->GetSprite()).at(0);
		if(evt.GetInt() > 0 && evt.GetInt() <= static_cast<int>(m_gd->GetSpriteData()->GetSpriteFrameCount(m_sprite->GetSprite())))
		{
			next_frame = m_gd->GetSpriteData()->GetSpriteFrames(m_sprite->GetSprite()).at(evt.GetInt() - 1);
		}
		m_sprite = m_gd->GetSpriteData()->GetSpriteFrame(next_frame);
		m_framectrl->SetSprite(m_sprite->GetSprite());
		m_animframectrl->SetAnimation(m_sprite->GetSprite(), m_anim);
		m_framectrl->SetSelected(m_gd->GetSpriteData()->GetSpriteFrameId(m_sprite->GetSprite(), next_frame));
		OpenFrame(m_sprite->GetSprite(), m_framectrl->GetSelected() - 1);
	}
}

void SpriteEditorFrame::OnSubSpriteSelect(wxCommandEvent& evt)
{
	m_spriteeditor->SelectSubSprite(evt.GetInt());
	m_subspritectrl->SetSelected(evt.GetInt());
}

void SpriteEditorFrame::OnSubSpriteAdd(wxCommandEvent& evt)
{
	if (m_sprite->GetData()->GetSubSpriteCount() < SpriteFrame::MAX_SUBSPRITES)
	{
		int pos = evt.GetInt();
		if (pos < 1)
		{
			pos = 1;
		}
		m_sprite->GetData()->AddSubSpriteBefore(pos - 1);
		m_spriteeditor->UpdateSubSprites();
		m_subspritectrl->SetSubsprites(m_sprite->GetData()->GetSubSprites());
		m_spriteeditor->SelectSubSprite(pos);
		m_subspritectrl->SetSelected(pos);
	}
}

void SpriteEditorFrame::OnSubSpriteDelete(wxCommandEvent& evt)
{
	if (m_sprite->GetData()->GetSubSpriteCount() > 0)
	{
		int pos = evt.GetInt();
		m_sprite->GetData()->DeleteSubSprite(pos - 1);
		m_spriteeditor->UpdateSubSprites();
		m_subspritectrl->SetSubsprites(m_sprite->GetData()->GetSubSprites());
	}
}

void SpriteEditorFrame::OnSubSpriteMoveUp(wxCommandEvent& evt)
{
	if (m_sprite->GetData()->GetSubSpriteCount() > 0)
	{
		std::size_t pos = evt.GetInt();
		if (pos > 1 && pos <= m_sprite->GetData()->GetSubSpriteCount())
		{
			m_sprite->GetData()->SwapSubSprite(pos - 1, pos - 2);
			m_spriteeditor->UpdateSubSprites();
			m_subspritectrl->SetSubsprites(m_sprite->GetData()->GetSubSprites());
			m_spriteeditor->SelectSubSprite(pos - 1);
			m_subspritectrl->SetSelected(pos - 1);
		}
	}
}

void SpriteEditorFrame::OnSubSpriteMoveDown(wxCommandEvent& evt)
{
	if (m_sprite->GetData()->GetSubSpriteCount() > 0)
	{
		std::size_t pos = evt.GetInt();
		if (pos > 0 && pos < m_sprite->GetData()->GetSubSpriteCount())
		{
			m_sprite->GetData()->SwapSubSprite(pos - 1, pos);
			m_spriteeditor->UpdateSubSprites();
			m_subspritectrl->SetSubsprites(m_sprite->GetData()->GetSubSprites());
			m_spriteeditor->SelectSubSprite(pos + 1);
			m_subspritectrl->SetSelected(pos + 1);
		}
	}
}

void SpriteEditorFrame::OnSubSpriteUpdate(wxCommandEvent& /*evt*/)
{
	m_spriteeditor->UpdateSubSprites();
	m_subspritectrl->SetSubsprites(m_sprite->GetData()->GetSubSprites());
}

void SpriteEditorFrame::OnAnimationSelect(wxCommandEvent& evt)
{
	if (evt.GetInt() > 0 && evt.GetInt() <= static_cast<int>(m_gd->GetSpriteData()->GetSpriteAnimationCount(m_sprite->GetSprite())))
	{
		m_anim = evt.GetInt() - 1;
		m_preview->SetAnimation(m_anim);
		m_animframectrl->SetAnimation(m_sprite->GetSprite(), m_anim);
		std::string first_frame_name = m_gd->GetSpriteData()->GetSpriteAnimationFrames(m_sprite->GetSprite(), m_anim)[0];
		m_frame = m_gd->GetSpriteData()->GetSpriteFrameId(m_sprite->GetSprite(), first_frame_name);
		OpenFrame(m_sprite->GetSprite(), m_frame, m_anim);
		m_framectrl->SetSelected(m_frame + 1);
		m_animctrl->SetSelected(m_anim + 1);
	}
}

void SpriteEditorFrame::OnAnimationAdd(wxCommandEvent& /*evt*/)
{
	std::string name = "";
	auto dlg = wxTextEntryDialog(this, "Enter a unique name for the new animation", "New animation");
	do
	{
		dlg.ShowModal();
		name = dlg.GetValue().ToStdString();
	} while (m_gd->GetSpriteData()->SpriteAnimationExists(name));
	m_gd->GetSpriteData()->AddSpriteAnimation(m_sprite->GetSprite(), name);
	m_anim = m_gd->GetSpriteData()->GetSpriteAnimationCount(m_sprite->GetSprite()) - 1;
	m_preview->SetAnimation(m_anim);
	m_animframectrl->SetAnimation(m_sprite->GetSprite(), m_anim);
	std::string first_frame_name = m_gd->GetSpriteData()->GetSpriteAnimationFrames(m_sprite->GetSprite(), m_anim)[0];
	m_frame = m_gd->GetSpriteData()->GetSpriteFrameId(m_sprite->GetSprite(), first_frame_name);
	OpenFrame(m_sprite->GetSprite(), m_frame, m_anim);
	m_framectrl->SetSelected(m_frame + 1);
	m_animctrl->SetSelected(m_anim + 1);
}

void SpriteEditorFrame::OnAnimationDelete(wxCommandEvent& evt)
{
	if (m_gd->GetSpriteData()->SpriteAnimationExists(evt.GetString().ToStdString()))
	{
		m_gd->GetSpriteData()->DeleteSpriteAnimation(evt.GetString().ToStdString());
		if (m_anim >= static_cast<int>(m_gd->GetSpriteData()->GetSpriteAnimationCount(m_sprite->GetSprite())))
		{
			m_anim = m_gd->GetSpriteData()->GetSpriteAnimationCount(m_sprite->GetSprite()) - 1;
		}
		m_preview->SetAnimation(m_anim);
		m_animframectrl->SetAnimation(m_sprite->GetSprite(), m_anim);
		std::string first_frame_name = m_gd->GetSpriteData()->GetSpriteAnimationFrames(m_sprite->GetSprite(), m_anim)[0];
		m_frame = m_gd->GetSpriteData()->GetSpriteFrameId(m_sprite->GetSprite(), first_frame_name);
		OpenFrame(m_sprite->GetSprite(), m_frame, m_anim);
		m_framectrl->SetSelected(m_frame + 1);
		m_animctrl->SetSelected(m_anim + 1);
	}
}

void SpriteEditorFrame::OnAnimationMoveUp(wxCommandEvent& evt)
{
	if (evt.GetInt() > 1 && evt.GetInt() <= static_cast<int>(m_gd->GetSpriteData()->GetSpriteAnimationCount(m_sprite->GetSprite())) &&
		m_gd->GetSpriteData()->SpriteAnimationExists(evt.GetString().ToStdString()))
	{
		m_anim = evt.GetInt() - 2;
		m_gd->GetSpriteData()->MoveSpriteAnimation(m_sprite->GetSprite(), evt.GetString().ToStdString(), evt.GetInt() - 2);
		m_preview->SetAnimation(m_anim);
		m_animframectrl->SetAnimation(m_sprite->GetSprite(), m_anim);
		std::string first_frame_name = m_gd->GetSpriteData()->GetSpriteAnimationFrames(m_sprite->GetSprite(), m_anim)[0];
		m_frame = m_gd->GetSpriteData()->GetSpriteFrameId(m_sprite->GetSprite(), first_frame_name);
		OpenFrame(m_sprite->GetSprite(), m_frame, m_anim);
		m_framectrl->SetSelected(m_frame + 1);
		m_animctrl->SetSelected(m_anim + 1);
	}
}

void SpriteEditorFrame::OnAnimationMoveDown(wxCommandEvent& evt)
{
	if (evt.GetInt() > 0 && evt.GetInt() < static_cast<int>(m_gd->GetSpriteData()->GetSpriteAnimationCount(m_sprite->GetSprite())) &&
		m_gd->GetSpriteData()->SpriteAnimationExists(evt.GetString().ToStdString()))
	{
		m_anim = evt.GetInt();
		m_gd->GetSpriteData()->MoveSpriteAnimation(m_sprite->GetSprite(), evt.GetString().ToStdString(), evt.GetInt());
		m_preview->SetAnimation(m_anim);
		m_animframectrl->SetAnimation(m_sprite->GetSprite(), m_anim);
		std::string first_frame_name = m_gd->GetSpriteData()->GetSpriteAnimationFrames(m_sprite->GetSprite(), m_anim)[0];
		m_frame = m_gd->GetSpriteData()->GetSpriteFrameId(m_sprite->GetSprite(), first_frame_name);
		OpenFrame(m_sprite->GetSprite(), m_frame, m_anim);
		m_framectrl->SetSelected(m_frame + 1);
		m_animctrl->SetSelected(m_anim + 1);
	}
}

void SpriteEditorFrame::OnAnimationFrameSelect(wxCommandEvent& evt)
{
	if (evt.GetInt() > 0 && evt.GetInt() <= static_cast<int>(m_gd->GetSpriteData()->GetSpriteAnimationFrames(m_sprite->GetSprite(), m_anim).size()))
	{
		m_preview->Pause();
		m_preview->SetAnimationFrame(evt.GetInt() - 1);
		m_frame = m_gd->GetSpriteData()->GetSpriteFrameId(m_sprite->GetSprite(), evt.GetString().ToStdString());
		m_sprite = m_gd->GetSpriteData()->GetSpriteFrame(m_sprite->GetSprite(), m_frame);
		OpenFrame(m_sprite->GetSprite(), m_frame, m_anim);
		m_framectrl->SetSelected(m_frame + 1);
		m_animctrl->SetSelected(m_anim + 1);
		UpdateUI();
	}
}

void SpriteEditorFrame::OnAnimationFrameAdd(wxCommandEvent& evt)
{
	if (evt.GetInt() > 0 && evt.GetInt() <= static_cast<int>(m_gd->GetSpriteData()->GetSpriteAnimationFrames(m_sprite->GetSprite(), m_anim).size()))
	{
		auto new_frame = ShowFrameDialog("Select Frame", "New Animation Frame");
		if (!new_frame.empty())
		{
			std::string anim = m_gd->GetSpriteData()->GetSpriteAnimations(m_sprite->GetSprite())[m_anim];
			m_gd->GetSpriteData()->InsertSpriteAnimationFrame(anim, evt.GetInt(), new_frame);
			m_animframectrl->SetAnimation(m_sprite->GetSprite(), m_anim);
			m_preview->SetAnimationFrame(evt.GetInt());
			m_frame = m_gd->GetSpriteData()->GetSpriteFrameId(m_sprite->GetSprite(), new_frame);
			m_sprite = m_gd->GetSpriteData()->GetSpriteFrame(m_sprite->GetSprite(), m_frame);
			OpenFrame(m_sprite->GetSprite(), m_frame, m_anim);
			m_framectrl->SetSelected(m_frame + 1);
			m_animctrl->SetSelected(m_anim + 1);
			m_animframectrl->SetSelected(evt.GetInt() + 1);
			UpdateUI();
		}
	}
}

void SpriteEditorFrame::OnAnimationFrameDelete(wxCommandEvent& evt)
{
	if (evt.GetInt() > 0 && evt.GetInt() <= static_cast<int>(m_gd->GetSpriteData()->GetSpriteAnimationFrames(m_sprite->GetSprite(), m_anim).size()))
	{
		std::string anim = m_gd->GetSpriteData()->GetSpriteAnimations(m_sprite->GetSprite())[m_anim];
		int new_frame = evt.GetInt() - 1;
		if (new_frame > static_cast<int>(m_gd->GetSpriteData()->GetSpriteAnimationFrameCount(anim)))
		{
			new_frame = static_cast<int>(m_gd->GetSpriteData()->GetSpriteAnimationFrameCount(anim)) - 1;
		}
		m_gd->GetSpriteData()->DeleteSpriteAnimationFrame(anim, new_frame);
		m_animframectrl->SetAnimation(m_sprite->GetSprite(), m_anim);
		m_preview->SetAnimationFrame(new_frame);
		m_frame = m_gd->GetSpriteData()->GetSpriteFrameId(m_sprite->GetSprite(), m_gd->GetSpriteData()->GetSpriteAnimationFrames(anim)[new_frame]);
		m_sprite = m_gd->GetSpriteData()->GetSpriteFrame(m_sprite->GetSprite(), m_frame);
		OpenFrame(m_sprite->GetSprite(), m_frame, m_anim);
		m_framectrl->SetSelected(m_frame + 1);
		m_animctrl->SetSelected(m_anim + 1);
		m_animframectrl->SetSelected(evt.GetInt());
		UpdateUI();
	}
}

void SpriteEditorFrame::OnAnimationFrameMoveUp(wxCommandEvent& evt)
{
	std::string anim = m_gd->GetSpriteData()->GetSpriteAnimations(m_sprite->GetSprite())[m_anim];
	if (evt.GetInt() > 1 && evt.GetInt() <= static_cast<int>(m_gd->GetSpriteData()->GetSpriteAnimationFrameCount(anim)))
	{
		int new_frame = evt.GetInt() - 2;
		int old_frame = evt.GetInt() - 1;
		m_gd->GetSpriteData()->MoveSpriteAnimationFrame(anim, old_frame, new_frame);
		m_preview->SetAnimation(m_anim);
		m_animframectrl->SetAnimation(m_sprite->GetSprite(), m_anim);
		m_preview->SetAnimationFrame(new_frame);
		m_animframectrl->SetSelected(new_frame + 1);
	}
}

void SpriteEditorFrame::OnAnimationFrameMoveDown(wxCommandEvent& evt)
{
	std::string anim = m_gd->GetSpriteData()->GetSpriteAnimations(m_sprite->GetSprite())[m_anim];
	if (evt.GetInt() > 0 && evt.GetInt() < static_cast<int>(m_gd->GetSpriteData()->GetSpriteAnimationFrameCount(anim)))
	{
		int new_frame = evt.GetInt();
		int old_frame = evt.GetInt() - 1;
		m_gd->GetSpriteData()->MoveSpriteAnimationFrame(anim, old_frame, new_frame);
		m_preview->SetAnimation(m_anim);
		m_animframectrl->SetAnimation(m_sprite->GetSprite(), m_anim);
		m_preview->SetAnimationFrame(new_frame);
		m_animframectrl->SetSelected(new_frame + 1);
	}
}

void SpriteEditorFrame::OnAnimationFrameChange(wxCommandEvent& evt)
{
	if (evt.GetInt() > 0 && evt.GetInt() <= static_cast<int>(m_gd->GetSpriteData()->GetSpriteAnimationFrames(m_sprite->GetSprite(), m_anim).size()))
	{
		std::string anim = m_gd->GetSpriteData()->GetSpriteAnimations(m_sprite->GetSprite())[m_anim];
		std::string new_frame = ShowFrameDialog(StrPrintf("Change frame \"%s\" to:", evt.GetString().c_str().AsChar()), "Change frame");
		m_gd->GetSpriteData()->ChangeSpriteAnimationFrame(anim, evt.GetInt() - 1, new_frame);
		m_animframectrl->SetAnimation(m_sprite->GetSprite(), m_anim);
		m_preview->SetAnimationFrame(evt.GetInt() - 1);
		m_frame = m_gd->GetSpriteData()->GetSpriteFrameId(m_sprite->GetSprite(), new_frame);
		m_sprite = m_gd->GetSpriteData()->GetSpriteFrame(m_sprite->GetSprite(), m_frame);
		OpenFrame(m_sprite->GetSprite(), m_frame, m_anim);
		m_framectrl->SetSelected(m_frame + 1);
		m_animctrl->SetSelected(m_anim + 1);
		m_animframectrl->SetSelected(evt.GetInt());
		UpdateUI();
	}
}

void SpriteEditorFrame::OnZoomChange(wxCommandEvent& evt)
{
	m_zoom = m_zoomslider->GetValue();
	m_spriteeditor->SetPixelSize(m_zoomslider->GetValue());
	FireEvent(EVT_STATUSBAR_UPDATE);
	evt.Skip();
}

void SpriteEditorFrame::OnSpeedChange(wxCommandEvent& evt)
{
	m_speed = m_speedslider->GetValue();
	m_preview->SetAnimSpeed(m_speedslider->GetValue());
	evt.Skip();
}

void SpriteEditorFrame::OnTileHovered(wxCommandEvent& evt)
{
	FireEvent(EVT_STATUSBAR_UPDATE);
	evt.Skip();
}

void SpriteEditorFrame::OnTileSelected(wxCommandEvent& evt)
{
	int tile = std::stoi(evt.GetString().ToStdString());
	if (tile != -1)
	{
		m_tileedit->SetTile(tile);
	}
	FireEvent(EVT_STATUSBAR_UPDATE);
	evt.Skip();
}

void SpriteEditorFrame::OnTileChanged(wxCommandEvent& evt)
{
	int tile = std::stoi(evt.GetString().ToStdString());
	m_spriteeditor->RedrawTiles(tile);
	if (m_tileedit->GetTile() == tile)
	{
		m_tileedit->SetTile(tile);
	}
	evt.Skip();
}

void SpriteEditorFrame::OnTileEditRequested(wxCommandEvent& evt)
{
	evt.Skip();
}

void SpriteEditorFrame::OnButtonClicked(wxCommandEvent& evt)
{
	ProcessEvent(evt.GetId());
	evt.Skip();
}

void SpriteEditorFrame::OnPaletteColourSelect(wxCommandEvent& evt)
{
	m_tileedit->SetPrimaryColour(m_paledit->GetPrimaryColour());
	m_tileedit->SetSecondaryColour(m_paledit->GetSecondaryColour());
	FireEvent(EVT_STATUSBAR_UPDATE);
	evt.Skip();
}

void SpriteEditorFrame::OnPaletteColourHover(wxCommandEvent& evt)
{
	FireEvent(EVT_STATUSBAR_UPDATE);
	evt.Skip();
}

void SpriteEditorFrame::OnTilePixelHover(wxCommandEvent& evt)
{
	FireEvent(EVT_STATUSBAR_UPDATE);
	evt.Skip();
}

void SpriteEditorFrame::OnExportFrm()
{
	const wxString default_file = m_sprite->GetName() + ".frm";
	wxFileDialog fd(this, _("Export Sprite As Binary"), "", default_file, "Compressed Sprite Frame (*.frm)|*.frm|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		ExportFrm(fd.GetPath().ToStdString());
	}
}

void SpriteEditorFrame::OnExportTiles()
{
	const wxString default_file = m_sprite->GetName() + ".bin";
	wxFileDialog fd(this, _("Export Sprite Tiles As Binary"), "", default_file, "Sprite Tiles (*.bin)|*.bin|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		ExportTiles(fd.GetPath().ToStdString());
	}
}

void SpriteEditorFrame::OnExportVdpSpritemap()
{
	const wxString default_file = m_sprite->GetName() + ".csv";
	wxFileDialog fd(this, _("Export Sprite VDP Spritemap As CSV"), "", default_file, "Comma-Separated Values file (*.csv)|*.csv|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		ExportVdpSpritemap(fd.GetPath().ToStdString());
	}
}

void SpriteEditorFrame::OnExportPng()
{
	const wxString default_file = m_sprite->GetName() + ".png";
	wxFileDialog fd(this, _("Export Sprite As PNG"), "", default_file, "PNG Image (*.png)|*.png|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		ExportPng(fd.GetPath().ToStdString());
	}
}

void SpriteEditorFrame::OnImportFrm()
{
	wxFileDialog fd(this, _("Import Sprite From FRM"), "", "", "FRM File (*.frm)|*.frm|All Files (*.*)|*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		std::string path = fd.GetPath().ToStdString();
		ImportFrm(path);
	}
	m_tileedit->SetTile(Tile(0));
	m_tileedit->SetTileset(m_spriteeditor->GetTileset());
	m_tileedit->SetTile(m_spriteeditor->GetFirstTile());
	m_spriteeditor->SelectTile(m_spriteeditor->GetFirstTile());
	Update();
}

void SpriteEditorFrame::OnImportTiles()
{
	wxFileDialog fd(this, _("Import Sprite Tiles From BIN"), "", "", "BIN File (*.bin)|*.bin|All Files (*.*)|*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		std::string path = fd.GetPath().ToStdString();
		ImportTiles(path);
	}
	m_tileedit->SetTile(Tile(0));
	m_tileedit->SetTileset(m_spriteeditor->GetTileset());
	m_tileedit->SetTile(m_spriteeditor->GetFirstTile());
	m_spriteeditor->SelectTile(m_spriteeditor->GetFirstTile());
	Update();
}

void SpriteEditorFrame::OnImportVdpSpritemap()
{
	wxFileDialog fd(this, _("Import Sprite VDP Spritemap From CSV"), "", "", "Comma-Separated Values file (*.csv)|*.csv|All Files (*.*)|*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		std::string path = fd.GetPath().ToStdString();
		ImportVdpSpritemap(path);
	}
	m_spriteeditor->SelectTile(0);
	m_tileedit->SetTile(Tile(0));
	m_tileedit->SetTileset(m_sprite->GetData()->GetTileset());
	Update();
}

void SpriteEditorFrame::InitStatusBar(wxStatusBar& status) const
{
	status.SetFieldsCount(3);
	status.SetStatusText("", 0);
	status.SetStatusText("", 1);
	status.SetStatusText("", 2);
	m_status_init = true;
}

void SpriteEditorFrame::UpdateStatusBar(wxStatusBar& status, wxCommandEvent& /*evt*/) const
{
	if (!m_status_init)
	{
		return;
	}
	std::ostringstream ss;
	int colour = m_paledit->GetHoveredColour();
	if (m_spriteeditor->IsSelectionValid())
	{
		ss << "Tile: " << m_spriteeditor->GetSelectedTile().GetIndex();
		if (m_tileedit->IsHoverValid())
		{
			const auto selection = m_tileedit->GetHoveredPixel();
			int idx = m_tileedit->GetColourAtPixel(selection);
			colour = m_tileedit->GetColour(idx);
			ss << ": (" << selection.x << ", " << selection.y << "): " << idx;
		}
	}
	status.SetStatusText(ss.str(), 0);
	ss.str(std::string());
	if (colour != -1)
	{
		const auto& name = m_palette->getOwner(colour);
		ss << StrPrintf("Colour at mouse: Index %d - Genesis 0x%04X, RGB #%06X %s", colour,
			m_palette->getGenesisColour(colour), m_palette->getRGB(colour), m_palette->getA(colour) == 0 ? " [Transparent]" : "");
		if (!name.empty())
		{
			ss << ", Palette: \"" << name << "\"";
		}
	}
	else if (m_spriteeditor->IsHoverValid())
	{
		auto pos = m_spriteeditor->GetHoveredTilePosition();
		ss << "Cursor at (" << pos.first << "," << pos.second << ")";
	}
	status.SetStatusText(ss.str(), 1);
}
