#ifndef _STRING_EDITOR_FRAME_H_
#define _STRING_EDITOR_FRAME_H_

#include "EditorFrame.h"
#include "GameData.h"
#include "ResizeableGrid.h"


class StringEditorFrame : public EditorFrame
{
public:
	enum class Mode : uint8_t
	{
		MODE_MAIN,
		MODE_CHARS,
		MODE_SPECIAL_CHARS,
		MODE_DEFAULT_CHAR,
		MODE_ITEMS,
		MODE_MENU,
		MODE_INTRO,
		MODE_END_CREDITS,
		MODE_SYSTEM
	};

	StringEditorFrame(wxWindow* parent);
	virtual ~StringEditorFrame();

	Mode GetMode() const { return m_mode; }
	void SetMode(Mode mode);
	void Update();

	virtual void SetGameData(std::shared_ptr<GameData> gd);
	virtual void ClearGameData();
private:
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

	void OnGridValueChanged(wxGridEvent& evt);

	Mode m_mode;
	mutable wxAuiManager m_mgr;
	ResizeableGrid* m_stringView;
	std::string m_title;
};

#endif // _STRING_EDITOR_FRAME_H_
