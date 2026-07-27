#include <script/CutsceneCodeEditor.h>

#include <algorithm>
#include <cctype>
#include <regex>
#include <thread>
#include <vector>

#include <wx/window.h>

#include <landstalker/main/GameData.h>
#include <landstalker/main/SpriteData.h>
#include <landstalker/main/StringData.h>
#include <landstalker/misc/Labels.h>
#include <misc/InputScriptDialog.h>
#include <misc/InputTableFrame.h>
#include <misc/IntroStringDialog.h>
#include <script/ScriptTableTreeEditorDialog.h>
#include <script/CutsceneAsmSugar.h>
#include <script/AsmSymbolHarvester.h>

namespace
{
	// Autocomplete user-list ids: stage 1 = call kind, stage 2 = the kind's argument id.
	constexpr int AC_KIND_LIST = 1;
	constexpr int AC_ID_LIST = 2;

	// The Labels category a token kind's id belongs to: input scripts for <Playback>, cutscene
	// scripts for the <PlayCutscene*> family.
	const std::wstring& CategoryFor(const std::string& kind)
	{
		return kind == "Playback" ? Landstalker::Labels::C_INPUT_SCRIPT
			: Landstalker::Labels::C_CUTSCENE_SCRIPT;
	}

	// Autocomplete user-list id for the constant/label symbol list (stages 1/2 are 1/2 above).
	constexpr int AC_SYMBOL_LIST = 3;

	bool IsSymbolChar(char c)
	{
		return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
	}
}

CutsceneCodeEditor::CutsceneCodeEditor(wxWindow* parent, wxWindowID id)
	: CodeEditorCtrl(parent, id)
{
}

void CutsceneCodeEditor::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	m_gd = std::move(gd);
	if (m_gd && !m_gd->GetAsmFilename().empty())
	{
		// Warm the symbol cache off-thread so typing '#' / Ctrl+Space is instant. The worker only
		// touches the harvester's own (mutex-guarded) cache and captures paths by value - it never
		// references this editor, so it is safe even if the dialog closes first.
		const auto top = m_gd->GetAsmFilename();
		const auto base = m_gd->GetBasePath();
		std::thread([top, base]() { AsmSymbolHarvester::Prewarm(top, base); }).detach();
	}
}

void CutsceneCodeEditor::LoadAsm(const std::string& asm_text, bool editable)
{
	// The previews show as inline hints and are re-attached to the asm on GetAsm.
	m_hints = CutsceneSugar::CallComments(asm_text);
	LoadCode(wxString::FromUTF8(CutsceneSugar::ToSugar(asm_text)), editable);
}

void CutsceneCodeEditor::ClearAsm()
{
	m_hints.clear();
	ClearCode();
}

std::string CutsceneCodeEditor::GetAsm() const
{
	// Expand the <...> tokens back to canonical asm, re-attaching the previews as inline comments.
	return CutsceneSugar::ToAsm(GetText().ToStdString(wxConvUTF8),
		[this](const CutsceneSugar::Token& t) { return HintFor(t.kind, t.id); });
}

std::string CutsceneCodeEditor::HintFor(const std::string& kind, int id) const
{
	// Intro strings are their own text, not a Labels name - pull the string straight from game data.
	if (kind == "ShowIntroString")
	{
		const auto text = IntroStringText(id);
		if (!text.empty())
		{
			return text;
		}
	}
	else if (kind == "Playback")
	{
		// Input scripts have no data-derived text; show their custom-or-generic PlaybackScript name.
		return InputTableFrame::DisplayName(id).ToStdString(wxConvUTF8);
	}
	else
	{
		const auto label = Landstalker::Labels::Get(CategoryFor(kind), id);
		if (label && !label->empty())
		{
			return wxString(*label).ToStdString(wxConvUTF8);
		}
	}
	const auto it = m_hints.find({ kind, id });
	return it != m_hints.end() ? it->second : std::string();
}

std::string CutsceneCodeEditor::IntroStringText(int id) const
{
	// `id` is the stored g_IntroStringToDisplay value, which is 1-based: the game displays
	// IntroStringPointers[id-1]. The string table itself is 0-based, so map it here.
	if (!m_gd || !m_gd->GetStringData() || id < 1)
	{
		return {};
	}
	auto sd = m_gd->GetStringData();
	const std::size_t idx = static_cast<std::size_t>(id - 1);
	if (idx >= sd->GetIntroStringCount())
	{
		return {};
	}
	const auto& s = sd->GetIntroString(idx);
	// The two lines are stored space-padded to 16 chars; trim and join them for a compact hint.
	auto trim = [](std::wstring t)
	{
		const auto e = t.find_last_not_of(L' ');
		return e == std::wstring::npos ? std::wstring() : t.substr(0, e + 1);
	};
	std::wstring l1 = trim(s.GetLine(0));
	std::wstring l2 = trim(s.GetLine(1));
	std::wstring text = l1;
	if (!l2.empty())
	{
		text += (text.empty() ? L"" : L" / ") + l2;
	}
	return wxString(text).ToStdString(wxConvUTF8);
}

bool CutsceneCodeEditor::IsTokenValid(const std::string& span) const
{
	return CutsceneSugar::ParseToken(span).has_value();
}

std::string CutsceneCodeEditor::HintForLine(const std::string& line) const
{
	if (const auto token = CutsceneSugar::ParseToken(line))
	{
		return HintFor(token->kind, token->id);
	}
	// Not a sugar token: a flag macro line gets its flag's meaning as an annotation.
	return FlagMacroHint(line);
}

std::string CutsceneCodeEditor::FlagMacroHint(const std::string& line) const
{
	// SetFlag / ClearFlag / ToggleFlag / TestFlag FLAG_X (optionally behind a `Label:`). The operand
	// is a bare word, so any trailing ; comment is naturally excluded from the capture.
	static const std::regex re(
		R"(^\s*(?:\w+:\s*)?(?:Set|Clear|Toggle|Test)Flag\s+(\w+))", std::regex::icase);
	std::smatch m;
	if (!std::regex_search(line, m, re))
	{
		return {};
	}
	const AsmSymbols* syms = Symbols();
	if (!syms)
	{
		return {};
	}
	const auto it = syms->descriptions.find(m[1].str());
	return it != syms->descriptions.end() ? it->second : std::string();
}

void CutsceneCodeEditor::ActivateLink(const std::string& line)
{
	const auto token = CutsceneSugar::ParseToken(line);
	if (!token)
	{
		return;
	}
	if (token->kind == "Playback")
	{
		OpenInputScript(token->id);
	}
	else if (token->kind == "ShowIntroString")
	{
		OpenIntroString(token->id);
	}
	else
	{
		OpenCutsceneScript(token->id);
	}
}

void CutsceneCodeEditor::OpenInputScript(int sequence)
{
	if (!m_gd)
	{
		return;
	}
	wxWindow* parent = wxGetTopLevelParent(this);
	auto gd = m_gd;
	CallAfter([parent, gd, sequence]()
	{
		InputScriptDialog dlg(parent, gd, sequence);
		dlg.ShowModal();
	});
}

void CutsceneCodeEditor::OpenIntroString(int id)
{
	if (!m_gd)
	{
		return;
	}
	wxWindow* parent = wxGetTopLevelParent(this);
	auto gd = m_gd;
	const int row = id - 1; // stored id is 1-based; the string editor's rows are 0-based
	CallAfter([parent, gd, row]()
	{
		IntroStringDialog dlg(parent, gd, row);
		dlg.ShowModal();
	});
}

void CutsceneCodeEditor::OpenCutsceneScript(int script_id)
{
	if (!m_gd)
	{
		return;
	}
	// Deferred so the click's event processing unwinds before the modal loop starts.
	wxWindow* parent = wxGetTopLevelParent(this);
	auto gd = m_gd;
	CallAfter([parent, gd, script_id]()
	{
		ScriptTableTreeEditorDialog dlg(parent, gd, ScriptTableTreeCategory::CUTSCENE, script_id);
		dlg.ShowModal();
	});
}

void CutsceneCodeEditor::OnCharTyped(int key)
{
	// Defer out of the CHARADDED notification: showing the list synchronously from inside the STC
	// event is unreliable (the popup never appeared).
	if (key == '<' && !AutoCompActive())
	{
		m_ac_start = GetCurrentPos() - 1; // the '<' just typed
		CallAfter([this]() { ShowKindAutocomplete(); });
	}
	else if (key == '#' && !AutoCompActive())
	{
		// '#' introduces an immediate operand - only constants make sense here.
		m_ac_sym_start = GetCurrentPos(); // just after the '#'
		CallAfter([this]() { ShowSymbolAutocomplete(true); });
	}
	else if (key == ' ' && !AutoCompActive())
	{
		// After "SetFlag " / "ClearFlag " / "ToggleFlag " / "TestFlag ", auto-open the flag picker
		// (the macros take a bare FLAG_x constant - no '#'), mirroring the <Playback/<PlayCutscene flow.
		const int caret = GetCurrentPos();
		const int line = LineFromPosition(caret);
		const std::string s = GetTextRange(PositionFromLine(line), caret).ToStdString(wxConvUTF8);
		static const std::regex macro_re(
			R"(^\s*(?:\w+:\s*)?(?:Set|Clear|Toggle|Test)Flag\s+$)", std::regex::icase);
		if (std::regex_search(s, macro_re))
		{
			m_ac_sym_start = caret; // the flag name goes right after the space just typed
			CallAfter([this]() { ShowSymbolAutocomplete(true, "FLAG_"); });
		}
	}
}

void CutsceneCodeEditor::ReopenAutocomplete()
{
	if (GetReadOnly())
	{
		return;
	}
	const int caret = GetCurrentPos();
	const int line = LineFromPosition(caret);
	const int line_start = PositionFromLine(line);
	const std::string s = GetTextRange(line_start, caret).ToStdString(wxConvUTF8);
	// Find the '<' that opens the token the caret sits in (none once a '>' closes it).
	int lt = -1;
	for (int k = static_cast<int>(s.size()) - 1; k >= 0; --k)
	{
		if (s[k] == '>') break;
		if (s[k] == '<') { lt = k; break; }
	}
	if (lt < 0)
	{
		// Not inside a <...> token: offer constants + labels for the word at the caret ('<' still
		// starts a token via OnCharTyped). Right after a '#', restrict to constants (immediates only).
		int wpos = static_cast<int>(s.size());
		while (wpos > 0 && IsSymbolChar(s[wpos - 1]))
		{
			--wpos;
		}
		const bool constants_only = (wpos > 0 && s[wpos - 1] == '#');
		m_ac_sym_start = line_start + wpos;
		ShowSymbolAutocomplete(constants_only);
		return;
	}
	const std::string seg = s.substr(lt + 1); // text after '<'
	for (const auto& k : CutsceneSugar::Kinds())
	{
		// Kind already chosen ("<Kind ...") -> reopen the id picker for whatever id is half-typed.
		if (seg.size() > k.size() && seg.compare(0, k.size(), k) == 0 && seg[k.size()] == ' ')
		{
			m_ac_kind = k;
			m_ac_id_start = line_start + lt + 1 + static_cast<int>(k.size()) + 1; // after "<Kind "
			ShowIdAutocomplete();
			return;
		}
	}
	// Still choosing the kind.
	m_ac_start = line_start + lt;
	ShowKindAutocomplete();
}

void CutsceneCodeEditor::ShowKindAutocomplete()
{
	// Stage 1: pick the call kind. The caller sets m_ac_start to the '<' the selection replaces from
	// (or the caret, when there is no '<' yet and the kind selection inserts it).
	std::vector<std::string> kinds = CutsceneSugar::Kinds();
	std::sort(kinds.begin(), kinds.end());
	std::string list;
	for (const auto& k : kinds)
	{
		if (!list.empty()) list += '\x1F';
		list += k;
	}
	AutoCompSetSeparator('\x1F');
	AutoCompSetIgnoreCase(true);
	AutoCompSetAutoHide(false);
	AutoCompSetCancelAtStart(false); // let backspacing to the anchor keep the list open
	UserListShow(AC_KIND_LIST, wxString::FromUTF8(list));
}

void CutsceneCodeEditor::ShowIdAutocomplete()
{
	// Stage 2: the ids for the kind chosen in stage 1, each mapped to its name (may be blank).
	std::map<int, std::wstring> ids;
	if (m_ac_kind == "ShowIntroString")
	{
		// Intro strings have no Labels category - enumerate them from game data, labelling each with
		// its own text so the picker shows what will be displayed.
		if (m_gd && m_gd->GetStringData())
		{
			const auto count = m_gd->GetStringData()->GetIntroStringCount();
			for (std::size_t i = 0; i < count; ++i)
			{
				const int id = static_cast<int>(i) + 1; // the picked/stored id is 1-based
				ids[id] = wxString::FromUTF8(IntroStringText(id)).ToStdWstring();
			}
		}
	}
	else if (m_ac_kind == "Playback")
	{
		// Input scripts have no id table - enumerate the sequences (0x80-terminated) in the playback
		// data so every id is pickable even before it is named, merging any names that do exist.
		if (m_gd && m_gd->GetSpriteData())
		{
			const auto& bytes = m_gd->GetSpriteData()->GetInputPlayback();
			int count = 0;
			for (auto b : bytes)
			{
				if (b == 0x80) ++count;
			}
			for (int i = 0; i < count; ++i)
			{
				ids[i] = InputTableFrame::DisplayName(i).ToStdWstring();
			}
		}
	}
	else
	{
		for (const auto& c : Landstalker::Labels::GetCategory(CategoryFor(m_ac_kind)))
		{
			ids[c.first] = c.second;
		}
	}
	if (ids.empty())
	{
		return;
	}
	m_ac_items.clear();
	std::vector<std::string> items;
	items.reserve(ids.size());
	for (const auto& c : ids)
	{
		// "0XX  name" - the id (bare hex, so typing digits filters and it reads as the real value)
		// first, then the name; the id is recovered via m_ac_items on selection.
		const std::string item = wxString::Format("%03X  ", static_cast<unsigned>(c.first)).ToStdString()
			+ wxString(c.second).ToStdString(wxConvUTF8);
		m_ac_items[item] = c.first;
		items.push_back(item);
	}
	// Presorted - fixed-width hex keys sort numerically, which Scintilla's incremental match needs.
	std::sort(items.begin(), items.end());

	std::string list;
	for (const auto& item : items)
	{
		if (!list.empty()) list += '\x1F';
		list += item;
	}
	// m_ac_id_start (the position just after "<Kind ") is set by the caller.
	AutoCompSetSeparator('\x1F');
	AutoCompSetIgnoreCase(true);
	AutoCompSetAutoHide(false);
	AutoCompSetCancelAtStart(false); // let backspacing to the anchor keep the list open
	UserListShow(AC_ID_LIST, wxString::FromUTF8(list));
}

const AsmSymbols* CutsceneCodeEditor::Symbols() const
{
	if (!m_gd)
	{
		return nullptr;
	}
	const auto top = m_gd->GetAsmFilename();
	if (top.empty())
	{
		return nullptr;
	}
	return &AsmSymbolHarvester::Get(top, m_gd->GetBasePath());
}

void CutsceneCodeEditor::ShowSymbolAutocomplete(bool constants_only, const std::string& name_prefix)
{
	const AsmSymbols* syms = Symbols();
	if (!syms)
	{
		return;
	}
	auto starts_with = [&](const std::string& s)
	{
		if (name_prefix.empty()) return true;
		if (s.size() < name_prefix.size()) return false;
		for (std::size_t i = 0; i < name_prefix.size(); ++i)
		{
			if (std::tolower(static_cast<unsigned char>(s[i]))
				!= std::tolower(static_cast<unsigned char>(name_prefix[i]))) return false;
		}
		return true;
	};
	std::vector<std::string> items;
	for (const auto& c : syms->constants)
	{
		if (starts_with(c)) items.push_back(c);
	}
	if (!constants_only)
	{
		for (const auto& l : syms->labels)
		{
			if (starts_with(l)) items.push_back(l);
		}
	}
	if (items.empty())
	{
		return;
	}
	std::sort(items.begin(), items.end());
	items.erase(std::unique(items.begin(), items.end()), items.end());

	std::string list;
	for (const auto& item : items)
	{
		if (!list.empty()) list += '\x1F';
		list += item;
	}
	// m_ac_sym_start (the start of the word being completed) is set by the caller.
	AutoCompSetSeparator('\x1F');
	AutoCompSetIgnoreCase(true);
	AutoCompSetAutoHide(false);
	AutoCompSetCancelAtStart(false);
	UserListShow(AC_SYMBOL_LIST, wxString::FromUTF8(list));
}

void CutsceneCodeEditor::OnUserListSelect(int list_type, const wxString& text)
{
	if (list_type == AC_KIND_LIST)
	{
		if (m_ac_start < 0)
		{
			return;
		}
		// Insert "<Kind " and immediately open the id picker for that kind.
		m_ac_kind = text.ToStdString(wxConvUTF8);
		const wxString prefix = wxString::FromUTF8("<" + m_ac_kind + " ");
		SetTargetStart(m_ac_start);
		SetTargetEnd(GetCurrentPos());
		ReplaceTarget(prefix);
		GotoPos(m_ac_start + static_cast<int>(prefix.length()));
		m_ac_start = -1;
		m_ac_id_start = GetCurrentPos(); // just after "<Kind ", where the id will go
		CallAfter([this]() { ShowIdAutocomplete(); });
		return;
	}
	if (list_type == AC_ID_LIST)
	{
		if (m_ac_id_start < 0)
		{
			return;
		}
		const auto it = m_ac_items.find(text.ToStdString(wxConvUTF8));
		if (it == m_ac_items.end())
		{
			return;
		}
		// PlayCutsceneAndWait carries a wait count too; seed it to 0 for the user to adjust.
		const wxString tail = (m_ac_kind == "PlayCutsceneAndWait")
			? wxString::Format("$%03X, 0>", static_cast<unsigned>(it->second))
			: wxString::Format("$%03X>", static_cast<unsigned>(it->second));
		SetTargetStart(m_ac_id_start);
		SetTargetEnd(GetCurrentPos());
		ReplaceTarget(tail);
		GotoPos(m_ac_id_start + static_cast<int>(tail.length()));
		m_ac_id_start = -1;
		RefreshDecorations();
		return;
	}
	if (list_type == AC_SYMBOL_LIST)
	{
		if (m_ac_sym_start < 0)
		{
			return;
		}
		// Replace the partially-typed word with the chosen constant/label name.
		SetTargetStart(m_ac_sym_start);
		SetTargetEnd(GetCurrentPos());
		ReplaceTarget(text);
		GotoPos(m_ac_sym_start + static_cast<int>(text.length()));
		m_ac_sym_start = -1;
	}
}
