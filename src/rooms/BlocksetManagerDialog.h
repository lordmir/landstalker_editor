#ifndef _BLOCKSET_MANAGER_DIALOG_H_
#define _BLOCKSET_MANAGER_DIALOG_H_

#include <memory>
#include <string>
#include <wx/wx.h>
#include <wx/statbmp.h>
#include <wx/treectrl.h>

#include <landstalker/main/GameData.h>

// Manages the game's blocksets: add, import, export, remove, reorder and rename.
//
// Blocksets are grouped by (tileset, primary set). Within a group the first entry is the
// base every room in the group draws, and the rest are alternates a room chooses between.
// The tree collapses the set level for the tilesets that only have one, which is most of
// them, and shows it only where a second set actually exists.
//
// RoomData applies each operation immediately and offers no rollback, so the dialog closes
// with a single Close button rather than pretending to support Cancel.
class BlocksetManagerDialog : public wxDialog
{
public:
	// Optional operation to run as soon as the dialog opens, for the browser's quick
	// add/delete buttons. On success the dialog closes itself (reporting the added
	// blockset as the one to open); on failure or cancellation it stays open so the user
	// can see why and continue by hand.
	enum class InitialAction
	{
		NONE,
		ADD,
		REMOVE
	};

	BlocksetManagerDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd,
		const std::string& select, InitialAction initial_action = InitialAction::NONE);
	virtual ~BlocksetManagerDialog();

	// True if any operation modified the game data, so the caller knows to refresh.
	bool HasChanges() const { return m_changed; }
	// Name the user double-clicked to open, or empty if the dialog was simply closed.
	std::string GetBlocksetToOpen() const { return m_to_open; }

private:
	// What a tree node stands for. The tree mixes three kinds of node, and most operations
	// only apply to one of them.
	enum class Kind
	{
		TILESET,
		GROUP,
		BLOCKSET
	};

	struct Selection
	{
		bool valid = false;
		Kind kind = Kind::TILESET;
		uint8_t tileset = 0;
		uint8_t primary = 0;
		// Slot within the group; only meaningful for a BLOCKSET node.
		uint8_t sec = 0;
		std::string name;
		// True when this blockset is its group's base rather than one of the alternates.
		bool is_base = false;
	};

	void PopulateTree(const std::string& select);
	void PopulateDetails();
	void PopulatePreview();
	void UpdateUI();
	Selection GetSelection() const;
	// Blocks a room in this group can address in total, and the ceiling it is measured
	// against. The base is drawn by every room, so it is counted against every alternate.
	std::pair<std::size_t, std::size_t> CombinedBudget(uint8_t tileset, uint8_t primary, uint8_t sec) const;
	// Palette a blockset is best previewed with: its tileset's default, else the first room one.
	std::shared_ptr<Landstalker::Palette> PreviewPalette(uint8_t tileset) const;
	void Move(int delta);
	// Names the rooms blocking a delete, for the refusal message.
	wxString DescribeRooms(const std::vector<uint16_t>& rooms) const;

	void RunInitialAction(InitialAction action);
	void OnSelectionChanged(wxTreeEvent& evt);
	void OnItemActivated(wxTreeEvent& evt);
	void OnAdd(wxCommandEvent& evt);
	void OnAddSet(wxCommandEvent& evt);
	void OnImport(wxCommandEvent& evt);
	void OnExport(wxCommandEvent& evt);
	void OnRemove(wxCommandEvent& evt);
	void OnMoveUp(wxCommandEvent& evt);
	void OnMoveDown(wxCommandEvent& evt);
	void OnRename(wxCommandEvent& evt);

	std::shared_ptr<Landstalker::GameData> m_gd;
	bool m_changed;
	std::string m_to_open;
	// Name the preview currently shows, so re-selecting it costs nothing.
	std::string m_previewed;

	wxTreeCtrl* m_tree;
	wxStaticBitmap* m_preview;
	wxStaticText* m_preview_message;
	static constexpr std::size_t DETAIL_ROWS = 5;
	wxStaticText* m_detail_captions[DETAIL_ROWS];
	wxStaticText* m_detail_values[DETAIL_ROWS];
	void SetDetail(std::size_t row, const wxString& caption, const wxString& value);
	wxButton* m_add;
	wxButton* m_add_set;
	wxButton* m_import;
	wxButton* m_export;
	wxButton* m_remove;
	wxButton* m_move_up;
	wxButton* m_move_down;
	wxButton* m_rename;
	wxButton* m_close;
};

#endif // _BLOCKSET_MANAGER_DIALOG_H_
