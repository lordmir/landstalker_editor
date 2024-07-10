#include "SpriteEditorFrame.h"
#include "MainFrame.h"

#include <wx/propgrid/advprops.h>
#include <fstream>
#include "Utils.h"

wxBEGIN_EVENT_TABLE(SpriteEditorFrame, wxWindow)
EVT_COMMAND(wxID_ANY, EVT_SPRITE_FRAME_SELECT, SpriteEditorFrame::OnTileSelected)
EVT_COMMAND(wxID_ANY, EVT_PALETTE_COLOUR_SELECT, SpriteEditorFrame::OnPaletteColourSelect)
EVT_COMMAND(wxID_ANY, EVT_PALETTE_COLOUR_HOVER, SpriteEditorFrame::OnPaletteColourHover)
EVT_COMMAND(wxID_ANY, EVT_SPRITE_FRAME_HOVER, SpriteEditorFrame::OnTileHovered)
EVT_COMMAND(wxID_ANY, EVT_TILE_PIXEL_HOVER, SpriteEditorFrame::OnTilePixelHover)
EVT_COMMAND(wxID_ANY, EVT_TILE_CHANGE, SpriteEditorFrame::OnTileChanged)
EVT_COMMAND(wxID_ANY, EVT_SPRITE_FRAME_EDIT_REQUEST, SpriteEditorFrame::OnTileEditRequested)
EVT_COMMAND(wxID_ANY, EVT_FRAME_SELECT, SpriteEditorFrame::OnFrameSelect)
EVT_COMMAND(wxID_ANY, EVT_FRAME_ADD, SpriteEditorFrame::OnFrameAdd)
EVT_COMMAND(wxID_ANY, EVT_FRAME_DELETE, SpriteEditorFrame::OnFrameDelete)
EVT_COMMAND(wxID_ANY, EVT_FRAME_MOVE_UP, SpriteEditorFrame::OnFrameMoveUp)
EVT_COMMAND(wxID_ANY, EVT_FRAME_MOVE_DOWN, SpriteEditorFrame::OnFrameMoveDown)
EVT_COMMAND(wxID_ANY, EVT_SUBSPRITE_SELECT, SpriteEditorFrame::OnSubSpriteSelect)
EVT_COMMAND(wxID_ANY, EVT_SUBSPRITE_ADD, SpriteEditorFrame::OnSubSpriteAdd)
EVT_COMMAND(wxID_ANY, EVT_SUBSPRITE_DELETE, SpriteEditorFrame::OnSubSpriteDelete)
EVT_COMMAND(wxID_ANY, EVT_SUBSPRITE_MOVE_UP, SpriteEditorFrame::OnSubSpriteMoveUp)
EVT_COMMAND(wxID_ANY, EVT_SUBSPRITE_MOVE_DOWN, SpriteEditorFrame::OnSubSpriteMoveDown)
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
wxEND_EVENT_TABLE()

enum MENU_IDS
{
	ID_FILE_EXPORT_FRM = 20000,
	ID_FILE_EXPORT_TILES,
	ID_FILE_EXPORT_VDPMAP,
	ID_FILE_EXPORT_PNG,
	ID_FILE_IMPORT_FRM,
	ID_FILE_IMPORT_TILES,
	ID_FILE_IMPORT_VDPMAP,
	ID_TOGGLE_GRIDLINES = 30000,
	ID_TOGGLE_TILE_NOS,
	ID_TOGGLE_ALPHA,
	ID_ADD_TILE_AFTER_SEL,
	ID_ADD_TILE_BEFORE_SEL,
	ID_EXTEND_TILESET,
	ID_DELETE_TILE,
	ID_SWAP_TILES,
	ID_CUT_TILE,
	ID_COPY_TILE,
	ID_PASTE_TILE,
	ID_EDIT_TILE
};

SpriteEditorFrame::SpriteEditorFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst)
{
	m_mgr.SetManagedWindow(this);

	m_spriteeditor = new SpriteFrameEditorCtrl(this);
	m_spriteanimeditor = new SpriteAnimationEditorCtrl(this);
	m_paledit = new PaletteEditor(this);
	m_tileedit = new TileEditor(this);
	m_framectrl = new FrameControlFrame(this, imglst);
	m_subspritectrl = new SubspriteControlFrame(this, imglst);
	m_animctrl = new AnimationControlFrame(this, imglst);
	m_animframectrl = new AnimationFrameControlFrame(this, imglst);

	// add the panes to the manager
	m_mgr.SetDockSizeConstraint(0.3, 0.3);
	m_mgr.AddPane(m_tileedit, wxAuiPaneInfo().Left().Layer(2).MinSize(100, 100).BestSize(450, 450).FloatingSize(450, 450).Caption("Editor"));
	m_mgr.AddPane(m_paledit, wxAuiPaneInfo().Bottom().Layer(1).MinSize(180, 40).BestSize(700, 100).FloatingSize(700, 100).Caption("Palette"));
	m_mgr.AddPane(m_framectrl, wxAuiPaneInfo().Left().Layer(2).Resizable(false).MinSize(220, 150)
		.BestSize(220, 200).FloatingSize(220, 200).Caption("Frames"));
	m_mgr.AddPane(m_subspritectrl, wxAuiPaneInfo().Left().Layer(2).Resizable(false).MinSize(220, 150)
		.BestSize(220, 200).FloatingSize(220, 200).Caption("Subsprites"));
	m_mgr.AddPane(m_animctrl, wxAuiPaneInfo().Right().Layer(2).Resizable(false).MinSize(220, 150)
		.BestSize(220, 200).FloatingSize(220, 200).Caption("Animations"));
	m_mgr.AddPane(m_animframectrl, wxAuiPaneInfo().Right().Layer(2).Movable(true).Resizable(true).MinSize(220, 150)
		.BestSize(220, 200).FloatingSize(220, 200).Caption("Animation Frames"));

	m_nb = new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_TAB_SPLIT);
	m_nb->Freeze();
	m_nb->AddPage(m_spriteeditor, "Sprite Frames");
	m_nb->AddPage(m_spriteanimeditor, "Sprite Animations");
	m_nb->Thaw();
	m_mgr.AddPane(m_nb, wxAuiPaneInfo().CenterPane());
	// tell the manager to "commit" all the changes just made
	m_mgr.Update();
	UpdateUI();

	FireEvent(EVT_STATUSBAR_UPDATE);
}

SpriteEditorFrame::~SpriteEditorFrame()
{
}

bool SpriteEditorFrame::Open(uint8_t spr, int frame, int anim, int ent)
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
		m_sprite = m_gd->GetSpriteData()->GetSpriteFrame(spr, anim, frame);
	}
	m_palette = m_gd->GetSpriteData()->GetEntityPalette(entity);
	m_spriteeditor->Open(m_sprite->GetData(), m_palette);
	m_paledit->SelectPalette(m_palette);
	m_tileedit->SetActivePalette(m_palette);
	m_tileedit->SetTile(Tile(0));
	m_tileedit->SetTileset(m_spriteeditor->GetTileset());
	m_tileedit->SetTile(m_spriteeditor->GetFirstTile());
	m_spriteeditor->SelectTile(m_spriteeditor->GetFirstTile());
	m_reset_props = true;
	Update();
	m_framectrl->SetSelected(m_frame + 1);
	m_animctrl->SetSelected(m_anim + 1);
	m_animframectrl->SetSelected(1);
	m_subspritectrl->SetSelected(0);
	return true;
}

void SpriteEditorFrame::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
	m_tileedit->SetGameData(gd);
	m_paledit->SetGameData(gd);
	m_animctrl->SetGameData(gd);
	m_spriteanimeditor->SetGameData(gd);
	m_framectrl->SetGameData(gd);
	m_animframectrl->SetGameData(gd);
}

void SpriteEditorFrame::ClearGameData()
{
	m_gd = nullptr;
	m_tileedit->SetGameData(nullptr);
	m_paledit->SetGameData(nullptr);
	m_animctrl->ClearGameData();
	m_spriteanimeditor->ClearGameData();
	m_framectrl->ClearGameData();
	m_animframectrl->ClearGameData();
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
			m_spriteanimeditor->SetActivePalette(m_palette);
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
		m_spriteanimeditor->SetActivePalette(m_palette);
		m_paledit->SelectPalette(m_palette);
		m_tileedit->SetActivePalette(m_palette);
	}
}

void SpriteEditorFrame::Redraw() const
{
	m_spriteeditor->RedrawTiles();
	m_paledit->Refresh(true);
	m_tileedit->Redraw();
}

void SpriteEditorFrame::RedrawTiles(int index) const
{
	m_spriteeditor->RedrawTiles(index);
	m_tileedit->Redraw();
	m_spriteanimeditor->Refresh(true);
}

void SpriteEditorFrame::Update()
{
	m_subspritectrl->SetSubsprites(m_sprite->GetData()->GetSubSprites());
	m_framectrl->SetSprite(m_sprite->GetSprite());
	m_animctrl->SetSprite(m_sprite->GetSprite());
	m_animframectrl->SetAnimation(m_sprite->GetSprite(), 0);
	Redraw();
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

void SpriteEditorFrame::InitMenu(wxMenuBar& menu, ImageList& /*ilist*/) const
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

	UpdateUI();

	m_mgr.Update();
}

void SpriteEditorFrame::OnMenuClick(wxMenuEvent& evt)
{
	switch (evt.GetId())
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
	default:
		break;
	}
	UpdateUI();
	FireEvent(EVT_STATUSBAR_UPDATE);
	FireEvent(EVT_PROPERTIES_UPDATE);
	evt.Skip();
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
	if (m_gd != nullptr)
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
		}
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
	ctrl->GetGrid()->Thaw();
}

void SpriteEditorFrame::OnFrameSelect(wxCommandEvent& evt)
{
	Open(m_sprite->GetSprite(), evt.GetInt() - 1);
}

void SpriteEditorFrame::OnFrameAdd(wxCommandEvent& evt)
{
	wxMessageBox("Frame Add", evt.GetString());
}

void SpriteEditorFrame::OnFrameDelete(wxCommandEvent& evt)
{
	wxMessageBox("Frame Delete", evt.GetString());
}

void SpriteEditorFrame::OnFrameMoveUp(wxCommandEvent& evt)
{
	wxMessageBox("Frame Move Up", evt.GetString());
}

void SpriteEditorFrame::OnFrameMoveDown(wxCommandEvent& evt)
{
	wxMessageBox("Frame Move Down", evt.GetString());
}

void SpriteEditorFrame::OnSubSpriteSelect(wxCommandEvent& evt)
{
	m_spriteeditor->SelectSubSprite(evt.GetInt());
}

void SpriteEditorFrame::OnSubSpriteAdd(wxCommandEvent& evt)
{
	wxMessageBox("SubSprite Add", evt.GetString());
}

void SpriteEditorFrame::OnSubSpriteDelete(wxCommandEvent& evt)
{
	wxMessageBox("SubSprite Delete", evt.GetString());
}

void SpriteEditorFrame::OnSubSpriteMoveUp(wxCommandEvent& evt)
{
	wxMessageBox("SubSprite Move Up", evt.GetString());
}

void SpriteEditorFrame::OnSubSpriteMoveDown(wxCommandEvent& evt)
{
	wxMessageBox("SubSprite Move Down", evt.GetString());
}

void SpriteEditorFrame::OnAnimationSelect(wxCommandEvent& evt)
{
	wxMessageBox("Animation Select", evt.GetString());
}

void SpriteEditorFrame::OnAnimationAdd(wxCommandEvent& evt)
{
	wxMessageBox("Animation Add", evt.GetString());
}

void SpriteEditorFrame::OnAnimationDelete(wxCommandEvent& evt)
{
	wxMessageBox("Animation Delete", evt.GetString());
}

void SpriteEditorFrame::OnAnimationMoveUp(wxCommandEvent& evt)
{
	wxMessageBox("Animation Move Up", evt.GetString());
}

void SpriteEditorFrame::OnAnimationMoveDown(wxCommandEvent& evt)
{
	wxMessageBox("Animation Move Down", evt.GetString());
}

void SpriteEditorFrame::OnAnimationFrameSelect(wxCommandEvent& evt)
{
	wxMessageBox("Animation Frame Select", evt.GetString());
}

void SpriteEditorFrame::OnAnimationFrameAdd(wxCommandEvent& evt)
{
	wxMessageBox("Animation Frame Add", evt.GetString());
}

void SpriteEditorFrame::OnAnimationFrameDelete(wxCommandEvent& evt)
{
	wxMessageBox("Animation Frame Delete", evt.GetString());
}

void SpriteEditorFrame::OnAnimationFrameMoveUp(wxCommandEvent& evt)
{
	wxMessageBox("Animation Frame Move Up", evt.GetString());
}

void SpriteEditorFrame::OnAnimationFrameMoveDown(wxCommandEvent& evt)
{
	wxMessageBox("Animation Frame Move Down", evt.GetString());
}

void SpriteEditorFrame::OnZoomChange(wxCommandEvent& evt)
{
	FireEvent(EVT_STATUSBAR_UPDATE);
	evt.Skip();
}

void SpriteEditorFrame::OnTileHovered(wxCommandEvent& evt)
{
	FireEvent(EVT_STATUSBAR_UPDATE);
	evt.Skip();
}

void SpriteEditorFrame::OnTileSelected(wxCommandEvent& evt)
{
	auto tile = std::stoi(evt.GetString().ToStdString());
	if (tile != -1)
	{
		m_tileedit->SetTile(tile);
	}
	FireEvent(EVT_STATUSBAR_UPDATE);
	evt.Skip();
}

void SpriteEditorFrame::OnTileChanged(wxCommandEvent& evt)
{
	auto tile = std::stoi(evt.GetString().ToStdString());
	m_spriteeditor->RedrawTiles(tile);
	evt.Skip();
}

void SpriteEditorFrame::OnTileEditRequested(wxCommandEvent& evt)
{
	evt.Skip();
}

void SpriteEditorFrame::OnButtonClicked(wxCommandEvent& evt)
{
	switch (evt.GetId())
	{
	case ID_TOGGLE_GRIDLINES:
		m_spriteeditor->SetBordersEnabled(!m_spriteeditor->GetBordersEnabled());
		break;
	case ID_TOGGLE_TILE_NOS:
		m_spriteeditor->SetTileNumbersEnabled(!m_spriteeditor->GetTileNumbersEnabled());
		break;
	case ID_TOGGLE_ALPHA:
		m_spriteeditor->SetAlphaEnabled(!m_spriteeditor->GetAlphaEnabled());
		break;
	case ID_ADD_TILE_AFTER_SEL:
		if (m_spriteeditor->IsSelectionValid())
		{
			m_spriteeditor->InsertTileAfter(m_spriteeditor->GetSelectedTile().GetIndex());
		}
		break;
	case ID_ADD_TILE_BEFORE_SEL:
		if (m_spriteeditor->IsSelectionValid())
		{
			m_spriteeditor->InsertTileBefore(m_spriteeditor->GetSelectedTile().GetIndex());
		}
		break;
	case ID_EXTEND_TILESET:
		m_spriteeditor->InsertTileAfter(m_spriteeditor->GetTilemapSize() - 1);
		break;
	case ID_DELETE_TILE:
		if (m_spriteeditor->IsSelectionValid())
		{
			m_spriteeditor->DeleteTileAt(m_spriteeditor->GetSelectedTile().GetIndex());
		}
		break;
	case ID_SWAP_TILES:
		if (m_spriteeditor->IsSelectionValid())
		{
			m_spriteeditor->SwapTile(m_spriteeditor->GetSelectedTile());
		}
		break;
	case ID_CUT_TILE:
		if (m_spriteeditor->IsSelectionValid())
		{
			m_spriteeditor->CutTile(m_spriteeditor->GetSelectedTile().GetIndex());
		}
		break;
	case ID_COPY_TILE:
		if (m_spriteeditor->IsSelectionValid())
		{
			m_spriteeditor->CopyTile(m_spriteeditor->GetSelectedTile().GetIndex());
		}
		break;
	case ID_PASTE_TILE:
		if (m_spriteeditor->IsSelectionValid() && !m_spriteeditor->IsClipboardEmpty())
		{
			m_spriteeditor->PasteTile(m_spriteeditor->GetSelectedTile().GetIndex());
		}
		break;
	case ID_EDIT_TILE:
		if (m_spriteeditor->IsSelectionValid())
		{
			m_spriteeditor->EditTile(m_spriteeditor->GetSelectedTile());
		}
		break;
	default:
		break;
	}
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
}

void SpriteEditorFrame::UpdateStatusBar(wxStatusBar& status, wxCommandEvent& /*evt*/) const
{
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
