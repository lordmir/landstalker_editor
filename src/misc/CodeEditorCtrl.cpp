#include <misc/CodeEditorCtrl.h>

#include <wx/font.h>

#include <string>

CodeEditorCtrl::CodeEditorCtrl(wxWindow* parent, wxWindowID id)
	: wxStyledTextCtrl(parent, id)
{
	Style();
	Bind(wxEVT_STC_MODIFIED, &CodeEditorCtrl::OnModified, this);
	Bind(wxEVT_STC_CHARADDED, &CodeEditorCtrl::OnCharAdded, this);
	Bind(wxEVT_STC_USERLISTSELECTION, &CodeEditorCtrl::OnUserListSelection, this);
	Bind(wxEVT_KEY_DOWN, &CodeEditorCtrl::OnKeyDown, this);
	Bind(wxEVT_LEFT_DOWN, &CodeEditorCtrl::OnLeftDown, this);
}

void CodeEditorCtrl::Style()
{
	wxFont font(wxFontInfo(10).Family(wxFONTFAMILY_TELETYPE));
	StyleSetFont(wxSTC_STYLE_DEFAULT, font);
	StyleClearAll();
	SetLexer(wxSTC_LEX_ASM);
	// Motorola 68000 mnemonics + registers, so the lexer can colour instructions. The ASM lexer
	// treats '.' as a word character, so "move.w" is one token that won't match the bare "move" -
	// expand each mnemonic with its size suffixes (.b/.w/.l/.s) so suffixed forms highlight too.
	const std::string mnemonics =
		"move movea moveq movem movep lea pea clr add adda addi addq addx sub suba subi subq subx "
		"muls mulu divs divu neg negx ext and andi or ori eor eori not cmp cmpa cmpi cmpm tst tas "
		"lsl lsr asl asr rol ror roxl roxr swap bchg bclr bset btst "
		"bra bsr beq bne bcc bcs bpl bmi bge bgt ble blt bhi bls bvc bvs jmp jsr rts rtr rte "
		"dbra dbf dbeq dbne dbcc dbcs dbpl dbmi scc trap nop link unlk stop reset "
		"dc dcb ds even align equ";
	static const char* const SUFFIXES[] = { "", ".b", ".w", ".l", ".s" };
	std::string keywords;
	for (std::size_t p = 0; p < mnemonics.size(); )
	{
		std::size_t q = mnemonics.find(' ', p);
		if (q == std::string::npos) q = mnemonics.size();
		const std::string m = mnemonics.substr(p, q - p);
		if (!m.empty())
		{
			for (const char* suf : SUFFIXES)
			{
				keywords += m; keywords += suf; keywords += ' ';
			}
		}
		p = q + 1;
	}
	SetKeyWords(0, keywords);
	SetKeyWords(1, "d0 d1 d2 d3 d4 d5 d6 d7 a0 a1 a2 a3 a4 a5 a6 a7 sp pc sr ccr usp");
	StyleSetForeground(wxSTC_ASM_COMMENT, wxColour(0, 128, 0));
	StyleSetForeground(wxSTC_ASM_COMMENTBLOCK, wxColour(0, 128, 0));
	StyleSetForeground(wxSTC_ASM_NUMBER, wxColour(160, 80, 0));
	StyleSetForeground(wxSTC_ASM_STRING, wxColour(160, 0, 0));
	StyleSetForeground(wxSTC_ASM_CPUINSTRUCTION, wxColour(0, 0, 200));
	StyleSetBold(wxSTC_ASM_CPUINSTRUCTION, true);
	StyleSetForeground(wxSTC_ASM_REGISTER, wxColour(128, 0, 128));
	StyleSetForeground(wxSTC_ASM_DIRECTIVE, wxColour(128, 64, 0));
	StyleSetForeground(wxSTC_ASM_IDENTIFIER, wxColour(0, 0, 0));
	SetMarginWidth(0, 0); // no line-number margin
	SetUseTabs(true);
	SetTabWidth(8);
	SetScrollWidthTracking(true);
	// Only fire wxEVT_STC_MODIFIED for real text edits, not styling/annotation churn.
	SetModEventMask(wxSTC_MOD_INSERTTEXT | wxSTC_MOD_DELETETEXT);
	// Inline hints shown at the end of a line (subclasses fill them via HintForLine).
	EOLAnnotationSetVisible(wxSTC_EOLANNOTATION_STANDARD);
	StyleSetForeground(HINT_STYLE, wxColour(120, 120, 120));
	StyleSetItalic(HINT_STYLE, true);
	// Indicators that recolour the text of <...> spans on top of the ASM lexer: one for a valid
	// token, one (red) for bad syntax. A valid token brightens on hover to hint it is Ctrl-clickable.
	IndicatorSetStyle(IND_TOKEN_OK, wxSTC_INDIC_TEXTFORE);
	IndicatorSetForeground(IND_TOKEN_OK, wxColour(0, 110, 160));
	IndicatorSetHoverStyle(IND_TOKEN_OK, wxSTC_INDIC_TEXTFORE);
	IndicatorSetHoverForeground(IND_TOKEN_OK, wxColour(0, 80, 220));
	IndicatorSetStyle(IND_TOKEN_ERR, wxSTC_INDIC_TEXTFORE);
	IndicatorSetForeground(IND_TOKEN_ERR, wxColour(200, 0, 0));
	SetReadOnly(true);
}

void CodeEditorCtrl::LoadCode(const wxString& text, bool editable)
{
	m_loading = true;
	SetReadOnly(false);
	SetText(text);
	EmptyUndoBuffer();
	SetSavePoint();
	m_loading = false;
	RefreshDecorations();
	SetReadOnly(!editable);
}

void CodeEditorCtrl::ClearCode()
{
	SetReadOnly(false);
	ClearAll();
	EOLAnnotationClearAll();
	SetSavePoint();
	SetReadOnly(true);
}

void CodeEditorCtrl::RefreshDecorations()
{
	ApplyHints();
	ColourTokens();
}

void CodeEditorCtrl::ApplyHints()
{
	EOLAnnotationClearAll();
	const int lines = GetLineCount();
	for (int i = 0; i < lines; ++i)
	{
		const std::string hint = HintForLine(GetLine(i).ToStdString(wxConvUTF8));
		if (!hint.empty())
		{
			EOLAnnotationSetText(i, "  " + wxString::FromUTF8(hint));
			EOLAnnotationSetStyle(i, HINT_STYLE);
		}
	}
}

void CodeEditorCtrl::ColourTokens()
{
	const std::string text = GetText().ToStdString(wxConvUTF8);
	const int len = static_cast<int>(text.size());
	// Positions are byte offsets, which match Scintilla's (UTF-8) internal buffer.
	for (int ind : { IND_TOKEN_OK, IND_TOKEN_ERR })
	{
		SetIndicatorCurrent(ind);
		IndicatorClearRange(0, len);
	}
	for (int i = 0; i < len; )
	{
		if (text[i] != '<')
		{
			++i;
			continue;
		}
		// Span the <...> up to its closing '>' on the same line; an unterminated '<' runs to EOL.
		int j = i + 1;
		while (j < len && text[j] != '>' && text[j] != '\n')
		{
			++j;
		}
		const bool closed = j < len && text[j] == '>';
		const int end = closed ? j + 1 : j; // one past the span
		const std::string span = text.substr(i, end - i);
		const bool ok = closed && IsTokenValid(span);
		SetIndicatorCurrent(ok ? IND_TOKEN_OK : IND_TOKEN_ERR);
		IndicatorFillRange(i, end - i);
		i = end;
	}
}

void CodeEditorCtrl::AutoIndentNewLine()
{
	const int line = GetCurrentLine();
	if (line <= 0)
	{
		return;
	}
	const int indent = GetLineIndentation(line - 1);
	if (indent <= 0)
	{
		return;
	}
	SetLineIndentation(line, indent);
	GotoPos(GetLineIndentPosition(line));
}

void CodeEditorCtrl::OnModified(wxStyledTextEvent& evt)
{
	// React only to actual text edits, not while LoadCode is populating. Scintilla forbids changing
	// the control (the EOL annotations here count) from inside a MODIFIED notification, so defer the
	// decoration refresh until the notification has unwound.
	if (!m_loading && (evt.GetModificationType() & (wxSTC_MOD_INSERTTEXT | wxSTC_MOD_DELETETEXT))
		&& !m_refresh_pending)
	{
		m_refresh_pending = true;
		CallAfter([this]()
		{
			m_refresh_pending = false;
			RefreshDecorations();
		});
	}
	evt.Skip();
}

void CodeEditorCtrl::OnCharAdded(wxStyledTextEvent& evt)
{
	if (evt.GetKey() == '\n')
	{
		AutoIndentNewLine();
	}
	else
	{
		OnCharTyped(evt.GetKey());
	}
	evt.Skip();
}

void CodeEditorCtrl::OnUserListSelection(wxStyledTextEvent& evt)
{
	OnUserListSelect(evt.GetListType(), evt.GetText());
}

void CodeEditorCtrl::OnKeyDown(wxKeyEvent& evt)
{
	// Ctrl+Space (re)opens autocomplete based on the caret context.
	if (evt.GetKeyCode() == WXK_SPACE && evt.ControlDown() && !GetReadOnly())
	{
		if (AutoCompActive())
		{
			AutoCompCancel();
		}
		CallAfter([this]() { ReopenAutocomplete(); });
		return; // consume
	}
	evt.Skip();
}

void CodeEditorCtrl::OnLeftDown(wxMouseEvent& evt)
{
	// Ctrl+Click follows a link on the clicked line (leaving a plain click free to place the caret).
	if (evt.ControlDown())
	{
		const int pos = PositionFromPoint(evt.GetPosition());
		if (pos >= 0)
		{
			const int line = LineFromPosition(pos);
			ActivateLink(GetLine(line).ToStdString(wxConvUTF8));
		}
	}
	evt.Skip();
}
