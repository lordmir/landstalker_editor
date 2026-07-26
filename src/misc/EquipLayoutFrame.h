#ifndef _EQUIP_LAYOUT_FRAME_H_
#define _EQUIP_LAYOUT_FRAME_H_

#include <misc/SlotGridEditorFrame.h>

// Editor for the equip-menu candidate table (EquipInventoryLayout / inventoryequip.bin). The
// table is a fixed 4-row (sword / armour / boots / ring) x 5-column grid of item ids - one byte
// per slot, read row-major (index = row * 5 + column). Bit 7 of a byte marks an "always shown"
// placeholder: a real item is only offered when owned, but a bit-7 entry (the No-Sword / No-Armour
// / ... "nothing" choices) is always available. The item id lives in bits 0-6.
class EquipLayoutFrame : public SlotGridEditorFrame
{
public:
	EquipLayoutFrame(wxWindow* parent, ImageList* imglst)
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
	// 4 rows (equip slots) x 5 columns (candidates) = 20 slots, matching the base ROM table.
	static constexpr std::size_t ROWS = 4;
	static constexpr std::size_t COLS = 5;
	// Item ids the pickers can represent; matches the engine's 0x00..0x3F item range.
	static constexpr int ITEM_COUNT = 64;
	// Bit 7 marks the "always shown" placeholder entries.
	static constexpr uint8_t ALWAYS_SHOWN_FLAG = 0x80;
};

#endif // _EQUIP_LAYOUT_FRAME_H_
