#ifndef _PROGRESS_FLAGS_CTRL_H_
#define _PROGRESS_FLAGS_CTRL_H_

#include <memory>

#include <wx/wx.h>
#include <wx/dataview.h>

#include <landstalker/main/include/GameData.h>
#include <user_interface/script/include/ProgressFlagsDataViewModel.h>

class ProgressFlagsEditorCtrl : public wxPanel
{
public:

	ProgressFlagsEditorCtrl(wxWindow* parent);
	virtual ~ProgressFlagsEditorCtrl();

	virtual void SetGameData(std::shared_ptr<GameData> gd);
	virtual void ClearGameData();

	void Open(int quest = -1, int prog = -1);
	void RefreshData();

	void AppendRow();
	void InsertRow();
	void DeleteRow();
	void MoveRowUp();
	void MoveRowDown();

	bool IsRowSelected() const;
	bool IsSelTop() const;
	bool IsSelBottom() const;

private:
	void OnSelectionChange(wxDataViewEvent& evt);
	void UpdateUI();
	void FireEvent(const wxEventType& e, const wxString& data = wxEmptyString, long numeric_data = 0, long extra_numeric_data = 0, long extra_extra_numeric_data = 0);

	wxDataViewCtrl* m_dvc_ctrl;
	ProgressFlagsDataViewModel* m_model;

	std::shared_ptr<GameData> m_gd;

	int m_last_tooltip_item = -1;

	wxDECLARE_EVENT_TABLE();
};

#endif // _PROGRESS_FLAGS_CTRL_H_
