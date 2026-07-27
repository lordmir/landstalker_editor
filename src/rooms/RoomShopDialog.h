#ifndef _ROOM_SHOP_DIALOG_H_
#define _ROOM_SHOP_DIALOG_H_

#include <memory>
#include <utility>
#include <vector>

#include <wx/dialog.h>

#include <landstalker/main/GameData.h>
#include <script/ScriptTableTreeEditorCtrl.h>

class wxListBox;
class wxButton;
class wxCheckBox;
class wxSpinCtrl;
class wxSpinCtrlDouble;
class wxStaticText;
class wxPanel;

// Focused shop editor for one room, launched from the room editor (overlay link / toolbar). The left
// pane lists the room's shop entry (Shop.room == room) and its custom item handlers (Item.shop ==
// room); the right pane hosts the reusable ScriptTableTreeEditorCtrl (list hidden) editing the
// selected entry's action script, plus the shop's markup spins or the custom item's optional-data
// controls. Add/Remove manage the underlying shop/item tables, one shop per room and one custom item
// per (room, item id). See [[cutscene-two-layer-architecture]].
class RoomShopDialog : public wxDialog
{
public:
	RoomShopDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd, int room);

private:
	// A left-pane entry: which table it lives in and its index there.
	struct Entry { ScriptTableTreeCategory category; int index; };

	void Rebuild(int select_row = 0);
	void ShowSelected();
	void UpdateButtons();
	int RoomShopIndex() const;                 // shop-table index for this room, or -1
	bool HasItem(int item_id) const;           // a custom item for (this room, item_id) already exists?

	void OnListSelect();
	void OnAddShop();
	void OnAddItem();
	void OnRemove();
	void OnMarkupChange();
	void OnExtraToggle();
	void OnExtraDataChange();
	void OnOk(wxCommandEvent& evt);

	std::shared_ptr<Landstalker::GameData> m_gd;
	int m_room;
	std::vector<Entry> m_entries;              // parallel to the list rows

	wxListBox* m_list = nullptr;
	wxButton* m_add_shop = nullptr;
	wxButton* m_add_item = nullptr;
	wxButton* m_remove = nullptr;
	ScriptTableTreeEditorCtrl* m_tree = nullptr;

	wxPanel* m_shop_panel = nullptr;           // markup controls (shown for a shop entry)
	wxSpinCtrlDouble* m_markup = nullptr;
	wxSpinCtrlDouble* m_lifestock_markup = nullptr;

	wxPanel* m_item_panel = nullptr;           // optional-data controls (shown for an item entry)
	wxStaticText* m_item_label = nullptr;
	wxCheckBox* m_extra_check = nullptr;
	wxSpinCtrl* m_extra_data = nullptr;
};

#endif // _ROOM_SHOP_DIALOG_H_
