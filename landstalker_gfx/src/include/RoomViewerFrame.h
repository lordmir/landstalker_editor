#ifndef _ROOM_VIEWER_FRAME_H_
#define _ROOM_VIEWER_FRAME_H_

#include "EditorFrame.h"
#include "RoomViewerCtrl.h"
#include "LayerControlFrame.h"
#include "GameData.h"
#include <memory>

class RoomViewerFrame : public EditorFrame
{
public:

	RoomViewerFrame(wxWindow* parent);
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
private:
	virtual void UpdateStatusBar(wxStatusBar& status) const;
	virtual void InitProperties(wxPropertyGridManager& props) const;
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

	void OnZoomChange(wxCommandEvent& evt);
	void OnOpacityChange(wxCommandEvent& evt);
	void FireEvent(const wxEventType& e);
	void FireEvent(const wxEventType& e, const std::string& userdata);

	RoomViewerCtrl::Mode m_mode;
	mutable wxAuiManager m_mgr;
	std::string m_title;
	RoomViewerCtrl* m_roomview;
	LayerControlFrame* m_layerctrl;
	std::shared_ptr<GameData> m_g;
	uint16_t m_roomnum;
	double m_zoom;

	wxDECLARE_EVENT_TABLE();
};

#endif // _ROOM_VIEWER_FRAME_H_
