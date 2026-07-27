#ifndef _CUTSCENE_CODE_EDITOR_H_
#define _CUTSCENE_CODE_EDITOR_H_

#include <map>
#include <memory>
#include <string>
#include <utility>

#include <misc/CodeEditorCtrl.h>

namespace Landstalker { class GameData; }

// A CodeEditorCtrl for one dialogueactions handler: it collapses the cutscene-dialogue idiom to
// <PlayCutscene ...> tokens, shows the dialogue preview as an inline hint, offers a two-stage
// (kind then cutscene-script id) autocomplete, and Ctrl+Click on a token (or its hint) opens the
// cutscene script it plays. Used by the Cutscenes editor frame's right pane and the focused
// single-handler CutsceneEditorDialog so both behave identically.
class CutsceneCodeEditor : public CodeEditorCtrl
{
public:
	CutsceneCodeEditor(wxWindow* parent, wxWindowID id = wxID_ANY);

	// Stores the game data and kicks off a background harvest of the disassembly's symbols so the
	// first autocomplete / annotation pass does not block on reading the include tree.
	void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
	// Load a handler's raw asm as editable sugar text (editable=false for external / blank slots).
	void LoadAsm(const std::string& asm_text, bool editable = true);
	// Empty the control and make it read-only (nothing selected).
	void ClearAsm();
	// The current sugar expanded back to canonical asm (tokens re-expanded, previews re-attached).
	std::string GetAsm() const;

protected:
	bool IsTokenValid(const std::string& span) const override;
	std::string HintForLine(const std::string& line) const override;
	void ActivateLink(const std::string& line) override;
	void OnCharTyped(int key) override;
	void ReopenAutocomplete() override;
	void OnUserListSelect(int list_type, const wxString& text) override;

private:
	// The inline hint / re-attached comment for a token: its Labels name (cutscene-script or
	// input-script, per the token kind) if set, else the authored preview from the handler's asm.
	std::string HintFor(const std::string& kind, int id) const;
	// The one-line text of intro string `index` (its two lines joined), for <ShowIntroString> hints.
	std::string IntroStringText(int index) const;
	// The annotation for a SetFlag/ClearFlag/ToggleFlag/TestFlag <FLAG_X> line (the flag's meaning),
	// or "" if the line is not such a macro or the flag is unknown.
	std::string FlagMacroHint(const std::string& line) const;
	void OpenCutsceneScript(int script_id);
	void OpenInputScript(int sequence);
	void OpenIntroString(int index);
	void ShowKindAutocomplete();
	void ShowIdAutocomplete();
	// Offer the disassembly's constants (and, unless constants_only, its labels too) at the caret,
	// optionally restricted to names starting with name_prefix (e.g. "FLAG_" for a SetFlag operand).
	void ShowSymbolAutocomplete(bool constants_only, const std::string& name_prefix = std::string());
	// The harvested symbols for the loaded project, or nullptr if no game data / no top-level asm.
	const struct AsmSymbols* Symbols() const;

	std::shared_ptr<Landstalker::GameData> m_gd;
	// Autocomplete state. m_ac_start = the '<' stage 1 replaces from; m_ac_kind = the chosen call
	// kind; m_ac_id_start = where stage 2 inserts the id; m_ac_items = stage-2 item text -> id.
	int m_ac_start = -1;
	std::string m_ac_kind;
	int m_ac_id_start = -1;
	std::map<std::string, int> m_ac_items;
	// Where a constant/label symbol completion replaces from (the start of the word being typed).
	int m_ac_sym_start = -1;
	// (kind, id) -> authored preview hint for the handler shown; drives the inline hints and
	// re-attaches the preview comments on GetAsm.
	std::map<std::pair<std::string, int>, std::string> m_hints;
};

#endif // _CUTSCENE_CODE_EDITOR_H_
