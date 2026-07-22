#ifndef _NEW_MAP_DIALOG_H_
#define _NEW_MAP_DIALOG_H_

#include <memory>
#include <string>
#include <wx/wx.h>
#include <wx/spinctrl.h>

#include <landstalker/main/GameData.h>

// Prompts for a map name and its six dimensions. Shared by the map manager and the
// new-room dialog, which both need to create a map from nothing.
class NewMapDialog : public wxDialog
{
public:
	NewMapDialog(wxWindow* parent, const std::string& suggested_name);

	std::string GetMapName() const;
	uint8_t GetMapWidth() const;
	uint8_t GetMapHeight() const;
	uint8_t GetHeightmapWidth() const;
	uint8_t GetHeightmapHeight() const;
	uint8_t GetHeightmapLeft() const;
	uint8_t GetHeightmapTop() const;

private:
	wxSpinCtrl* AddSpin(wxFlexGridSizer* fields, const char* label, int value, int minimum, int maximum);

	wxTextCtrl* m_name;
	wxSpinCtrl* m_map_width;
	wxSpinCtrl* m_map_height;
	wxSpinCtrl* m_heightmap_width;
	wxSpinCtrl* m_heightmap_height;
	wxSpinCtrl* m_heightmap_left;
	wxSpinCtrl* m_heightmap_top;
};

// The first unused "MapNNN" name, for pre-filling the name field.
std::string SuggestMapName(const Landstalker::RoomData& rooms);

// "Foo" becomes "FooCopy", then "FooCopy2", "FooCopy3" and so on until one is unused.
std::string SuggestMapCopyName(const Landstalker::RoomData& rooms, const std::string& source);

// Runs NewMapDialog and creates the map it describes, re-prompting if the name is
// rejected. Returns the new map's internal name, or an empty string if the user
// cancelled - in which case nothing was created.
std::string PromptCreateMap(wxWindow* parent, const std::shared_ptr<Landstalker::GameData>& gd);

// Asks for an unused map name, re-prompting until it is usable or the user cancels.
bool PromptForMapName(wxWindow* parent, const wxString& title, const Landstalker::RoomData& rooms,
	const std::string& initial, std::string& name);

// Creates a copy of an existing map, tilemap and heightmap included, under a new name.
// Returns false without changing anything if the source is unknown or the name is taken
// or malformed.
bool DuplicateMap(const std::shared_ptr<Landstalker::GameData>& gd,
	const std::string& source, const std::string& new_name);

#endif // _NEW_MAP_DIALOG_H_
