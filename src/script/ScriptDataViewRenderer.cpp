#include <script/ScriptDataViewRenderer.h>
#include <script/ScriptEntryEditors.h>
#include <script/ScriptEntryLink.h>
#include <functional>
#include <wx/dc.h>
#include <wx/renderer.h>
#include <wx/settings.h>
#include <wx/time.h>
#include <landstalker/main/GameData.h>
#include <misc/DataViewEditorKeys.h>

static const int LABEL_WIDTH = 180;
static const int COLUMN_WIDTH = 80;

ScriptDataViewRenderer::ScriptDataViewRenderer(wxDataViewCellMode mode, std::shared_ptr<Landstalker::GameData> gd)
	: wxDataViewCustomRenderer("long", mode, wxALIGN_LEFT),
	  m_index(-1),
	  m_gd(gd)
{
}

bool ScriptDataViewRenderer::Render(wxRect rect, wxDC* dc, int state)
{
	if (m_value)
	{
		m_cell_rect = rect;
		m_link_rects.erase(m_index);
		RenderLabel(rect, dc, state);
		if (m_value && m_value->GetEnd())
		{
			auto win = GetOwner()->GetOwner();
			dc->SetPen(wxPen(wxColour(128, 128, 128), 1, wxPENSTYLE_SHORT_DASH));
			dc->DrawLine(rect.GetLeft(), rect.GetBottom() + 6, rect.GetLeft() + win->GetSize().GetWidth(), rect.GetBottom() + 6);
			dc->SetPen(*wxTRANSPARENT_PEN);
		}
	}
	return true;
}

bool ScriptDataViewRenderer::RenderLabel(wxRect rect, wxDC* dc, int state)
{
	rect.y += 2;
	switch (m_value->GetType())
	{
	case Landstalker::ScriptTableEntryType::STRING:
		return RenderStringProperties(rect, dc, state);
	case Landstalker::ScriptTableEntryType::ITEM_LOAD:
		return RenderSetItemProperties(rect, dc, state);
	case Landstalker::ScriptTableEntryType::NUMBER_LOAD:
		return RenderSetNumberProperties(rect, dc, state);
	case Landstalker::ScriptTableEntryType::GIVE_ITEM:
		return RenderGiveItemProperties(rect, dc, state);
	case Landstalker::ScriptTableEntryType::GIVE_MONEY:
		return RenderGiveMoneyProperties(rect, dc, state);
	case Landstalker::ScriptTableEntryType::SET_FLAG:
		return RenderSetFlagProperties(rect, dc, state);
	case Landstalker::ScriptTableEntryType::PLAY_BGM:
		return RenderPlayBGMProperties(rect, dc, state);
	case Landstalker::ScriptTableEntryType::PLAY_CUTSCENE:
		return RenderCutsceneProperties(rect, dc, state);
	case Landstalker::ScriptTableEntryType::SET_SPEAKER:
		return RenderSetSpeakerProperties(rect, dc, state);
	case Landstalker::ScriptTableEntryType::SET_GLOBAL_SPEAKER:
		return RenderSetGlobalSpeakerProperties(rect, dc, state);
	case Landstalker::ScriptTableEntryType::GLOBAL_CHAR_LOAD:
		return RenderLoadGlobalSpeakerProperties(rect, dc, state);
	case Landstalker::ScriptTableEntryType::INVALID:
	default:
		return RenderInvalidProperties(rect, dc, state);
	}
}

void ScriptDataViewRenderer::InsertRenderLabel(wxRect& rect, wxDC* dc, int state, const wxString& text, int min_width, const wxFont* font, const wxColour* text_colour)
{
	wxFont orig_font;
	if (font)
	{
		orig_font = dc->GetFont();
		dc->SetFont(*font);
	}
	auto extent = dc->GetTextExtent(text);
	int y_offset = std::max((rect.GetHeight() - extent.GetHeight()) / 2, 0);
	rect.y += y_offset;

	// See InsertRenderBubble()'s comment: RenderText() on GTK ignores `dc`, so forcing a colour
	// needs the attribute override, not just dc->SetTextForeground().
	const wxDataViewItemAttr orig_attr = GetAttr();
	wxColour orig_dc_colour;
	if (text_colour)
	{
		wxDataViewItemAttr attr = orig_attr;
		attr.SetColour(*text_colour);
		SetAttr(attr);
		orig_dc_colour = dc->GetTextForeground();
		dc->SetTextForeground(*text_colour);
	}
	RenderText(text, 2, rect, dc, state);
	if (text_colour)
	{
		SetAttr(orig_attr);
		dc->SetTextForeground(orig_dc_colour);
	}

	rect.y -= y_offset;
	int new_width = std::max(extent.GetWidth() + 2, min_width);
	rect.x += new_width;
	rect.width -= new_width;
	if (font)
	{
		dc->SetFont(orig_font);
	}
}

void ScriptDataViewRenderer::InsertRenderBubble(wxRect& rect, wxDC* dc, int state, const wxString& text, const wxColour& colour, int min_width, const wxFont* font, const wxColour* text_colour)
{
	wxFont orig_font = dc->GetFont();
	if (font)
	{
		orig_font = dc->GetFont();
		dc->SetFont(*font);
	}
	auto extent = dc->GetTextExtent(text);
	wxRect draw_rect = rect;

	draw_rect.width = extent.GetWidth() + 4;
	draw_rect.x += 2;

	// On GTK, a selected row's text is always painted in the theme's own selected-text colour -
	// any foreground colour/attribute we request is silently ignored while selected (this is
	// intentional upstream, so apps can't make selected text illegible). Our bubble background is
	// NOT ignored though, so forcing e.g. dark text against a light bubble would leave GTK's
	// (typically light) selected-text colour on top of it - illegible. Skip painting the bubble
	// fill while selected instead, so the native selection highlight shows through underneath;
	// that combination is guaranteed legible since it's what GTK itself renders selected text against.
	const bool selected = (state & wxDATAVIEW_CELL_SELECTED) != 0;
	if (!selected)
	{
		dc->SetBrush(wxBrush(colour));
		dc->DrawRoundedRectangle(draw_rect, 3);
	}

	// On GTK, RenderText() renders through a native GtkCellRendererText driven by GetAttr()
	// (see wx's src/gtk/dataview.cpp), completely ignoring `dc` - a dc->SetTextForeground() call
	// around this has no effect there. Push a temporary attribute override instead, which is what
	// actually reaches the renderer on that backend; other backends' RenderText() does read `dc`,
	// so set that too for them.
	const wxDataViewItemAttr orig_attr = GetAttr();
	wxColour orig_dc_colour;
	if (text_colour && !selected)
	{
		wxDataViewItemAttr attr = orig_attr;
		attr.SetColour(*text_colour);
		SetAttr(attr);
		orig_dc_colour = dc->GetTextForeground();
		dc->SetTextForeground(*text_colour);
	}
	RenderText(text, 2, draw_rect, dc, state);
	if (text_colour && !selected)
	{
		SetAttr(orig_attr);
		dc->SetTextForeground(orig_dc_colour);
	}

	int new_width = std::max(draw_rect.GetWidth() + 2, min_width);
	rect.x += new_width;
	rect.width -= new_width;
	if (font)
	{
		dc->SetFont(orig_font);
	}
}

void ScriptDataViewRenderer::InsertRenderCheckbox(wxRect& rect, wxDC* dc, int state, const wxString& text, bool checkstate, int min_width, const wxFont* font)
{
	wxFont orig_font = dc->GetFont();
	if (font)
	{
		dc->SetFont(*font);
	}
	else
	{
		dc->SetFont(dc->GetFont().Bold());
	}
	// Checkbox first, then label - matches the editor's own checkboxes (see ScriptEntryEditors.cpp's
	// ScriptStringEntryEditorCtrl), which are plain wxCheckBox without wxALIGN_RIGHT (GTK's default,
	// indicator-before-label layout - see that file's comment on why wxALIGN_RIGHT isn't used there).
	const wxRect start_rect = rect;
	wxWindow* const win = GetOwner()->GetOwner();
	wxRendererNative& renderer = wxRendererNative::Get();
	wxRect check_rect(wxRendererNative::Get().GetCheckBoxSize(GetView()));
	check_rect.x = rect.x + 2;
	check_rect.y = rect.y + (check_rect.height < rect.height ? (rect.height - check_rect.height) / 2 : 0);
	renderer.DrawCheckBox(win, *dc, check_rect, checkstate ? wxCONTROL_CHECKED : wxCONTROL_NONE);
	rect.x += check_rect.GetWidth() + 7;
	rect.width -= check_rect.GetWidth() + 7;
	InsertRenderLabel(rect, dc, state, text, 2);
	int new_width = std::max(rect.x - start_rect.x, min_width);
	rect.x = start_rect.x + new_width;
	rect.width = start_rect.width - new_width;
	dc->SetFont(orig_font);
}

void ScriptDataViewRenderer::InsertRenderName(wxRect& rect, wxDC* dc, int state, const wxString& name, int min_width)
{
	wxFont font = dc->GetFont().Italic();
	font.SetFamily(wxFONTFAMILY_TELETYPE);

	std::optional<ScriptEntryLink::Target> target;
	if (m_value)
	{
		target = ScriptEntryLink::Resolve(m_gd, *m_value);
	}
	if (!target)
	{
		InsertRenderLabel(rect, dc, state, name, min_width, &font);
		return;
	}

	font.SetUnderlined(true);
	// Same link blue the Script Function Editor's preview rows use (ScriptTreeDataViewModel::GetAttr).
	const wxColour link_colour = wxSystemSettings::SelectLightDark(wxColour(0, 102, 204), wxColour(100, 170, 255));
	const wxFont orig_font = dc->GetFont();
	dc->SetFont(font);
	const wxSize extent = dc->GetTextExtent(name);
	dc->SetFont(orig_font);
	// Full cell height rather than just the text band - a more forgiving click/hover target, and
	// there's nothing else stacked vertically within the row to mis-hit.
	m_link_rects[m_index] = wxRect(rect.x - m_cell_rect.x, 0, extent.GetWidth() + 4, m_cell_rect.height);
	InsertRenderLabel(rect, dc, state, name, min_width, &font, &link_colour);
}

std::optional<wxRect> ScriptDataViewRenderer::GetLinkHitRect(long row) const
{
	const auto it = m_link_rects.find(row);
	if (it == m_link_rects.end())
	{
		return std::nullopt;
	}
	return it->second;
}

bool ScriptDataViewRenderer::RenderInvalidProperties(wxRect& rect, wxDC* dc, int state)
{
	wxFont font = dc->GetFont().Bold();
	InsertRenderBubble(rect, dc, state, "INVALID", *wxRED, LABEL_WIDTH, &font, wxWHITE);
	font.SetFamily(wxFONTFAMILY_TELETYPE);
	InsertRenderLabel(rect, dc, state, Landstalker::StrPrintf("%04X", m_index), 40, &font, wxRED);
	return true;
}

bool ScriptDataViewRenderer::RenderStringProperties(wxRect& rect, wxDC * dc, int state)
{
	const auto& string = dynamic_cast<Landstalker::ScriptStringEntry&>(*m_value);
	wxFont font = dc->GetFont().Bold();
	std::wstring string_preview = L"";
	if (m_gd && m_gd->GetStringData()->GetStringCount(Landstalker::StringData::Type::MAIN) > static_cast<std::size_t>(m_gd->GetScriptData()->GetStringStart() + string.string))
	{
		string_preview = m_gd->GetStringData()->GetString(Landstalker::StringData::Type::MAIN, m_gd->GetScriptData()->GetStringStart() + string.string);
	}
	InsertRenderBubble(rect, dc, state, "STRING", *wxCYAN, LABEL_WIDTH, &font, wxBLACK);
	InsertRenderLabel(rect, dc, state, "String:", COLUMN_WIDTH, &font);
	InsertRenderLabel(rect, dc, state, Landstalker::StrWPrintf(L"%04d", string.string + m_gd->GetScriptData()->GetStringStart()), 40);
	InsertRenderCheckbox(rect, dc, state, "Clear", string.GetClear(), 70, &font);
	InsertRenderCheckbox(rect, dc, state, "End", string.GetEnd(), 60, &font);
	font = dc->GetFont().Italic();
	font.SetFamily(wxFONTFAMILY_TELETYPE);
	InsertRenderLabel(rect, dc, state, string_preview, 0, &font);
	return string.end;
}

bool ScriptDataViewRenderer::RenderCutsceneProperties(wxRect& rect, wxDC* dc, int state)
{
	const auto& cutscene = dynamic_cast<Landstalker::ScriptInitiateCutsceneEntry&>(*m_value);
	wxFont font = dc->GetFont().Bold();
	InsertRenderBubble(rect, dc, state, "PLAY CUTSCENE", *wxBLUE, LABEL_WIDTH, &font, wxWHITE);
	InsertRenderLabel(rect, dc, state, _("Cutscene Index: "), COLUMN_WIDTH, &font);
	InsertRenderLabel(rect, dc, state, Landstalker::StrPrintf("%04d", cutscene.cutscene), 40);
	InsertRenderName(rect, dc, state, Landstalker::ScriptData::GetCutsceneDisplayName(cutscene.cutscene), 0);
	return true;
}

bool ScriptDataViewRenderer::RenderSetItemProperties(wxRect& rect, wxDC* dc, int state)
{
	const auto& item_set = dynamic_cast<Landstalker::ScriptItemLoadEntry&>(*m_value);
	std::wstring item_name = m_gd->GetStringData()->GetItemDisplayName(item_set.item);
	wxFont font = dc->GetFont().Bold();
	InsertRenderBubble(rect, dc, state, "SET ITEM", wxColour("DARK ORCHID"), LABEL_WIDTH, &font, wxWHITE);
	InsertRenderLabel(rect, dc, state, _("Slot: "), COLUMN_WIDTH, &font);
	InsertRenderLabel(rect, dc, state, Landstalker::StrPrintf("%01d", item_set.slot + 1), 40);
	InsertRenderLabel(rect, dc, state, _("Item: "), 0, &font);
	InsertRenderLabel(rect, dc, state, Landstalker::StrPrintf("%02d", item_set.item), 40);
	font = dc->GetFont().Italic();
	font.SetFamily(wxFONTFAMILY_TELETYPE);
	InsertRenderLabel(rect, dc, state, item_name, 0, &font);
	return false;
}

bool ScriptDataViewRenderer::RenderSetNumberProperties(wxRect& rect, wxDC* dc, int state)
{
	const auto& num_set = dynamic_cast<Landstalker::ScriptNumLoadEntry&>(*m_value);
	wxFont font = dc->GetFont().Bold();
	InsertRenderBubble(rect, dc, state, "SET NUMBER", wxColour("MAROON"), LABEL_WIDTH, &font, wxWHITE);
	InsertRenderLabel(rect, dc, state, _("Number: "), COLUMN_WIDTH, &font);
	InsertRenderLabel(rect, dc, state, Landstalker::StrPrintf("%d", num_set.num), 40);
	return false;
}

bool ScriptDataViewRenderer::RenderGiveItemProperties(wxRect& rect, wxDC* dc, int state)
{
	wxFont font = dc->GetFont().Bold();
	InsertRenderBubble(rect, dc, state, "GIVE ITEM TO PLAYER", wxColour("FOREST GREEN"), LABEL_WIDTH, &font, wxWHITE);
	return false;
}

bool ScriptDataViewRenderer::RenderGiveMoneyProperties(wxRect& rect, wxDC* dc, int state)
{
	wxFont font = dc->GetFont().Bold();
	InsertRenderBubble(rect, dc, state, "GIVE MONEY TO PLAYER", wxColour("DARK SLATE BLUE"), LABEL_WIDTH, &font, wxWHITE);
	return false;
}

bool ScriptDataViewRenderer::RenderSetFlagProperties(wxRect& rect, wxDC* dc, int state)
{
	const auto& flag_set = dynamic_cast<Landstalker::ScriptSetFlagEntry&>(*m_value);
	wxFont font = dc->GetFont().Bold();
	InsertRenderBubble(rect, dc, state, "SET FLAG", wxColour("MEDIUM SPRING GREEN"), LABEL_WIDTH, &font, wxBLACK);
	InsertRenderLabel(rect, dc, state, _("Flag: "), COLUMN_WIDTH, &font);
	InsertRenderLabel(rect, dc, state, Landstalker::StrPrintf("%04d", flag_set.flag), 0);
	font = dc->GetFont().Italic();
	font.SetFamily(wxFONTFAMILY_TELETYPE);
	InsertRenderLabel(rect, dc, state, Landstalker::ScriptData::GetFlagDisplayName(flag_set.flag), 0, &font);
	return false;
}

bool ScriptDataViewRenderer::RenderPlayBGMProperties(wxRect& rect, wxDC* dc, int state)
{
	const auto& bgm = dynamic_cast<Landstalker::ScriptPlayBgmEntry&>(*m_value);
	wxFont font = dc->GetFont().Bold();
	InsertRenderBubble(rect, dc, state, "PLAY BGM", wxColour("LIGHT STEEL BLUE"), LABEL_WIDTH, &font, wxBLACK);
	InsertRenderLabel(rect, dc, state, _("BGM: "), COLUMN_WIDTH, &font);
	InsertRenderLabel(rect, dc, state, Landstalker::StrPrintf("%01d", bgm.bgm), 40);
	if (bgm.bgm < Landstalker::ScriptPlayBgmEntry::BGMS.size() && Landstalker::Labels::Get(Landstalker::Labels::C_SOUNDS, Landstalker::ScriptPlayBgmEntry::BGMS.at(bgm.bgm)))
	{
		font = dc->GetFont().Italic();
		font.SetFamily(wxFONTFAMILY_TELETYPE);
		InsertRenderLabel(rect, dc, state, *Landstalker::Labels::Get(Landstalker::Labels::C_SOUNDS, Landstalker::ScriptPlayBgmEntry::BGMS.at(bgm.bgm)), 40, &font);
	}
	return false;
}

bool ScriptDataViewRenderer::RenderSetSpeakerProperties(wxRect& rect, wxDC* dc, int state)
{
	const auto& speaker = dynamic_cast<Landstalker::ScriptSetSpeakerEntry&>(*m_value);
	wxFont font = dc->GetFont().Bold();
	InsertRenderBubble(rect, dc, state, "SET CHARACTER", wxColour("DARK GREEN"), LABEL_WIDTH, &font, wxWHITE);
	InsertRenderLabel(rect, dc, state, _("Character: "), COLUMN_WIDTH, &font);
	InsertRenderLabel(rect, dc, state, Landstalker::StrPrintf("%03X", speaker.chr), 40);
	InsertRenderName(rect, dc, state, m_gd->GetStringData()->GetCharacterDisplayName(speaker.chr), 100);
	return false;
}

bool ScriptDataViewRenderer::RenderSetGlobalSpeakerProperties(wxRect& rect, wxDC* dc, int state)
{
	const auto& speaker = dynamic_cast<Landstalker::ScriptSetGlobalSpeakerEntry&>(*m_value);
	wxFont font = dc->GetFont().Bold();
	InsertRenderBubble(rect, dc, state, "SET GLOBAL CHARACTER", wxColour("ORANGE"), LABEL_WIDTH, &font, wxBLACK);
	InsertRenderLabel(rect, dc, state, _("Character: "), COLUMN_WIDTH, &font);
	InsertRenderLabel(rect, dc, state, Landstalker::StrPrintf("%03X", speaker.chr), 40);
	font = dc->GetFont().Italic();
	font.SetFamily(wxFONTFAMILY_TELETYPE);
	InsertRenderLabel(rect, dc, state, m_gd->GetStringData()->GetGlobalCharacterDisplayName(speaker.chr), 100, &font);
	return false;
}

bool ScriptDataViewRenderer::RenderLoadGlobalSpeakerProperties(wxRect& rect, wxDC* dc, int state)
{
	const auto& chr = dynamic_cast<Landstalker::ScriptGlobalCharLoadEntry&>(*m_value);
	wxFont font = dc->GetFont().Bold();
	InsertRenderBubble(rect, dc, state, "LOAD GLOBAL CHARACTER", wxColour("GOLD"), LABEL_WIDTH, &font, wxBLACK);
	InsertRenderLabel(rect, dc, state, _("Slot: "), COLUMN_WIDTH, &font);
	InsertRenderLabel(rect, dc, state, Landstalker::StrPrintf("%01d", chr.slot + 1), 40);
	InsertRenderLabel(rect, dc, state, _("Character: "), 0, &font);
	InsertRenderLabel(rect, dc, state, Landstalker::StrPrintf("%03X", chr.chr), 40);
	font = dc->GetFont().Italic();
	font.SetFamily(wxFONTFAMILY_TELETYPE);
	InsertRenderLabel(rect, dc, state, m_gd->GetStringData()->GetGlobalCharacterDisplayName(chr.chr), 100, &font);
	return false;
}

bool ScriptDataViewRenderer::ActivateCell(const wxRect& /*cell*/, wxDataViewModel* /*model*/, const wxDataViewItem& /*item*/, unsigned int /*col*/, const wxMouseEvent* /*mouseEvent*/)
{
	return false;
}

wxSize ScriptDataViewRenderer::GetSize() const
{
	// This width feeds GTK's own cell-area computation for BOTH Render()'s `rect` and, separately,
	// the editor's placement (`labelRect` in CreateEditorCtrl()/StartEditing()) - the two share this
	// exact same source, just queried at very different times. Render() re-queries it continuously,
	// on every single paint, so it's always current. Editing-start queries it exactly *once*, at the
	// instant a cell is double-clicked, and GTK freezes that one answer into the editor's placement
	// for its whole lifetime - nothing re-queries it afterward, and we've independently confirmed
	// (via SetSize()-forcing attempts that GTK always silently reverted) that nothing here can
	// correct it after the fact either. Plain GetOwner()->GetWidth() (the "Value" column's own live,
	// auto-fill (-1) width) can go transiently stale for that one-shot query: right after
	// ClearColumns()/InsertColumn() (a file reload - see ScriptEditorCtrl::SetGameData()), the
	// auto-fill column's width negotiation may not have settled yet at the moment a user's very next
	// double-click queries it, baking a wrong width into that edit session for good.
	//
	// A single large fixed constant (tried first) removed the raciness but overcorrected: reporting
	// a width far larger than the column's real content apparently makes GTK's editing placement try
	// to accommodate the oversized "wanted" size by shifting the editor's left edge further left to
	// fit more of it within the row, which is exactly the "shifted too far left" regression that
	// produced. Cache the last *live* width actually observed instead, and only fall back to that
	// cached value when the live query looks like it's mid-negotiation (implausibly small) - accurate
	// in the normal (settled) case, since it keeps tracking the real live width every frame same as
	// before, but stable across the one narrow race window that actually matters, without ever
	// reporting a width wildly larger than what's really there.
	static int s_lastKnownWidth = 800; // ScriptDataViewModel::InitControl()'s own SetMinWidth() floor
	const int live_width = GetOwner()->GetWidth();
	if (live_width > 200)
	{
		s_lastKnownWidth = live_width;
	}
	// Height was GetTextExtent(m_value->ToString(m_gd)).GetHeight()) - ToString() embeds the raw
	// string content (e.g. ScriptStringEntry::ToString() appends the actual message text), and if
	// that text contains embedded newlines, GetTextExtent() reports a multi-line height even though
	// every Render*Properties() function only ever draws a single visual line (the preview text is
	// truncated/clipped, never wrapped). Japanese message text apparently embeds newlines far more
	// often than the other languages, so this was much more noticeable there specifically - the row
	// grew tall enough that the "End" separator line (drawn at a fixed offset below the row) ended
	// up well past where the actual single-line content was rendered. A fixed single-line probe
	// keeps row height consistent with what's actually drawn, regardless of the value's own content.
	return { s_lastKnownWidth, GetTextExtent("Xy").GetHeight() };
}

bool ScriptDataViewRenderer::SetValue(const wxVariant& value)
{
	m_value = Landstalker::ScriptTableEntry::FromBytes(m_gd->GetScriptData()->GetScript()->GetScriptLine(static_cast<uint16_t>(value.GetLong())).ToBytes());
	m_index = value.GetLong();
	return true;
}

bool ScriptDataViewRenderer::GetValue(wxVariant& value) const
{
	if (m_value)
	{
		value = static_cast<long>(m_value->ToBytes());
		return true;
	}
	return false;
}

bool ScriptDataViewRenderer::HasEditorCtrl() const
{
	return true;
}

wxWindow* ScriptDataViewRenderer::CreateEditorCtrl(wxWindow* parent, wxRect labelRect, const wxVariant& value)
{
	const auto& entry = m_gd->GetScriptData()->GetScript()->GetScriptLine(static_cast<uint16_t>(value.GetLong()));
	wxWindow* editor = CreateScriptEntryEditor(parent, labelRect, entry, m_gd);
	BindDataViewEditorEscapeEnter(editor, this);

	// GTK's own placement of this window starts some distance to the left of where the Index column
	// actually ends on screen, so a strip of this window always sits over the Index cell. Rather than
	// rely on that area getting redrawn by whatever's underneath once this window covers it (tried
	// previously, never confirmed reliable - see ScriptEntryEditorCtrl's comment), the editor draws
	// its own copy of the index number directly. GetItemRect() (scroll-aware, unlike the column's
	// plain logical GetWidth()) tells it how wide that strip actually is.
	wxDataViewColumn* value_column = GetOwner();
	wxDataViewCtrl* dvc = value_column->GetOwner();
	wxDataViewColumn* index_column = dvc->GetColumn(0);
	if (auto* entry_editor = dynamic_cast<ScriptEntryEditorCtrl*>(editor))
	{
		entry_editor->SetIndexCellReference(dvc, m_editingItem, index_column, wxString::Format("%ld", value.GetLong()));
	}
	// Separately from the self-drawn text above (which only helps where this editor's own window
	// actually covers the Index cell), explicitly ask the Index cell to repaint. On at least one
	// observed case this window doesn't cover that cell at all (no overlap, nothing for the self-
	// drawn text to help with) yet the cell still went blank while its row was being edited -
	// meaning nothing was invalidating that region on its own. Deferred via CallAfter() because an
	// immediate call here (tried previously, didn't help) runs before GTK's own edit-mode setup for
	// this row has actually happened - whatever that setup does to the row afterward would just
	// overwrite an immediate refresh; running after it is the only way this can be the last word.
	if (index_column)
	{
		wxDataViewItem editing_item = m_editingItem;
		editor->CallAfter([dvc, editing_item, index_column]()
		{
			dvc->RefreshRect(dvc->GetItemRect(editing_item, index_column));
		});
	}
	return editor;
}

bool ScriptDataViewRenderer::StartEditing(const wxDataViewItem& item, wxRect labelRect)
{
	// See the declaration's comment - refuse to (re-)start editing immediately after a previous
	// session on this renderer just ended, to break a rapid finish/restart cascade that otherwise
	// segfaults. 250ms is comfortably longer than the whole cascade observed via gdb (a handful of
	// cycles within well under a second) but short enough that a deliberate, separate double-click
	// to edit a cell again still works normally.
	if (m_lastEditEndTimeMs > 0 && (wxGetLocalTimeMillis() - m_lastEditEndTimeMs) < 250)
	{
		return false;
	}
	m_editingItem = item;
	return wxDataViewCustomRenderer::StartEditing(item, labelRect);
}

bool ScriptDataViewRenderer::FinishEditing()
{
	m_lastEditEndTimeMs = wxGetLocalTimeMillis();
	return wxDataViewCustomRenderer::FinishEditing();
}

void ScriptDataViewRenderer::CancelEditing()
{
	m_lastEditEndTimeMs = wxGetLocalTimeMillis();
	wxDataViewCustomRenderer::CancelEditing();
}

bool ScriptDataViewRenderer::GetValueFromEditorCtrl(wxWindow* ctrl, wxVariant& value)
{
	value = static_cast<long>(static_cast<ScriptEntryEditorCtrl*>(ctrl)->GetValue()->ToBytes());
	return true;
}
