#include <misc/InventoryLayoutFrame.h>

wxString InventoryLayoutFrame::GetIntroText() const
{
	return "Item menu ordering. Each slot holds the item shown at that inventory position; "
		"choose <None> to leave the slot empty.";
}

wxArrayString InventoryLayoutFrame::BuildChoices() const
{
	// Choice 0 is the empty-slot sentinel; choices 1.. are the item ids in order, so an item id
	// maps to selection (id + 1) and 0xFF maps to selection 0.
	wxArrayString choices;
	choices.Alloc(ITEM_COUNT + 1);
	choices.Add("<None>");
	auto strings = m_gd->GetStringData();
	for (int i = 0; i < ITEM_COUNT; ++i)
	{
		choices.Add(wxString::Format("[%02X] %s", i, wxString(strings->GetItemDisplayName(i))));
	}
	return choices;
}

int InventoryLayoutFrame::ByteToSelection(uint8_t value) const
{
	if (value == EMPTY_SLOT)
	{
		return 0;
	}
	if (value < ITEM_COUNT)
	{
		return value + 1;
	}
	return wxNOT_FOUND;
}

uint8_t InventoryLayoutFrame::SelectionToByte(int selection) const
{
	return (selection <= 0) ? EMPTY_SLOT : static_cast<uint8_t>(selection - 1);
}

std::vector<uint8_t> InventoryLayoutFrame::GetTableBytes() const
{
	return m_gd->GetSpriteData()->GetInventoryItems();
}

void InventoryLayoutFrame::SetTableBytes(const std::vector<uint8_t>& bytes)
{
	m_gd->GetSpriteData()->SetInventoryItems(bytes);
}
