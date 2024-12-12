#ifndef _STRING_EDITOR_FRAME_H_
#define _STRING_EDITOR_FRAME_H_

#include <vector>
#include <wx/dataview.h>
#include <landstalker/main/include/GameData.h>
#include <user_interface/main/include/EditorFrame.h>
#include <user_interface/text/include/StringDataViewModel.h>


class StringEditorFrame : public EditorFrame
{
public:

	StringEditorFrame(wxWindow* parent, ImageList* imglst);
	virtual ~StringEditorFrame();

	StringData::Type GetMode() const { return m_type; }
	void SetMode(StringData::Type mode);

	virtual void SetGameData(std::shared_ptr<GameData> gd);
	virtual void ClearGameData();

	bool ExportStrings(const std::filesystem::path& filename, StringData::Type mode);
	bool ImportStrings(const std::filesystem::path& filename, StringData::Type mode);

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

	StringData::Type m_type;
	mutable wxAuiManager m_mgr;
	wxDataViewCtrl* m_stringView;
	StringDataViewModel* m_model;
	std::string m_title;

	wxDECLARE_EVENT_TABLE();
};

#endif // _STRING_EDITOR_FRAME_H_
