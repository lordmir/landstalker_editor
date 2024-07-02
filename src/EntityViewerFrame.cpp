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
		auto palette_idxs = m_gd->GetSpriteData()->GetSpritePaletteIdxs(entity);
		m_palette = m_gd->GetSpriteData()->GetSpritePalette(palette_idxs.first, palette_idxs.second);
		m_entity_id = entity;

		m_entity_ctrl->Open(entity, m_palette);
		m_reset_props = true;
		FireEvent(EVT_PROPERTIES_UPDATE);
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
		EditorFrame::InitProperties(props);
		RefreshProperties(props);
	}
}

void EntityViewerFrame::RefreshLists() const
{
	if (m_gd && m_entity_id != -1)
	{
		auto epals = m_gd->GetSpriteData()->GetSpritePaletteIdxs(m_entity_id);
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
		props.GetGrid()->GetProperty("Sprite")->SetChoiceSelection(sprite_index);
		props.GetGrid()->SetPropertyValue("Sprite ID", sprite_index);
		props.GetGrid()->GetProperty("Low Palette")->SetChoices(m_lo_palettes);
		props.GetGrid()->GetProperty("High Palette")->SetChoices(m_hi_palettes);
		props.GetGrid()->GetProperty("Low Palette")->SetChoiceSelection(sd->GetSpritePaletteIdxs(m_entity_id).first + 1);
		props.GetGrid()->GetProperty("High Palette")->SetChoiceSelection(sd->GetSpritePaletteIdxs(m_entity_id).second + 1);
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
	if (name == "Sprite ID")
	{
		int value = property->GetValuePlain().GetLong();
		if (value != sprite_index)
		{
			sprite_index = std::clamp<uint8_t>(value, 0, 255);
			if (sd->IsSprite(sprite_index))
			{
				/// sd->SetSpriteIndex(...)
				FireEvent(EVT_PROPERTIES_UPDATE);
			}
		}
	}
	else if (name == "Low Palette" || name == "High Palette")
	{
		std::vector<std::string> palettes;
		int lo_pal = ctrl->GetGrid()->GetPropertyByName("Low Palette")->GetValue().GetLong();
		int hi_pal = ctrl->GetGrid()->GetPropertyByName("High Palette")->GetValue().GetLong();
		//SetActivePalette(palettes);
	}
	ctrl->GetGrid()->Thaw();
}
