#ifndef _STRING_EDITOR_FRAME_H_
#define _STRING_EDITOR_FRAME_H_

#include <vector>
#include <wx/dataview.h>
#include <landstalker/main/GameData.h>
#include <main/EditorFrame.h>
#include <text/StringDataViewModel.h>


class StringEditorFrame : public EditorFrame
{
public:

	StringEditorFrame(wxWindow* parent, ImageList* imglst);
	virtual ~StringEditorFrame();

	Landstalker::StringData::Type GetMode() const { return m_type; }
	void SetMode(Landstalker::StringData::Type mode);

	virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
	virtual void ClearGameData();

	bool ExportStrings(const std::filesystem::path& filename, Landstalker::StringData::Type mode);
	bool ImportStrings(const std::filesystem::path& filename, Landstalker::StringData::Type mode);

	void UpdateUI() const;
private:
	void OnSelectionChange(wxDataViewEvent& evt);
	virtual void InitMenu(wxMenuBar& menu, ImageList& ilist) const;
	virtual void OnMenuClick(wxMenuEvent& evt);
	void OnMenuImport();
	void OnMenuExport();
	void OnAppend();
	void OnInsert();
	void OnDelete();
	void OnMoveUp();
	void OnMoveDown();

	bool IsAddRemoveAllowed() const;
	bool IsSelTop() const;
	bool IsSelBottom() const;

	Landstalker::StringData::Type m_type;
	mutable wxAuiManager m_mgr;
	wxDataViewCtrl* m_stringView;
	StringDataViewModel* m_model;
	std::string m_title;

	wxDECLARE_EVENT_TABLE();
};

#endif // _STRING_EDITOR_FRAME_H_
