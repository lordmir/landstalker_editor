#ifndef _ROOM_MANAGER_DIALOG_H_
#define _ROOM_MANAGER_DIALOG_H_

#include <memory>
#include <string>
#include <wx/wx.h>
#include <wx/statbmp.h>

#include <landstalker/main/GameData.h>

// Manages the game's room list: add, import, export, delete, reorder and rename. Every
// operation is applied to the game data immediately and none of them can be undone, so
// the dialog closes with a single Close button rather than pretending to support Cancel.
//
// Reordering and deleting renumber every room reference in the project (see
// GameData::MoveRoom and GameData::DeleteRoom). Raw room numbers outside the project -
// hand-written asm, patches, external tools - are not covered.
class RoomManagerDialog : public wxDialog
{
public:
	// Optional operation to run as soon as the dialog opens, for the browser's quick
	// add/delete buttons. On success the dialog closes itself (reporting the added room
	// as the one to open); on failure or cancellation it stays open so the user can see
	// why and continue by hand.
	enum class InitialAction
	{
		NONE,
		ADD,
		REMOVE
	};

	RoomManagerDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd, uint16_t roomnum,
		InitialAction initial_action = InitialAction::NONE);
	virtual ~RoomManagerDialog();

	// True if any operation modified the game data, so the caller knows to refresh.
	bool HasChanges() const { return m_changed; }
	// Room the user double-clicked to open, or -1 if the dialog was simply closed.
	int GetRoomToOpen() const { return m_room_to_open; }

private:
	void RunInitialAction(InitialAction action);
	void PopulateRoomList(int select);
	void PopulateDetails();
	void PopulatePreview();
	void UpdateUI();

	// Room number behind the current selection, or -1 if nothing is selected.
	int GetSelectedRoom() const;
	// "InternalName (Display Name)", or just the internal name when no label is set.
	wxString RoomListLabel(uint16_t room) const;
	// Rooms other than the given one that draw the same map.
	std::size_t CountOtherRoomsUsingMap(uint16_t room) const;
	// "Foo" becomes "FooCopy", then "FooCopy2" and so on until one is unused.
	std::string SuggestRoomCopyName(const std::string& source) const;
	// Moves a freshly appended room to sit just below the selection. Returns where it
	// ended up, which is its original index if it could not be moved.
	int PlaceAfterSelection(uint16_t added, int selection);
	void Move(int delta);
	// Runs the three delete confirmations. False means the user backed out.
	bool ConfirmDelete(uint16_t room, bool& delete_map);

	void OnRoomSelected(wxCommandEvent& evt);
	void OnRoomActivated(wxCommandEvent& evt);
	void OnAdd(wxCommandEvent& evt);
	void OnDuplicate(wxCommandEvent& evt);
	void OnImport(wxCommandEvent& evt);
	void OnExport(wxCommandEvent& evt);
	void OnDelete(wxCommandEvent& evt);
	void OnMoveUp(wxCommandEvent& evt);
	void OnMoveDown(wxCommandEvent& evt);
	void OnRename(wxCommandEvent& evt);

	std::shared_ptr<Landstalker::GameData> m_gd;
	bool m_changed;
	int m_room_to_open;
	// Room the preview currently shows, so re-selecting it costs nothing.
	int m_previewed_room;

	wxListBox* m_room_list;
	wxStaticBitmap* m_preview;
	wxStaticText* m_preview_message;
	wxStaticText* m_detail_map;
	wxStaticText* m_detail_tileset;
	wxStaticText* m_detail_blocksets;
	wxStaticText* m_detail_palette;
	wxStaticText* m_detail_heights;
	wxStaticText* m_detail_bgm;
	wxStaticText* m_detail_entities;
	wxStaticText* m_detail_warps;
	wxButton* m_add;
	wxButton* m_duplicate;
	wxButton* m_import;
	wxButton* m_export;
	wxButton* m_delete;
	wxButton* m_move_up;
	wxButton* m_move_down;
	wxButton* m_rename;
	wxButton* m_close;
};

#endif // _ROOM_MANAGER_DIALOG_H_
