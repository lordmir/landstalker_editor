#include <sprites/EntityViewerFrame.h>
#include <wx/propgrid/advprops.h>
#include <wx/listbox.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/textdlg.h>
#include <wx/choice.h>
#include <wx/splitter.h>
#include <wx/scrolwin.h>
#include <wx/hyperlink.h>
#include <wx/stattext.h>
#include <wx/wrapsizer.h>
#include <wx/dirdlg.h>
#include <filesystem>
#include <landstalker/misc/Labels.h>

enum MENU_IDS
{
	ID_FILE_EXPORT_ENTITY_PROPERTIES_YAML = 20000,
	ID_FILE_EXPORT_ENTITY_SPRITESHEET,
	ID_FILE_EXPORT_ALL_ENTITY_SPRITESHEETS,
	ID_FILE_IMPORT_ENTITY_METADATA
};

namespace
{

// The sprites a new entity can be based on, kept alongside their ids so a choice index maps back.
struct SpriteChoices
{
	wxArrayString names;
	std::vector<uint8_t> ids;
};

SpriteChoices GatherSprites(const Landstalker::SpriteData& sprites)
{
	SpriteChoices result;
	for (int i = 0; i < 256; ++i)
	{
		if (sprites.IsSprite(static_cast<uint8_t>(i)))
		{
			result.names.Add(wxString::Format("%03d: ", i) +
				wxString(Landstalker::SpriteData::GetSpriteDisplayName(static_cast<uint8_t>(i))));
			result.ids.push_back(static_cast<uint8_t>(i));
		}
	}
	return result;
}

// Prompts for the sprite and palette(s) a new entity should use.
class AddEntityDialog : public wxDialog
{
public:
	AddEntityDialog(wxWindow* parent, const Landstalker::SpriteData& sprites)
		: wxDialog(parent, wxID_ANY, "New Entity"),
		  m_sprites(GatherSprites(sprites))
	{
		auto* outer = new wxBoxSizer(wxVERTICAL);
		outer->Add(new wxStaticText(this, wxID_ANY,
			"Choose the sprite graphics and palettes the new entity should use.\n"
			"It takes the lowest free (non-item) id; everything else can be edited afterwards."),
			0, wxLEFT | wxRIGHT | wxTOP, 10);

		auto* fields = new wxFlexGridSizer(2, 6, 6);
		fields->AddGrowableCol(1, 1);

		fields->Add(new wxStaticText(this, wxID_ANY, "Name"), 0, wxALIGN_CENTER_VERTICAL);
		m_name = new wxTextCtrl(this, wxID_ANY);
		m_name->SetHint("(optional display name)");
		fields->Add(m_name, 1, wxEXPAND);

		fields->Add(new wxStaticText(this, wxID_ANY, "Sprite"), 0, wxALIGN_CENTER_VERTICAL);
		m_sprite = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_sprites.names);
		if (!m_sprites.names.IsEmpty())
		{
			m_sprite->SetSelection(0);
		}
		fields->Add(m_sprite, 1, wxEXPAND);

		fields->Add(new wxStaticText(this, wxID_ANY, "Low palette"), 0, wxALIGN_CENTER_VERTICAL);
		m_lo = new wxChoice(this, wxID_ANY);
		m_lo->Append("(None)");
		for (int i = 0; i < sprites.GetLoPaletteCount(); ++i)
		{
			m_lo->Append(wxString(Landstalker::SpriteData::GetSpriteLowPaletteDisplayName(i)));
		}
		m_lo->SetSelection(sprites.GetLoPaletteCount() > 0 ? 1 : 0);
		fields->Add(m_lo, 1, wxEXPAND);

		fields->Add(new wxStaticText(this, wxID_ANY, "High palette"), 0, wxALIGN_CENTER_VERTICAL);
		m_hi = new wxChoice(this, wxID_ANY);
		m_hi->Append("(None)");
		for (int i = 0; i < sprites.GetHiPaletteCount(); ++i)
		{
			m_hi->Append(wxString(Landstalker::SpriteData::GetSpriteHighPaletteDisplayName(i)));
		}
		m_hi->SetSelection(0);
		fields->Add(m_hi, 1, wxEXPAND);

		outer->Add(fields, 1, wxALL | wxEXPAND, 10);
		outer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL), 0, wxALL | wxEXPAND, 10);
		SetSizerAndFit(outer);
		SetMinSize(wxSize(420, -1));
		CentreOnParent();

		m_sprite->Enable(!m_sprites.names.IsEmpty());
	}

	bool HasSprites() const { return !m_sprites.ids.empty(); }
	int GetSpriteId() const
	{
		const int sel = m_sprite->GetSelection();
		return sel == wxNOT_FOUND ? -1 : m_sprites.ids[sel];
	}
	int GetLowPalette() const { return m_lo->GetSelection() - 1; }
	int GetHighPalette() const { return m_hi->GetSelection() - 1; }
	// The display name to give the entity, or empty to leave it at the default EntityNN label.
	// (Not GetName - that is a non-virtual wxWindow method returning wxString.)
	std::wstring GetEntityName() const { return m_name->GetValue().Trim().Trim(false).ToStdWstring(); }

private:
	SpriteChoices m_sprites;
	wxTextCtrl* m_name;
	wxChoice* m_sprite;
	wxChoice* m_lo;
	wxChoice* m_hi;
};

// Names the rooms blocking a delete, for the refusal message.
wxString DescribeRooms(const Landstalker::RoomData& rooms, const std::vector<uint16_t>& list)
{
	wxString described;
	std::size_t named = 0;
	for (; named < list.size() && named < 3; ++named)
	{
		if (!described.IsEmpty()) described += ", ";
		described += wxString(rooms.GetRoomDisplayName(list[named]));
	}
	if (list.size() > named)
	{
		described += wxString::Format(" and %d more", static_cast<int>(list.size() - named));
	}
	return described;
}

}

EntityViewerFrame::EntityViewerFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst)
{
	m_mgr.SetManagedWindow(this);

	// The centre area splits top/bottom: the animated preview above, a clickable stats panel
	// below. The user can drag the sash to trade space between them.
	m_split = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxSP_LIVE_UPDATE | wxSP_3DSASH);
	m_entity_ctrl = new EntityViewerCtrl(m_split);
	m_stats = new wxScrolledWindow(m_split, wxID_ANY);
	m_stats->SetScrollRate(0, 8);
	m_split->SplitHorizontally(m_entity_ctrl, m_stats);
	m_split->SetMinimumPaneSize(60);
	m_split->SetSashGravity(0.3);
	// The pane has no height at construction, so set the 30% preview / 70% stats sash the first
	// time it is laid out with a real size; the gravity keeps that ratio on later resizes.
	m_split->Bind(wxEVT_SIZE, [this](wxSizeEvent& e)
	{
		if (!m_sash_set && m_split)
		{
			const int h = m_split->GetClientSize().GetHeight();
			if (h > 120)
			{
				m_split->SetSashPosition(h * 30 / 100);
				m_sash_set = true;
			}
		}
		e.Skip();
	});

	// Left pane: the entity list over a grid of management buttons.
	wxPanel* left = new wxPanel(this, wxID_ANY);
	wxBoxSizer* lv = new wxBoxSizer(wxVERTICAL);
	m_entity_list = new wxListBox(left, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE);
	m_entity_list->SetToolTip("Entities in id order. Items (id 0xC0+) are shown but locked.");
	lv->Add(m_entity_list, 1, wxEXPAND | wxALL, 3);

	wxGridSizer* buttons = new wxGridSizer(0, 2, 3, 3);
	m_add = new wxButton(left, wxID_ANY, "Add...");
	m_remove = new wxButton(left, wxID_ANY, "Remove");
	m_move_up = new wxButton(left, wxID_ANY, "Move Up");
	m_move_down = new wxButton(left, wxID_ANY, "Move Down");
	m_rename = new wxButton(left, wxID_ANY, "Rename...");
	for (auto* b : { m_add, m_remove, m_move_up, m_move_down, m_rename })
	{
		buttons->Add(b, 0, wxEXPAND);
	}
	lv->Add(buttons, 0, wxEXPAND | wxALL, 3);
	left->SetSizer(lv);

	m_entity_list->Bind(wxEVT_LISTBOX, [this](wxCommandEvent&) { OnEntitySelected(); });
	m_add->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnAddEntity(); });
	m_remove->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnRemoveEntity(); });
	m_move_up->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnMoveEntity(-1); });
	m_move_down->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnMoveEntity(1); });
	m_rename->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnRenameEntity(); });

	// add the panes to the manager
	m_mgr.SetDockSizeConstraint(0.5, 0.5);
	m_mgr.AddPane(left, wxAuiPaneInfo().Left().Caption("Entities").MinSize(wxSize(200, -1))
		.BestSize(wxSize(240, -1)).CloseButton(false).Floatable(false).Resizable());
	m_mgr.AddPane(m_split, wxAuiPaneInfo().CenterPane());

	// tell the manager to "commit" all the changes just made
	m_mgr.Update();
	RefreshEntityButtons();
}

EntityViewerFrame::~EntityViewerFrame()
{
}

bool EntityViewerFrame::Open(int entity)
{
	if (!m_gd)
	{
		return false;
	}
	if (m_entity_ids.empty())
	{
		PopulateEntityList();
	}
	// A negative id (the "Entities" node passes -1) keeps the current entity, or lands on the
	// first one when nothing is open yet.
	if (entity < 0 || entity >= 256 || !m_gd->GetSpriteData()->IsEntity(static_cast<uint8_t>(entity)))
	{
		if (m_entity_id >= 0 && m_gd->GetSpriteData()->IsEntity(static_cast<uint8_t>(m_entity_id)))
		{
			entity = m_entity_id;
		}
		else
		{
			entity = m_entity_ids.empty() ? -1 : m_entity_ids.front();
		}
	}
	if (entity < 0)
	{
		return false;
	}
	m_entity_id = entity;
	SelectEntityInList(entity);
	RefreshEntityButtons();
	Update();
	// Record the entity for the browser back/forward history (MainFrame ignores this while it
	// is itself replaying a history entry).
	FireEvent(EVT_RECORD_NAV_LOCATION, "Entities", entity);
	return true;
}

void EntityViewerFrame::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	m_gd = gd;
	m_entity_ctrl->SetGameData(gd);
	PopulateEntityList();
}

void EntityViewerFrame::ClearGameData()
{
	m_entity_id = -1;
	m_entity_ctrl->ClearGameData();
	m_gd.reset();
	if (m_entity_list)
	{
		m_entity_list->Clear();
	}
	m_entity_ids.clear();
	RefreshEntityButtons();
	PopulateStats();
	m_reset_props = true;
	FireEvent(EVT_PROPERTIES_UPDATE);
}

void EntityViewerFrame::Update()
{
	if (m_gd && m_entity_id >= 0 && m_entity_id < 256)
	{
		auto palette_idxs = m_gd->GetSpriteData()->GetEntityPaletteIdxs(m_entity_id);
		m_palette = m_gd->GetSpriteData()->GetSpritePalette(palette_idxs.first, palette_idxs.second);

		m_entity_ctrl->Open(m_entity_id, m_palette);
		if (m_gd->GetSpriteData()->IsEntityItem(m_entity_id))
		{
			m_entity_ctrl->Pause();
		}
		else
		{
			m_entity_ctrl->Play();
		}
		m_reset_props = true;
		PopulateStats();
		FireEvent(EVT_PROPERTIES_UPDATE);
	}
}

void EntityViewerFrame::InitProperties(wxPropertyGridManager& props) const
{
	if (m_gd && m_entity_id != -1 && ArePropsInitialised() == false)
	{
		RefreshLists();
		props.GetGrid()->Clear();
		auto sd = m_gd->GetSpriteData();
		int sprite_index = sd->GetSpriteFromEntity(m_entity_id);

		props.Append(new wxPropertyCategory("Main", "Main"));
		props.Append(new wxStringProperty("Name", "Name", sd->GetEntityDisplayName(m_entity_id)));
		props.Append(new wxIntProperty("ID", "ID", m_entity_id))->Enable(false);
		props.Append(new wxEnumProperty("Sprite", "Sprite", m_sprites));
		auto sprite_id = new wxIntProperty("Sprite ID", "Sprite ID", sprite_index);
		sprite_id->SetAttribute(wxPG_ATTR_MIN, 0);
		sprite_id->SetAttribute(wxPG_ATTR_MAX, 255);
		sprite_id->SetAttribute(wxPG_ATTR_SPINCTRL_STEP, 1);
		sprite_id->SetEditor(wxPGEditor_SpinCtrl);
		props.Append(sprite_id);
		props.Append(new wxEnumProperty("Low Palette", "Low Palette", m_lo_palettes));
		props.Append(new wxEnumProperty("High Palette", "High Palette", m_hi_palettes));
		props.Append(new wxEnumProperty("Talk Sound FX", "Talk Sound FX", m_sounds));
		props.Append(new wxPropertyCategory("Item", "Item"));
		auto is_item = new wxBoolProperty("Is Item", "Is Item", sd->IsEntityItem(m_entity_id));
		is_item->SetAttribute(wxPG_BOOL_USE_CHECKBOX, true);
		props.Append(is_item)->Enable(false);
		props.Append(new wxEnumProperty("Use Text", "Use Text", m_verbs))->Enable(false);
		auto max_qty = new wxIntProperty("Maximum Quantity", "Maximum Quantity", 0);
		max_qty->SetAttribute(wxPG_ATTR_MIN, 0);
		max_qty->SetAttribute(wxPG_ATTR_MAX, 15);
		max_qty->SetAttribute(wxPG_ATTR_SPINCTRL_STEP, 1);
		max_qty->SetEditor(wxPGEditor_SpinCtrl);
		props.Append(max_qty)->Enable(false);
		auto equip_idx = new wxIntProperty("Equipment Index", "Equipment Index", 0);
		equip_idx->SetAttribute(wxPG_ATTR_MIN, 0);
		equip_idx->SetAttribute(wxPG_ATTR_MAX, 255);
		equip_idx->SetAttribute(wxPG_ATTR_SPINCTRL_STEP, 1);
		equip_idx->SetEditor(wxPGEditor_SpinCtrl);
		props.Append(equip_idx)->Enable(false);
		auto price = new wxIntProperty("Normal Buy Price", "Normal Buy Price", 0);
		price->SetAttribute(wxPG_ATTR_MIN, 0);
		price->SetAttribute(wxPG_ATTR_MAX, 65535);
		price->SetAttribute(wxPG_ATTR_SPINCTRL_STEP, 1);
		price->SetEditor(wxPGEditor_SpinCtrl);
		props.Append(price)->Enable(false);
		// Grammatical article form (FR/DE localisation hack) - only editable when the loaded
		// region has an item article table.
		props.Append(new wxEnumProperty("Article", "Article", m_articles))->Enable(false);
		props.Append(new wxPropertyCategory("Enemy", "Enemy"));
		auto is_enemy = new wxBoolProperty("Is Enemy", "Is Enemy", sd->IsEntityEnemy(m_entity_id));
		is_enemy->SetAttribute(wxPG_BOOL_USE_CHECKBOX, true);
		props.Append(is_enemy);
		auto health = new wxIntProperty("Health", "Health", 0);
		health->SetAttribute(wxPG_ATTR_MIN, 0);
		// 255 is reserved as the game's "indestructible" sentinel, set via the Invulnerable box.
		health->SetAttribute(wxPG_ATTR_MAX, 254);
		health->SetAttribute(wxPG_ATTR_SPINCTRL_STEP, 1);
		health->SetEditor(wxPGEditor_SpinCtrl);
		props.Append(health)->Enable(false);
		auto invulnerable = new wxBoolProperty("Invulnerable", "Invulnerable", false);
		invulnerable->SetAttribute(wxPG_BOOL_USE_CHECKBOX, true);
		invulnerable->SetHelpString("Sets health to 255, the value the game treats as indestructible.");
		props.Append(invulnerable)->Enable(false);
		auto def = new wxIntProperty("Defence", "Defence", 0);
		def->SetAttribute(wxPG_ATTR_MIN, 0);
		def->SetAttribute(wxPG_ATTR_MAX, 255);
		def->SetAttribute(wxPG_ATTR_SPINCTRL_STEP, 1);
		def->SetEditor(wxPGEditor_SpinCtrl);
		props.Append(def)->Enable(false);
		auto atk = new wxIntProperty("Attack", "Attack", 0);
		atk->SetAttribute(wxPG_ATTR_MIN, 0);
		atk->SetAttribute(wxPG_ATTR_MAX, 127);
		atk->SetAttribute(wxPG_ATTR_SPINCTRL_STEP, 1);
		atk->SetEditor(wxPGEditor_SpinCtrl);
		props.Append(atk)->Enable(false);
		auto gold = new wxIntProperty("Gold Drop", "Gold Drop", 0);
		gold->SetAttribute(wxPG_ATTR_MIN, 0);
		gold->SetAttribute(wxPG_ATTR_MAX, 255);
		gold->SetAttribute(wxPG_ATTR_SPINCTRL_STEP, 1);
		gold->SetEditor(wxPGEditor_SpinCtrl);
		props.Append(gold)->Enable(false);
		props.Append(new wxEnumProperty("Item Drop", "Item Drop", m_verbs))->Enable(false);
		props.Append(new wxEnumProperty("Drop Probability", "Drop Probability", m_probabilities))->Enable(false);
		auto solid = new wxBoolProperty("Solid", "Solid", false);
		solid->SetAttribute(wxPG_BOOL_USE_CHECKBOX, true);
		solid->SetHelpString("Marks a hostile as a fixed obstacle (guaranteed drop with zero gold): immune "
			"to the Statue of Gaia and the quake attack, and solid through hurt-invulnerability.");
		props.Append(solid)->Enable(false);
		EditorFrame::InitProperties(props);
		RefreshProperties(props);
	}
}

void EntityViewerFrame::RefreshLists() const
{
	if (m_gd && m_entity_id != -1)
	{
		auto epals = m_gd->GetSpriteData()->GetEntityPaletteIdxs(m_entity_id);
		m_lo_palettes.Clear();
		m_lo_palettes.Add("<None>");
		for (int i = 0; i < m_gd->GetSpriteData()->GetLoPaletteCount(); ++i)
		{
			m_lo_palettes.Add(wxString(m_gd->GetSpriteData()->GetSpriteLowPaletteDisplayName(i)));
		}
		wxFont font = m_lo_palettes.Item(epals.first + 1).GetFont();
		font.SetWeight(wxFontWeight::wxFONTWEIGHT_BOLD);
		m_lo_palettes.Item(epals.first + 1).SetFont(font);

		m_hi_palettes.Clear();
		m_hi_palettes.Add("<None>");
		for (int i = 0; i < m_gd->GetSpriteData()->GetHiPaletteCount(); ++i)
		{
			m_hi_palettes.Add(wxString(m_gd->GetSpriteData()->GetSpriteHighPaletteDisplayName(i)));
		}
		font = m_hi_palettes.Item(epals.second + 1).GetFont();
		font.SetWeight(wxFontWeight::wxFONTWEIGHT_BOLD);
		m_hi_palettes.Item(epals.second + 1).SetFont(font);

		m_sprites.Clear();
		for (int i = 0; i < 255; ++i)
		{
			if (!m_gd->GetSpriteData()->IsSprite(i))
			{
				continue;
			}
			m_sprites.Add(m_gd->GetSpriteData()->GetSpriteDisplayName(i), i);
		}

		m_verbs.Clear();
		for (int i = 12; i < 20; ++i)
		{
			m_verbs.Add(m_gd->GetStringData()->GetString(Landstalker::StringData::Type::MAIN, i), i);
		}

		m_items.Clear();
		for (std::size_t i = 0; i < m_gd->GetStringData()->GetStringCount(Landstalker::StringData::Type::ITEM_NAMES); ++i)
		{
			m_items.Add(m_gd->GetStringData()->GetItemDisplayName(i), i);
		}

		m_sounds.Clear();
		for (int i = 0; i <= 0xFF; ++i)
		{
			if (Landstalker::Labels::Get(Landstalker::Labels::C_SOUNDS, i))
			{
				m_sounds.Add(*Landstalker::Labels::Get(Landstalker::Labels::C_SOUNDS, i), i);
			}
		}

		m_probabilities.Clear();
		m_probabilities.Add("1/64");
		m_probabilities.Add("1/128");
		m_probabilities.Add("1/256");
		m_probabilities.Add("1/512");
		m_probabilities.Add("1/1024");
		m_probabilities.Add("1/2048");
		m_probabilities.Add("Never");
		m_probabilities.Add("Guaranteed");

		m_articles.Clear();
		m_articles.Add("[0] Vowel/Neuter");
		m_articles.Add("[1] Fem");
		m_articles.Add("[2] Masc");
		m_articles.Add("[3] Plural");
		m_articles.Add("[4] None");
		m_articles.Add("[5]");
		m_articles.Add("[6]");
		m_articles.Add("[7]");
	}
}

void EntityViewerFrame::UpdateProperties(wxPropertyGridManager& props) const
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

void EntityViewerFrame::RefreshProperties(wxPropertyGridManager& props) const
{

	if (m_gd != nullptr && m_entity_id != -1)
	{
		RefreshLists();
		props.GetGrid()->Freeze();

		auto sd = m_gd->GetSpriteData();
		int sprite_index = sd->GetSpriteFromEntity(m_entity_id);
		bool is_item = sd->IsEntityItem(m_entity_id);
		auto item_props = sd->GetItemProperties(m_entity_id);
		bool is_enemy = sd->IsEntityEnemy(m_entity_id);
		auto enemy_stats = sd->GetEnemyStats(m_entity_id);
		// The two derived toggles: 255 health is the indestructible sentinel, and zero gold with a
		// guaranteed drop is the fixed-obstacle ("solid") sentinel.
		const bool invulnerable = is_enemy && enemy_stats.health == 255;
		const bool solid = is_enemy && enemy_stats.gold_drop == 0
			&& enemy_stats.drop_probability == Landstalker::SpriteData::EnemyStats::DropProbability::GUARANTEED_DROP;

		props.GetGrid()->SetPropertyValue("Name", wxString(sd->GetEntityDisplayName(m_entity_id)));
		props.GetGrid()->SetPropertyValue("ID", m_entity_id);
		props.GetGrid()->GetProperty("Sprite")->SetChoices(m_sprites);
		props.GetGrid()->GetProperty("Sprite")->SetChoiceSelection(m_sprites.Index(sprite_index));
		props.GetGrid()->SetPropertyValue("Sprite ID", sprite_index);
		props.GetGrid()->GetProperty("Low Palette")->SetChoices(m_lo_palettes);
		props.GetGrid()->GetProperty("High Palette")->SetChoices(m_hi_palettes);
		props.GetGrid()->GetProperty("Talk Sound FX")->SetChoices(m_sounds);
		props.GetGrid()->GetProperty("Low Palette")->SetChoiceSelection(sd->GetEntityPaletteIdxs(m_entity_id).first + 1);
		props.GetGrid()->GetProperty("High Palette")->SetChoiceSelection(sd->GetEntityPaletteIdxs(m_entity_id).second + 1);
		props.GetGrid()->GetProperty("Talk Sound FX")->SetChoiceSelection(m_sounds.Index(m_gd->GetStringData()->GetEntityTalkSound(m_entity_id)));
		props.GetGrid()->SetPropertyValue("Is Item", is_item);
		props.GetGrid()->GetProperty("Use Text")->Enable(is_item);
		props.GetGrid()->GetProperty("Use Text")->SetChoices(is_item ? m_verbs : m_empty_choices);
		if (is_item)
		{
			props.GetGrid()->GetProperty("Use Text")->SetChoiceSelection(m_verbs.Index(item_props.verb));
		}
		props.GetGrid()->GetProperty("Maximum Quantity")->Enable(is_item);
		props.GetGrid()->SetPropertyValue("Maximum Quantity", is_item ? item_props.max_quantity : 0);
		props.GetGrid()->GetProperty("Equipment Index")->Enable(is_item);
		props.GetGrid()->SetPropertyValue("Equipment Index", is_item ? item_props.equipment_index : 0);
		props.GetGrid()->GetProperty("Normal Buy Price")->Enable(is_item);
		props.GetGrid()->SetPropertyValue("Normal Buy Price", is_item ? item_props.price : 0);
		const bool has_articles = is_item && m_gd->GetScriptData() && m_gd->GetScriptData()->HasItemArticles();
		props.GetGrid()->GetProperty("Article")->Enable(has_articles);
		props.GetGrid()->GetProperty("Article")->SetChoices(has_articles ? m_articles : m_empty_choices);
		if (has_articles)
		{
			props.GetGrid()->GetProperty("Article")->SetChoiceSelection(m_gd->GetScriptData()->GetItemArticle(m_entity_id - 0xC0));
		}
		props.GetGrid()->SetPropertyValue("Is Enemy", sd->IsEntityEnemy(m_entity_id));
		// Health is driven by the Invulnerable box while it is set, so grey it out then.
		props.GetGrid()->GetProperty("Health")->Enable(is_enemy && !invulnerable);
		props.GetGrid()->SetPropertyValue("Health", is_enemy ? enemy_stats.health : 0);
		props.GetGrid()->GetProperty("Invulnerable")->Enable(is_enemy);
		props.GetGrid()->SetPropertyValue("Invulnerable", invulnerable);
		props.GetGrid()->GetProperty("Defence")->Enable(is_enemy);
		props.GetGrid()->SetPropertyValue("Defence", is_enemy ? enemy_stats.defence : 0);
		props.GetGrid()->GetProperty("Attack")->Enable(is_enemy);
		props.GetGrid()->SetPropertyValue("Attack", is_enemy ? enemy_stats.attack : 0);
		// Gold Drop and Drop Probability are fixed (0 gold, guaranteed) while Solid is set, so grey them.
		props.GetGrid()->GetProperty("Gold Drop")->Enable(is_enemy && !solid);
		props.GetGrid()->SetPropertyValue("Gold Drop", is_enemy ? enemy_stats.gold_drop : 0);
		props.GetGrid()->GetProperty("Item Drop")->Enable(is_enemy);
		props.GetGrid()->GetProperty("Item Drop")->SetChoices(is_enemy ? m_items : m_empty_choices);
		if (is_enemy)
		{
			props.GetGrid()->GetProperty("Item Drop")->SetChoiceSelection(enemy_stats.item_drop);
		}
		props.GetGrid()->GetProperty("Drop Probability")->Enable(is_enemy && !solid);
		props.GetGrid()->GetProperty("Drop Probability")->SetChoices(is_enemy ? m_probabilities : m_empty_choices);
		if (is_enemy)
		{
			props.GetGrid()->GetProperty("Drop Probability")->SetChoiceSelection(static_cast<uint8_t>(enemy_stats.drop_probability));
		}
		props.GetGrid()->GetProperty("Solid")->Enable(is_enemy);
		props.GetGrid()->SetPropertyValue("Solid", solid);
		props.GetGrid()->Thaw();
	}
}

void EntityViewerFrame::OnPropertyChange(wxPropertyGridEvent& evt)
{
	auto* ctrl = static_cast<wxPropertyGridManager*>(evt.GetEventObject());
	wxPGProperty* property = evt.GetProperty();
	if (property == nullptr || m_gd == nullptr || m_entity_id == -1)
	{
		return;
	}
	ctrl->GetGrid()->Freeze();
	auto sd = m_gd->GetSpriteData();
	int sprite_index = sd->GetSpriteFromEntity(m_entity_id);
	const wxString& name = property->GetName();
	if (name == "Name")
	{
		const std::wstring new_name = property->GetValueAsString().ToStdWstring();
		const std::wstring old_name = sd->GetEntityDisplayName(m_entity_id);
		if (Landstalker::Labels::IsValid(new_name))
		{
			Landstalker::Labels::Update(Landstalker::Labels::C_ENTITIES, m_entity_id, new_name);
			PopulateEntityList();
		}
		else
		{
			property->SetValueFromString(old_name);
		}
	}
	else if (name == "Sprite" || name == "Sprite ID")
	{
		int value = property->GetValuePlain().GetLong();
		if (value != sprite_index)
		{
			sprite_index = std::clamp<uint8_t>(value, 0, 255);
			if (sd->IsSprite(sprite_index))
			{
				sd->SetEntitySprite(m_entity_id, sprite_index);
				Update();
			}
		}
	}
	else if (name == "Low Palette" || name == "High Palette")
	{
		std::vector<std::string> palettes;
		int lo_pal = ctrl->GetGrid()->GetPropertyByName("Low Palette")->GetValue().GetLong();
		int hi_pal = ctrl->GetGrid()->GetPropertyByName("High Palette")->GetValue().GetLong();
		sd->SetEntityPalette(m_entity_id, lo_pal - 1, hi_pal - 1);
		Update();
	}
	else if (name == "Talk Sound FX")
	{
		int value = property->GetValuePlain().GetLong();
		m_gd->GetStringData()->SetEntityTalkSound(m_entity_id, value);
		FireEvent(EVT_PROPERTIES_UPDATE);
	}
	else if (name == "Is Enemy")
	{
		int value = property->GetValuePlain().GetBool();
		if (value)
		{
			m_gd->GetSpriteData()->SetEnemyStats(m_entity_id, Landstalker::SpriteData::EnemyStats());
		}
		else
		{
			m_gd->GetSpriteData()->ClearEnemyStats(m_entity_id);
		}
		// The [enemy] tag in the list follows this.
		PopulateEntityList();
		FireEvent(EVT_PROPERTIES_UPDATE);
	}
	else if (name == "Use Text")
	{
		int value = property->GetValuePlain().GetInteger();
		auto item_props = m_gd->GetSpriteData()->GetItemProperties(m_entity_id);
		if (value != item_props.verb)
		{
			item_props.verb = std::clamp<uint8_t>(value, 12, 19);
			m_gd->GetSpriteData()->SetItemProperties(m_entity_id, item_props);
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
	}
	else if (name == "Maximum Quantity")
	{
		int value = property->GetValuePlain().GetInteger();
		auto item_props = m_gd->GetSpriteData()->GetItemProperties(m_entity_id);
		if (value != item_props.max_quantity)
		{
			item_props.max_quantity = std::clamp<uint8_t>(value, 0, 15);
			m_gd->GetSpriteData()->SetItemProperties(m_entity_id, item_props);
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
	}
	else if (name == "Equipment Index")
	{
		int value = property->GetValuePlain().GetInteger();
		auto item_props = m_gd->GetSpriteData()->GetItemProperties(m_entity_id);
		if (value != item_props.equipment_index)
		{
			item_props.equipment_index = std::clamp<uint8_t>(value, 0, 255);
			m_gd->GetSpriteData()->SetItemProperties(m_entity_id, item_props);
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
	}
	else if (name == "Normal Buy Price")
	{
		int value = property->GetValuePlain().GetInteger();
		auto item_props = m_gd->GetSpriteData()->GetItemProperties(m_entity_id);
		if (value != item_props.price)
		{
			item_props.price = std::clamp<uint16_t>(value, 0, 65535);
			m_gd->GetSpriteData()->SetItemProperties(m_entity_id, item_props);
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
	}
	else if (name == "Article")
	{
		int value = property->GetValuePlain().GetInteger();
		auto scd = m_gd->GetScriptData();
		if (scd && scd->HasItemArticles() && sd->IsEntityItem(m_entity_id)
			&& value != scd->GetItemArticle(m_entity_id - 0xC0))
		{
			scd->SetItemArticle(m_entity_id - 0xC0, std::clamp<uint8_t>(value, 0, 7));
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
	}
	else if (name == "Health")
	{
		int value = property->GetValuePlain().GetInteger();
		auto enemy_stats = m_gd->GetSpriteData()->GetEnemyStats(m_entity_id);
		if (value != enemy_stats.health)
		{
			enemy_stats.health = std::clamp<uint8_t>(value, 0, 255);
			m_gd->GetSpriteData()->SetEnemyStats(m_entity_id, enemy_stats);
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
	}
	else if (name == "Defence")
	{
		int value = property->GetValuePlain().GetInteger();
		auto enemy_stats = m_gd->GetSpriteData()->GetEnemyStats(m_entity_id);
		if (value != enemy_stats.defence)
		{
			enemy_stats.defence = std::clamp<uint8_t>(value, 0, 255);
			m_gd->GetSpriteData()->SetEnemyStats(m_entity_id, enemy_stats);
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
	}
	else if (name == "Attack")
	{
		int value = property->GetValuePlain().GetInteger();
		auto enemy_stats = m_gd->GetSpriteData()->GetEnemyStats(m_entity_id);
		if (value != enemy_stats.attack)
		{
			enemy_stats.attack = std::clamp<uint8_t>(value, 0, 127);
			m_gd->GetSpriteData()->SetEnemyStats(m_entity_id, enemy_stats);
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
	}
	else if (name == "Gold Drop")
	{
		int value = property->GetValuePlain().GetInteger();
		auto enemy_stats = m_gd->GetSpriteData()->GetEnemyStats(m_entity_id);
		if (value != enemy_stats.gold_drop)
		{
			enemy_stats.gold_drop = std::clamp<uint8_t>(value, 0, 255);
			m_gd->GetSpriteData()->SetEnemyStats(m_entity_id, enemy_stats);
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
	}
	else if (name == "Item Drop")
	{
		int value = property->GetValuePlain().GetInteger();
		auto enemy_stats = m_gd->GetSpriteData()->GetEnemyStats(m_entity_id);
		if (value != enemy_stats.item_drop)
		{
			enemy_stats.item_drop = std::clamp<uint8_t>(value, 0, 63);
			m_gd->GetSpriteData()->SetEnemyStats(m_entity_id, enemy_stats);
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
	}
	else if (name == "Drop Probability")
	{
		int value = property->GetValuePlain().GetInteger();
		auto enemy_stats = m_gd->GetSpriteData()->GetEnemyStats(m_entity_id);
		if (value != static_cast<int>(enemy_stats.drop_probability))
		{
			enemy_stats.drop_probability = static_cast<Landstalker::SpriteData::EnemyStats::DropProbability>(std::clamp<uint8_t>(value, 0, 7));
			m_gd->GetSpriteData()->SetEnemyStats(m_entity_id, enemy_stats);
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
	}
	else if (name == "Invulnerable")
	{
		bool value = property->GetValuePlain().GetBool();
		auto enemy_stats = m_gd->GetSpriteData()->GetEnemyStats(m_entity_id);
		// 255 is the game's indestructible sentinel; clearing it drops back to the capped maximum.
		uint8_t new_health = value ? 255 : (enemy_stats.health >= 255 ? 254 : enemy_stats.health);
		if (new_health != enemy_stats.health)
		{
			enemy_stats.health = new_health;
			m_gd->GetSpriteData()->SetEnemyStats(m_entity_id, enemy_stats);
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
	}
	else if (name == "Solid")
	{
		bool value = property->GetValuePlain().GetBool();
		auto enemy_stats = m_gd->GetSpriteData()->GetEnemyStats(m_entity_id);
		using DropProbability = Landstalker::SpriteData::EnemyStats::DropProbability;
		const bool currently_solid = enemy_stats.gold_drop == 0
			&& enemy_stats.drop_probability == DropProbability::GUARANTEED_DROP;
		if (value != currently_solid)
		{
			if (value)
			{
				// The fixed-obstacle sentinel: no gold plus a guaranteed drop.
				enemy_stats.gold_drop = 0;
				enemy_stats.drop_probability = DropProbability::GUARANTEED_DROP;
			}
			else
			{
				// Drop the guaranteed roll so the pair is no longer the sentinel; leave gold as-is.
				enemy_stats.drop_probability = DropProbability::NO_DROP;
			}
			m_gd->GetSpriteData()->SetEnemyStats(m_entity_id, enemy_stats);
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
	}
	// Keep the stats panel in step with whatever the edit changed.
	PopulateStats();
	ctrl->GetGrid()->Thaw();
}

void EntityViewerFrame::InitMenu(wxMenuBar& menu, ImageList& /*ilist*/) const
{
	ClearMenu(menu);
	auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
	AddMenuItem(fileMenu, 0, ID_FILE_EXPORT_ENTITY_PROPERTIES_YAML, "Export Entity Properties as YAML...");
	AddMenuItem(fileMenu, 1, ID_FILE_EXPORT_ENTITY_SPRITESHEET, "Export Entity Sprite Sheet with Metadata...");
	AddMenuItem(fileMenu, 2, ID_FILE_EXPORT_ALL_ENTITY_SPRITESHEETS, "Export All Entity Sprite Sheets with Metadata...");
	AddMenuItem(fileMenu, 3, ID_FILE_IMPORT_ENTITY_METADATA, "Import Entity Metadata from YAML...");

	UpdateUI();

	m_mgr.Update();
}

void EntityViewerFrame::PopulateEntityList()
{
	if (!m_entity_list || !m_gd)
	{
		return;
	}
	const auto sprite_data = m_gd->GetSpriteData();
	m_populating = true;
	m_entity_ids = sprite_data->GetEntityIds();
	m_entity_list->Freeze();
	m_entity_list->Clear();
	for (const auto id : m_entity_ids)
	{
		auto label = wxString::Format("%03d: ", static_cast<int>(id)) +
			wxString(Landstalker::SpriteData::GetEntityDisplayName(id));
		if (sprite_data->IsEntityItem(id))
		{
			label += "  [item]";
		}
		else if (sprite_data->IsEntityEnemy(id))
		{
			label += "  [enemy]";
		}
		m_entity_list->Append(label);
	}
	m_entity_list->Thaw();
	SelectEntityInList(m_entity_id);
	m_populating = false;
	RefreshEntityButtons();
}

void EntityViewerFrame::SelectEntityInList(int entity_id)
{
	if (!m_entity_list)
	{
		return;
	}
	for (std::size_t i = 0; i < m_entity_ids.size(); ++i)
	{
		if (m_entity_ids[i] == entity_id)
		{
			m_entity_list->SetSelection(static_cast<int>(i));
			m_entity_list->EnsureVisible(static_cast<int>(i));
			return;
		}
	}
}

int EntityViewerFrame::SelectedListEntity() const
{
	if (!m_entity_list)
	{
		return -1;
	}
	const int row = m_entity_list->GetSelection();
	if (row == wxNOT_FOUND || row >= static_cast<int>(m_entity_ids.size()))
	{
		return -1;
	}
	return m_entity_ids[row];
}

void EntityViewerFrame::RefreshEntityButtons()
{
	if (!m_add)
	{
		return;
	}
	const bool has_data = m_gd != nullptr;
	const int id = SelectedListEntity();
	const auto sprite_data = has_data ? m_gd->GetSpriteData() : nullptr;
	const bool valid = has_data && id >= 0;
	const bool is_item = valid && sprite_data->IsEntityItem(static_cast<uint8_t>(id));
	const int row = m_entity_list ? m_entity_list->GetSelection() : wxNOT_FOUND;

	// A move swaps with the list neighbour, which must itself be a non-item entity.
	const auto neighbour_movable = [&](int delta)
	{
		const int nrow = row + delta;
		if (!valid || is_item || nrow < 0 || nrow >= static_cast<int>(m_entity_ids.size()))
		{
			return false;
		}
		return !sprite_data->IsEntityItem(m_entity_ids[nrow]);
	};

	m_add->Enable(has_data && sprite_data->GetFreeEntityId().has_value());
	m_rename->Enable(valid);
	m_remove->Enable(valid && !is_item && !sprite_data->IsEntityUsedInRooms(static_cast<uint8_t>(id)));
	m_move_up->Enable(neighbour_movable(-1));
	m_move_down->Enable(neighbour_movable(1));
}

void EntityViewerFrame::OnEntitySelected()
{
	if (m_populating)
	{
		return;
	}
	const int id = SelectedListEntity();
	if (id >= 0)
	{
		Open(id);
	}
}

void EntityViewerFrame::OnAddEntity()
{
	if (!m_gd)
	{
		return;
	}
	const auto sprite_data = m_gd->GetSpriteData();
	AddEntityDialog dialog(this, *sprite_data);
	if (!dialog.HasSprites())
	{
		wxMessageBox("There are no sprites to base an entity on.", "New Entity", wxOK | wxICON_ERROR, this);
		return;
	}
	if (dialog.ShowModal() != wxID_OK || dialog.GetSpriteId() < 0)
	{
		return;
	}
	const auto added = sprite_data->AddEntity(static_cast<uint8_t>(dialog.GetSpriteId()),
		dialog.GetLowPalette(), dialog.GetHighPalette());
	if (!added)
	{
		wxMessageBox("Unable to create the entity. Every non-item id may already be in use.",
			"New Entity", wxOK | wxICON_ERROR, this);
		return;
	}
	// Apply the display name if one was given and is valid; otherwise the entity keeps its default
	// EntityNN label.
	const auto name = dialog.GetEntityName();
	if (!name.empty() && Landstalker::Labels::IsValid(name, Landstalker::Labels::C_ENTITIES, *added))
	{
		Landstalker::Labels::Update(Landstalker::Labels::C_ENTITIES, *added, name);
	}
	PopulateEntityList();
	Open(*added);
}

void EntityViewerFrame::OnRemoveEntity()
{
	if (!m_gd)
	{
		return;
	}
	const int id = SelectedListEntity();
	if (id < 0)
	{
		return;
	}
	const auto sprite_data = m_gd->GetSpriteData();
	const auto eid = static_cast<uint8_t>(id);
	const auto name = wxString(Landstalker::SpriteData::GetEntityDisplayName(eid));

	const auto rooms = sprite_data->GetRoomsUsingEntity(eid);
	if (!rooms.empty())
	{
		wxMessageBox(wxString::Format(
			"'%s' cannot be deleted: %d room(s) still place it.\n\n  %s\n\n"
			"Remove it from those rooms first.",
			name, static_cast<int>(rooms.size()), DescribeRooms(*m_gd->GetRoomData(), rooms)),
			"Remove Entity", wxOK | wxICON_ERROR, this);
		return;
	}
	if (wxMessageBox(wxString::Format(
		"Delete entity '%s'?\n\nThis cannot be undone. Its sprite assignment, palettes, stats, "
		"talk sound and name are removed. Other entities keep their ids.", name),
		"Remove Entity", wxYES_NO | wxNO_DEFAULT | wxICON_WARNING, this) != wxYES)
	{
		return;
	}

	const int row = m_entity_list->GetSelection();
	int next = -1;
	if (row + 1 < static_cast<int>(m_entity_ids.size())) next = m_entity_ids[row + 1];
	else if (row > 0) next = m_entity_ids[row - 1];

	if (!sprite_data->DeleteEntity(eid, m_gd->GetStringData()))
	{
		wxMessageBox("Unable to delete the selected entity.", "Remove Entity", wxOK | wxICON_ERROR, this);
		return;
	}
	m_entity_id = -1;
	PopulateEntityList();
	Open(next);
}

void EntityViewerFrame::OnMoveEntity(int delta)
{
	if (!m_gd || !m_entity_list)
	{
		return;
	}
	const int row = m_entity_list->GetSelection();
	const int nrow = row + delta;
	if (row == wxNOT_FOUND || nrow < 0 || nrow >= static_cast<int>(m_entity_ids.size()))
	{
		return;
	}
	const uint8_t a = m_entity_ids[row];
	const uint8_t b = m_entity_ids[nrow];
	if (m_gd->GetSpriteData()->SwapEntities(a, b, m_gd->GetStringData()))
	{
		// The content moved to the neighbour's id, so follow it there.
		m_entity_id = b;
		PopulateEntityList();
		Open(b);
	}
}

void EntityViewerFrame::OnRenameEntity()
{
	if (!m_gd)
	{
		return;
	}
	const int id = SelectedListEntity();
	if (id < 0)
	{
		return;
	}
	const auto eid = static_cast<uint8_t>(id);
	const auto old_name = Landstalker::SpriteData::GetEntityDisplayName(eid);
	wxTextEntryDialog dlg(this, "Display name for this entity (used only in the editor):",
		"Rename Entity", wxString(old_name));
	while (dlg.ShowModal() == wxID_OK)
	{
		const auto new_name = dlg.GetValue().ToStdWstring();
		if (new_name == old_name)
		{
			return;
		}
		if (!Landstalker::Labels::IsValid(new_name, Landstalker::Labels::C_ENTITIES, id))
		{
			wxMessageBox("The name must not be empty, must be unique, and must not contain "
				"non-printable characters.", "Rename Entity", wxOK | wxICON_ERROR, this);
			continue;
		}
		Landstalker::Labels::Update(Landstalker::Labels::C_ENTITIES, id, new_name);
		PopulateEntityList();
		// Refresh the Name field in the property grid too.
		m_reset_props = true;
		FireEvent(EVT_PROPERTIES_UPDATE);
		return;
	}
}

void EntityViewerFrame::NavigateTo(const wxString& path)
{
	if (path.IsEmpty())
	{
		return;
	}
	// The tree carries the target's value (sprite id, room number, palette mode), so the path
	// alone is enough - the same mechanism the other editors use to jump between items.
	wxCommandEvent evt(EVT_GO_TO_NAV_ITEM);
	evt.SetString(path);
	evt.SetClientData(this);
	wxPostEvent(this, evt);
}

void EntityViewerFrame::PopulateStats()
{
	if (!m_stats)
	{
		return;
	}
	m_stats->Freeze();
	m_stats->DestroyChildren();

	auto* grid = new wxFlexGridSizer(2, 5, 12);
	grid->AddGrowableCol(1, 1);

	const auto caption = [this, grid](const wxString& text, int flags)
	{
		auto* c = new wxStaticText(m_stats, wxID_ANY, text);
		wxFont f = c->GetFont();
		f.SetWeight(wxFONTWEIGHT_BOLD);
		c->SetFont(f);
		grid->Add(c, 0, flags);
	};
	const auto add_text = [this, grid, &caption](const wxString& label, const wxString& value)
	{
		caption(label, wxALIGN_TOP);
		grid->Add(new wxStaticText(m_stats, wxID_ANY, value), 1, wxEXPAND);
	};
	const auto add_link = [this, grid, &caption](const wxString& label, const wxString& text, const wxString& path)
	{
		caption(label, wxALIGN_CENTER_VERTICAL);
		auto* link = new wxHyperlinkCtrl(m_stats, wxID_ANY, text, wxEmptyString);
		link->Bind(wxEVT_HYPERLINK, [this, path](wxHyperlinkEvent&) { NavigateTo(path); });
		grid->Add(link, 1, wxALIGN_CENTER_VERTICAL);
	};

	if (m_gd && m_entity_id >= 0 && m_gd->GetSpriteData()->IsEntity(static_cast<uint8_t>(m_entity_id)))
	{
		const auto sd = m_gd->GetSpriteData();
		const auto eid = static_cast<uint8_t>(m_entity_id);

		const wxString kind = sd->IsEntityItem(eid) ? "Item" : (sd->IsEntityEnemy(eid) ? "Enemy" : "NPC");
		add_text("ID", wxString::Format("%d (0x%02X) - %s", m_entity_id, m_entity_id, kind));

		const auto spr = sd->GetSpriteFromEntity(eid);
		const auto spr_name = Landstalker::SpriteData::GetSpriteDisplayName(spr);
		add_link("Sprite", wxString::Format("%d: ", static_cast<int>(spr)) + wxString(spr_name),
			wxString(L"Sprites/" + spr_name));

		const auto pals = sd->GetEntityPaletteIdxs(eid);
		if (pals.first >= 0)
		{
			add_link("Low palette",
				wxString(Landstalker::SpriteData::GetSpriteLowPaletteDisplayName(static_cast<uint8_t>(pals.first))),
				"Palettes/Sprite Low Palettes");
		}
		else
		{
			add_text("Low palette", "None");
		}
		if (pals.second >= 0)
		{
			add_link("High palette",
				wxString(Landstalker::SpriteData::GetSpriteHighPaletteDisplayName(static_cast<uint8_t>(pals.second))),
				"Palettes/Sprite High Palettes");
		}
		else
		{
			add_text("High palette", "None");
		}

		const auto sfx = m_gd->GetStringData()->GetEntityTalkSound(eid);
		add_text("Talk sound", sfx == 0 ? wxString("None")
			: wxString::Format("0x%02X ", sfx) +
			  wxString(Landstalker::Labels::Get(Landstalker::Labels::C_SOUNDS, sfx).value_or(L"")));

		const auto hb = sd->GetEntityHitbox(eid);
		add_text("Hitbox", wxString::Format("%d x %d", static_cast<int>(hb.base), static_cast<int>(hb.height)));

		if (sd->IsEntityEnemy(eid))
		{
			const auto es = sd->GetEnemyStats(eid);
			add_text("Health", wxString::Format("%d", static_cast<int>(es.health)));
			add_text("Attack", wxString::Format("%d", static_cast<int>(es.attack)));
			add_text("Defence", wxString::Format("%d", static_cast<int>(es.defence)));
		}
		if (sd->IsEntityItem(eid))
		{
			const auto ip = sd->GetItemProperties(eid);
			add_text("Max quantity", wxString::Format("%d", static_cast<int>(ip.max_quantity)));
			add_text("Price", wxString::Format("%d", static_cast<int>(ip.price)));
		}

		// The rooms that place this entity, each a link back to that room.
		const auto rooms = sd->GetRoomsUsingEntity(eid);
		caption(wxString::Format("Used by %d room%s", static_cast<int>(rooms.size()),
			rooms.size() == 1 ? "" : "s"), wxALIGN_TOP);
		if (rooms.empty())
		{
			grid->Add(new wxStaticText(m_stats, wxID_ANY, "None"), 1, wxEXPAND);
		}
		else
		{
			auto* wrap = new wxWrapSizer(wxHORIZONTAL);
			const std::size_t cap = 40;
			for (std::size_t i = 0; i < rooms.size() && i < cap; ++i)
			{
				const auto rname = m_gd->GetRoomData()->GetRoomDisplayName(rooms[i]);
				auto* link = new wxHyperlinkCtrl(m_stats, wxID_ANY, wxString(rname), wxEmptyString);
				const wxString path = L"Rooms/" + rname;
				link->Bind(wxEVT_HYPERLINK, [this, path](wxHyperlinkEvent&) { NavigateTo(path); });
				wrap->Add(link, 0, wxRIGHT | wxBOTTOM, 6);
			}
			if (rooms.size() > cap)
			{
				wrap->Add(new wxStaticText(m_stats, wxID_ANY,
					wxString::Format("+%d more", static_cast<int>(rooms.size() - cap))),
					0, wxALIGN_CENTER_VERTICAL);
			}
			grid->Add(wrap, 1, wxEXPAND);
		}
	}

	auto* outer = new wxBoxSizer(wxVERTICAL);
	outer->Add(grid, 1, wxEXPAND | wxALL, 8);
	m_stats->SetSizer(outer);
	m_stats->FitInside();
	m_stats->Layout();
	m_stats->Thaw();
}

void EntityViewerFrame::OnMenuClick(wxMenuEvent& evt)
{
	ProcessEvent(evt.GetId());
	evt.Skip();
}

void EntityViewerFrame::ProcessEvent(int id)
{
	switch (id)
	{
	case ID_FILE_EXPORT_ENTITY_PROPERTIES_YAML:
		OnExportPropertiesYaml();
		break;
	case ID_FILE_EXPORT_ENTITY_SPRITESHEET:
		OnExportEntitySpritesheet();
		break;
	case ID_FILE_EXPORT_ALL_ENTITY_SPRITESHEETS:
		OnExportAllEntitySpritesheets();
		break;
	case ID_FILE_IMPORT_ENTITY_METADATA:
		OnImportEntityMetadata();
		break;
	}
}

void EntityViewerFrame::OnExportPropertiesYaml()
{
	const wxString default_file = Landstalker::StrPrintf("Entity%03dProperties.yaml", m_entity_id);
	wxFileDialog fd(this, _("Export Entity Properties As YAML"), "", default_file, "YAML Files (*.yml, *.yaml)|*.yml;*.yaml|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		ExportPropertiesYaml(fd.GetPath().ToStdString());
	}
}

void EntityViewerFrame::ExportPropertiesYaml(const std::string& filename)
{   
    std::ostringstream ss;
    auto sd = m_gd->GetSpriteData();
    int sprite_index = sd->GetSpriteFromEntity(m_entity_id);
    bool is_item = sd->IsEntityItem(m_entity_id);
    bool is_enemy = sd->IsEntityEnemy(m_entity_id);
    auto palette_idxs = sd->GetEntityPaletteIdxs(m_entity_id);
    
    // Main properties
    ss << "Name: " << Landstalker::wstr_to_utf8(sd->GetEntityDisplayName(m_entity_id)) << std::endl;
    ss << "ID: " << m_entity_id << std::endl;
    ss << "SpriteID: " << sprite_index << std::endl;
    ss << "LowPalette: " << palette_idxs.first << std::endl;
    ss << "HighPalette: " << palette_idxs.second << std::endl;
    ss << "TalkSoundFX: " << static_cast<int>(m_gd->GetStringData()->GetEntityTalkSound(m_entity_id)) << std::endl;
    
    // Item properties
    ss << "IsItem: " << (is_item ? "true" : "false") << std::endl;
    if (is_item)
    {
        auto item_props = sd->GetItemProperties(m_entity_id);
        ss << "Item:" << std::endl;
        ss << "  UseText: " << static_cast<int>(item_props.verb) << std::endl;
        ss << "  MaximumQuantity: " << static_cast<int>(item_props.max_quantity) << std::endl;
        ss << "  EquipmentIndex: " << static_cast<int>(item_props.equipment_index) << std::endl;
        ss << "  NormalBuyPrice: " << static_cast<int>(item_props.price) << std::endl;
    }
    
    // Enemy properties
    ss << "IsEnemy: " << (is_enemy ? "true" : "false") << std::endl;
    if (is_enemy)
    {
        auto enemy_stats = sd->GetEnemyStats(m_entity_id);
        ss << "Enemy:" << std::endl;
        ss << "  Health: " << static_cast<int>(enemy_stats.health) << std::endl;
        ss << "  Defence: " << static_cast<int>(enemy_stats.defence) << std::endl;
        ss << "  Attack: " << static_cast<int>(enemy_stats.attack) << std::endl;
        ss << "  GoldDrop: " << static_cast<int>(enemy_stats.gold_drop) << std::endl;
        ss << "  ItemDrop: " << static_cast<int>(enemy_stats.item_drop) << std::endl;
        ss << "  DropProbability: " << static_cast<int>(enemy_stats.drop_probability) << std::endl;
    }
    
    // Write to file
    std::ofstream file(filename);
    if (file.is_open())
    {
        file << ss.str();
        file.close();
    }
}

void EntityViewerFrame::OnExportEntitySpritesheet()
{
	if (m_entity_id < 0)
	{
		return;
	}
	const wxString default_file = Landstalker::StrPrintf("Entity%03d_sheet.png", m_entity_id);
	wxFileDialog fd(this, _("Export Entity Sprite Sheet with Metadata"), "", default_file,
		"PNG Image (*.png)|*.png|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		ExportEntitySpritesheet(static_cast<uint8_t>(m_entity_id), fd.GetPath().ToStdString());
	}
}

void EntityViewerFrame::OnExportAllEntitySpritesheets()
{
	wxDirDialog dd(this, "Select Sprite Sheet Output Directory");
	if (dd.ShowModal() != wxID_CANCEL)
	{
		ExportAllEntitySpritesheets(dd.GetPath().ToStdString());
	}
}

void EntityViewerFrame::ExportEntitySpritesheet(uint8_t entity_id, const std::string& filename)
{
	using Result = Landstalker::SpriteData::SpriteSheetResult;
	auto sd = m_gd->GetSpriteData();
	const uint8_t sid = sd->GetSpriteFromEntity(entity_id);
	// Use the entity's own palette (the colours the editor shows it in), falling back if absent.
	auto palette = sd->GetEntityPalette(entity_id);
	if (!palette)
	{
		palette = sd->GetSpriteDisplayPalette(sid);
	}
	// Entity metadata precedes the sprite metadata and grid layout in the YAML.
	const std::string entity_yaml = sd->GetEntityMetadataYaml(entity_id, m_gd->GetStringData());
	switch (sd->WriteSpriteSheet(sid, std::filesystem::path(filename), { palette }, 8, entity_yaml))
	{
	case Result::NoFrames:
		wxMessageBox("This entity's sprite has no frames to export.", "Export Entity Sprite Sheet", wxOK | wxICON_WARNING, this);
		break;
	case Result::ImageWriteFailed:
		wxMessageBox("Unable to write the sprite sheet image.", "Export Entity Sprite Sheet", wxOK | wxICON_ERROR, this);
		break;
	case Result::MetadataWriteFailed:
		wxMessageBox("The sheet image was written, but its metadata could not be saved.",
			"Export Entity Sprite Sheet", wxOK | wxICON_WARNING, this);
		break;
	case Result::Written:
		break;
	}
}

void EntityViewerFrame::ExportAllEntitySpritesheets(const std::string& dir)
{
	using Result = Landstalker::SpriteData::SpriteSheetResult;
	auto sd = m_gd->GetSpriteData();
	const std::filesystem::path out_dir(dir);

	int written = 0;
	int failed = 0;
	for (const auto eid : sd->GetEntityIds())
	{
		const uint8_t sid = sd->GetSpriteFromEntity(eid);
		auto palette = sd->GetEntityPalette(eid);
		if (!palette)
		{
			palette = sd->GetSpriteDisplayPalette(sid);
		}
		const std::string entity_yaml = sd->GetEntityMetadataYaml(eid, m_gd->GetStringData());
		const std::filesystem::path png_path = out_dir / Landstalker::StrPrintf("Entity%03d_sheet.png", eid);
		switch (sd->WriteSpriteSheet(sid, png_path, { palette }, 8, entity_yaml))
		{
		case Result::Written:
		case Result::MetadataWriteFailed:
			++written;
			break;
		case Result::ImageWriteFailed:
			++failed;
			break;
		case Result::NoFrames:
			break;
		}
	}

	const wxString summary = wxString::Format("Exported %d entity sprite sheet%s to:\n%s", written,
		written == 1 ? "" : "s", wxString::FromUTF8(dir))
		+ (failed > 0 ? wxString::Format("\n\n%d entit%s could not be written.", failed, failed == 1 ? "y" : "ies") : wxString());
	wxMessageBox(summary, "Export All Entity Sprite Sheets", wxOK | (failed > 0 ? wxICON_WARNING : wxICON_INFORMATION), this);
}

void EntityViewerFrame::OnImportEntityMetadata()
{
	if (!m_gd || m_entity_id < 0)
	{
		return;
	}
	wxFileDialog fd(this, _("Import Entity Metadata From YAML"), "", "",
		"YAML Files (*.yml, *.yaml)|*.yml;*.yaml|All Files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return;
	}
	std::ifstream in(fd.GetPath().ToStdString(), std::ios::binary);
	std::stringstream ss;
	ss << in.rdbuf();

	if (!m_gd->GetSpriteData()->ApplyEntityMetadataYaml(static_cast<uint8_t>(m_entity_id), ss.str(), m_gd->GetStringData()))
	{
		wxMessageBox("Could not read entity metadata from the selected file.",
			"Import Entity Metadata", wxOK | wxICON_ERROR, this);
		return;
	}
	// The import may flip the enemy/item tags and change palettes, so refresh the list, the
	// property grid and the stats panel.
	PopulateEntityList();
	FireEvent(EVT_PROPERTIES_UPDATE);
	PopulateStats();
}
