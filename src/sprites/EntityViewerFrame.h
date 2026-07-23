#ifndef _ENTITY_VIEWER_FRAME_H_
#define _ENTITY_VIEWER_FRAME_H_

#include <landstalker/main/GameData.h>
#include <main/EditorFrame.h>
#include <sprites/EntityViewerCtrl.h>

#include <string>
#include <vector>
#include <memory>

class wxListBox;
class wxButton;
class wxScrolledWindow;
class wxSplitterWindow;

// The entity editor: a resizable left pane lists the entities and lets them be added, removed,
// moved and renamed (the former Entity Manager, now embedded); the centre pane animates the
// selected entity and its properties appear in the shared property grid. Selection lives here
// rather than in the navigation tree, which carries a single "Entities" node.
class EntityViewerFrame : public EditorFrame
{
public:
	EntityViewerFrame(wxWindow* parent, ImageList* imglst);
	virtual ~EntityViewerFrame();

	// Opens an entity by id. A negative id keeps the current one, or falls back to the first
	// entity - the "Entities" navigation node passes -1 to mean "just show the editor".
	bool Open(int entity_id);
	virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
	virtual void ClearGameData();
	void Update();
private:
	virtual void InitProperties(wxPropertyGridManager& props) const;
	void RefreshLists() const;
	virtual void UpdateProperties(wxPropertyGridManager& props) const;
	void RefreshProperties(wxPropertyGridManager& props) const;
	virtual void OnPropertyChange(wxPropertyGridEvent& evt);
	void InitMenu(wxMenuBar& menu, ImageList& ilist) const;
	void OnMenuClick(wxMenuEvent& evt);
	void ProcessEvent(int id);
	void OnExportPropertiesYaml();
	void ExportPropertiesYaml(const std::string& filename);

	// Left-pane entity list and its management buttons.
	void PopulateEntityList();
	void SelectEntityInList(int entity_id);
	// The entity id behind the current list row, or -1 if nothing is selected.
	int SelectedListEntity() const;
	void RefreshEntityButtons();
	void OnEntitySelected();
	void OnAddEntity();
	void OnRemoveEntity();
	void OnMoveEntity(int delta);
	void OnRenameEntity();

	// Rebuilds the bottom-right stats panel for the current entity, with the sprite, palettes
	// and rooms rendered as links that navigate to those items.
	void PopulateStats();
	// Posts a navigation request for a "Category/Name" tree path.
	void NavigateTo(const wxString& path);

	mutable wxPGChoices m_hi_palettes;
	mutable wxPGChoices m_lo_palettes;
	mutable wxPGChoices m_sprites;
	mutable wxPGChoices m_sounds;
	mutable wxPGChoices m_items;
	mutable wxPGChoices m_verbs;
	mutable wxPGChoices m_probabilities;
	mutable wxPGChoices m_articles;
	mutable wxPGChoices m_empty_choices;
	mutable bool m_reset_props = false;

	mutable wxAuiManager m_mgr;
	EntityViewerCtrl* m_entity_ctrl = nullptr;
	std::shared_ptr<Landstalker::Palette> m_palette;
	int m_entity_id = -1;

	wxSplitterWindow* m_split = nullptr;
	// Set once the splitter first has a real height, so the initial 30/70 sash is applied only
	// after the pane is sized (not to the transient zero size at construction).
	bool m_sash_set = false;
	wxScrolledWindow* m_stats = nullptr;
	wxListBox* m_entity_list = nullptr;
	// The entity ids in list order, so a row maps back to an id.
	std::vector<uint8_t> m_entity_ids;
	// True while the list is being rebuilt, so programmatic selection does not re-enter Open.
	bool m_populating = false;
	wxButton* m_add = nullptr;
	wxButton* m_remove = nullptr;
	wxButton* m_move_up = nullptr;
	wxButton* m_move_down = nullptr;
	wxButton* m_rename = nullptr;
};

#endif // _ENTITY_VIEWER_FRAME_H_
