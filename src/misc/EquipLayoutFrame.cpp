#include <misc/EquipLayoutFrame.h>

wxString EquipLayoutFrame::GetIntroText() const
{
	return "Equip menu candidates. Each row is an equipment slot (sword, armour, boots, ring) and "
		"each column an item offered there. An \"[always shown]\" entry is always selectable (the "
		"\"nothing\" placeholders); a plain entry only appears when the item is owned.";
}

wxArrayString EquipLayoutFrame::BuildChoices() const
{
	// The first ITEM_COUNT choices are the plain item ids (bit 7 clear); the next ITEM_COUNT are
	// the same items as "always shown" placeholders (bit 7 set). So a plain item id maps to
	// selection id and an always-shown one to selection (ITEM_COUNT + id).
	wxArrayString choices;
	choices.Alloc(ITEM_COUNT * 2);
	auto strings = m_gd->GetStringData();
	for (int i = 0; i < ITEM_COUNT; ++i)
	{
		choices.Add(wxString::Format("[%02X] %s", i, wxString(strings->GetItemDisplayName(i))));
	}
	for (int i = 0; i < ITEM_COUNT; ++i)
	{
		choices.Add(wxString::Format("[%02X] %s [always shown]", i, wxString(strings->GetItemDisplayName(i))));
	}
	return choices;
}

int EquipLayoutFrame::ByteToSelection(uint8_t value) const
{
	const int id = value & ~ALWAYS_SHOWN_FLAG;
	if (id >= ITEM_COUNT)
	{
		return wxNOT_FOUND;
	}
	return ((value & ALWAYS_SHOWN_FLAG) != 0) ? (ITEM_COUNT + id) : id;
}

uint8_t EquipLayoutFrame::SelectionToByte(int selection) const
{
	if (selection >= ITEM_COUNT)
	{
		return static_cast<uint8_t>((selection - ITEM_COUNT) | ALWAYS_SHOWN_FLAG);
	}
	return static_cast<uint8_t>(selection);
}

std::vector<uint8_t> EquipLayoutFrame::GetTableBytes() const
{
	return m_gd->GetSpriteData()->GetEquipInventoryLayout();
}

void EquipLayoutFrame::SetTableBytes(const std::vector<uint8_t>& bytes)
{
	m_gd->GetSpriteData()->SetEquipInventoryLayout(bytes);
}
