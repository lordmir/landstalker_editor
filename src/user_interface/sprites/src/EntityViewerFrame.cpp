#include <user_interface/sprites/include/EntityViewerFrame.h>
#include <wx/propgrid/advprops.h>

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
		props.Append(new wxStringProperty("Name", "Name", Labels::Get(L"entities", m_entity_id).value_or(L"Entity" + std::to_wstring(m_entity_id))));
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
		for (int i = 0; i <= 0xFF; ++i)
		{
			if (Labels::Get(L"sounds", i))
			{
				m_sounds.Add(*Labels::Get(L"sounds", i), i);
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

		props.GetGrid()->SetPropertyValue("Name", _(Labels::Get(L"entities", m_entity_id).value_or(L"Entity" + std::to_wstring(m_entity_id))));
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
	if (name == "Name")
	{
		FireRenameNavItemEvent(property->GetValueAsString().ToStdWstring(), Labels::Get(L"entities", m_entity_id).value_or(L"Entity" + std::to_wstring(m_entity_id)));
		Labels::Update(L"entities", m_entity_id, property->GetValueAsString().ToStdWstring());
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

void EntityViewerFrame::FireRenameNavItemEvent(const std::wstring& old_name, const std::wstring& new_name)
{
	wxCommandEvent evt(EVT_RENAME_NAV_ITEM);
	std::wstring lbl(L"Entities/" + old_name + L"\1Entities/" + new_name);
	evt.SetString(lbl);
	wxPostEvent(this, evt);
}
