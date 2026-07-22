#ifndef _NEW_ROOM_DIALOG_H_
#define _NEW_ROOM_DIALOG_H_

#include <memory>
#include <string>
#include <vector>
#include <wx/wx.h>
#include <wx/spinctrl.h>

#include <landstalker/main/GameData.h>

class LookupChoiceControl;

// Collects everything GameData::AddRoom needs for a new room. Nothing is created until
// the caller acts on the accessors, with one exception: the New Map button creates its
// map immediately, because the map has to exist before it can be picked from the list.
// MapsChanged reports that, so a caller that cancels still knows to refresh.
class NewRoomDialog : public wxDialog
{
public:
	NewRoomDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd);
	virtual ~NewRoomDialog();

	std::string GetRoomName() const;
	std::wstring GetDisplayName() const;
	std::string GetMap() const;
	uint8_t GetTileset() const;
	uint8_t GetPalette() const;
	uint8_t GetPrimaryBlockset() const;
	uint8_t GetSecondaryBlockset() const;
	uint8_t GetFloorHeight() const;
	uint8_t GetCeilingHeight() const;
	uint8_t GetBgm() const;

	// True if a map was created from the New Map button, whatever the dialog returned.
	bool MapsChanged() const { return m_maps_changed; }

private:
	// Choices and the raw values behind them. Blockset and tileset ids are not always a
	// dense 0..N-1, so every list carries its values rather than relying on the position.
	struct Choices
	{
		wxArrayString labels;
		std::vector<uint8_t> values;
		int IndexOf(uint8_t value) const;
	};

	void BuildMapChoices();
	void BuildTilesetChoices();
	Choices BuildPrimaryBlocksetChoices(uint8_t tileset) const;
	Choices BuildSecondaryBlocksetChoices(uint8_t tileset, uint8_t primary) const;
	void RebuildBlocksetControls(uint8_t preferred_pri, uint8_t preferred_sec);
	// Swaps a lookup control for one holding a different set of choices. The control takes
	// its list at construction, so changing the list means replacing the window.
	void ReplaceControl(LookupChoiceControl*& control, const Choices& choices, int selection);
	// Copies tileset, palette, blocksets, heights and BGM from the rooms already drawing
	// the selected map, but only for the fields every one of them agrees on.
	void AdoptSettingsFromMap(const std::string& map);
	uint8_t SelectedValue(const LookupChoiceControl* control, const Choices& choices, uint8_t fallback) const;
	std::string SuggestRoomName() const;
	bool Validate();

	void OnMapChanged(wxCommandEvent& evt);
	void OnTilesetChanged(wxCommandEvent& evt);
	void OnPrimaryBlocksetChanged(wxCommandEvent& evt);
	void OnNewMap(wxCommandEvent& evt);
	void OnOk(wxCommandEvent& evt);

	std::shared_ptr<Landstalker::GameData> m_gd;
	bool m_maps_changed;

	Choices m_tilesets;
	Choices m_palettes;
	Choices m_bgms;
	Choices m_pri_blocksets;
	Choices m_sec_blocksets;
	wxArrayString m_maps;

	wxTextCtrl* m_name;
	wxTextCtrl* m_display_name;
	LookupChoiceControl* m_map;
	wxButton* m_new_map;
	LookupChoiceControl* m_tileset;
	LookupChoiceControl* m_palette;
	LookupChoiceControl* m_pri_blockset;
	LookupChoiceControl* m_sec_blockset;
	wxSpinCtrl* m_floor;
	wxSpinCtrl* m_ceiling;
	LookupChoiceControl* m_bgm;
	// The blockset controls are rebuilt when the tileset changes, so their slot in the
	// layout is kept to hand.
	wxFlexGridSizer* m_fields;
};

#endif // _NEW_ROOM_DIALOG_H_
