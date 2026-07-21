#ifndef _CHARSET_EDITOR_FRAME_H_
#define _CHARSET_EDITOR_FRAME_H_

#include <map>
#include <string>
#include <vector>
#include <wx/dataview.h>
#include <wx/notebook.h>
#include <landstalker/main/GameData.h>
#include <landstalker/text/Charset.h>
#include <main/EditorFrame.h>

class CharsetEditorFrame : public EditorFrame
{
public:
	CharsetEditorFrame(wxWindow* parent, ImageList* imglst);
	virtual ~CharsetEditorFrame();

	bool Open();
	virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
	virtual void ClearGameData();

private:
	enum class FontPage
	{
		MAIN,
		MENU,
		INTRO,
		CREDITS
	};

	wxWindow* CreateFontPage(FontPage page);
	wxWindow* CreateControlCharPage();
	wxWindow* CreateDiacriticPage();

	// The end credit font is indexed differently to the other fonts. Code 0 terminates a credit
	// line rather than naming a character, so it has no glyph and is not listed, and the game
	// draws glyph record (code - 1) - see _drawGlyph in endcredits2.asm. Every other font indexes
	// its tiles by the character code itself and has a real glyph at code 0.
	static int FirstCode(FontPage page);
	static int CodeForRow(FontPage page, int row);
	static int GlyphIndexForCode(FontPage page, int code);

	void Populate();
	void PopulateFontPage(FontPage page);
	void PopulateControlChars();
	void PopulateDiacritics();

	Landstalker::LSString::CharacterSet& GetCharsetFor(FontPage page);
	std::shared_ptr<Landstalker::Tileset> GetFontFor(FontPage page) const;
	std::vector<std::shared_ptr<Landstalker::Palette>> GetPalettesFor(FontPage page) const;
	wxBitmap RenderGlyph(const std::shared_ptr<Landstalker::Tileset>& font,
	                     const std::vector<std::shared_ptr<Landstalker::Palette>>& palettes, int glyph_index) const;

	void OnFontValueChanged(wxDataViewEvent& evt);
	void OnControlValueChanged(wxDataViewEvent& evt);
	void OnDiacriticValueChanged(wxDataViewEvent& evt);
	void OnAddDiacritic(wxCommandEvent& evt);
	void OnDeleteDiacritic(wxCommandEvent& evt);
	void RebuildDiacriticsFromGrid();
	void ApplyToGameData();

	wxNotebook* m_notebook = nullptr;
	std::map<FontPage, wxDataViewListCtrl*> m_font_views;
	wxDataViewListCtrl* m_control_view = nullptr;
	wxDataViewListCtrl* m_diacritic_view = nullptr;

	Landstalker::Charset::Charsets m_charsets;
	std::vector<std::string> m_control_names;

	mutable wxAuiManager m_mgr;
};

#endif // _CHARSET_EDITOR_FRAME_H_
