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

	bool ExportStrings(const filesystem::path& filename, StringData::Type mode);
	bool ImportStrings(const filesystem::path& filename, StringData::Type mode);
private:
	virtual void InitMenu(wxMenuBar& menu, ImageList& ilist) const;
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
