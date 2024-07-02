#include <EntityViewerFrame.h>

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
		props.Append(new wxIntProperty("Sprite ID", "Sprite ID", sprite_index));
		props.Append(new wxEnumProperty("Low Palette", "Low Palette", m_lo_palettes));
		props.Append(new wxEnumProperty("High Palette", "High Palette", m_hi_palettes));
		props.Append(new wxEnumProperty("Talk Sound FX", "Talk Sound FX", m_sounds));
		props.Append(new wxPropertyCategory("Item", "Item"));
		auto is_item = new wxBoolProperty("Is Item", "Is Item", sd->IsEntityItem(m_entity_id));
		is_item->SetAttribute(wxPG_BOOL_USE_CHECKBOX, true);
		props.Append(is_item)->Enable(false);
		props.Append(new wxPropertyCategory("Enemy", "Enemy"));
		auto is_enemy = new wxBoolProperty("Is Enemy", "Is Enemy", sd->IsEntityEnemy(m_entity_id));
		is_enemy->SetAttribute(wxPG_BOOL_USE_CHECKBOX, true);
		props.Append(is_enemy);
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
		auto font = m_lo_palettes.Item(epals.first + 1).GetFont();
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
		props.GetGrid()->SetPropertyValue("Is Item", sd->IsEntityItem(m_entity_id));
		props.GetGrid()->SetPropertyValue("Is Enemy", sd->IsEntityEnemy(m_entity_id));
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
		wxMessageBox(std::to_string(value));
		if (value != sprite_index)
		{
			sprite_index = std::clamp<uint8_t>(value, 0, 255);
			if (sd->IsSprite(sprite_index))
			{
				wxMessageBox(std::to_string(value));
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
	ctrl->GetGrid()->Thaw();
}
