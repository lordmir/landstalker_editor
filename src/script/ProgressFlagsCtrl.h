#ifndef _PROGRESS_FLAGS_CTRL_H_
#define _PROGRESS_FLAGS_CTRL_H_

#include <memory>

#include <wx/wx.h>
#include <wx/dataview.h>

#include <landstalker/main/GameData.h>
#include <script/ProgressFlagsDataViewModel.h>

class ProgressFlagsEditorCtrl : public wxPanel
{
public:

	ProgressFlagsEditorCtrl(wxWindow* parent);
	virtual ~ProgressFlagsEditorCtrl();

	virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
	virtual void ClearGameData();

	void Open(int quest = -1, int prog = -1);
	void RefreshData();

	void AddQuest();
	void DeleteQuest();
	void AppendRow();
	void InsertRow();
	void DeleteRow();
	void MoveRowUp();
	void MoveRowDown();

	bool IsRowSelected() const;
	bool IsSelTop() const;
	bool IsSelBottom() const;
	std::pair<int, int> GetSelectedQuestProgress() const;
	const ProgressFlagsDataViewModel* GetModel() const;

private:
	void OnSelectionChange(wxDataViewEvent& evt);
	void UpdateUI();
	void UpdateButtonStates();
	void FireEvent(const wxEventType& e, const wxString& data = wxEmptyString, long numeric_data = 0, long extra_numeric_data = 0, long extra_extra_numeric_data = 0);

	wxDataViewCtrl* m_dvc_ctrl;
	ProgressFlagsDataViewModel* m_model;

	wxButton* m_append_button = nullptr;
	wxButton* m_insert_button = nullptr;
	wxButton* m_delete_button = nullptr;
	wxButton* m_move_up_button = nullptr;
	wxButton* m_move_down_button = nullptr;
	wxButton* m_new_quest_button = nullptr;
	wxButton* m_delete_quest_button = nullptr;

	std::shared_ptr<Landstalker::GameData> m_gd;

	int m_last_tooltip_item = -1;

	wxDECLARE_EVENT_TABLE();
};

#endif // _PROGRESS_FLAGS_CTRL_H_
