#ifndef _SLOT_GRID_EDITOR_FRAME_H_
#define _SLOT_GRID_EDITOR_FRAME_H_

#include <cstdint>
#include <vector>

#include <wx/aui/aui.h>

#include <landstalker/main/GameData.h>
#include <main/EditorFrame.h>

class wxScrolledWindow;
class LookupChoiceControl;

// Shared base for the small fixed-size "grid of item pickers" data editors (the inventory item
// ordering and the equip-menu layout). It lays out a rows x cols grid of searchable pickers with
// row/column headers and keeps them in sync with a flat byte table, read row-major. Subclasses
// supply the table geometry, the picker choices, the byte<->choice mapping and where the bytes
// live in the game data; the base owns the widgets, loading and change handling.
class SlotGridEditorFrame : public EditorFrame
{
public:
	SlotGridEditorFrame(wxWindow* parent, ImageList* imglst);
	virtual ~SlotGridEditorFrame();

	bool Open();
	virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
	virtual void ClearGameData();

protected:
	// --- Configuration supplied by subclasses ---
	virtual std::size_t GetRows() const = 0;
	virtual std::size_t GetCols() const = 0;
	// One-line description shown above the grid.
	virtual wxString GetIntroText() const = 0;
	// The picker entries, in selection-index order. Called with m_gd non-null.
	virtual wxArrayString BuildChoices() const = 0;
	// Maps a stored table byte to a picker selection index, or wxNOT_FOUND when no entry
	// represents it (the picker is then left blank and the byte untouched until edited).
	virtual int ByteToSelection(uint8_t value) const = 0;
	// Maps a picker selection index back to the stored table byte.
	virtual uint8_t SelectionToByte(int selection) const = 0;
	// The table bytes in the game data (row-major).
	virtual std::vector<uint8_t> GetTableBytes() const = 0;
	virtual void SetTableBytes(const std::vector<uint8_t>& bytes) = 0;

private:
	void RebuildGrid();
	void LoadValues();
	void OnSlotChanged(wxCommandEvent& evt);

	wxAuiManager m_mgr;
	wxScrolledWindow* m_panel = nullptr;
	std::vector<LookupChoiceControl*> m_slots; // row-major, size rows*cols once built
};

#endif // _SLOT_GRID_EDITOR_FRAME_H_
