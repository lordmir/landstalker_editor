#ifndef _ROOM_VIEWER_FRAME_H_
#define _ROOM_VIEWER_FRAME_H_

#include "EditorFrame.h"
#include "LayerControlFrame.h"
#include "EntityControlFrame.h"
#include "WarpControlFrame.h"
#include "HeightmapEditorCtrl.h"
#include "Map3DEditor.h"
#include "GameData.h"
#include <memory>

class RoomViewerCtrl;

class RoomViewerFrame : public EditorFrame
{
public:
	enum class Mode : uint8_t
	{
		NORMAL,
		HEIGHTMAP,
		BACKGROUND,
		FOREGROUND
	};

	RoomViewerFrame(wxWindow* parent, ImageList* imglst);
	virtual ~RoomViewerFrame();

	Mode GetMode() const { return m_mode; }
	void SetMode(Mode mode);
	void UpdateFrame();

	virtual void SetGameData(std::shared_ptr<GameData> gd);
	virtual void ClearGameData();

	void SetRoomNum(uint16_t roomnum);
	uint16_t GetRoomNum() const { return m_roomnum; }

	bool ExportBin(const std::string& path);
	bool ExportCsv(const std::array<std::string, 3>& paths);
	bool ExportPng(const std::string& path);
	bool ImportBin(const std::string& path);
	bool ImportCsv(const std::array<std::string, 3>& paths);

	bool HandleKeyDown(unsigned int key, unsigned int modifiers);

	void ShowFlagDialog();
	void ShowChestsDialog();
	void ShowCharDialog();
	void ShowErrorDialog();
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

	void OnWarpUpdate(wxCommandEvent& evt);
	void OnWarpSelect(wxCommandEvent& evt);
	void OnWarpOpenProperties(wxCommandEvent& evt);
	void OnWarpAdd(wxCommandEvent& evt);
	void OnWarpDelete(wxCommandEvent& evt);

	void OnHeightmapUpdate(wxCommandEvent& evt);
	void OnHeightmapMove(wxCommandEvent& evt);
	void OnHeightmapSelect(wxCommandEvent& evt);
	void OnHMTypeSelect(wxCommandEvent& evt);
	void OnHMZoom(wxCommandEvent& evt);

	void OnSize(wxSizeEvent& evt);
	void OnTabChange(wxAuiNotebookEvent& evt);

	void FireEvent(const wxEventType& e);
	void FireEvent(const wxEventType& e, const std::string& userdata);

	void SetPaneSizes();

	Mode m_mode;
	mutable wxAuiManager m_mgr;
	mutable wxAuiNotebook* m_nb;
	std::string m_title;
	RoomViewerCtrl* m_roomview;
	HeightmapEditorCtrl* m_hmedit;
	Map3DEditor* m_bgedit;
	Map3DEditor* m_fgedit;
	LayerControlFrame* m_layerctrl;
	EntityControlFrame* m_entityctrl;
	WarpControlFrame* m_warpctrl;

	std::shared_ptr<GameData> m_g;
	uint16_t m_roomnum;
	double m_zoom;

	bool m_layerctrl_visible;
	bool m_entityctrl_visible;
	bool m_warpctrl_visible;

	mutable bool m_sizes_set;
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
