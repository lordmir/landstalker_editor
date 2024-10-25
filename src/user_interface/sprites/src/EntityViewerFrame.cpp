#include <user_interface/sprites/include/EntityViewerFrame.h>
#include <wx/propgrid/advprops.h>

enum MENU_IDS
{
	ID_FILE_EXPORT_PNG = 20000,
	ID_VIEW_SEP1
};

EntityViewerFrame::EntityViewerFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst)
{
	m_mgr.SetManagedWindow(this);
	m_entity_ctrl = new EntityViewerCtrl(this);

	// add the panes to the manager
	m_mgr.SetDockSizeConstraint(0.3, 0.3);
	m_mgr.AddPane(m_entity_ctrl, wxAuiPaneInfo().CenterPane());

	// tell the manager to "commit" all the changes just made
	m_mgr.Update();
}

EntityViewerFrame::~EntityViewerFrame()
{
}

bool EntityViewerFrame::Open(int entity)
{
	if (m_gd && entity >= 0 && entity < 256)
	{
		m_entity_id = entity;
		Update();
		return true;
	}
	return false;
}

void EntityViewerFrame::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
	m_entity_ctrl->SetGameData(gd);
}

void EntityViewerFrame::ClearGameData()
{
	m_entity_id = -1;
	m_entity_ctrl->ClearGameData();
	m_gd.reset();
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
		props.Append(new wxStringProperty("Name", "Name", _(Entity::EntityNames.at(m_entity_id))))->Enable(false);
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
		props.Append(new wxPropertyCategory("Enemy", "Enemy"));
		auto is_enemy = new wxBoolProperty("Is Enemy", "Is Enemy", sd->IsEntityEnemy(m_entity_id));
		is_enemy->SetAttribute(wxPG_BOOL_USE_CHECKBOX, true);
		props.Append(is_enemy);
		auto health = new wxIntProperty("Health", "Health", 0);
		health->SetAttribute(wxPG_ATTR_MIN, 0);
		health->SetAttribute(wxPG_ATTR_MAX, 255);
		health->SetAttribute(wxPG_ATTR_SPINCTRL_STEP, 1);
		health->SetEditor(wxPGEditor_SpinCtrl);
		props.Append(health)->Enable(false);
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
			m_lo_palettes.Add(_(m_gd->GetSpriteData()->GetLoPalette(i)->GetName()));
		}
		wxFont font = m_lo_palettes.Item(epals.first + 1).GetFont();
		font.SetWeight(wxFontWeight::wxFONTWEIGHT_BOLD);
		m_lo_palettes.Item(epals.first + 1).SetFont(font);

		m_hi_palettes.Clear();
		m_hi_palettes.Add("<None>");
		for (int i = 0; i < m_gd->GetSpriteData()->GetHiPaletteCount(); ++i)
		{
			m_hi_palettes.Add(_(m_gd->GetSpriteData()->GetHiPalette(i)->GetName()));
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
			m_sprites.Add(m_gd->GetSpriteData()->GetSpriteName(i), i);
		}

		m_verbs.Clear();
		for (int i = 12; i < 20; ++i)
		{
			m_verbs.Add(m_gd->GetStringData()->GetString(StringData::Type::MAIN, i), i);
		}

		m_items.Clear();
		for (std::size_t i = 0; i < m_gd->GetStringData()->GetStringCount(StringData::Type::ITEM_NAMES); ++i)
		{
			m_items.Add(m_gd->GetStringData()->GetString(StringData::Type::ITEM_NAMES, i), i);
		}

		m_sounds.Clear();
		m_sounds.Add("<None>", 0);
		m_sounds.Add("[01] Bustling Street", 1);
		m_sounds.Add("[02] Torchlight", 2);
		m_sounds.Add("[03] Premonition of Trouble", 3);
		m_sounds.Add("[04] Fanfare 1", 4);
		m_sounds.Add("[05] Get Item", 5);
		m_sounds.Add("[06] Rest", 6);
		m_sounds.Add("[07] Save Game", 7);
		m_sounds.Add("[08] Heal", 8);
		m_sounds.Add("[09] Poison", 9);
		m_sounds.Add("[10] The Death God's Invitation", 10);
		m_sounds.Add("[11] Duke's Fanfare", 11);
		m_sounds.Add("[12] Fanfare 2", 12);
		m_sounds.Add("[13] Einstein Whistle", 13);
		m_sounds.Add("[14] Black Market", 14);
		m_sounds.Add("[15] Surprise", 15);
		m_sounds.Add("[16] Title", 16);
		m_sounds.Add("[17] Let's Go On An Adventure", 17);
		m_sounds.Add("[18] Treasure Hunter Lyle", 18);
		m_sounds.Add("[19] Seeking Treasure", 19);
		m_sounds.Add("[20] Deserted Street Corner", 20);
		m_sounds.Add("[21] Friday And A Soft Breeze", 21);
		m_sounds.Add("[22] Hey, It's Your Turn", 22);
		m_sounds.Add("[27] To The Great Adventurers", 27);
		m_sounds.Add("[29] Beneath The Mysterious Tree", 29);
		m_sounds.Add("[31] Urgent, Lyle!", 31);
		m_sounds.Add("[33] Prayers To God", 33);
		m_sounds.Add("[34] Light Of The Setting Sun", 34);
		m_sounds.Add("[35] Wicked God's Banquet", 35);
		m_sounds.Add("[36] Heh Heh, Think I'll Disrupt This Good Cheer", 36);
		m_sounds.Add("[37] The Marquis' Invitation", 37);
		m_sounds.Add("[38] The King's Chamber", 38);
		m_sounds.Add("[39] Mysterious Island", 39);
		m_sounds.Add("[40] Friday's Memory", 40);
		m_sounds.Add("[41] Divine Guardian Of The Maze", 41);
		m_sounds.Add("[42] The Silence, The Darkness, And", 42);
		m_sounds.Add("[43] Labrynth", 43);
		m_sounds.Add("[44] A Ballad For Princess Loria", 44);
		m_sounds.Add("[45] A Ballad For Princess Loria (Loop)", 45);
		m_sounds.Add("[46] Pink Book", 46);
		m_sounds.Add("[47] Adventure Spirit", 47);
		m_sounds.Add("[65] Skeleton Talk", 65);
		m_sounds.Add("[66] Cursor Move", 66);
		m_sounds.Add("[67] Cursor Select", 67);
		m_sounds.Add("[68] Error Buzz", 68);
		m_sounds.Add("[69] Speaker Pitch 8", 69);
		m_sounds.Add("[70] Speaker Pitch 7", 70);
		m_sounds.Add("[71] Speaker Pitch 6", 71);
		m_sounds.Add("[72] Speaker Pitch 5", 72);
		m_sounds.Add("[73] Speaker Pitch 4", 73);
		m_sounds.Add("[74] Speaker Pitch 3", 74);
		m_sounds.Add("[75] Speaker Pitch 2", 75);
		m_sounds.Add("[76] Speaker Pitch 1", 76);
		m_sounds.Add("[77] Warp", 77);
		m_sounds.Add("[78] Drum", 78);
		m_sounds.Add("[79] Cymbal", 79);
		m_sounds.Add("[80] Snare Drum", 80);
		m_sounds.Add("[81] Sword Swing", 81);
		m_sounds.Add("[82] Sword Hitting Stone", 82);
		m_sounds.Add("[83] Jump", 83);
		m_sounds.Add("[84] Fall", 84);
		m_sounds.Add("[85] Exit", 85);
		m_sounds.Add("[86] Throw", 86);
		m_sounds.Add("[87] Enemy Cry", 87);
		m_sounds.Add("[88] Door Lock", 88);
		m_sounds.Add("[89] Rumble", 89);
		m_sounds.Add("[90] Health Recover", 90);
		m_sounds.Add("[91] Dog Transform", 91);
		m_sounds.Add("[92] HP Absorb", 92);
		m_sounds.Add("[93] Land", 93);
		m_sounds.Add("[94] Climb", 94);
		m_sounds.Add("[95] Throw Alt", 95);
		m_sounds.Add("[96] Bounce", 96);
		m_sounds.Add("[97] Player Take Damage", 97);
		m_sounds.Add("[98] Player Drop Object Alt", 98);
		m_sounds.Add("[99] Player Drop Object", 99);
		m_sounds.Add("[100] Player Take Damage Alt", 100);
		m_sounds.Add("[101] Player Die", 101);
		m_sounds.Add("[102] Enemy Die", 102);
		m_sounds.Add("[103] Enemy Take Damage", 103);
		m_sounds.Add("[104] Open Menu", 104);
		m_sounds.Add("[105] Pick Up Item", 105);
		m_sounds.Add("[106] Pick Up Gold", 106);
		m_sounds.Add("[107] Kazalt Warp", 107);
		m_sounds.Add("[108] Switch", 108);
		m_sounds.Add("[109] Explosion", 109);
		m_sounds.Add("[110] Thud", 110);
		m_sounds.Add("[111] Enemy Die Alt", 111);
		m_sounds.Add("[112] Enemy Take Damage Alt", 112);
		m_sounds.Add("[113] Fireball 1", 113);
		m_sounds.Add("[114] Fireball 2", 114);
		m_sounds.Add("[115] Short Rumble", 115);
		m_sounds.Add("[116] Permanent Switch", 116);
		m_sounds.Add("[117] Latch", 117);
		m_sounds.Add("[118] Friday Magic", 118);
		m_sounds.Add("[119] Slash 1", 119);
		m_sounds.Add("[120] Slash 2", 120);
		m_sounds.Add("[121] Mir Warp", 121);
		m_sounds.Add("[122] Poison", 122);

		m_probabilities.Clear();
		m_probabilities.Add("1/64");
		m_probabilities.Add("1/128");
		m_probabilities.Add("1/256");
		m_probabilities.Add("1/512");
		m_probabilities.Add("1/1024");
		m_probabilities.Add("1/2048");
		m_probabilities.Add("Never");
		m_probabilities.Add("Guaranteed");
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

		props.GetGrid()->SetPropertyValue("Name", _(Entity::EntityNames.at(m_entity_id)));
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
		props.GetGrid()->SetPropertyValue("Is Enemy", sd->IsEntityEnemy(m_entity_id));
		props.GetGrid()->GetProperty("Health")->Enable(is_enemy);
		props.GetGrid()->SetPropertyValue("Health", is_enemy ? enemy_stats.health : 0);
		props.GetGrid()->GetProperty("Defence")->Enable(is_enemy);
		props.GetGrid()->SetPropertyValue("Defence", is_enemy ? enemy_stats.defence : 0);
		props.GetGrid()->GetProperty("Attack")->Enable(is_enemy);
		props.GetGrid()->SetPropertyValue("Attack", is_enemy ? enemy_stats.attack : 0);
		props.GetGrid()->GetProperty("Gold Drop")->Enable(is_enemy);
		props.GetGrid()->SetPropertyValue("Gold Drop", is_enemy ? enemy_stats.gold_drop : 0);
		props.GetGrid()->GetProperty("Item Drop")->Enable(is_enemy);
		props.GetGrid()->GetProperty("Item Drop")->SetChoices(is_enemy ? m_items : m_empty_choices);
		if (is_enemy)
		{
			props.GetGrid()->GetProperty("Item Drop")->SetChoiceSelection(enemy_stats.item_drop);
		}
		props.GetGrid()->GetProperty("Drop Probability")->Enable(is_enemy);
		props.GetGrid()->GetProperty("Drop Probability")->SetChoices(is_enemy ? m_probabilities : m_empty_choices);
		if (is_enemy)
		{
			props.GetGrid()->GetProperty("Drop Probability")->SetChoiceSelection(static_cast<uint8_t>(enemy_stats.drop_probability));
		}
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
	if (name == "Sprite" || name == "Sprite ID")
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
			m_gd->GetSpriteData()->SetEnemyStats(m_entity_id, SpriteData::EnemyStats());
		}
		else
		{
			m_gd->GetSpriteData()->ClearEnemyStats(m_entity_id);
		}
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
			enemy_stats.drop_probability = static_cast<SpriteData::EnemyStats::DropProbability>(std::clamp<uint8_t>(value, 0, 7));
			m_gd->GetSpriteData()->SetEnemyStats(m_entity_id, enemy_stats);
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
	}
	ctrl->GetGrid()->Thaw();
}


void EntityViewerFrame::ExportPng(const std::string& filename) const
{
	auto sd = m_gd->GetSpriteData();
	int sprite_index = sd->GetSpriteFromEntity(m_entity_id);

	std::shared_ptr<SpriteFrameEntry> frame = m_gd->GetSpriteData()->GetSpriteFrame(sprite_index, 0);

	int frame_count = m_gd->GetSpriteData()->GetSpriteFrameCount(sprite_index);

	int width = 0;
	int height = 0;
	for (int frame_index = 0; frame_index < frame_count; ++frame_index) {
	    std::shared_ptr<SpriteFrameEntry> spriteFrame = m_gd->GetSpriteData()->GetSpriteFrame(sprite_index, frame_index);
		width += spriteFrame->GetData()->GetWidth();
		if(spriteFrame->GetData()->GetHeight() > height) {
			height = spriteFrame->GetData()->GetHeight();
		}
	}

	ImageBuffer buf(width, height);
	int draw_x = 0;
	for (int frame_index = 0; frame_index < frame_count; ++frame_index) {
		std::shared_ptr<SpriteFrameEntry> spriteFrame = m_gd->GetSpriteData()->GetSpriteFrame(sprite_index, frame_index);
		int draw_y = height - spriteFrame->GetData()->GetHeight();
		buf.InsertSprite(-spriteFrame->GetData()->GetLeft() + draw_x, -spriteFrame->GetData()->GetTop() + draw_y, 0, *spriteFrame->GetData());
		draw_x += spriteFrame->GetData()->GetWidth();
	}

	buf.WritePNG(filename, { m_palette }, true);
}

void EntityViewerFrame::OnExportPng()
{
	const wxString default_file = StrPrintf("Entity%03d.png", m_entity_id);
	wxFileDialog fd(this, _("Export Entity Sprite As PNG"), "", default_file, "PNG Image (*.png)|*.png|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		ExportPng(fd.GetPath().ToStdString());
	}
}

void EntityViewerFrame::InitMenu(wxMenuBar& menu, ImageList& /*ilist*/) const
{
	ClearMenu(menu);
	auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
	AddMenuItem(fileMenu, 0, ID_FILE_EXPORT_PNG, "Export Entity Sprite as PNG...");
	AddMenuItem(fileMenu, 1, ID_VIEW_SEP1, "", wxITEM_SEPARATOR);
	UpdateUI();

	m_mgr.Update();
}

void EntityViewerFrame::OnMenuClick(wxMenuEvent& evt)
{
	switch (evt.GetId())
	{
	case ID_FILE_EXPORT_PNG:
		OnExportPng();
		break;
	default:
		break;
	}
	UpdateUI();
	FireEvent(EVT_STATUSBAR_UPDATE);
	FireEvent(EVT_PROPERTIES_UPDATE);
	evt.Skip();
}
