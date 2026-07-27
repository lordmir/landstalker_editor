#ifndef _CUTSCENE_ASM_SUGAR_H_
#define _CUTSCENE_ASM_SUGAR_H_

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// Two-way sugar for the "load an id into d0, then call a known subroutine" idioms in the action
// handlers (dialogueactions.asm etc.). Each idiom renders as a compact <Kind $id> token the editor
// shows instead of the raw two/three-line m68k, and expands back to canonical asm on save:
//
//   move.w #$0009,d0 / bsr(.s|.w) LoadCutsceneDialogue        <->  <PlayCutscene $009>
//   move.w #$000F,d0 / bra(.s|.w) LoadCutsceneDialogue        <->  <PlayCutsceneAndReturn $00F>
//   move.w #$010E,d0 / move.w #90,d1 / bsr ShowCutsceneDialogueAndWait
//                                                             <->  <PlayCutsceneAndWait $10E, 90>
//   move.b #$08,d0   / bsr(.s|.w) PlaybackInput               <->  <Playback $008>
//   move.b #$0C,(g_IntroStringToDisplay).l                    <->  <ShowIntroString $00C>
//
// Two idiom shapes are supported: a "Call" (load an id into d0, optional wait in d1, then bra/bsr a
// known subroutine) and a "Store" (move an immediate id straight into an absolute memory symbol).
//
// New idioms are added by extending the internal spec table. The bar is a binary-exact assemble,
// not source-exact (see [[dialogueactions-asm-format]]), so ToAsm emits canonical `.w` branch forms;
// the editor only ever round-trips a handler the user actually edited (unmodified handlers keep
// their verbatim text), so this canonicalisation never touches untouched code. A collapse only
// happens when the d0-move size matches the idiom's, so the expansion is exactly reversible.
// Anything not matching an idiom passes through verbatim.
namespace CutsceneSugar
{
// A parsed sugar token: its kind (e.g. "PlayCutscene", "Playback"), the id argument, and an optional
// extra argument (the wait count for PlayCutsceneAndWait; -1 when the kind has none).
struct Token
{
	std::string kind;
	int id = 0;
	int extra = -1;
};

// The token kind names, in the order a picker should offer them.
const std::vector<std::string>& Kinds();

// Collapse the call idioms to bare <Kind ...> tokens (no trailing comment - the editor shows the
// description as an inline hint instead).
std::string ToSugar(const std::string& asm_text);

// Expand the tokens back to canonical asm. describe(token) supplies an optional comment to attach to
// each expanded call (e.g. the dialogue preview); return "" for none.
std::string ToAsm(const std::string& sugar_text,
	const std::function<std::string(const Token&)>& describe = {});

// (kind, id) -> authored inline comment for every collapsible call in a handler's raw asm. Used to
// show inline hints and to re-attach comments on save.
std::map<std::pair<std::string, int>, std::string> CallComments(const std::string& asm_text);

// The token a single sugar line represents, or nullopt if the line isn't a (known) token.
std::optional<Token> ParseToken(const std::string& sugar_line);
}

#endif // _CUTSCENE_ASM_SUGAR_H_
