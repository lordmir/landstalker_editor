#ifndef _CODE_EDITOR_CTRL_H_
#define _CODE_EDITOR_CTRL_H_

#include <string>

#include <wx/stc/stc.h>

// A reusable monospace m68k code editor built on wxStyledTextCtrl: assembler syntax highlighting,
// auto-indent, and a framework for "sugar" tokens written as <...> spans - inline hint annotations,
// valid/invalid colouring, Ctrl+Click links, Ctrl+Space / typed-character autocomplete. A plain
// instance is just a highlighted asm editor; subclasses override the customisation points to give
// the tokens meaning (e.g. CutsceneCodeEditor's <PlayCutscene ...>). Designed to be shared by any
// editor that edits a snippet of the disassembly.
class CodeEditorCtrl : public wxStyledTextCtrl
{
public:
	CodeEditorCtrl(wxWindow* parent, wxWindowID id = wxID_ANY);

	// Replace the content; editable=false makes it read-only (e.g. an external / empty snippet).
	void LoadCode(const wxString& text, bool editable = true);
	void ClearCode();
	bool IsDirty() const { return GetModify(); }
	void MarkClean() { SetSavePoint(); }

protected:
	// Indicator numbers (container range) for the <...> token colouring, and the style index (above
	// the ASM lexer's styles) for the inline hints - available to subclasses building on the tokens.
	static constexpr int IND_TOKEN_OK = 8;
	static constexpr int IND_TOKEN_ERR = 9;
	static constexpr int HINT_STYLE = 40;

	// Re-run the decorations (hints + token colouring) now. Called after a load and, coalesced via
	// CallAfter, after each edit.
	void RefreshDecorations();

	// ---- customisation points (defaults give a plain asm editor with no tokens or links) ----
	// Whether a <...> span (including the angle brackets) is a valid token, for colouring.
	virtual bool IsTokenValid(const std::string& /*span*/) const { return false; }
	// The inline hint to show at the end of this line, or empty for none.
	virtual std::string HintForLine(const std::string& /*line*/) const { return {}; }
	// Ctrl+Click landed on this line - follow whatever it links to.
	virtual void ActivateLink(const std::string& /*line*/) {}
	// A character was just typed (CHARADDED, after auto-indent handling) - e.g. open autocomplete.
	virtual void OnCharTyped(int /*key*/) {}
	// Ctrl+Space - (re)open autocomplete based on the caret context.
	virtual void ReopenAutocomplete() {}
	// A user-list (autocomplete) item was chosen.
	virtual void OnUserListSelect(int /*list_type*/, const wxString& /*text*/) {}

private:
	void Style();
	void ApplyHints();
	void ColourTokens();
	void AutoIndentNewLine();
	void OnModified(wxStyledTextEvent& evt);
	void OnCharAdded(wxStyledTextEvent& evt);
	void OnUserListSelection(wxStyledTextEvent& evt);
	void OnKeyDown(wxKeyEvent& evt);
	void OnLeftDown(wxMouseEvent& evt);

	// True while LoadCode is populating, to suppress the edit-driven decoration refresh.
	bool m_loading = false;
	// True while a deferred decoration refresh is queued, so bursts of edits coalesce into one.
	bool m_refresh_pending = false;
};

#endif // _CODE_EDITOR_CTRL_H_
