#ifndef _STRING_EDITOR_FRAME_H_
#define _STRING_EDITOR_FRAME_H_

#include "EditorFrame.h"
#include "GameData.h"
#include <wx/dataview.h>
#include "StringDataViewModel.h"


class StringEditorFrame : public EditorFrame
{
public:

	StringEditorFrame(wxWindow* parent);
	virtual ~StringEditorFrame();

	StringData::Type GetMode() const { return m_type; }
	void SetMode(StringData::Type mode);
	void Update();

	virtual void SetGameData(std::shared_ptr<GameData> gd);
	virtual void ClearGameData();

	bool ExportStrings(const filesystem::path& filename, StringData::Type mode);
	bool ImportStrings(const filesystem::path& filename, StringData::Type mode);
private:
	virtual void InitMenu(wxMenuBar& menu, ImageList& ilist) const;

	void InitColumns();
	void ShowMainStrings();
	void ShowCharacterStrings();
	void ShowSpecialCharacterStrings();
	void ShowDefaultCharacterString();
	void ShowItemStrings();
	void ShowMenuStrings();
	void ShowIntroStrings();
	void ShowEndingStrings();
	void ShowSystemStrings();

	void RefreshMainString(int idx);
	void RefreshCharacterString(int idx);
	void RefreshSpecialCharacterString(int idx);
	void RefreshDefaultCharacterString(int idx);
	void RefreshItemString(int idx);
	void RefreshMenuString(int idx);
	void RefreshIntroString(int idx);
	void RefreshEndingString(int idx);
	void RefreshSystemString(int idx);

	virtual void OnMenuClick(wxMenuEvent& evt);
	void OnMenuImport();
	void OnMenuExport();

	StringData::Type m_type;
	mutable wxAuiManager m_mgr;
	wxDataViewCtrl* m_stringView;
	StringDataViewModel* m_model;
	std::string m_title;
};

#endif // _STRING_EDITOR_FRAME_H_
