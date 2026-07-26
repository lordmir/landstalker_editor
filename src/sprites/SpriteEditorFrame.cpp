#include <sprites/SpriteEditorFrame.h>
#include <sprites/SpriteImportDialog.h>
#include <main/MainFrame.h>

#include <wx/propgrid/advprops.h>
#include <wx/textdlg.h>
#include <cctype>
#include <fstream>
#include <filesystem>
#include <landstalker/misc/Utils.h>
#include <landstalker/misc/Labels.h>

enum MENU_IDS
{
	ID_FILE_EXPORT_FRM = 20000,
	ID_FILE_EXPORT_TILES,
	ID_FILE_EXPORT_VDPMAP,
	ID_FILE_EXPORT_PNG,
	ID_FILE_EXPORT_SPRITE_PROPERTIES_YAML,
	ID_FILE_EXPORT_ALL_SPRITESHEETS,
	ID_FILE_IMPORT_FRM,
	ID_FILE_IMPORT_TILES,
	ID_FILE_IMPORT_VDPMAP,
	ID_FILE_IMPORT_SPRITE_METADATA,
	ID_FILE_IMPORT_SPRITESHEET_NEW,
	ID_FILE_IMPORT_SPRITESHEET_CURRENT,
	ID_EDIT,
	ID_EDIT_SPRITES,
	ID_EDIT_SEP,
	ID_EDIT_UNDO,
	ID_EDIT_REDO,
	ID_VIEW,
	ID_VIEW_TOGGLE_GRIDLINES,
	ID_VIEW_TOGGLE_ALPHA,
	ID_VIEW_TOGGLE_HITBOX,
	ID_VIEW_SEP1,
	ID_VIEW_SEP2,
	ID_VIEW_TOOLBAR,
	ID_VIEW_TOOLS_TOOLBAR,
	ID_VIEW_FRAMES,
	ID_VIEW_SUBSPRITES,
	ID_VIEW_ANIMATIONS,
	ID_VIEW_ANIM_FRAMES,
	ID_VIEW_PALETTE,
	ID_VIEW_PREVIEW,
	ID_TOGGLE_GRIDLINES = 30000,
	ID_TOGGLE_ALPHA,
	ID_TOGGLE_HITBOX,
	ID_COMPRESS_FRAME,
	ID_OPTIMISE_SUBSPRITES,
	ID_SWAP_TILES,
	ID_CUT_TILE,
	ID_COPY_TILE,
	ID_PASTE_TILE,
	ID_CLEAR_TILE,
	ID_HFLIP_SEL,
	ID_VFLIP_SEL,
	ID_ZOOM,
	ID_PLAY_PAUSE,
	ID_PLAY_SPEED,
	ID_SELECT,
	ID_SUBSPRITE_MODE,
	ID_PENCIL,
	ID_LINE,
	ID_RECT_FILLED,
	ID_RECT_OUTLINE,
	ID_CIRCLE_FILLED,
	ID_CIRCLE_OUTLINE,
	ID_FILL,
	ID_PICKER,
	ID_PIXEL_SELECT
};

wxBEGIN_EVENT_TABLE(SpriteEditorFrame, wxWindow)
EVT_CHAR(SpriteEditorFrame::OnKeyDown)
EVT_SLIDER(ID_ZOOM, SpriteEditorFrame::OnZoomChange)
EVT_SLIDER(ID_PLAY_SPEED, SpriteEditorFrame::OnSpeedChange)
EVT_COMMAND(wxID_ANY, EVT_SPRITE_FRAME_SELECT, SpriteEditorFrame::OnTileSelected)
EVT_COMMAND(wxID_ANY, EVT_PALETTE_COLOUR_SELECT, SpriteEditorFrame::OnPaletteColourSelect)
EVT_COMMAND(wxID_ANY, EVT_PALETTE_COLOUR_HOVER, SpriteEditorFrame::OnPaletteColourHover)
EVT_COMMAND(wxID_ANY, EVT_SPRITE_FRAME_HOVER, SpriteEditorFrame::OnTileHovered)
EVT_COMMAND(wxID_ANY, EVT_SPRITE_FRAME_CHANGE, SpriteEditorFrame::OnTileChanged)
EVT_COMMAND(wxID_ANY, EVT_SPRITE_FRAME_COLOUR_PICK, SpriteEditorFrame::OnColourPicked)
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
	m_framectrl = new FrameControlFrame(this, imglst);
	m_subspritectrl = new SubspriteControlFrame(this, imglst);
	m_animctrl = new AnimationControlFrame(this, imglst);
	m_animframectrl = new AnimationFrameControlFrame(this, imglst);

	m_preview->SetPixelSize(2);

	// add the panes to the manager
	m_mgr.SetDockSizeConstraint(0.3, 0.3);
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
	m_framectrl->Connect(wxEVT_CHAR, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_subspritectrl->Connect(wxEVT_CHAR, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_animctrl->Connect(wxEVT_CHAR, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_animframectrl->Connect(wxEVT_CHAR, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	this->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_spriteeditor->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_preview->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_paledit->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
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
	m_framectrl->Disconnect(wxEVT_CHAR, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_subspritectrl->Disconnect(wxEVT_CHAR, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_animctrl->Disconnect(wxEVT_CHAR, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_animframectrl->Disconnect(wxEVT_CHAR, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	this->Disconnect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_spriteeditor->Disconnect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_preview->Disconnect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_paledit->Disconnect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_framectrl->Disconnect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_subspritectrl->Disconnect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_animctrl->Disconnect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
	m_animframectrl->Disconnect(wxEVT_KEY_DOWN, wxKeyEventHandler(SpriteEditorFrame::OnKeyDown), nullptr, this);
}

bool SpriteEditorFrame::Open(uint8_t spr, int frame, int anim, int ent)
{
	if (!OpenFrame(spr, frame, anim, ent))
	{
		return false;
	}
	// A sprite with no entity previews itself by its graphics id; one with an entity previews
	// through that entity so it picks up the entity's default animation. Going through entity 0
	// for a lone sprite would wrongly show entity 0's sprite, not this one.
	const auto preview_entities = m_gd->GetSpriteData()->GetEntitiesFromSprite(m_sprite->GetSprite());
	if (preview_entities.empty())
	{
		m_preview->OpenSprite(m_sprite->GetSprite(), static_cast<uint8_t>(m_anim < 0 ? 0 : m_anim), m_palette);
	}
	else
	{
		m_preview->Open(preview_entities[0], m_anim, m_palette);
	}
	m_framectrl->SetSelected(m_frame + 1);
	m_animctrl->SetSelected(m_anim + 1);
	m_animframectrl->SetSelected(1);
	m_subspritectrl->SetSelected(0);
	m_spriteeditor->SelectSubSprite(-1);
	return true;
}

bool SpriteEditorFrame::OpenFrame(uint8_t spr, int frame, int anim, int ent, bool fullUpdate)
{
	if (m_gd == nullptr)
	{
		return false;
	}
	// A sprite added through the sprite manager has no entity pointing at it yet, so the
	// entity-driven defaults below are unavailable: fall back to the sprite's own first
	// animation and a default palette rather than dereferencing an empty entity list.
	const auto sprite_data = m_gd->GetSpriteData();
	const auto entities = sprite_data->GetEntitiesFromSprite(spr);
	const bool has_entity = ent != -1 || !entities.empty();
	const uint8_t entity = ent != -1 ? static_cast<uint8_t>(ent)
		: (entities.empty() ? 0 : entities[0]);
	m_frame = frame;
	m_anim = anim;
	if (frame == -1 && anim == -1)
	{
		if (has_entity)
		{
			m_sprite = sprite_data->GetDefaultEntityFrame(entity);
			m_frame = sprite_data->GetDefaultAbsFrameId(entity);
			m_anim = sprite_data->GetDefaultEntityAnimationId(entity);
		}
		else
		{
			m_anim = 0;
			m_frame = 0;
			m_sprite = sprite_data->GetSpriteFrame(spr, 0, 0);
		}
	}
	else if (anim == -1)
	{
		m_sprite = sprite_data->GetSpriteFrame(spr, frame);
		m_anim = has_entity ? sprite_data->GetDefaultEntityAnimationId(entity) : 0;
	}
	else if (frame == -1)
	{
		m_sprite = sprite_data->GetSpriteFrame(spr, anim, 0);
	}
	else
	{
		m_sprite = sprite_data->GetSpriteFrame(spr, frame);
		m_anim = anim;
	}
	if (m_sprite == nullptr)
	{
		return false;
	}
	// GetEntityPalette needs an entity; a lone sprite gets the first sprite palette instead.
	m_palette = has_entity ? sprite_data->GetEntityPalette(entity) : sprite_data->GetSpritePalette(0);
	// A sprite imported through the dialog carries the low/high palettes the user picked; keep the
	// preview on those for every frame of it rather than reverting to the default sprite palette.
	if (m_forced_palette_sprite == static_cast<int>(spr) && !m_forced_palette_names.empty())
	{
		std::vector<std::shared_ptr<Landstalker::Palette>> pals;
		for (const auto& pal_name : m_forced_palette_names)
		{
			if (const auto pe = m_gd->GetPalette(pal_name))
			{
				pals.push_back(pe->GetData());
			}
		}
		if (!pals.empty())
		{
			m_palette = std::make_shared<Landstalker::Palette>(pals);
		}
	}
	m_spriteeditor->Open(m_sprite->GetData(), m_palette, m_sprite->GetSprite());
	m_paledit->SelectPalette(m_palette);
	m_spriteeditor->SetPrimaryColour(m_paledit->GetPrimaryColour());
	m_spriteeditor->SetSecondaryColour(m_paledit->GetSecondaryColour());
	m_spriteeditor->SelectTile(m_spriteeditor->GetFirstTile());
	m_reset_props = true;
	// Keep the reservation valid for whatever frame is now current.
	EnsureMaxTileCount();

	if(fullUpdate)
	{
		Update();
	}
	else{
		UpdateUI();
		FireEvent(EVT_PROPERTIES_UPDATE);
		FireEvent(EVT_STATUSBAR_UPDATE);
	}

	return true;
}

void SpriteEditorFrame::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	m_gd = gd;
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
			m_palette = std::make_shared<Landstalker::Palette>(*pal->GetData());
			m_spriteeditor->SetActivePalette(m_palette);
			m_preview->SetActivePalette(m_palette);
			m_paledit->SelectPalette(m_palette);
		}
	}
}

void SpriteEditorFrame::SetActivePalette(const std::vector<std::string>& names)
{
	if (m_gd != nullptr)
	{
		std::vector<std::shared_ptr<Landstalker::Palette>> pals;
		std::transform(names.cbegin(), names.cend(), std::back_inserter<std::vector<std::shared_ptr<Landstalker::Palette>>>(pals),
			[this](const auto& name)
			{
				return m_gd->GetPalette(name)->GetData();
			});
		m_palette = std::make_shared<Landstalker::Palette>(pals);
		m_spriteeditor->SetActivePalette(m_palette);
		m_preview->SetActivePalette(m_palette);
		m_paledit->SelectPalette(m_palette);
	}
}

void SpriteEditorFrame::Redraw() const
{
	m_spriteeditor->UpdateSubSprites();
	m_paledit->Refresh(true);
}

void SpriteEditorFrame::RedrawTiles(int index) const
{
	m_spriteeditor->RedrawTiles(index);
	m_preview->Refresh(true);
}

void SpriteEditorFrame::Update()
{
	if (m_sprite != nullptr)
	{
		m_subspritectrl->SetSubsprites(m_sprite->GetData()->GetSubSprites());
		m_framectrl->SetSprite(m_sprite->GetSprite());
		m_animctrl->SetSprite(m_sprite->GetSprite());
		m_animframectrl->SetAnimation(m_sprite->GetSprite(), m_anim);
		Redraw();
	}
	UpdateUI();
	FireEvent(EVT_PROPERTIES_UPDATE);
	FireEvent(EVT_STATUSBAR_UPDATE);
}

void SpriteEditorFrame::EnsureMaxTileCount()
{
	if (!m_gd || !m_sprite)
	{
		return;
	}
	const uint8_t sid = m_sprite->GetSprite();
	const auto frame_tiles = static_cast<uint16_t>(m_sprite->GetData()->GetTileCount());
	if (frame_tiles > m_gd->GetSpriteData()->GetSpriteMaxTileCount(sid))
	{
		// The frame outgrew the reservation - raise it so the sprite still fits in VRAM.
		m_gd->GetSpriteData()->SetSpriteMaxTileCount(sid, frame_tiles);
		FireEvent(EVT_PROPERTIES_UPDATE);
	}
}

void SpriteEditorFrame::OnOptimiseSubsprites()
{
	if (!m_gd || !m_sprite)
	{
		return;
	}
	const auto subs = m_gd->GetSpriteData()->ComputeOptimalSubsprites(m_sprite->GetName());
	if (!subs)
	{
		wxMessageBox("This frame could not be repacked within 6 subsprites.",
			"Optimise Subsprites", wxOK | wxICON_INFORMATION, this);
		return;
	}
	// Apply through the canvas so the tiles re-derive from it and the change joins the undo history.
	m_spriteeditor->ApplyOptimisedSubsprites(*subs);
	FireEvent(EVT_PROPERTIES_UPDATE);
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
	AddMenuItem(fileMenu, 4, ID_FILE_EXPORT_SPRITE_PROPERTIES_YAML, "Export Sprite Properties as YAML...");
	AddMenuItem(fileMenu, 5, ID_FILE_EXPORT_ALL_SPRITESHEETS, "Export All Sprite Sheets with Metadata...");
	AddMenuItem(fileMenu, 6, ID_VIEW_SEP1, "", wxITEM_SEPARATOR);
	AddMenuItem(fileMenu, 7, ID_FILE_IMPORT_FRM, "Import Sprite Frame from Binary...");
	AddMenuItem(fileMenu, 8, ID_FILE_IMPORT_TILES, "Import Sprite Tileset from Binary...");
	AddMenuItem(fileMenu, 9, ID_FILE_IMPORT_VDPMAP, "Import VDP Sprite Map from CSV...");
	AddMenuItem(fileMenu, 10, ID_FILE_IMPORT_SPRITE_METADATA, "Import Sprite Metadata from YAML...");
	AddMenuItem(fileMenu, 11, ID_FILE_IMPORT_SPRITESHEET_NEW, "Import Sprite Sheet into New Sprite...");
	AddMenuItem(fileMenu, 12, ID_FILE_IMPORT_SPRITESHEET_CURRENT, "Import Sprite Sheet into Current Sprite...");
	auto& editMenu = AddMenu(menu, 1, ID_EDIT, "Edit");
	AddMenuItem(editMenu, 0, ID_EDIT_SPRITES, "Sprites...\tF10");
	AddMenuItem(editMenu, 1, ID_EDIT_SEP, "", wxITEM_SEPARATOR);
	AddMenuItem(editMenu, 2, ID_EDIT_UNDO, "Undo\tCtrl+Z");
	AddMenuItem(editMenu, 3, ID_EDIT_REDO, "Redo\tCtrl+Y");
	auto& viewMenu = AddMenu(menu, 2, ID_VIEW, "View");
	AddMenuItem(viewMenu, 0, ID_VIEW_TOGGLE_GRIDLINES, "Gridlines", wxITEM_CHECK);
	AddMenuItem(viewMenu, 1, ID_VIEW_TOGGLE_ALPHA, "Show Alpha as Black", wxITEM_CHECK);
	AddMenuItem(viewMenu, 2, ID_VIEW_TOGGLE_HITBOX, "Hitbox", wxITEM_CHECK);
	AddMenuItem(viewMenu, 3, ID_VIEW_SEP2, "", wxITEM_SEPARATOR);
	AddMenuItem(viewMenu, 4, ID_VIEW_TOOLBAR, "Toolbar", wxITEM_CHECK);
	AddMenuItem(viewMenu, 5, ID_VIEW_TOOLS_TOOLBAR, "Tools Toolbar", wxITEM_CHECK);
	AddMenuItem(viewMenu, 6, ID_VIEW_PALETTE, "Palette", wxITEM_CHECK);
	AddMenuItem(viewMenu, 7, ID_VIEW_PREVIEW, "Preview", wxITEM_CHECK);
	AddMenuItem(viewMenu, 8, ID_VIEW_FRAMES, "Frames", wxITEM_CHECK);
	AddMenuItem(viewMenu, 9, ID_VIEW_SUBSPRITES, "Subsprites", wxITEM_CHECK);
	AddMenuItem(viewMenu, 10, ID_VIEW_ANIMATIONS, "Animations", wxITEM_CHECK);
	AddMenuItem(viewMenu, 11, ID_VIEW_ANIM_FRAMES, "Animation Frames", wxITEM_CHECK);

	auto* parent = m_mgr.GetManagedWindow();
	wxAuiToolBar* toolbar = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORIZONTAL);
	// Both sliders read "right = more": zoom grows to the right (no inverse), and the speed slider
	// is inverted because its underlying value is a frame period (a smaller value animates faster).
	m_zoomslider = new wxSlider(toolbar, ID_ZOOM, m_zoom, 1, 8, wxDefaultPosition, wxSize(80, wxDefaultCoord), wxSL_HORIZONTAL);
	m_speedslider = new wxSlider(toolbar, ID_PLAY_SPEED, m_speed, 1, 10, wxDefaultPosition, wxSize(80, wxDefaultCoord), wxSL_HORIZONTAL | wxSL_INVERSE);
	// Same IDs as the Edit menu entries, so they share the handler and enable state.
	toolbar->AddTool(ID_EDIT_UNDO, "Undo", ilist.GetImage("undo"), "Undo (Ctrl+Z)");
	toolbar->AddTool(ID_EDIT_REDO, "Redo", ilist.GetImage("redo"), "Redo (Ctrl+Y)");
	toolbar->AddSeparator();
	toolbar->AddTool(ID_TOGGLE_GRIDLINES, "Toggle Gridlines", ilist.GetImage("gridlines"), "Toggle Gridlines", wxITEM_CHECK);
	toolbar->AddTool(ID_TOGGLE_ALPHA, "Toggle Alpha", ilist.GetImage("alpha"), "Toggle Alpha", wxITEM_CHECK);
	toolbar->AddTool(ID_TOGGLE_HITBOX, "Toggle Hitbox", ilist.GetImage("ehitbox"), "Toggle Hitbox", wxITEM_CHECK);
	toolbar->AddSeparator();
	toolbar->AddTool(ID_COMPRESS_FRAME, "Compress Frame", ilist.GetImage("compress"), "Compress Frame", wxITEM_CHECK);
	toolbar->AddTool(ID_OPTIMISE_SUBSPRITES, "Optimise Subsprites", ilist.GetImage("lightning"),
		"Repack this frame's subsprites to waste the fewest tiles (max 6 subsprites)");
	toolbar->AddSeparator();
	toolbar->AddTool(ID_CUT_TILE, "Cut", ilist.GetImage("cut"), "Cut");
	toolbar->AddTool(ID_COPY_TILE, "Copy", ilist.GetImage("copy"), "Copy");
	toolbar->AddTool(ID_PASTE_TILE, "Paste", ilist.GetImage("paste"), "Paste");
	toolbar->AddTool(ID_SWAP_TILES, "Swap", ilist.GetImage("swap"), "Swap");
	toolbar->AddTool(ID_CLEAR_TILE, "Clear", ilist.GetImage("delete"), "Clear");
	toolbar->AddSeparator();
	toolbar->AddTool(ID_HFLIP_SEL, "Flip Selection Horizontally", ilist.GetImage("hflip"),
		"Flip Selection Horizontally (Ctrl+H)");
	toolbar->AddTool(ID_VFLIP_SEL, "Flip Selection Vertically", ilist.GetImage("vflip"),
		"Flip Selection Vertically (Ctrl+E)");
	toolbar->AddSeparator();
	toolbar->AddLabel(wxID_ANY, "Zoom:");
	toolbar->AddControl(m_zoomslider, "Zoom");
	toolbar->AddSeparator();
	toolbar->AddTool(ID_PLAY_PAUSE, "Play/Pause", ilist.GetImage("play"), "Play/Pause", wxITEM_CHECK);
	toolbar->AddSeparator();
	toolbar->AddLabel(wxID_ANY, "Speed:");
	toolbar->AddControl(m_speedslider, "Play Speed");

	AddToolbar(m_mgr, *toolbar, "Sprite", "Sprite Tools", wxAuiPaneInfo().ToolbarPane().Top().Row(1).Position(1));

	// Vertical tools toolbar, matching the other editors. Check items with exclusivity
	// managed in UpdateUI, since wxAuiToolBar cannot untoggle radio items programmatically.
	wxAuiToolBar* tools_tb = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_VERTICAL);
	tools_tb->SetToolBitmapSize(wxSize(16, 16));
	tools_tb->AddTool(ID_SELECT, "Select", ilist.GetImage("mouse"), "Select", wxITEM_CHECK);
	tools_tb->AddTool(ID_SUBSPRITE_MODE, "Edit Subsprites", ilist.GetImage("subsprite"),
		"Edit Subsprites (drag to move, handles to resize, right-click or Ins/Del to add and remove)", wxITEM_CHECK);
	tools_tb->AddSeparator();
	tools_tb->AddTool(ID_PIXEL_SELECT, "Select Pixels", ilist.GetImage("select_rect"),
		"Select Pixels (drag to move, Shift+drag to copy, Ctrl+drag to stamp, Ctrl+C/X/V, Ctrl+H/E to flip)", wxITEM_CHECK);
	tools_tb->AddTool(ID_PICKER, "Colour Picker", ilist.GetImage("dropper"),
		"Colour Picker (left click: primary, right click: secondary)", wxITEM_CHECK);
	tools_tb->AddTool(ID_PENCIL, "Pencil", ilist.GetImage("pencil"), "Pencil", wxITEM_CHECK);
	tools_tb->AddTool(ID_LINE, "Draw Line", ilist.GetImage("line"), "Draw Line", wxITEM_CHECK);
	tools_tb->AddTool(ID_RECT_FILLED, "Draw Filled Rectangle", ilist.GetImage("rect_filled"), "Draw Filled Rectangle", wxITEM_CHECK);
	tools_tb->AddTool(ID_RECT_OUTLINE, "Draw Outlined Rectangle", ilist.GetImage("rect_outline"), "Draw Outlined Rectangle", wxITEM_CHECK);
	tools_tb->AddTool(ID_CIRCLE_FILLED, "Draw Filled Circle", ilist.GetImage("circle_filled"), "Draw Filled Circle", wxITEM_CHECK);
	tools_tb->AddTool(ID_CIRCLE_OUTLINE, "Draw Outlined Circle", ilist.GetImage("circle_outline"), "Draw Outlined Circle", wxITEM_CHECK);
	tools_tb->AddTool(ID_FILL, "Fill", ilist.GetImage("fill"), "Fill", wxITEM_CHECK);
	AddToolbar(m_mgr, *tools_tb, "Tools", "Tools", wxAuiPaneInfo().ToolbarPane().Left().Row(1).Position(1));

	UpdateUI();

	m_mgr.Update();
}

void SpriteEditorFrame::ClearMenu(wxMenuBar& menu) const
{
	// The toolbar destructor deletes these, but doesn't clear the pointer
	m_zoomslider = nullptr;
	m_speedslider = nullptr;
	EditorFrame::ClearMenu(menu);
}

void SpriteEditorFrame::ShowSpriteManagerDialog()
{
	if (!m_gd)
	{
		return;
	}
	// Open on the sprite being edited, so the dialog lands on what is in front of you.
	const int select = m_sprite ? m_sprite->GetSprite() : 0;
	SpriteManagerDialog dlg(this, m_gd, select);
	dlg.ShowModal();

	const int to_open = dlg.GetSpriteToOpen();
	if (!dlg.HasChanges() && to_open < 0)
	{
		return;
	}

	// A rename or delete can leave the open sprite's id meaningless, so fall back to whatever
	// the dialog left selected, then to the first sprite there is.
	const auto sprite_data = m_gd->GetSpriteData();
	int target = to_open >= 0 ? to_open : select;
	if (target < 0 || !sprite_data->IsSprite(static_cast<uint8_t>(target)))
	{
		target = sprite_data->IsSprite(0) ? 0 : -1;
	}
	std::wstring path;
	if (target >= 0)
	{
		path = L"Sprites/" + Landstalker::SpriteData::GetSpriteDisplayName(static_cast<uint8_t>(target));
	}

	if (!dlg.HasChanges())
	{
		// Nothing moved, so the tree is still correct - just follow the double-click.
		wxCommandEvent evt(EVT_GO_TO_NAV_ITEM);
		evt.SetString(wxString(path));
		evt.SetInt(target);
		evt.SetClientData(this);
		wxPostEvent(this, evt);
		return;
	}

	// Adding, deleting, renaming or moving a sprite changes the name and number of entries
	// under Sprites, which the editor cannot patch one at a time, so the tree is rebuilt from
	// the game data.
	wxCommandEvent evt(EVT_REBUILD_NAV_TREE);
	evt.SetInt(target);
	evt.SetString(wxString(path));
	evt.SetClientData(this);
	wxPostEvent(this, evt);
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
	case ID_FILE_EXPORT_SPRITE_PROPERTIES_YAML:
		OnExportPropertiesYaml();
		break;
	case ID_FILE_EXPORT_ALL_SPRITESHEETS:
		OnExportAllSpritesheets();
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
	case ID_FILE_IMPORT_SPRITE_METADATA:
		OnImportSpriteMetadata();
		break;
	case ID_FILE_IMPORT_SPRITESHEET_NEW:
		OnImportSpriteSheetNew();
		break;
	case ID_FILE_IMPORT_SPRITESHEET_CURRENT:
		OnImportSpriteSheetCurrent();
		break;
	case ID_EDIT_SPRITES:
		ShowSpriteManagerDialog();
		break;
	case ID_EDIT_UNDO:
		if (m_spriteeditor->CanUndo())
		{
			m_spriteeditor->Undo();
			RefreshAfterUndoRedo();
		}
		break;
	case ID_EDIT_REDO:
		if (m_spriteeditor->CanRedo())
		{
			m_spriteeditor->Redo();
			RefreshAfterUndoRedo();
		}
		break;
	case ID_SELECT:
		m_spriteeditor->SetMode(SpriteFrameEditorCtrl::Mode::SELECT);
		break;
	case ID_SUBSPRITE_MODE:
		m_spriteeditor->SetMode(SpriteFrameEditorCtrl::Mode::SUBSPRITE);
		break;
	case ID_PENCIL:
		SelectDrawTool(SpriteFrameEditorCtrl::Tool::Pencil);
		break;
	case ID_LINE:
		SelectDrawTool(SpriteFrameEditorCtrl::Tool::Line);
		break;
	case ID_RECT_FILLED:
		SelectDrawTool(SpriteFrameEditorCtrl::Tool::RectangleFilled);
		break;
	case ID_RECT_OUTLINE:
		SelectDrawTool(SpriteFrameEditorCtrl::Tool::RectangleOutline);
		break;
	case ID_CIRCLE_FILLED:
		SelectDrawTool(SpriteFrameEditorCtrl::Tool::CircleFilled);
		break;
	case ID_CIRCLE_OUTLINE:
		SelectDrawTool(SpriteFrameEditorCtrl::Tool::CircleOutline);
		break;
	case ID_FILL:
		SelectDrawTool(SpriteFrameEditorCtrl::Tool::Fill);
		break;
	case ID_PICKER:
		SelectDrawTool(SpriteFrameEditorCtrl::Tool::Picker);
		break;
	case ID_PIXEL_SELECT:
		SelectDrawTool(SpriteFrameEditorCtrl::Tool::PixelSelect);
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
	case ID_OPTIMISE_SUBSPRITES:
		OnOptimiseSubsprites();
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
	case ID_HFLIP_SEL:
		m_spriteeditor->FlipSelection(true);
		break;
	case ID_VFLIP_SEL:
		m_spriteeditor->FlipSelection(false);
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
	case ID_VIEW_TOOLS_TOOLBAR:
		SetToolbarVisibility("Tools", !IsToolbarVisible("Tools"));
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
	Landstalker::WriteBytes(bytes, filename);
}

void SpriteEditorFrame::ExportTiles(const std::string& filename) const
{
	auto bytes = m_sprite->GetData()->GetTileset()->GetBits(false);
	Landstalker::WriteBytes(bytes, filename);
}

void SpriteEditorFrame::ExportVdpSpritemap(const std::string& filename) const
{
	std::ofstream fs(filename, std::ios::out | std::ios::trunc);
	fs << (m_sprite->GetData()->GetCompressed() ? "1" : "0") << std::endl;
	for (std::size_t i = 0; i < m_sprite->GetData()->GetSubSpriteCount(); ++i)
	{
		const auto& data = m_sprite->GetData()->GetSubSprite(i);
		fs << Landstalker::StrPrintf("% 03d,% 03d,%01d,%01d\n", data.x, data.y, data.w, data.h);
	}
}

void SpriteEditorFrame::ExportPng(const std::string& filename) const
{
	int width = m_sprite->GetData()->GetWidth();
	int height = m_sprite->GetData()->GetHeight();
	ImageBufferWx buf(width, height);
	buf.InsertSprite(-m_sprite->GetData()->GetLeft(), -m_sprite->GetData()->GetTop(), 0, *m_sprite->GetData());
	buf.WritePNG(filename, { m_palette }, true);
}

void SpriteEditorFrame::ExportPropertiesYaml(const std::string& filename)
{   
    auto sd = m_gd->GetSpriteData();
    std::ostringstream ss;
    int sprite_index = m_sprite->GetSprite();
    
    // Main properties
    ss << "ID: " << sprite_index << std::endl;
    ss << "Name: " << Landstalker::wstr_to_utf8(sd->GetSpriteDisplayName(sprite_index)) << std::endl;
    ss << "Label: " << sd->GetSpriteName(sprite_index) << std::endl;
    ss << "StartAddress: " << Landstalker::StrPrintf("0x%06X", m_sprite->GetStartAddress()) << std::endl;
    ss << "EndAddress: " << Landstalker::StrPrintf("0x%06X", m_sprite->GetEndAddress()) << std::endl;
    ss << "Size: " << m_sprite->GetDataLength() << std::endl;
    ss << "Compressed: " << (m_sprite->GetData()->GetCompressed() ? "true" : "false") << std::endl;
    
    // Animation properties
    auto flags = sd->GetSpriteAnimationFlags(sprite_index);
    ss << std::endl << "Animation:" << std::endl;
    
    ss << "  IdleAnimationFrameCount: " << 
        (flags.idle_animation_frames == Landstalker::SpriteData::AnimationFlags::IdleAnimationFrameCount::TWO_FRAMES ? 2 : 1) << std::endl;
    
    ss << "  IdleAnimationSource: " << 
        (flags.idle_animation_source == Landstalker::SpriteData::AnimationFlags::IdleAnimationSource::USE_WALK_FRAMES ? 
            "UseWalkFrames" : "Dedicated") << std::endl;
    
    ss << "  JumpAnimationSource: " << 
        (flags.jump_animation_source == Landstalker::SpriteData::AnimationFlags::JumpAnimationSource::USE_IDLE_FRAMES ? 
            "UseIdleFrames" : "Dedicated") << std::endl;
    
    ss << "  WalkCycleFrameCount: " << 
        (flags.walk_animation_frame_count == Landstalker::SpriteData::AnimationFlags::WalkAnimationFrameCount::FOUR_FRAMES ? 4 : 2) << std::endl;
    
    ss << "  TakeDamageAnimationSource: " << 
        (flags.take_damage_animation_source == Landstalker::SpriteData::AnimationFlags::TakeDamageAnimationSource::USE_IDLE_FRAMES ? 
            "UseIdleFrames" : "Dedicated") << std::endl;
    
    ss << "  DoNotRotate: " << (flags.do_not_rotate ? "true" : "false") << std::endl;
    ss << "  FullAnimations: " << (flags.has_full_animations ? "true" : "false") << std::endl;
    
    // Hitbox properties
    auto hitbox = sd->GetSpriteHitbox(sprite_index);
    ss << std::endl << "Hitbox:" << std::endl;
    ss << "  Width: " << std::fixed << std::setprecision(2) << (hitbox.base / 8.0) << std::endl;
    ss << "  Height: " << std::fixed << std::setprecision(2) << (hitbox.height / 16.0) << std::endl;
    ss << "MaxTileCount: " << static_cast<int>(sd->GetSpriteMaxTileCount(sprite_index)) << std::endl;
    
    // Write to file
    std::ofstream file(filename);
    if (file.is_open())
    {
        file << ss.str();
        file.close();
    }
}

void SpriteEditorFrame::ExportAllSpritesheets(const std::string& dir)
{
	using Result = Landstalker::SpriteData::SpriteSheetResult;
	auto sd = m_gd->GetSpriteData();
	const std::filesystem::path out_dir(dir);

	int written = 0;
	int failed = 0;
	for (int id = 0; id < static_cast<int>(Landstalker::SpriteData::MAX_SPRITES); ++id)
	{
		const uint8_t sid = static_cast<uint8_t>(id);
		if (!sd->IsSprite(sid))
		{
			continue;
		}
		// Colour each sprite with the same palette the editor previews it with.
		const auto palette = sd->GetSpriteDisplayPalette(sid);
		const std::filesystem::path png_path = out_dir / (sd->GetSpriteName(sid) + "_sheet.png");
		switch (sd->WriteSpriteSheet(sid, png_path, { palette }))
		{
		case Result::Written:
		case Result::MetadataWriteFailed:
			++written;
			break;
		case Result::ImageWriteFailed:
			++failed;
			break;
		case Result::NoFrames:
			break;  // Nothing to draw for this sprite - skip it.
		}
	}

	const wxString summary = wxString::Format("Exported %d sprite sheet%s to:\n%s", written,
		written == 1 ? "" : "s", wxString::FromUTF8(dir))
		+ (failed > 0 ? wxString::Format("\n\n%d sprite%s could not be written.", failed, failed == 1 ? "" : "s") : wxString());
	wxMessageBox(summary, "Export All Sprite Sheets", wxOK | (failed > 0 ? wxICON_WARNING : wxICON_INFORMATION), this);
}

void SpriteEditorFrame::ImportFrm(const std::string& filename)
{
	auto bytes = Landstalker::ReadBytes(filename);
	m_sprite->GetData()->SetBits(bytes);
	EnsureMaxTileCount();
	m_spriteeditor->Open(m_sprite->GetData(), m_palette, m_sprite->GetSprite());
	m_preview->Open(m_sprite->GetData(), m_palette);
	m_subspritectrl->SetSubsprites(m_sprite->GetData()->GetSubSprites());
	m_framectrl->SetSprite(m_sprite->GetSprite());
	m_animctrl->SetSprite(m_sprite->GetSprite());
	m_animframectrl->SetAnimation(m_sprite->GetSprite(), m_anim);
	Redraw();
	UpdateUI();
	FireEvent(EVT_PROPERTIES_UPDATE);
	FireEvent(EVT_STATUSBAR_UPDATE);
}

void SpriteEditorFrame::ImportTiles(const std::string& filename)
{
	auto bytes = Landstalker::ReadBytes(filename);
	m_sprite->GetData()->GetTileset()->SetBits(bytes, false);
	EnsureMaxTileCount();
	m_spriteeditor->Open(m_sprite->GetData(), m_palette, m_sprite->GetSprite());
	m_preview->Open(m_sprite->GetData(), m_palette);
	m_subspritectrl->SetSubsprites(m_sprite->GetData()->GetSubSprites());
	m_framectrl->SetSprite(m_sprite->GetSprite());
	m_animctrl->SetSprite(m_sprite->GetSprite());
	m_animframectrl->SetAnimation(m_sprite->GetSprite(), m_anim);
	Redraw();
	UpdateUI();
	FireEvent(EVT_PROPERTIES_UPDATE);
	FireEvent(EVT_STATUSBAR_UPDATE);
}

void SpriteEditorFrame::ImportVdpSpritemap(const std::string& filename)
{
	std::ifstream fs(filename, std::ios::in);
	bool compressed = false;
	std::vector<Landstalker::SpriteFrame::SubSprite> subs;
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
		Landstalker::StrToInt(cell, x);
		std::getline(ss, cell, ',');
		Landstalker::StrToInt(cell, y);
		std::getline(ss, cell, ',');
		Landstalker::StrToInt(cell, w);
		std::getline(ss, cell, ',');
		Landstalker::StrToInt(cell, h);
		subs.emplace_back(x, y, w, h);
	}
	m_sprite->GetData()->SetCompressed(compressed);
	m_sprite->GetData()->SetSubSprites(subs);
	m_spriteeditor->Open(m_sprite->GetData(), m_palette, m_sprite->GetSprite());
	m_preview->Open(m_sprite->GetData(), m_palette);
	m_subspritectrl->SetSubsprites(m_sprite->GetData()->GetSubSprites());
	m_framectrl->SetSprite(m_sprite->GetSprite());
	m_animctrl->SetSprite(m_sprite->GetSprite());
	m_animframectrl->SetAnimation(m_sprite->GetSprite(), m_anim);
	Redraw();
	UpdateUI();
	FireEvent(EVT_PROPERTIES_UPDATE);
	FireEvent(EVT_STATUSBAR_UPDATE);
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
		props.Append(new wxStringProperty("Name", "Name", wxString(sd->GetSpriteDisplayName(sprite_index))));
		props.Append(new wxStringProperty("Label", "Label", wxString(sd->GetSpriteName(sprite_index))))->Enable(false);
		props.Append(new wxIntProperty("ID", "ID", sprite_index))->Enable(false);
		props.Append(new wxStringProperty("Start Address", "Start Address", wxString(Landstalker::StrPrintf("0x%06X", m_sprite->GetStartAddress()))))->Enable(false);
		props.Append(new wxStringProperty("End Address", "End Address", wxString(Landstalker::StrPrintf("0x%06X", m_sprite->GetEndAddress()))))->Enable(false);
		props.Append(new wxIntProperty("Size (bytes)", "Size", m_sprite->GetDataLength()))->Enable(false);
		props.Append(new wxEnumProperty("Low Palette", "Low Palette", m_lo_palettes));
		props.Append(new wxEnumProperty("High Palette", "High Palette", m_hi_palettes));
		props.Append(new wxEnumProperty("Projectile/Misc Palette 1", "Projectile/Misc Palette 1", m_misc_palettes));
		props.Append(new wxEnumProperty("Projectile/Misc Palette 2", "Projectile/Misc Palette 2", m_misc_palettes));
		wxPGProperty* tile_prop = new wxIntProperty("Max Tile Count", "Max Tile Count", sd->GetSpriteMaxTileCount(sprite_index));
		tile_prop->SetAttribute(wxPG_ATTR_MIN, 1);
		tile_prop->SetAttribute(wxPG_ATTR_MAX, 65535);
		tile_prop->SetAttribute(wxPG_ATTR_SPINCTRL_STEP, 1);
		tile_prop->SetEditor(wxPGEditor_SpinCtrl);
		props.Append(tile_prop);
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
		wxPGProperty* base_prop = new wxFloatProperty("Width/Length", "Width/Length", sd->GetSpriteHitbox(sprite_index).base / 8.0);
		base_prop->SetAttribute(wxPG_ATTR_MIN, 0.0);
		base_prop->SetAttribute(wxPG_ATTR_MAX, 31.875);
		base_prop->SetAttribute(wxPG_ATTR_SPINCTRL_STEP, 0.125);
		base_prop->SetEditor(wxPGEditor_SpinCtrl);
		props.Append(base_prop);
		wxPGProperty* height_prop = new wxFloatProperty("Height", "Height", sd->GetSpriteHitbox(sprite_index).height / 16.0);
		height_prop->SetAttribute(wxPG_ATTR_MIN, 0.0);
		height_prop->SetAttribute(wxPG_ATTR_MAX, 15.9375);
		height_prop->SetAttribute(wxPG_ATTR_SPINCTRL_STEP, 0.0625);
		height_prop->SetEditor(wxPGEditor_SpinCtrl);
		props.Append(height_prop);
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
			m_lo_palettes.Add(wxString(m_gd->GetSpriteData()->GetSpriteLowPaletteDisplayName(i)));
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
			m_hi_palettes.Add(wxString(m_gd->GetSpriteData()->GetSpriteHighPaletteDisplayName(i)));
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
				m_misc_palettes.Add(wxString(p.first));
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
		// A sprite the manager just created has no entity, so its palette selection is left
		// blank rather than read from an entity that does not exist.
		const auto property_entities = sd->GetEntitiesFromSprite(sprite_index);
		const int entity_index = property_entities.empty() ? -1 : property_entities[0];

		props.GetGrid()->SetPropertyValue("Name", wxString(sd->GetSpriteDisplayName(sprite_index)));
		props.GetGrid()->SetPropertyValue("Label", wxString(sd->GetSpriteName(sprite_index)));
		props.GetGrid()->SetPropertyValue("ID", static_cast<int>(sprite_index));
		props.GetGrid()->SetPropertyValue("Start Address", wxString(Landstalker::StrPrintf("0x%06X", m_sprite->GetStartAddress())));
		props.GetGrid()->SetPropertyValue("End Address", wxString(Landstalker::StrPrintf("0x%06X", m_sprite->GetEndAddress())));
		props.GetGrid()->SetPropertyValue("Size", static_cast<int>(m_sprite->GetDataLength()));
		props.GetGrid()->GetProperty("Low Palette")->SetChoices(m_lo_palettes);
		props.GetGrid()->GetProperty("High Palette")->SetChoices(m_hi_palettes);
		props.GetGrid()->GetProperty("Low Palette")->SetChoiceSelection(
			entity_index < 0 ? 0 : sd->GetEntityPaletteIdxs(entity_index).first + 1);
		props.GetGrid()->GetProperty("High Palette")->SetChoiceSelection(
			entity_index < 0 ? 0 : sd->GetEntityPaletteIdxs(entity_index).second + 1);
		props.GetGrid()->GetProperty("Projectile/Misc Palette 1")->SetChoiceSelection(0);
		props.GetGrid()->GetProperty("Projectile/Misc Palette 2")->SetChoiceSelection(0);
		props.GetGrid()->SetPropertyValue("Max Tile Count", static_cast<int>(sd->GetSpriteMaxTileCount(sprite_index)));
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
		props.GetGrid()->SetPropertyValue("Width/Length", sd->GetSpriteHitbox(sprite_index).base / 8.0);
		props.GetGrid()->SetPropertyValue("Height", sd->GetSpriteHitbox(sprite_index).height / 16.0);
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
	if (name == "Name")
	{
		const std::wstring new_name = property->GetValueAsString().ToStdWstring();
		const std::wstring old_name = sd->GetSpriteDisplayName(sprite_index);
		if (Landstalker::Labels::IsValid(new_name))
		{
			FireRenameNavItemEvent(new_name, old_name);
			Landstalker::Labels::Update(Landstalker::Labels::C_SPRITES, sprite_index, new_name);
		}
		else
		{
			property->SetValueFromString(old_name);
		}
	}
	else if (name == "Width/Length")
	{
		auto hitbox = sd->GetSpriteHitbox(sprite_index);
		int value = static_cast<int>(property->GetValuePlain().GetDouble() * 8.0);
		if (value != hitbox.base)
		{
			hitbox.base = std::clamp<uint8_t>(value, 0, 255);
			sd->SetSpriteHitbox(sprite_index, {hitbox.base, hitbox.height});
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
		if (value != hitbox.height)
		{
			hitbox.height = std::clamp<uint8_t>(value, 0, 255);
			sd->SetSpriteHitbox(sprite_index, {hitbox.base, hitbox.height});
			FireEvent(EVT_PROPERTIES_UPDATE);
			if (m_spriteeditor->GetHitboxEnabled())
			{
				m_spriteeditor->RedrawTiles();
			}
		}
	}
	else if (name == "Max Tile Count")
	{
		sd->SetSpriteMaxTileCount(sprite_index, static_cast<uint16_t>(property->GetValuePlain().GetLong()));
		// The reservation can never drop below what the current frame needs.
		EnsureMaxTileCount();
		FireEvent(EVT_PROPERTIES_UPDATE);
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
		Landstalker::SpriteData::AnimationFlags flags;
		flags.idle_animation_frames = ctrl->GetGrid()->GetPropertyByName("Idle Animation Frame Count")->GetValue().GetLong() == 0 ?
			Landstalker::SpriteData::AnimationFlags::IdleAnimationFrameCount::ONE_FRAME : Landstalker::SpriteData::AnimationFlags::IdleAnimationFrameCount::TWO_FRAMES;
		flags.idle_animation_source = ctrl->GetGrid()->GetPropertyByName("Idle Animation Source")->GetValue().GetLong() == 0 ?
			Landstalker::SpriteData::AnimationFlags::IdleAnimationSource::USE_WALK_FRAMES : Landstalker::SpriteData::AnimationFlags::IdleAnimationSource::DEDICATED;
		flags.jump_animation_source = ctrl->GetGrid()->GetPropertyByName("Jump Animation Source")->GetValue().GetLong() == 0 ?
			Landstalker::SpriteData::AnimationFlags::JumpAnimationSource::USE_IDLE_FRAMES : Landstalker::SpriteData::AnimationFlags::JumpAnimationSource::DEDICATED;
		flags.walk_animation_frame_count = ctrl->GetGrid()->GetPropertyByName("Walk Cycle Frame Count")->GetValue().GetLong() == 0 ?
			Landstalker::SpriteData::AnimationFlags::WalkAnimationFrameCount::TWO_FRAMES : Landstalker::SpriteData::AnimationFlags::WalkAnimationFrameCount::FOUR_FRAMES;
		flags.take_damage_animation_source = ctrl->GetGrid()->GetPropertyByName("Take Damage Animation Source")->GetValue().GetLong() == 0 ?
			Landstalker::SpriteData::AnimationFlags::TakeDamageAnimationSource::USE_IDLE_FRAMES : Landstalker::SpriteData::AnimationFlags::TakeDamageAnimationSource::DEDICATED;
		flags.do_not_rotate = ctrl->GetGrid()->GetPropertyByName("Do Not Rotate")->GetValue().GetBool();
		flags.has_full_animations = ctrl->GetGrid()->GetPropertyByName("Full Animations")->GetValue().GetBool();
		sd->SetSpriteAnimationFlags(m_sprite->GetSprite(), flags);
	}
	ctrl->GetGrid()->Thaw();
}

void SpriteEditorFrame::UpdateUI() const
{
	CheckMenuItem(ID_VIEW_TOOLBAR, IsToolbarVisible("Sprite"));
	CheckMenuItem(ID_VIEW_TOOLS_TOOLBAR, IsToolbarVisible("Tools"));
	if (m_spriteeditor != nullptr)
	{
		const auto mode = m_spriteeditor->GetMode();
		const auto tool = m_spriteeditor->GetDrawTool();
		const bool draw = (mode == SpriteFrameEditorCtrl::Mode::DRAW);
		CheckToolbarItem("Tools", ID_SELECT, mode == SpriteFrameEditorCtrl::Mode::SELECT);
		CheckToolbarItem("Tools", ID_SUBSPRITE_MODE, mode == SpriteFrameEditorCtrl::Mode::SUBSPRITE);
		CheckToolbarItem("Tools", ID_PENCIL, draw && (tool == SpriteFrameEditorCtrl::Tool::Pencil));
		CheckToolbarItem("Tools", ID_LINE, draw && (tool == SpriteFrameEditorCtrl::Tool::Line));
		CheckToolbarItem("Tools", ID_RECT_FILLED, draw && (tool == SpriteFrameEditorCtrl::Tool::RectangleFilled));
		CheckToolbarItem("Tools", ID_RECT_OUTLINE, draw && (tool == SpriteFrameEditorCtrl::Tool::RectangleOutline));
		CheckToolbarItem("Tools", ID_CIRCLE_FILLED, draw && (tool == SpriteFrameEditorCtrl::Tool::CircleFilled));
		CheckToolbarItem("Tools", ID_CIRCLE_OUTLINE, draw && (tool == SpriteFrameEditorCtrl::Tool::CircleOutline));
		CheckToolbarItem("Tools", ID_FILL, draw && (tool == SpriteFrameEditorCtrl::Tool::Fill));
		CheckToolbarItem("Tools", ID_PICKER, draw && (tool == SpriteFrameEditorCtrl::Tool::Picker));
		CheckToolbarItem("Tools", ID_PIXEL_SELECT, draw && (tool == SpriteFrameEditorCtrl::Tool::PixelSelect));
	}
	CheckMenuItem(ID_VIEW_PALETTE, IsPaneVisible(m_paledit));
	CheckMenuItem(ID_VIEW_PREVIEW, IsPaneVisible(m_preview));
	CheckMenuItem(ID_VIEW_FRAMES, IsPaneVisible(m_framectrl));
	CheckMenuItem(ID_VIEW_SUBSPRITES, IsPaneVisible(m_subspritectrl));
	CheckMenuItem(ID_VIEW_ANIMATIONS, IsPaneVisible(m_animctrl));
	CheckMenuItem(ID_VIEW_ANIM_FRAMES, IsPaneVisible(m_animframectrl));
	if (m_spriteeditor != nullptr && m_sprite != nullptr)
	{
		m_last_can_undo = m_spriteeditor->CanUndo();
		m_last_can_redo = m_spriteeditor->CanRedo();
		EnableMenuItem(ID_EDIT_UNDO, m_last_can_undo);
		EnableMenuItem(ID_EDIT_REDO, m_last_can_redo);
		EnableToolbarItem("Sprite", ID_EDIT_UNDO, m_last_can_undo);
		EnableToolbarItem("Sprite", ID_EDIT_REDO, m_last_can_redo);
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
		// The cell operations act on the tile selection, which only select mode drives.
		const bool select_mode = (m_spriteeditor->GetMode() == SpriteFrameEditorCtrl::Mode::SELECT);
		const bool cell_ops = select_mode && m_spriteeditor->IsSelectionValid();
		EnableToolbarItem("Sprite", ID_CUT_TILE, cell_ops);
		EnableToolbarItem("Sprite", ID_COPY_TILE, cell_ops);
		EnableToolbarItem("Sprite", ID_PASTE_TILE, cell_ops && !m_spriteeditor->IsClipboardEmpty());
		EnableToolbarItem("Sprite", ID_SWAP_TILES, cell_ops);
		EnableToolbarItem("Sprite", ID_CLEAR_TILE, cell_ops);
		// The flips act on the pixel selection, which only that tool creates.
		const bool flips = (m_spriteeditor->GetMode() == SpriteFrameEditorCtrl::Mode::DRAW) &&
			(m_spriteeditor->GetDrawTool() == SpriteFrameEditorCtrl::Tool::PixelSelect);
		EnableToolbarItem("Sprite", ID_HFLIP_SEL, flips);
		EnableToolbarItem("Sprite", ID_VFLIP_SEL, flips);
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
		m_last_can_undo = false;
		m_last_can_redo = false;
		EnableMenuItem(ID_EDIT_UNDO, false);
		EnableMenuItem(ID_EDIT_REDO, false);
		EnableToolbarItem("Sprite", ID_EDIT_UNDO, false);
		EnableToolbarItem("Sprite", ID_EDIT_REDO, false);
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
		EnableToolbarItem("Sprite", ID_HFLIP_SEL, false);
		EnableToolbarItem("Sprite", ID_VFLIP_SEL, false);
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

void SpriteEditorFrame::UpdateUndoRedoUI() const
{
	if (m_spriteeditor == nullptr)
	{
		return;
	}
	const bool can_undo = m_spriteeditor->CanUndo();
	const bool can_redo = m_spriteeditor->CanRedo();
	if ((can_undo != m_last_can_undo) || (can_redo != m_last_can_redo))
	{
		m_last_can_undo = can_undo;
		m_last_can_redo = can_redo;
		EnableMenuItem(ID_EDIT_UNDO, can_undo);
		EnableMenuItem(ID_EDIT_REDO, can_redo);
		EnableToolbarItem("Sprite", ID_EDIT_UNDO, can_undo);
		EnableToolbarItem("Sprite", ID_EDIT_REDO, can_redo);
	}
}

void SpriteEditorFrame::RefreshAfterUndoRedo()
{
	// A restored state can differ in both artwork and subsprite layout, so every view of the
	// frame is refreshed: the canvas redraws itself during the restore, the rest is done here.
	m_subspritectrl->SetSubsprites(m_sprite->GetData()->GetSubSprites());
	m_preview->Refresh(true);
}

void SpriteEditorFrame::SelectDrawTool(SpriteFrameEditorCtrl::Tool tool)
{
	m_spriteeditor->SetDrawTool(tool);
	m_spriteeditor->SetMode(SpriteFrameEditorCtrl::Mode::DRAW);
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
		// Cancelling used to fall through and create a frame with an empty name.
		if (dlg.ShowModal() != wxID_OK)
		{
			return;
		}
		name = dlg.GetValue().ToStdString();
	} while (name.empty() || m_gd->GetSpriteData()->SpriteFrameExists(name));
	m_gd->GetSpriteData()->AddSpriteFrame(m_sprite->GetSprite(), name);
	auto new_frame = m_gd->GetSpriteData()->GetSpriteFrame(name);
	// Seed the new frame as a duplicate of the one being edited - a copy is almost always
	// a better starting point for a new animation frame than a blank canvas.
	if (new_frame && new_frame->GetData() && m_sprite->GetData())
	{
		*new_frame->GetData() = *m_sprite->GetData();
	}
	m_sprite = new_frame;
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
	if (m_sprite->GetData()->GetSubSpriteCount() < Landstalker::SpriteFrame::MAX_SUBSPRITES)
	{
		int pos = evt.GetInt();
		if (pos < 1)
		{
			pos = 1;
		}
		m_spriteeditor->PushUndo();
		m_sprite->GetData()->AddSubSpriteBefore(pos - 1);
		m_spriteeditor->UpdateSubSprites();
		EnsureMaxTileCount();
		m_subspritectrl->SetSubsprites(m_sprite->GetData()->GetSubSprites());
		m_spriteeditor->SelectSubSprite(pos);
		m_subspritectrl->SetSelected(pos);
		// Adding a subsprite is a subsprite-editing act: switch to the tool that can
		// place and size it.
		m_spriteeditor->SetMode(SpriteFrameEditorCtrl::Mode::SUBSPRITE);
		UpdateUI();
	}
}

void SpriteEditorFrame::OnSubSpriteDelete(wxCommandEvent& evt)
{
	int pos = evt.GetInt();
	if (pos > 0 && pos <= static_cast<int>(m_sprite->GetData()->GetSubSpriteCount()))
	{
		m_spriteeditor->PushUndo();
		m_sprite->GetData()->DeleteSubSprite(pos - 1);
		m_spriteeditor->UpdateSubSprites();
		m_subspritectrl->SetSubsprites(m_sprite->GetData()->GetSubSprites());
		m_spriteeditor->SetMode(SpriteFrameEditorCtrl::Mode::SUBSPRITE);
		UpdateUI();
	}
}

void SpriteEditorFrame::OnSubSpriteMoveUp(wxCommandEvent& evt)
{
	if (m_sprite->GetData()->GetSubSpriteCount() > 0)
	{
		std::size_t pos = evt.GetInt();
		if (pos > 1 && pos <= m_sprite->GetData()->GetSubSpriteCount())
		{
			m_spriteeditor->PushUndo();
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
			m_spriteeditor->PushUndo();
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
	// A canvas resize can grow the frame's tile count past the reservation.
	EnsureMaxTileCount();
	m_subspritectrl->SetSubsprites(m_sprite->GetData()->GetSubSprites());
	// Keyboard subsprite moves arrive here without passing through ProcessEvent/UpdateUI.
	UpdateUndoRedoUI();
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

		OpenFrame(m_sprite->GetSprite(), m_frame, m_anim, -1, false);
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
		if (dlg.ShowModal() != wxID_OK)
		{
			// User cancelled - do not create an animation.
			return;
		}
		name = dlg.GetValue().ToStdString();
	} while (name.empty() || m_gd->GetSpriteData()->SpriteAnimationExists(name));
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
	// Resolve the animation by its list position, not the displayed label (which carries a role tag).
	const auto anims = m_gd->GetSpriteData()->GetSpriteAnimations(m_sprite->GetSprite());
	if (evt.GetInt() > 0 && evt.GetInt() <= static_cast<int>(anims.size()))
	{
		m_gd->GetSpriteData()->DeleteSpriteAnimation(anims[evt.GetInt() - 1]);
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
	const auto anims = m_gd->GetSpriteData()->GetSpriteAnimations(m_sprite->GetSprite());
	if (evt.GetInt() > 1 && evt.GetInt() <= static_cast<int>(anims.size()))
	{
		m_anim = evt.GetInt() - 2;
		m_gd->GetSpriteData()->MoveSpriteAnimation(m_sprite->GetSprite(), anims[evt.GetInt() - 1], evt.GetInt() - 2);
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
	const auto anims = m_gd->GetSpriteData()->GetSpriteAnimations(m_sprite->GetSprite());
	if (evt.GetInt() > 0 && evt.GetInt() < static_cast<int>(anims.size()))
	{
		m_anim = evt.GetInt();
		m_gd->GetSpriteData()->MoveSpriteAnimation(m_sprite->GetSprite(), anims[evt.GetInt() - 1], evt.GetInt());
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
		// Resolve the frame by its position in the animation, not the role-tagged display label.
		const auto frames = m_gd->GetSpriteData()->GetSpriteAnimationFrames(m_sprite->GetSprite(), m_anim);
		m_frame = m_gd->GetSpriteData()->GetSpriteFrameId(m_sprite->GetSprite(), frames[evt.GetInt() - 1]);
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
	int framesCount = static_cast<int>(m_gd->GetSpriteData()->GetSpriteAnimationFrames(m_sprite->GetSprite(), m_anim).size());

	if (evt.GetInt() > 0 && evt.GetInt() <= framesCount && framesCount > 1)
	{
		int deleteIndex = evt.GetInt() - 1;

		std::string anim = m_gd->GetSpriteData()->GetSpriteAnimations(m_sprite->GetSprite())[m_anim];

		// Delete the frame
		m_gd->GetSpriteData()->DeleteSpriteAnimationFrame(anim, deleteIndex);
		framesCount--;

		// Update preview frame
		int selectIndex = deleteIndex;
		if(selectIndex >= framesCount){
			selectIndex = framesCount - 1;

		}

		m_animframectrl->SetAnimation(m_sprite->GetSprite(), m_anim);
		m_preview->SetAnimationFrame(selectIndex);
		m_frame = m_gd->GetSpriteData()->GetSpriteFrameId(m_sprite->GetSprite(), m_gd->GetSpriteData()->GetSpriteAnimationFrames(anim)[selectIndex]);
		m_sprite = m_gd->GetSpriteData()->GetSpriteFrame(m_sprite->GetSprite(), m_frame);
		OpenFrame(m_sprite->GetSprite(), m_frame, m_anim);
		m_framectrl->SetSelected(m_frame + 1);
		m_animctrl->SetSelected(m_anim + 1);

		if(framesCount == deleteIndex )
		{
			m_animframectrl->SetSelected(framesCount);
		}
		
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
		const std::string old_frame = m_gd->GetSpriteData()->GetSpriteAnimationFrames(anim)[evt.GetInt() - 1];
		std::string new_frame = ShowFrameDialog(Landstalker::StrPrintf("Change frame \"%s\" to:", old_frame.c_str()), "Change frame");
		if (!new_frame.empty())
		{
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
	FireEvent(EVT_STATUSBAR_UPDATE);
	evt.Skip();
}

void SpriteEditorFrame::OnTileChanged(wxCommandEvent& evt)
{
	int tile = std::stoi(evt.GetString().ToStdString());
	m_spriteeditor->RedrawTiles(tile);
	m_preview->Refresh(true);
	// Fires at most once per stroke/operation; only the (guarded) undo state refresh runs
	// here, not the full UpdateUI.
	UpdateUndoRedoUI();
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
	m_spriteeditor->SetPrimaryColour(m_paledit->GetPrimaryColour());
	m_spriteeditor->SetSecondaryColour(m_paledit->GetSecondaryColour());
	FireEvent(EVT_STATUSBAR_UPDATE);
	evt.Skip();
}

void SpriteEditorFrame::OnColourPicked(wxCommandEvent& evt)
{
	// Keeps the palette pane's primary/secondary markers in step with the eyedropper.
	const int value = evt.GetInt();
	if (value & 0x100)
	{
		m_paledit->SetSecondaryColour(value & 0xFF);
	}
	else
	{
		m_paledit->SetPrimaryColour(value & 0xFF);
	}
	FireEvent(EVT_STATUSBAR_UPDATE);
	evt.Skip();
}

void SpriteEditorFrame::OnPaletteColourHover(wxCommandEvent& evt)
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

void SpriteEditorFrame::OnExportPropertiesYaml()
{
	const wxString default_file = Landstalker::StrPrintf("Sprite%03dProperties.yaml", m_sprite->GetSprite());
	wxFileDialog fd(this, _("Export Sprite Properties As YAML"), "", default_file, "YAML Files (*.yml, *.yaml)|*.yml;*.yaml|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		ExportPropertiesYaml(fd.GetPath().ToStdString());
	}
}

void SpriteEditorFrame::OnExportAllSpritesheets()
{
	wxDirDialog dd(this, "Select Sprite Sheet Output Directory");
	if (dd.ShowModal() != wxID_CANCEL)
	{
		ExportAllSpritesheets(dd.GetPath().ToStdString());
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
	Update();
}

void SpriteEditorFrame::OnImportSpriteMetadata()
{
	if (!m_gd || !m_sprite)
	{
		return;
	}
	wxFileDialog fd(this, _("Import Sprite Metadata From YAML"), "", "",
		"YAML Files (*.yml, *.yaml)|*.yml;*.yaml|All Files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return;
	}
	std::ifstream in(fd.GetPath().ToStdString(), std::ios::binary);
	std::stringstream ss;
	ss << in.rdbuf();

	const uint8_t sid = m_sprite->GetSprite();
	if (!m_gd->GetSpriteData()->ApplySpriteMetadataYaml(sid, ss.str()))
	{
		wxMessageBox("Could not read sprite metadata from the selected file.",
			"Import Sprite Metadata", wxOK | wxICON_ERROR, this);
		return;
	}
	// A stale metadata file could specify a reservation below the current frame; keep it valid.
	EnsureMaxTileCount();
	// The animation flags feed the role labels and the hitbox drives the overlay, so refresh both
	// the lists and the canvas along with the property panel.
	m_animctrl->SetSprite(sid);
	m_animframectrl->SetAnimation(sid, m_anim);
	Redraw();
	UpdateUI();
	FireEvent(EVT_PROPERTIES_UPDATE);
	FireEvent(EVT_STATUSBAR_UPDATE);
}

namespace
{
	// A valid sprite label must start with a letter and hold only [A-Za-z0-9_]. Turns a file stem
	// into something close so the name prompt starts from a usable suggestion.
	std::string SanitiseSpriteName(const std::string& raw)
	{
		std::string out;
		for (char c : raw)
		{
			if (std::isalnum(static_cast<unsigned char>(c)) || c == '_')
			{
				out.push_back(c);
			}
		}
		while (!out.empty() && !std::isalpha(static_cast<unsigned char>(out.front())))
		{
			out.erase(out.begin());
		}
		if (out.size() > 30)
		{
			out.resize(30);
		}
		return out.empty() ? std::string("Sprite") : out;
	}

	wxString DescribeSpriteSheetImportError(Landstalker::SpriteData::SpriteSheetImportResult result)
	{
		using R = Landstalker::SpriteData::SpriteSheetImportResult;
		switch (result)
		{
		case R::BadName:         return "The sprite name is invalid or already in use.";
		case R::YamlMissing:     return "The metadata YAML file could not be found.";
		case R::YamlInvalid:     return "The YAML could not be parsed or has no 'spritesheet' block.";
		case R::PngMissing:      return "The PNG image named by the YAML could not be found.";
		case R::PngUnreadable:   return "The PNG image could not be decoded.";
		case R::PngNotIndexed:   return "The PNG must be a colour-indexed (palette) image.";
		case R::PngWrongSize:    return "The PNG size does not match the grid in the YAML.";
		case R::PngBadColour:    return "The PNG uses colour indices above 15; sprites are 4bpp.";
		case R::FrameTooComplex: return "A frame is too large to represent with 8 hardware sprites.";
		case R::NoFrames:        return "The sheet describes no frames.";
		case R::IdSpaceFull:     return "There is no free sprite slot to import into.";
		default:                 return "The sprite sheet could not be imported.";
		}
	}

	// Turns the dialog's low/high palette choices into the palette names the preview should use,
	// creating and colouring a new low/high palette for any half the user asked to add. The names
	// come back in draw order (low then high) for building a combined preview palette.
	std::vector<std::string> ResolveImportPalettes(Landstalker::SpriteData* sd,
		const SpriteImportDialog::PaletteResult& low, const SpriteImportDialog::PaletteResult& high)
	{
		std::vector<std::string> names;
		const auto resolve = [&](const SpriteImportDialog::PaletteResult& r, bool high_half) {
			if (!r.used)
			{
				return;
			}
			uint8_t idx = 0;
			if (r.create_new)
			{
				const auto add = high_half ? sd->AddHiPalette() : sd->AddLoPalette();
				if (!add)
				{
					return;
				}
				idx = *add;
				const auto pal = (high_half ? sd->GetHiPalette(idx) : sd->GetLoPalette(idx))->GetData();
				for (int n = 0; n < static_cast<int>(r.colours.size()); ++n)
				{
					pal->SetNthUnlockedGenesisColour(static_cast<uint8_t>(n), r.colours[n]);
				}
				if (!r.new_name.empty())
				{
					Landstalker::Labels::Update(high_half ? Landstalker::Labels::C_HIGH_PALETTES
						: Landstalker::Labels::C_LOW_PALETTES, idx, r.new_name);
				}
			}
			else
			{
				idx = static_cast<uint8_t>(r.existing_index);
			}
			const auto entry = high_half ? sd->GetHiPalette(idx) : sd->GetLoPalette(idx);
			if (entry)
			{
				names.push_back(entry->GetName());
			}
		};
		resolve(low, false);
		resolve(high, true);
		return names;
	}

	// Shared front-half of both sprite-sheet imports: decode the PNG, reject anything that is not a
	// readable indexed image (reporting why), then seed `info` with the plain defaults and, if a
	// sibling .yaml/.yml sits beside the PNG, its geometry, animations and metadata. Returns false
	// (having shown a message) when the image cannot be used.
	bool LoadSpriteSheetImageAndInfo(wxWindow* parent, const std::filesystem::path& png_path,
		Landstalker::ImageBuffer::IndexedImage& image, Landstalker::SpriteData::SpriteSheetInfo& info)
	{
		image = Landstalker::ImageBuffer::ReadIndexedPNG(png_path.string());
		if (!image.ok)
		{
			wxMessageBox("The PNG image could not be decoded.", "Import Sprite Sheet",
				wxOK | wxICON_ERROR, parent);
			return false;
		}
		if (!image.indexed)
		{
			wxMessageBox("The PNG must be a colour-indexed (palette) image.", "Import Sprite Sheet",
				wxOK | wxICON_ERROR, parent);
			return false;
		}
		// Plain defaults; a sibling YAML overrides them (count -1 => every whole cell).
		info.cell_width = 32;
		info.cell_height = 32;
		info.frame_count = -1;
		info.origin_x = 16;
		info.origin_y = 16;
		for (const char* ext : { ".yaml", ".yml" })
		{
			if (Landstalker::SpriteData::ReadSpriteSheetInfo(
				std::filesystem::path(png_path).replace_extension(ext), info))
			{
				break;
			}
		}
		return true;
	}
}

// Rebuilds the Sprites branch of the nav tree from the game data and jumps to sprite `id`, the
// way to surface an import that added a sprite or replaced one's frames and animations wholesale.
void SpriteEditorFrame::RebuildTreeAndOpenSprite(int id)
{
	const std::wstring path = L"Sprites/" +
		Landstalker::SpriteData::GetSpriteDisplayName(static_cast<uint8_t>(id));
	wxCommandEvent evt(EVT_REBUILD_NAV_TREE);
	evt.SetInt(id);
	evt.SetString(wxString(path));
	evt.SetClientData(this);
	wxPostEvent(this, evt);
}

void SpriteEditorFrame::OnImportSpriteSheetNew()
{
	if (!m_gd)
	{
		return;
	}
	wxFileDialog fd(this, _("Import Sprite Sheet PNG"), "", "",
		"PNG Image (*.png)|*.png|All Files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return;
	}
	const std::filesystem::path png_path = fd.GetPath().ToStdString();

	Landstalker::ImageBuffer::IndexedImage image;
	Landstalker::SpriteData::SpriteSheetInfo info;
	if (!LoadSpriteSheetImageAndInfo(this, png_path, image, info))
	{
		return;
	}

	SpriteImportDialog dlg(this, m_gd, image, SanitiseSpriteName(png_path.stem().string()),
		info.cell_width, info.cell_height, info.frame_count, info.origin_x, info.origin_y,
		info.found, info.animation_names);
	if (dlg.ShowModal() != wxID_OK)
	{
		return;
	}

	const auto sprite_data = m_gd->GetSpriteData();
	Landstalker::SpriteData::SpriteSheetImportResult result;
	const auto added = sprite_data->ImportSpriteSheetPixels(dlg.GetInternalName(), image.pixels,
		static_cast<int>(image.width), static_cast<int>(image.height),
		dlg.GetCellWidth(), dlg.GetCellHeight(), dlg.GetFrameCount(), dlg.GetOrigin(), info, result);
	if (!added)
	{
		wxMessageBox(DescribeSpriteSheetImportError(result), "Import Sprite Sheet", wxOK | wxICON_ERROR, this);
		return;
	}
	Landstalker::Labels::Update(Landstalker::Labels::C_SPRITES, *added, dlg.GetDisplayName());

	// Resolve (or create) the low and high palettes the dialog chose, and remember them so the
	// reopened sprite previews with those colours (it has no entity to derive a palette from).
	m_forced_palette_sprite = *added;
	m_forced_palette_names = ResolveImportPalettes(sprite_data.get(), dlg.GetLowResult(), dlg.GetHighResult());
	RebuildTreeAndOpenSprite(*added);
}

void SpriteEditorFrame::OnImportSpriteSheetCurrent()
{
	if (!m_gd || !m_sprite)
	{
		return;
	}
	const auto sprite_data = m_gd->GetSpriteData();
	const uint8_t sid = m_sprite->GetSprite();
	if (wxMessageBox(wxString::Format(
		"Replace the frames and animations of '%s' with an imported sprite sheet?\n\n"
		"This overwrites the sprite's current graphics and cannot be undone.",
		wxString::FromUTF8(sprite_data->GetSpriteName(sid))),
		"Import Sprite Sheet into Current Sprite", wxYES_NO | wxICON_WARNING, this) != wxYES)
	{
		return;
	}
	wxFileDialog fd(this, _("Import Sprite Sheet PNG"), "", "",
		"PNG Image (*.png)|*.png|All Files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return;
	}
	const std::filesystem::path png_path = fd.GetPath().ToStdString();

	Landstalker::ImageBuffer::IndexedImage image;
	Landstalker::SpriteData::SpriteSheetInfo info;
	if (!LoadSpriteSheetImageAndInfo(this, png_path, image, info))
	{
		return;
	}

	// The dialog runs in existing-sprite mode: it shows the sprite's names read-only rather than
	// asking for new ones, since the import keeps the sprite's identity.
	SpriteImportDialog dlg(this, m_gd, image, sprite_data->GetSpriteName(sid),
		info.cell_width, info.cell_height, info.frame_count, info.origin_x, info.origin_y,
		info.found, info.animation_names, false,
		Landstalker::SpriteData::GetSpriteDisplayName(sid));
	if (dlg.ShowModal() != wxID_OK)
	{
		return;
	}

	Landstalker::SpriteData::SpriteSheetImportResult result;
	if (!sprite_data->ImportSpriteSheetIntoExistingPixels(sid, image.pixels,
		static_cast<int>(image.width), static_cast<int>(image.height),
		dlg.GetCellWidth(), dlg.GetCellHeight(), dlg.GetFrameCount(), dlg.GetOrigin(), info, result))
	{
		wxMessageBox(DescribeSpriteSheetImportError(result), "Import Sprite Sheet", wxOK | wxICON_ERROR, this);
		return;
	}
	// Apply the low/high palettes the dialog chose to the preview, and reopen the sprite from
	// scratch so its editor, lists and preview all refresh.
	m_forced_palette_sprite = sid;
	m_forced_palette_names = ResolveImportPalettes(sprite_data.get(), dlg.GetLowResult(), dlg.GetHighResult());
	RebuildTreeAndOpenSprite(sid);
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
	}
	if (m_spriteeditor->IsPixelHoverValid())
	{
		const auto pixel = m_spriteeditor->GetHoveredPixel();
		const int idx = m_spriteeditor->GetColourAtPixel(pixel);
		if (idx >= 0)
		{
			colour = idx;
			// Sprite-space coordinates, matching the tile position readout.
			const int tw = static_cast<int>(m_spriteeditor->GetTileset()->GetTileWidth());
			const int th = static_cast<int>(m_spriteeditor->GetTileset()->GetTileHeight());
			if (ss.tellp() > 0)
			{
				ss << ", ";
			}
			ss << "Pixel: (" << (pixel.x - 16 * tw) << ", " << (pixel.y - 16 * th) << "): " << idx;
		}
	}
	status.SetStatusText(ss.str(), 0);
	ss.str(std::string());
	if (colour != -1)
	{
		const auto& name = m_palette->getOwner(colour);
		ss << Landstalker::StrPrintf("Colour at mouse: Index %d - Genesis 0x%04X, RGB #%06X %s", colour,
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
	ss.str(std::string());
	ss << "Pen: " << static_cast<int>(m_spriteeditor->GetPrimaryColour())
	   << " / " << static_cast<int>(m_spriteeditor->GetSecondaryColour());
	if (m_spriteeditor->IsPixelHoverValid())
	{
		const int idx = m_spriteeditor->GetColourAtPixel(m_spriteeditor->GetHoveredPixel());
		if (idx >= 0)
		{
			ss << ", Hover: " << idx;
		}
	}
	status.SetStatusText(ss.str(), 2);
}

void SpriteEditorFrame::FireRenameNavItemEvent(const std::wstring& old_name, const std::wstring& new_name)
{
	wxCommandEvent evt(EVT_RENAME_NAV_ITEM);
	std::wstring lbl(L"Sprites/" + old_name + L"\1Sprites/" + new_name);
	evt.SetString(lbl);
	wxPostEvent(this, evt);
}
