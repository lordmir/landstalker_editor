#ifndef _INVENTORY_LAYOUT_FRAME_H_
#define _INVENTORY_LAYOUT_FRAME_H_

#include <misc/SlotGridEditorFrame.h>

// Editor for the item-menu ordering table (InventoryItems / inventoryitems.bin). The table is
// a fixed 20-row, 2-column grid of item ids - one byte per slot, read row-major - that decides
// where each item sits in the inventory screen. 0xFF marks an empty slot the game skips, shown
// here as "<None>". Each slot is a searchable item picker.
class InventoryLayoutFrame : public SlotGridEditorFrame
{
public:
	InventoryLayoutFrame(wxWindow* parent, ImageList* imglst)
		: SlotGridEditorFrame(parent, imglst) {}

protected:
	std::size_t GetRows() const override { return ROWS; }
	std::size_t GetCols() const override { return COLS; }
	wxString GetIntroText() const override;
	wxArrayString BuildChoices() const override;
	int ByteToSelection(uint8_t value) const override;
	uint8_t SelectionToByte(int selection) const override;
	std::vector<uint8_t> GetTableBytes() const override;
	void SetTableBytes(const std::vector<uint8_t>& bytes) override;

private:
	// 20 rows x 2 columns = 40 slots, matching the fixed table size in every base ROM.
	static constexpr std::size_t ROWS = 20;
	static constexpr std::size_t COLS = 2;
	// Item ids the pickers can represent; matches the engine's 0x00..0x3F item range (unnamed
	// ids still get a generic fallback label from GetItemDisplayName).
	static constexpr int ITEM_COUNT = 64;
	// Sentinel byte for an empty slot; choice index 0 ("<None>") maps to it.
	static constexpr uint8_t EMPTY_SLOT = 0xFF;
};

#endif // _INVENTORY_LAYOUT_FRAME_H_
