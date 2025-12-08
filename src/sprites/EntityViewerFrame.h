#ifndef _ENTITY_VIEWER_FRAME_H_
#define _ENTITY_VIEWER_FRAME_H_

#include <landstalker/main/GameData.h>
#include <main/EditorFrame.h>
#include <sprites/EntityViewerCtrl.h>

#include <string>
#include <memory>

class EntityViewerFrame : public EditorFrame
{
public:
	EntityViewerFrame(wxWindow* parent, ImageList* imglst);
	virtual ~EntityViewerFrame();

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
	void FireRenameNavItemEvent(const std::wstring& old_name, const std::wstring& new_name);
	void InitMenu(wxMenuBar& menu, ImageList& ilist) const;
	void OnMenuClick(wxMenuEvent& evt);
	void ProcessEvent(int id);
	void OnExportPropertiesYaml();
	void ExportPropertiesYaml(const std::string& filename);

	mutable wxPGChoices m_hi_palettes;
	mutable wxPGChoices m_lo_palettes;
	mutable wxPGChoices m_sprites;
	mutable wxPGChoices m_sounds;
	mutable wxPGChoices m_items;
	mutable wxPGChoices m_verbs;
	mutable wxPGChoices m_probabilities;
	mutable wxPGChoices m_empty_choices;
	mutable bool m_reset_props = false;

	mutable wxAuiManager m_mgr;
	EntityViewerCtrl* m_entity_ctrl = nullptr;
	std::shared_ptr<Landstalker::Palette> m_palette;
	int m_entity_id = -1;
};

#endif // _ENTITY_VIEWER_FRAME_H_
