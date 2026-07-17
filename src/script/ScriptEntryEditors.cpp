#include <script/ScriptEntryEditors.h>

#include <algorithm>
#include <functional>
#include <map>
#include <wx/spinctrl.h>
#include <misc/LookupDataViewRenderer.h>

namespace
{

// Height/min-width used for embedded LookupEditor combos (see LookupDataViewRenderer.h) - position
// is irrelevant since these are always placed by a sizer; kept modest since these editors also
// have a label (and sometimes a slot choice) to their left sharing the same cell width, unlike
// LookupDataViewRenderer's own use as a whole dataview column's sole editor.
constexpr int LOOKUP_COMBO_WIDTH = 320;
constexpr int LOOKUP_COMBO_HEIGHT = 24;
// Narrower than LOOKUP_COMBO_WIDTH - the slot combo's own choices are always just "[N] Slot N+1",
// far shorter than name lookups, so it doesn't need nearly as much room.
constexpr int SLOT_COMBO_WIDTH = 140;

// TUNABLE: empirical fudge factor for how far right an editor's *content* starts within its own
// window (independent of the window's own on-screen x - see ScriptDataViewRenderer::CreateEditorCtrl()
// for that one). Adjust directly if the bubble/labels drift left or right of their read-only
// counterparts.
constexpr int CONTENT_LEFT_FUDGE_PX = 36;

// Text/colours for the bubble ScriptEntryEditorCtrl::CreateContentSizer() draws at the start of
// every editor - mirrors ScriptDataViewRenderer.cpp's Render*Properties() InsertRenderBubble()
// calls exactly; keep in sync if those ever change.
struct BubbleStyle
{
	const char* text;
	wxColour bg;
	wxColour fg;
};

const BubbleStyle& GetBubbleStyle(Landstalker::ScriptTableEntryType type)
{
	using T = Landstalker::ScriptTableEntryType;
	static const std::map<T, BubbleStyle> styles = {
		{ T::INVALID, { "INVALID", *wxRED, *wxWHITE } },
		{ T::STRING, { "STRING", *wxCYAN, *wxBLACK } },
		{ T::PLAY_CUTSCENE, { "PLAY CUTSCENE", *wxBLUE, *wxWHITE } },
		{ T::ITEM_LOAD, { "SET ITEM", wxColour("DARK ORCHID"), *wxWHITE } },
		{ T::NUMBER_LOAD, { "SET NUMBER", wxColour("MAROON"), *wxWHITE } },
		{ T::GIVE_ITEM, { "GIVE ITEM TO PLAYER", wxColour("FOREST GREEN"), *wxWHITE } },
		{ T::GIVE_MONEY, { "GIVE MONEY TO PLAYER", wxColour("DARK SLATE BLUE"), *wxWHITE } },
		{ T::SET_FLAG, { "SET FLAG", wxColour("MEDIUM SPRING GREEN"), *wxBLACK } },
		{ T::PLAY_BGM, { "PLAY BGM", wxColour("LIGHT STEEL BLUE"), *wxBLACK } },
		{ T::SET_SPEAKER, { "SET CHARACTER", wxColour("DARK GREEN"), *wxWHITE } },
		{ T::SET_GLOBAL_SPEAKER, { "SET GLOBAL CHAR", wxColour("ORANGE"), *wxBLACK } },
		{ T::GLOBAL_CHAR_LOAD, { "LOAD GLOBAL CHAR", wxColour("GOLD"), *wxBLACK } },
	};
	static const BubbleStyle fallback{ "???", *wxLIGHT_GREY, *wxBLACK };
	const auto it = styles.find(type);
	return it != styles.end() ? it->second : fallback;
}

// A real (non-dataview-renderer) widget replicating InsertRenderBubble()'s look - plain wxPaintDC
// drawing, so (unlike wxDataViewCustomRenderer::RenderText() on GTK) dc->SetTextForeground() works
// perfectly normally here, no attribute-based workaround needed.
class EntryBubbleCtrl : public wxWindow
{
public:
	EntryBubbleCtrl(wxWindow* parent, const wxString& text, const wxColour& bg, const wxColour& fg)
		: wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize),
		  m_text(text),
		  m_bg(bg),
		  m_fg(fg)
	{
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		SetFont(GetFont().Bold());
		// Sized to the text, like InsertRenderBubble() draws it (draw_rect.width = extent.width + 4)
		// - NOT the full LEADING_OFFSET reservation, which is only a minimum *spacing* before the
		// next field in the renderer, not the bubble's own drawn width. A bare wxWindow also has no
		// content-based best-size of its own, so leaving height unset here would collapse to ~0 and
		// the OnPaint below would have no visible area to draw into even though it still runs.
		const wxSize text_extent = GetTextExtent(text);
		SetMinSize(wxSize(text_extent.GetWidth() + 8, text_extent.GetHeight() + 6));
		Bind(wxEVT_PAINT, &EntryBubbleCtrl::OnPaint, this);
	}

private:
	void OnPaint(wxPaintEvent&)
	{
		wxPaintDC dc(this);
		const wxSize size = GetSize();
		// This control's own small area, not the editor window's overall bounds - a manual partial
		// rectangle here (tried previously) can't fix the window itself sitting too far left; that's
		// now handled directly in ScriptDataViewRenderer::CreateEditorCtrl() instead. Just clear the
		// whole thing to the parent's background before drawing the bubble on top.
		dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
		dc.Clear();
		dc.SetBrush(wxBrush(m_bg));
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.DrawRoundedRectangle(2, 2, size.GetWidth() - 4, size.GetHeight() - 4, 3);
		dc.SetFont(GetFont());
		dc.SetTextForeground(m_fg);
		const wxSize text_size = dc.GetTextExtent(m_text);
		dc.DrawText(m_text, std::max((size.GetWidth() - text_size.GetWidth()) / 2, 4), std::max((size.GetHeight() - text_size.GetHeight()) / 2, 0));
	}

	wxString m_text;
	wxColour m_bg;
	wxColour m_fg;
};

// Prefixes every entry with its ID (so combos over a range wider than what has real names, like
// character/cutscene/flag IDs, stay identifiable/selectable rather than collapsing into a wall of
// identical fallback names) - hex for character fields (matches how their IDs are conventionally
// written elsewhere), decimal for everything else (matches how item/flag/BGM/cutscene IDs are
// already displayed in the row renderer).
wxArrayString BuildIdPrefixedNameChoices(std::size_t count, const std::function<std::wstring(int)>& name_of, bool hex)
{
	wxArrayString choices;
	choices.Alloc(count);
	for (std::size_t i = 0; i < count; ++i)
	{
		choices.Add(wxString::Format(hex ? "[%03X] %s" : "[%u] %s", static_cast<unsigned>(i), wxString(name_of(static_cast<int>(i)))));
	}
	return choices;
}

// ---------------------------------------------------------------------------------------------
// STRING
// ---------------------------------------------------------------------------------------------
class ScriptStringEntryEditorCtrl : public ScriptEntryEditorCtrl
{
public:
	ScriptStringEntryEditorCtrl(wxWindow* parent, const wxRect& rect, const Landstalker::ScriptStringEntry& entry,
		std::shared_ptr<const Landstalker::GameData> gd)
		: ScriptEntryEditorCtrl(parent, rect, entry),
		  m_gd(gd),
		  m_string_start(gd->GetScriptData()->GetStringStart())
	{
		auto* sizer = CreateContentSizer();
		m_string_select = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
			wxSP_ARROW_KEYS | wxWANTS_CHARS, m_string_start,
			static_cast<int>(m_gd->GetStringData()->GetStringCount(Landstalker::StringData::Type::MAIN)) - 1);
		m_string_select->SetValue(static_cast<int>(entry.string + m_string_start));
		m_string_preview = new wxTextCtrl(this, wxID_PREVIEW, wxEmptyString, wxDefaultPosition, wxDefaultSize,
			wxTE_PROCESS_ENTER | wxWANTS_CHARS);
		// A bare wxTextCtrl reports a tiny natural minimum - without this, CreateScriptEntryEditor's
		// Fit() would shrink the whole editor down to something too cramped to usefully preview or
		// edit the string text in.
		m_string_preview->SetMinSize(wxSize(220, -1));
		wxFont tt_font = m_string_preview->GetFont();
		tt_font.SetFamily(wxFONTFAMILY_TELETYPE);
		m_string_preview->SetFont(tt_font);
		RefreshPreviewText();
		// Deliberately NOT wxALIGN_RIGHT: on GTK that style makes wxCheckBox build a composite
		// widget (a wrapper GtkBox as m_widget, with the real GtkCheckButton packed inside it as
		// m_widgetCheckbox - two separate GObjects in a parent/child relationship, versus a single
		// native widget without it). That composite form is what crashes here: something upstream
		// destroys this editor's widget tree earlier than wx's own deferred C++ delete gets to it,
		// and GTK's container semantics auto-destroy the inner GtkCheckButton along with the
		// wrapper - so by the time wxCheckBox::~wxCheckBox() runs its own extra
		// GTKDisconnect(m_widgetCheckbox) call, that widget's GObject is already gone (confirmed via
		// gdb: SIGSEGV in g_signal_handlers_disconnect_matched, "instance with invalid (NULL) class
		// pointer"). Without wxALIGN_RIGHT, m_widget == m_widgetCheckbox (single widget), so that
		// extra disconnect call is skipped entirely.
		m_clear_check = new wxCheckBox(this, wxID_ANY, "Clear");
		m_end_check = new wxCheckBox(this, wxID_ANY, "End");
		m_clear_check->SetValue(entry.GetClear());
		m_end_check->SetValue(entry.GetEnd());

		sizer->Add(new wxStaticText(this, wxID_ANY, "String:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
		sizer->Add(m_string_select, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
		sizer->Add(m_clear_check, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
		sizer->Add(m_end_check, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
		// Unlike every other editor, STRING is the one exception that should use all the available
		// width rather than staying compact + a dummy filler (see FinishContentSizer()) - the preview
		// text is the whole point of this editor, so let it take whatever room GTK gives the window.
		sizer->Add(m_string_preview, 1, wxEXPAND | wxLEFT, 8);
		SetSizer(sizer);

		m_string_select->Bind(wxEVT_SPINCTRL, &ScriptStringEntryEditorCtrl::OnStringIdChanged, this);
	}

	std::unique_ptr<Landstalker::ScriptTableEntry> GetValue() const override
	{
		const std::size_t string_idx = static_cast<std::size_t>(m_string_select->GetValue()) - m_string_start;
		if (string_idx < m_gd->GetStringData()->GetStringCount(Landstalker::StringData::Type::MAIN))
		{
			// Only committed here, once, at the true end of the edit session (Escape never reaches
			// GetValue() at all) - comparing against whatever ID is CURRENTLY selected avoids the
			// stale-ID-vs-new-ID mismatch that used to let scrolling the ID spinner corrupt an
			// unrelated string entry.
			const std::wstring current = m_gd->GetStringData()->GetString(Landstalker::StringData::Type::MAIN, m_string_select->GetValue());
			if (m_string_preview->GetValue().ToStdWstring() != current)
			{
				m_gd->GetStringData()->SetString(Landstalker::StringData::Type::MAIN, m_string_select->GetValue(), m_string_preview->GetValue().ToStdWstring());
			}
		}
		return std::make_unique<Landstalker::ScriptStringEntry>(static_cast<uint16_t>(string_idx), m_clear_check->GetValue(), m_end_check->GetValue());
	}

	void SetFocus() override
	{
		m_string_select->SetFocus();
	}

private:
	void RefreshPreviewText()
	{
		const std::size_t string_idx = static_cast<std::size_t>(m_string_select->GetValue());
		if (string_idx < m_gd->GetStringData()->GetStringCount(Landstalker::StringData::Type::MAIN))
		{
			m_string_preview->SetValue(m_gd->GetStringData()->GetString(Landstalker::StringData::Type::MAIN, string_idx));
		}
		else
		{
			m_string_preview->SetValue("???");
		}
	}

	void OnStringIdChanged(wxSpinEvent& evt)
	{
		// Purely a display refresh - no persistence happens here, only inside GetValue().
		RefreshPreviewText();
		evt.Skip();
	}

	std::shared_ptr<const Landstalker::GameData> m_gd;
	std::size_t m_string_start;
	wxSpinCtrl* m_string_select;
	wxTextCtrl* m_string_preview;
	wxCheckBox* m_clear_check;
	wxCheckBox* m_end_check;
};

// ---------------------------------------------------------------------------------------------
// ITEM_LOAD / GLOBAL_CHAR_LOAD - shared "slot + name lookup" shape
// ---------------------------------------------------------------------------------------------
class SlotAndLookupEntryEditorCtrl : public ScriptEntryEditorCtrl
{
public:
	SlotAndLookupEntryEditorCtrl(wxWindow* parent, const wxRect& rect, const Landstalker::ScriptTableEntry& entry,
		const wxString& label, int slot, int value, wxArrayString choices)
		: ScriptEntryEditorCtrl(parent, rect, entry),
		  m_choices(std::move(choices)),
		  m_fallback_value(value),
		  m_fallback_slot(slot)
	{
		auto* sizer = CreateContentSizer();
		// Bracketed number is the raw (0-based) choice index, same convention BuildIdPrefixedNameChoices
		// uses - ParseValue() treats a bare bracketed number as a direct index, not a display value, so
		// it has to match the slot's own 0-based storage exactly ("Slot 1" text is 1-based purely for
		// human readability, same as the renderer's own "slot + 1" display).
		for (int i = 0; i < 4; ++i)
		{
			m_slot_choices.Add(wxString::Format("[%d] Slot %d", i, i + 1));
		}
		m_slot_combo = LookupEditor::Create(this, wxRect(wxPoint(0, 0), wxSize(SLOT_COMBO_WIDTH, LOOKUP_COMBO_HEIGHT)),
			LookupEditor::FormatLabel(m_slot_choices, slot), m_slot_choices, nullptr);
		m_value_combo = LookupEditor::Create(this, wxRect(wxPoint(0, 0), wxSize(LOOKUP_COMBO_WIDTH, LOOKUP_COMBO_HEIGHT)),
			LookupEditor::FormatLabel(m_choices, value), m_choices, nullptr);

		sizer->Add(new wxStaticText(this, wxID_ANY, "Slot:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
		sizer->Add(m_slot_combo, 0, wxLEFT, 4);
		sizer->Add(new wxStaticText(this, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
		// Proportion 0 - see ScriptStringEntryEditorCtrl's m_string_preview comment.
		sizer->Add(m_value_combo, 0, wxLEFT, 4);
		FinishContentSizer(sizer);
		SetSizer(sizer);
	}

	void SetFocus() override
	{
		m_value_combo->SetFocus();
	}

protected:
	long ResolveValue() const
	{
		LookupEditor::CommitPendingSelection(m_value_combo);
		return LookupEditor::ParseValue(m_choices, LookupEditor::GetValueText(m_value_combo), m_fallback_value);
	}

	uint8_t ResolveSlot() const
	{
		LookupEditor::CommitPendingSelection(m_slot_combo);
		return static_cast<uint8_t>(LookupEditor::ParseValue(m_slot_choices, LookupEditor::GetValueText(m_slot_combo), m_fallback_slot));
	}

private:
	wxArrayString m_choices;
	long m_fallback_value;
	wxArrayString m_slot_choices;
	long m_fallback_slot;
	wxWindow* m_slot_combo;
	wxWindow* m_value_combo;
};

class ScriptItemLoadEntryEditorCtrl : public SlotAndLookupEntryEditorCtrl
{
public:
	ScriptItemLoadEntryEditorCtrl(wxWindow* parent, const wxRect& rect, const Landstalker::ScriptItemLoadEntry& entry,
		std::shared_ptr<const Landstalker::GameData> gd)
		// Fixed at 64, not GetItemNameCount() - ScriptItemLoadEntry::SetData() masks item with
		// `& 0x3F`, so 0-63 is the field's true encodable range regardless of how many items happen
		// to have real names (GetItemDisplayName() falls back to a generic "ItemNN" label past the
		// named range, so every encodable value stays selectable) - a wider range here would let the
		// user pick a value the ROM encoding can't actually represent, silently truncated on commit.
		: SlotAndLookupEntryEditorCtrl(parent, rect, entry, "Item:", entry.slot, entry.item,
			BuildIdPrefixedNameChoices(64, [gd](int i) { return gd->GetStringData()->GetItemDisplayName(i); }, false))
	{
	}

	std::unique_ptr<Landstalker::ScriptTableEntry> GetValue() const override
	{
		return std::make_unique<Landstalker::ScriptItemLoadEntry>(static_cast<uint8_t>(ResolveValue()), ResolveSlot(), m_clear, m_end);
	}
};

class ScriptGlobalCharLoadEntryEditorCtrl : public SlotAndLookupEntryEditorCtrl
{
public:
	ScriptGlobalCharLoadEntryEditorCtrl(wxWindow* parent, const wxRect& rect, const Landstalker::ScriptGlobalCharLoadEntry& entry,
		std::shared_ptr<const Landstalker::GameData> gd)
		// Fixed at 24, not GetSpecialCharNameCount() - ScriptGlobalCharLoadEntry::SetData() clamps
		// chr to [0, 23], so that's the field's true encodable range regardless of how many global
		// characters happen to have real names (GetGlobalCharacterDisplayName() falls back to a
		// generic label past the named range, so every encodable value stays selectable) - offering
		// more would let the user pick a value the ROM encoding can't actually represent.
		: SlotAndLookupEntryEditorCtrl(parent, rect, entry, "Character:", entry.slot, entry.chr,
			BuildIdPrefixedNameChoices(24, [gd](int i) { return gd->GetStringData()->GetGlobalCharacterDisplayName(i); }, true))
	{
	}

	std::unique_ptr<Landstalker::ScriptTableEntry> GetValue() const override
	{
		return std::make_unique<Landstalker::ScriptGlobalCharLoadEntry>(static_cast<uint8_t>(ResolveValue()), ResolveSlot(), m_clear, m_end);
	}
};

// ---------------------------------------------------------------------------------------------
// Plain "name lookup, no slot" shape - SET_FLAG / SET_SPEAKER / SET_GLOBAL_SPEAKER / PLAY_BGM
// ---------------------------------------------------------------------------------------------
class LookupOnlyEntryEditorCtrl : public ScriptEntryEditorCtrl
{
public:
	LookupOnlyEntryEditorCtrl(wxWindow* parent, const wxRect& rect, const Landstalker::ScriptTableEntry& entry,
		const wxString& label, int value, wxArrayString choices)
		: ScriptEntryEditorCtrl(parent, rect, entry),
		  m_choices(std::move(choices)),
		  m_fallback_value(value)
	{
		auto* sizer = CreateContentSizer();
		m_value_combo = LookupEditor::Create(this, wxRect(wxPoint(0, 0), wxSize(LOOKUP_COMBO_WIDTH, LOOKUP_COMBO_HEIGHT)),
			LookupEditor::FormatLabel(m_choices, value), m_choices, nullptr);

		sizer->Add(new wxStaticText(this, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
		// Proportion 0 - see ScriptStringEntryEditorCtrl's m_string_preview comment.
		sizer->Add(m_value_combo, 0, wxLEFT, 8);
		FinishContentSizer(sizer);
		SetSizer(sizer);
	}

	void SetFocus() override
	{
		m_value_combo->SetFocus();
	}

protected:
	long ResolveValue() const
	{
		LookupEditor::CommitPendingSelection(m_value_combo);
		return LookupEditor::ParseValue(m_choices, LookupEditor::GetValueText(m_value_combo), m_fallback_value);
	}

private:
	wxArrayString m_choices;
	long m_fallback_value;
	wxWindow* m_value_combo;
};

class ScriptFlagEntryEditorCtrl : public LookupOnlyEntryEditorCtrl
{
public:
	ScriptFlagEntryEditorCtrl(wxWindow* parent, const wxRect& rect, const Landstalker::ScriptSetFlagEntry& entry,
		std::shared_ptr<const Landstalker::GameData> gd)
		// Fixed at 1000, not 2048 - ScriptSetFlagEntry::SetData() clamps flag to [0, 999], so a
		// choice beyond that (unlike the broader 0-2047 flag range used elsewhere, e.g. the
		// standalone Progress Flags browser) would silently get truncated on commit here.
		: LookupOnlyEntryEditorCtrl(parent, rect, entry, "Flag:", entry.flag,
			BuildIdPrefixedNameChoices(1000, [gd](int i) { return gd->GetScriptData()->GetFlagDisplayName(i); }, false))
	{
	}

	std::unique_ptr<Landstalker::ScriptTableEntry> GetValue() const override
	{
		return std::make_unique<Landstalker::ScriptSetFlagEntry>(static_cast<uint16_t>(ResolveValue()), m_clear, m_end);
	}
};

class ScriptSpeakerEntryEditorCtrl : public LookupOnlyEntryEditorCtrl
{
public:
	ScriptSpeakerEntryEditorCtrl(wxWindow* parent, const wxRect& rect, const Landstalker::ScriptSetSpeakerEntry& entry,
		std::shared_ptr<const Landstalker::GameData> gd)
		: LookupOnlyEntryEditorCtrl(parent, rect, entry, "Character:", entry.chr,
			// chr is a free-form 0-999 ID (only some are named), not bounded by GetCharNameCount() -
			// see BuildIdPrefixedNameChoices, StringData::GetCharacterDisplayName() already falls
			// back to a default name past the named range.
			BuildIdPrefixedNameChoices(1000, [gd](int i) { return gd->GetStringData()->GetCharacterDisplayName(i); }, true))
	{
	}

	std::unique_ptr<Landstalker::ScriptTableEntry> GetValue() const override
	{
		return std::make_unique<Landstalker::ScriptSetSpeakerEntry>(static_cast<uint16_t>(ResolveValue()), m_clear, m_end);
	}
};

class ScriptGlobalSpeakerEntryEditorCtrl : public LookupOnlyEntryEditorCtrl
{
public:
	ScriptGlobalSpeakerEntryEditorCtrl(wxWindow* parent, const wxRect& rect, const Landstalker::ScriptSetGlobalSpeakerEntry& entry,
		std::shared_ptr<const Landstalker::GameData> gd)
		// Fixed at 24 (ScriptSetGlobalSpeakerEntry::SetData() clamps chr to [0, 23]) - see
		// ScriptGlobalCharLoadEntryEditorCtrl's comment.
		: LookupOnlyEntryEditorCtrl(parent, rect, entry, "Character:", entry.chr,
			BuildIdPrefixedNameChoices(24, [gd](int i) { return gd->GetStringData()->GetGlobalCharacterDisplayName(i); }, true))
	{
	}

	std::unique_ptr<Landstalker::ScriptTableEntry> GetValue() const override
	{
		return std::make_unique<Landstalker::ScriptSetGlobalSpeakerEntry>(static_cast<uint8_t>(ResolveValue()), m_clear, m_end);
	}
};

wxArrayString BuildBgmChoices()
{
	return BuildIdPrefixedNameChoices(Landstalker::ScriptPlayBgmEntry::BGMS.size(), [](int i)
	{
		return *Landstalker::Labels::Get(Landstalker::Labels::C_SOUNDS, Landstalker::ScriptPlayBgmEntry::BGMS.at(i));
	}, false);
}

class ScriptBgmEntryEditorCtrl : public LookupOnlyEntryEditorCtrl
{
public:
	ScriptBgmEntryEditorCtrl(wxWindow* parent, const wxRect& rect, const Landstalker::ScriptPlayBgmEntry& entry,
		std::shared_ptr<const Landstalker::GameData> /*gd*/)
		: LookupOnlyEntryEditorCtrl(parent, rect, entry, "BGM:", entry.bgm, BuildBgmChoices())
	{
	}

	std::unique_ptr<Landstalker::ScriptTableEntry> GetValue() const override
	{
		return std::make_unique<Landstalker::ScriptPlayBgmEntry>(static_cast<uint8_t>(ResolveValue()), m_clear, m_end);
	}
};

// ---------------------------------------------------------------------------------------------
// Plain spin-only shapes - NUMBER_LOAD / PLAY_CUTSCENE (no dense enumerable name list) / INVALID
// ---------------------------------------------------------------------------------------------
class ScriptNumberEntryEditorCtrl : public ScriptEntryEditorCtrl
{
public:
	ScriptNumberEntryEditorCtrl(wxWindow* parent, const wxRect& rect, const Landstalker::ScriptNumLoadEntry& entry,
		std::shared_ptr<const Landstalker::GameData> /*gd*/)
		: ScriptEntryEditorCtrl(parent, rect, entry)
	{
		auto* sizer = CreateContentSizer();
		m_value_select = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
			wxSP_ARROW_KEYS | wxWANTS_CHARS, 0, 999);
		m_value_select->SetValue(entry.num);
		sizer->Add(new wxStaticText(this, wxID_ANY, "Number:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
		sizer->Add(m_value_select, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
		FinishContentSizer(sizer);
		SetSizer(sizer);
	}

	std::unique_ptr<Landstalker::ScriptTableEntry> GetValue() const override
	{
		return std::make_unique<Landstalker::ScriptNumLoadEntry>(static_cast<uint16_t>(m_value_select->GetValue()), m_clear, m_end);
	}

	void SetFocus() override
	{
		m_value_select->SetFocus();
	}

private:
	wxSpinCtrl* m_value_select;
};

class ScriptCutsceneEntryEditorCtrl : public LookupOnlyEntryEditorCtrl
{
public:
	ScriptCutsceneEntryEditorCtrl(wxWindow* parent, const wxRect& rect, const Landstalker::ScriptInitiateCutsceneEntry& entry,
		std::shared_ptr<const Landstalker::GameData> /*gd*/)
		: LookupOnlyEntryEditorCtrl(parent, rect, entry, "Cutscene:", entry.cutscene,
			BuildIdPrefixedNameChoices(1024, [](int i) { return Landstalker::ScriptData::GetCutsceneDisplayName(i); }, false))
	{
	}

	std::unique_ptr<Landstalker::ScriptTableEntry> GetValue() const override
	{
		return std::make_unique<Landstalker::ScriptInitiateCutsceneEntry>(static_cast<uint16_t>(ResolveValue()), m_clear, m_end);
	}
};

class ScriptInvalidEntryEditorCtrl : public ScriptEntryEditorCtrl
{
public:
	ScriptInvalidEntryEditorCtrl(wxWindow* parent, const wxRect& rect, const Landstalker::ScriptTableEntry& entry)
		: ScriptEntryEditorCtrl(parent, rect, entry)
	{
		auto* sizer = CreateContentSizer();
		// The full raw 16-bit word, Clear/End bits included - "Custom" is a deliberate escape hatch
		// for arbitrary encodings, so unlike every other type it edits the whole word directly
		// rather than exposing Clear/End as separate controls.
		m_value_select = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
			wxSP_ARROW_KEYS | wxWANTS_CHARS, 0, 65535);
		m_value_select->SetValue(entry.ToBytes());
		sizer->Add(new wxStaticText(this, wxID_ANY, "Value:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
		sizer->Add(m_value_select, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
		FinishContentSizer(sizer);
		SetSizer(sizer);
	}

	std::unique_ptr<Landstalker::ScriptTableEntry> GetValue() const override
	{
		return Landstalker::ScriptTableEntry::FromBytes(static_cast<uint16_t>(m_value_select->GetValue()));
	}

	void SetFocus() override
	{
		m_value_select->SetFocus();
	}

private:
	wxSpinCtrl* m_value_select;
};

class ScriptNoParamsEntryEditorCtrl : public ScriptEntryEditorCtrl
{
public:
	ScriptNoParamsEntryEditorCtrl(wxWindow* parent, const wxRect& rect, const Landstalker::ScriptTableEntry& entry, const wxString& label)
		: ScriptEntryEditorCtrl(parent, rect, entry)
	{
		auto* sizer = CreateContentSizer();
		sizer->Add(new wxStaticText(this, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
		FinishContentSizer(sizer);
		SetSizer(sizer);
	}

	std::unique_ptr<Landstalker::ScriptTableEntry> GetValue() const override
	{
		auto entry = Landstalker::ScriptTableEntry::MakeEntry(m_type);
		entry->SetClear(m_clear);
		entry->SetEnd(m_end);
		return entry;
	}
};

} // namespace

wxBoxSizer* ScriptEntryEditorCtrl::CreateContentSizer()
{
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	sizer->AddSpacer(CONTENT_LEFT_FUDGE_PX);
	const BubbleStyle& style = GetBubbleStyle(m_type);
	auto* bubble = new EntryBubbleCtrl(this, style.text, style.bg, style.fg);
	sizer->Add(bubble, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
	// The rest of LEADING_OFFSET is blank, matching InsertRenderBubble()'s own behaviour: the bubble
	// itself is only ever as wide as its text, but it still reserves the full LABEL_WIDTH of space
	// before whatever comes next - here, that's what keeps every field (e.g. "String:") lined up
	// with its read-only counterpart regardless of how wide any particular bubble's text is.
	const int reserved = LEADING_OFFSET - bubble->GetMinSize().GetWidth() - 4;
	sizer->AddSpacer(std::max(reserved, 4));
	return sizer;
}

void ScriptEntryEditorCtrl::FinishContentSizer(wxBoxSizer* sizer)
{
	sizer->AddStretchSpacer(1);
	sizer->Add(new wxStaticText(this, wxID_ANY, wxEmptyString), 1, wxEXPAND | wxLEFT, 8);
}

wxWindow* CreateScriptEntryEditor(wxWindow* parent, const wxRect& rect, const Landstalker::ScriptTableEntry& entry,
	std::shared_ptr<const Landstalker::GameData> gd)
{
	using T = Landstalker::ScriptTableEntryType;
	ScriptEntryEditorCtrl* editor = nullptr;
	switch (entry.GetType())
	{
	case T::STRING:
		editor = new ScriptStringEntryEditorCtrl(parent, rect, dynamic_cast<const Landstalker::ScriptStringEntry&>(entry), gd);
		break;
	case T::ITEM_LOAD:
		editor = new ScriptItemLoadEntryEditorCtrl(parent, rect, dynamic_cast<const Landstalker::ScriptItemLoadEntry&>(entry), gd);
		break;
	case T::GLOBAL_CHAR_LOAD:
		editor = new ScriptGlobalCharLoadEntryEditorCtrl(parent, rect, dynamic_cast<const Landstalker::ScriptGlobalCharLoadEntry&>(entry), gd);
		break;
	case T::NUMBER_LOAD:
		editor = new ScriptNumberEntryEditorCtrl(parent, rect, dynamic_cast<const Landstalker::ScriptNumLoadEntry&>(entry), gd);
		break;
	case T::SET_FLAG:
		editor = new ScriptFlagEntryEditorCtrl(parent, rect, dynamic_cast<const Landstalker::ScriptSetFlagEntry&>(entry), gd);
		break;
	case T::SET_SPEAKER:
		editor = new ScriptSpeakerEntryEditorCtrl(parent, rect, dynamic_cast<const Landstalker::ScriptSetSpeakerEntry&>(entry), gd);
		break;
	case T::SET_GLOBAL_SPEAKER:
		editor = new ScriptGlobalSpeakerEntryEditorCtrl(parent, rect, dynamic_cast<const Landstalker::ScriptSetGlobalSpeakerEntry&>(entry), gd);
		break;
	case T::PLAY_BGM:
		editor = new ScriptBgmEntryEditorCtrl(parent, rect, dynamic_cast<const Landstalker::ScriptPlayBgmEntry&>(entry), gd);
		break;
	case T::PLAY_CUTSCENE:
		editor = new ScriptCutsceneEntryEditorCtrl(parent, rect, dynamic_cast<const Landstalker::ScriptInitiateCutsceneEntry&>(entry), gd);
		break;
	case T::GIVE_ITEM:
		editor = new ScriptNoParamsEntryEditorCtrl(parent, rect, entry, "Give Item To Player");
		break;
	case T::GIVE_MONEY:
		editor = new ScriptNoParamsEntryEditorCtrl(parent, rect, entry, "Give Money To Player");
		break;
	case T::INVALID:
	default:
		editor = new ScriptInvalidEntryEditorCtrl(parent, rect, entry);
		break;
	}
	// Each subclass sets its own sizer but is constructed at the full (offset) column rect - shrink
	// down to the sizer's natural minimum so the editor doesn't stretch across the whole remaining
	// (auto-fill) column width when its content needs far less.
	if (wxSizer* sizer = editor->GetSizer())
	{
		sizer->SetSizeHints(editor);
		sizer->Fit(editor);
	}
	return editor;
}
