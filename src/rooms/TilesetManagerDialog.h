#ifndef _TILESET_MANAGER_DIALOG_H_
#define _TILESET_MANAGER_DIALOG_H_

#include <memory>
#include <string>
#include <wx/wx.h>
#include <wx/statbmp.h>
#include <wx/treectrl.h>

#include <landstalker/main/GameData.h>

// Manages the game's tilesets: add, import, export, remove, reorder and rename, along with
// the animations belonging to each one. Animated tilesets are shown nested under the
// tileset they animate, matching the main navigation pane.
//
// RoomData applies each operation immediately and offers no rollback, so the dialog closes
// with a single Close button rather than pretending to support Cancel.
class TilesetManagerDialog : public wxDialog
{
public:
	TilesetManagerDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd,
		const std::string& select);
	virtual ~TilesetManagerDialog();

	// True if any operation modified the game data, so the caller knows to refresh.
	bool HasChanges() const { return m_changed; }
	// Name the user double-clicked to open, or empty if the dialog was simply closed.
	std::string GetTilesetToOpen() const { return m_to_open; }
	// True when the item to open is an animated tileset rather than a plain one.
	bool IsTilesetToOpenAnimated() const { return m_to_open_animated; }

private:
	// What the tree selection currently refers to. Most operations only apply to one kind,
	// so resolving this once keeps every handler from re-deriving it.
	struct Selection
	{
		bool valid = false;
		bool animated = false;
		// Slot of the tileset itself, or of an animation's parent.
		uint8_t tileset = 0;
		// Animation slot within the parent; only meaningful when animated.
		uint8_t anim = 0;
		std::string name;
	};

	// Records that the project changed and brings GameData's name lookups back in step.
	// Those caches are built when the project is opened, so without this a tileset added
	// here cannot be found by name afterwards and a renamed one answers to its old name.
	void MarkChanged();
	void PopulateTree(const std::string& select);
	void PopulateDetails();
	void PopulatePreview();
	void UpdateUI();
	Selection GetSelection() const;
	// Palette a tileset is best previewed with: its own default, else the first room one.
	std::shared_ptr<Landstalker::Palette> PreviewPalette(const Landstalker::PalettePreferences& entry) const;
	void Move(int delta);

	void OnSelectionChanged(wxTreeEvent& evt);
	void OnItemActivated(wxTreeEvent& evt);
	void OnAdd(wxCommandEvent& evt);
	void OnAddAnimation(wxCommandEvent& evt);
	void OnImport(wxCommandEvent& evt);
	void OnExport(wxCommandEvent& evt);
	void OnRemove(wxCommandEvent& evt);
	void OnMoveUp(wxCommandEvent& evt);
	void OnMoveDown(wxCommandEvent& evt);
	void OnRename(wxCommandEvent& evt);

	std::shared_ptr<Landstalker::GameData> m_gd;
	bool m_changed;
	std::string m_to_open;
	bool m_to_open_animated;
	// Name the preview currently shows, so re-selecting it costs nothing.
	std::string m_previewed;

	wxTreeCtrl* m_tree;
	wxStaticBitmap* m_preview;
	wxStaticText* m_preview_message;
	// Tilesets and animations are described by different things, so each row's caption is
	// retitled along with its value rather than showing one kind's headings for the other.
	static constexpr std::size_t DETAIL_ROWS = 6;
	wxStaticText* m_detail_captions[DETAIL_ROWS];
	wxStaticText* m_detail_values[DETAIL_ROWS];
	// Sets one row's caption and value, or blanks it when the row is unused.
	void SetDetail(std::size_t row, const wxString& caption, const wxString& value);
	wxButton* m_add;
	wxButton* m_add_animation;
	wxButton* m_import;
	wxButton* m_export;
	wxButton* m_remove;
	wxButton* m_move_up;
	wxButton* m_move_down;
	wxButton* m_rename;
	wxButton* m_close;
};

#endif // _TILESET_MANAGER_DIALOG_H_
