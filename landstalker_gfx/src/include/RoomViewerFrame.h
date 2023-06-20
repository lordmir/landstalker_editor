#ifndef _ROOM_VIEWER_FRAME_H_
#define _ROOM_VIEWER_FRAME_H_

#include "EditorFrame.h"
#include "RoomViewerCtrl.h"
#include "LayerControlFrame.h"
#include "EntityControlFrame.h"
#include "GameData.h"
#include <memory>

class RoomViewerFrame : public EditorFrame
{
public:

	RoomViewerFrame(wxWindow* parent, ImageList* imglst);
	virtual ~RoomViewerFrame();

	RoomViewerCtrl::Mode GetMode() const { return m_mode; }
	void SetMode(RoomViewerCtrl::Mode mode);
	void Update();

	virtual void SetGameData(std::shared_ptr<GameData> gd);
	virtual void ClearGameData();

	void SetRoomNum(uint16_t roomnum, RoomViewerCtrl::Mode mode = RoomViewerCtrl::Mode::NORMAL);
	uint16_t GetRoomNum() const { return m_roomnum; }

	bool ExportBin(const std::string& path);
	bool ExportCsv(const std::array<std::string, 3>& paths);
	bool ExportPng(const std::string& path);
	bool ImportBin(const std::string& path);
	bool ImportCsv(const std::array<std::string, 3>& paths);

	bool HandleKeyDown(unsigned int key, unsigned int modifiers);
private:
	virtual void InitStatusBar(wxStatusBar& status) const;
	virtual void UpdateStatusBar(wxStatusBar& status) const;
	virtual void InitProperties(wxPropertyGridManager& props) const;
	void RefreshLists() const;
	virtual void UpdateProperties(wxPropertyGridManager& props) const;
	void RefreshProperties(wxPropertyGridManager& props) const;
	virtual void OnPropertyChange(wxPropertyGridEvent& evt);
	virtual void InitMenu(wxMenuBar& menu, ImageList& ilist) const;
	virtual void OnMenuClick(wxMenuEvent& evt);
	void OnExportBin();
	void OnExportCsv();
	void OnExportPng();
	void OnImportBin();
	void OnImportCsv();
	void UpdateUI() const;

	void OnKeyDown(wxKeyEvent& evt);
	void OnZoomChange(wxCommandEvent& evt);
	void OnOpacityChange(wxCommandEvent& evt);
	void OnEntityUpdate(wxCommandEvent& evt);
	void OnEntitySelect(wxCommandEvent& evt);
	void OnEntityOpenProperties(wxCommandEvent& evt);
	void OnEntityAdd(wxCommandEvent& evt);
	void OnEntityDelete(wxCommandEvent& evt);
	void OnEntityMoveUp(wxCommandEvent& evt);
	void OnEntityMoveDown(wxCommandEvent& evt);
	void FireEvent(const wxEventType& e);
	void FireEvent(const wxEventType& e, const std::string& userdata);

	RoomViewerCtrl::Mode m_mode;
	mutable wxAuiManager m_mgr;
	std::string m_title;
	RoomViewerCtrl* m_roomview;
	LayerControlFrame* m_layerctrl;
	EntityControlFrame* m_entityctrl;
	std::shared_ptr<GameData> m_g;
	uint16_t m_roomnum;
	double m_zoom;

	bool m_layers_visible;
	bool m_entities_visible;

	mutable bool m_reset_props;
	mutable wxPGChoices m_palettes;
	mutable wxPGChoices m_bgms;
	mutable wxPGChoices m_tilesets;
	mutable wxPGChoices m_pri_blocksets;
	mutable wxPGChoices m_sec_blocksets;
	mutable wxPGChoices m_maps;
	mutable wxPGChoices m_rooms;
	mutable wxPGChoices m_menustrings;

	wxDECLARE_EVENT_TABLE();
};

#endif // _ROOM_VIEWER_FRAME_H_
