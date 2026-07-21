#ifndef _MAP_MANAGER_DIALOG_H_
#define _MAP_MANAGER_DIALOG_H_

#include <memory>
#include <string>
#include <vector>
#include <wx/wx.h>
#include <wx/statbmp.h>

#include <landstalker/main/GameData.h>

// Manages the game's list of 3D maps: add, import, export, remove, reorder and rename.
// RoomData applies each operation immediately and offers no rollback, so the dialog
// closes with a single Close button rather than pretending to support Cancel.
class MapManagerDialog : public wxDialog
{
public:
	MapManagerDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd, uint16_t roomnum);
	virtual ~MapManagerDialog();

	// True if any operation modified the game data, so the caller knows to refresh.
	bool HasChanges() const { return m_changed; }
	// Room the user double-clicked to open, or -1 if the dialog was simply closed.
	int GetRoomToOpen() const { return m_room_to_open; }

private:
	void PopulateMapList(const std::string& select);
	void PopulateRoomList();
	void PopulateDetails();
	void PopulatePreview();
	void UpdateUI();
	// Room number behind the current room list selection, or -1 if nothing is selected.
	int GetSelectedRoom() const;

	std::string GetSelectedMap() const;
	int GetSelectedMapIndex() const;
	// "InternalName (Display Name)", or just the internal name when no label is set.
	wxString MapListLabel(const std::string& name) const;
	bool CanDeleteSelectedMap() const;
	// Index of the first room drawing the given map, or -1 when nothing uses it.
	int FirstRoomUsing(const std::string& map) const;
	std::string SuggestMapName() const;
	// Asks for an internal name for a new map, re-prompting until it is usable or cancelled.
	bool PromptForName(const wxString& title, const std::string& initial, std::string& name);
	void Move(int delta);

	void OnMapSelected(wxCommandEvent& evt);
	void OnRoomSelected(wxCommandEvent& evt);
	void OnRoomActivated(wxCommandEvent& evt);
	void OnAdd(wxCommandEvent& evt);
	void OnImport(wxCommandEvent& evt);
	void OnExport(wxCommandEvent& evt);
	void OnRemove(wxCommandEvent& evt);
	void OnMoveUp(wxCommandEvent& evt);
	void OnMoveDown(wxCommandEvent& evt);
	void OnRename(wxCommandEvent& evt);

	std::shared_ptr<Landstalker::GameData> m_gd;
	uint16_t m_roomnum;
	bool m_changed;
	int m_room_to_open;
	// Room numbers behind each entry of the room list, in the same order.
	std::vector<uint16_t> m_listed_rooms;
	// Room the preview currently shows, so re-selecting it costs nothing.
	int m_previewed_room;

	wxListBox* m_map_list;
	wxListBox* m_room_list;
	wxStaticBitmap* m_preview;
	wxStaticText* m_preview_message;
	wxStaticText* m_detail_size;
	wxStaticText* m_detail_heightmap;
	wxStaticText* m_detail_tileset;
	wxStaticText* m_detail_blocksets;
	wxStaticText* m_detail_palette;
	wxButton* m_add;
	wxButton* m_import;
	wxButton* m_export;
	wxButton* m_remove;
	wxButton* m_move_up;
	wxButton* m_move_down;
	wxButton* m_rename;
	wxButton* m_close;
};

#endif // _MAP_MANAGER_DIALOG_H_
