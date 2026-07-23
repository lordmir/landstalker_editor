#ifndef _PALETTE_LIST_FRAME_H_
#define _PALETTE_LIST_FRAME_H_

#include <wx/dataview.h>
#include <palettes/DataViewCtrlPaletteModel.h>
#include <palettes/DataViewCtrlPaletteRenderer.h>
#include <main/EditorFrame.h>
#include <landstalker/main/GameData.h>

class PaletteListFrame : public EditorFrame
{
public:
	enum class Mode : uint8_t
	{
		ROOM,
		ROOM_MISC,
		SPRITE_LO,
		SPRITE_HI,
		PROJECTILE,
		EQUIP,
		MISC
	};

	PaletteListFrame(wxWindow* parent, ImageList* imglst);
	virtual ~PaletteListFrame();

	Mode GetMode() const { return m_mode; }
	void SetMode(Mode mode);
	void Update();

	virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
	virtual void ClearGameData();

	bool ExportAllPalettes(const std::filesystem::path& filename);
	bool ImportPalettes(const std::filesystem::path& filename);
private:
	virtual void UpdateStatusBar(wxStatusBar& status, wxCommandEvent& evt) const;
	virtual void InitMenu(wxMenuBar& menu, ImageList& ilist) const;
	virtual void OnMenuClick(wxMenuEvent& evt);

	void OnMouseMove(wxMouseEvent& evt);
	void OnMouseLeave(wxMouseEvent& evt);
	void OnKeyPress(wxKeyEvent& evt);
	void OnMenuImport();
	void OnMenuExport();

	// Only the flat, editable lists (room and sprite low/high palettes) get the add/remove/move
	// buttons; the others are fixed-shape and stay button-less, as the derived data they hold
	// cannot be reordered independently.
	bool IsEditableMode() const;
	std::size_t GetPaletteCount() const;
	// The row the palette buttons act on - the last one the mouse hovered, since the list
	// selects on hover so a single click still edits a swatch.
	int GetSelectedRow() const;
	void SelectRow(int row);
	void UpdatePaletteButtons();
	void OnAddPalette();
	void OnRemovePalette();
	void OnMovePalette(int delta);
	void OnRenamePalette();
	// The label category and display name for the current editable mode.
	const std::wstring& PaletteLabelCategory() const;
	std::wstring PaletteDisplayName(int row) const;

	Mode m_mode;
	mutable wxAuiManager m_mgr;
	wxDataViewCtrl* m_list;
	DataViewCtrlPaletteModel* m_model;
	DataViewCtrlPaletteRenderer* m_renderer;
	mutable wxDataViewItem m_prev_itm;
	mutable int m_prev_colour;
	std::string m_title;

	wxPanel* m_button_panel;
	wxButton* m_add;
	wxButton* m_remove;
	wxButton* m_move_up;
	wxButton* m_move_down;
	wxButton* m_rename;
};

#endif // _PALETTE_LIST_FRAME_H_
