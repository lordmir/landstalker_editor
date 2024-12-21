#include <user_interface/script/include/ScriptDataViewRenderer.h>
#include <user_interface/script/include/ScriptDataViewEditorControl.h>
#include <functional>
#include <wx/dc.h>
#include <wx/renderer.h>
#include <landstalker/main/include/GameData.h>

static const int LABEL_WIDTH = 180;
static const int COLUMN_WIDTH = 80;

ScriptDataViewRenderer::ScriptDataViewRenderer(wxDataViewCellMode mode, std::shared_ptr<GameData> gd)
	: wxDataViewCustomRenderer("long", mode, wxALIGN_LEFT),
	  m_index(-1),
	  m_gd(gd)
{
}

bool ScriptDataViewRenderer::Render(wxRect rect, wxDC* dc, int state)
{
	if (m_value)
	{
		RenderLabel(rect, dc, state);
		if (m_value && m_value->GetEnd())
		{
			auto win = GetOwner()->GetOwner();
			dc->SetPen(wxPen(wxColour(128, 128, 128), 1, wxPENSTYLE_SHORT_DASH));
			dc->DrawLine(rect.GetLeft(), rect.GetBottom() + 4, rect.GetLeft() + win->GetSize().GetWidth(), rect.GetBottom() + 4);
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
	case ScriptTableEntryType::STRING:
		return RenderStringProperties(rect, dc, state);
	case ScriptTableEntryType::ITEM_LOAD:
		return RenderSetItemProperties(rect, dc, state);
	case ScriptTableEntryType::NUMBER_LOAD:
		return RenderSetNumberProperties(rect, dc, state);
	case ScriptTableEntryType::GIVE_ITEM:
		return RenderGiveItemProperties(rect, dc, state);
	case ScriptTableEntryType::GIVE_MONEY:
		return RenderGiveMoneyProperties(rect, dc, state);
	case ScriptTableEntryType::SET_FLAG:
		return RenderSetFlagProperties(rect, dc, state);
	case ScriptTableEntryType::PLAY_BGM:
		return RenderPlayBGMProperties(rect, dc, state);
	case ScriptTableEntryType::PLAY_CUTSCENE:
		return RenderCutsceneProperties(rect, dc, state);
	case ScriptTableEntryType::SET_SPEAKER:
		return RenderSetSpeakerProperties(rect, dc, state);
	case ScriptTableEntryType::SET_GLOBAL_SPEAKER:
		return RenderSetGlobalSpeakerProperties(rect, dc, state);
	case ScriptTableEntryType::GLOBAL_CHAR_LOAD:
		return RenderLoadGlobalSpeakerProperties(rect, dc, state);
	case ScriptTableEntryType::INVALID:
	default:
		return RenderInvalidProperties(rect, dc, state);
	}
}

void ScriptDataViewRenderer::InsertRenderLabel(wxRect& rect, wxDC* dc, int state, const wxString& text, int min_width, const wxFont* font)
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
	RenderText(text, 2, rect, dc, state);
	rect.y -= y_offset;
	int new_width = std::max(extent.GetWidth() + 2, min_width);
	rect.x += new_width;
	rect.width -= new_width;
	if (font)
	{
		dc->SetFont(orig_font);
	}
}

void ScriptDataViewRenderer::InsertRenderBubble(wxRect& rect, wxDC* dc, int state, const wxString& text, const wxColour& colour, int min_width, const wxFont* font)
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

	dc->SetBrush(wxBrush(colour));
	dc->DrawRoundedRectangle(draw_rect, 3);
	RenderText(text, 2, draw_rect, dc, state);
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
	InsertRenderLabel(rect, dc, state, text, 2);
	wxWindow* const win = GetOwner()->GetOwner();
	wxRendererNative& renderer = wxRendererNative::Get();
	wxRect check_rect(wxRendererNative::Get().GetCheckBoxSize(GetView()));
	check_rect.x = rect.x + 5;
	check_rect.y = rect.y + (check_rect.height < rect.height ? (rect.height - check_rect.height) / 2 : 0);
	renderer.DrawCheckBox(win, *dc, check_rect, checkstate ? wxCONTROL_CHECKED : wxCONTROL_NONE);
	int new_width = std::max(check_rect.GetWidth() + 12, min_width);
	rect.x += new_width;
	rect.width -= new_width;
	dc->SetFont(orig_font);
}

bool ScriptDataViewRenderer::RenderInvalidProperties(wxRect& rect, wxDC* dc, int state)
{
	wxFont font = dc->GetFont().Bold();
	wxColour orig_col = dc->GetTextForeground();
	dc->SetTextForeground(*wxWHITE);
	InsertRenderBubble(rect, dc, state, "INVALID", *wxRED, LABEL_WIDTH, &font);
	dc->SetTextForeground(*wxRED);
	font.SetFamily(wxFONTFAMILY_TELETYPE);
	InsertRenderLabel(rect, dc, state, StrPrintf("%04X", m_index), 40, &font);
	dc->SetTextForeground(orig_col);
	return true;
}

bool ScriptDataViewRenderer::RenderStringProperties(wxRect& rect, wxDC * dc, int state)
{
	const auto& string = dynamic_cast<ScriptStringEntry&>(*m_value);
	wxFont font = dc->GetFont().Bold();
	std::wstring string_preview = L"";
	if (m_gd && m_gd->GetStringData()->GetStringCount(StringData::Type::MAIN) > static_cast<std::size_t>(m_gd->GetScriptData()->GetStringStart() + string.string))
	{
		string_preview = m_gd->GetStringData()->GetString(StringData::Type::MAIN, m_gd->GetScriptData()->GetStringStart() + string.string);
	}
	InsertRenderBubble(rect, dc, state, "STRING", *wxCYAN, LABEL_WIDTH, &font);
	InsertRenderLabel(rect, dc, state, "String:", COLUMN_WIDTH, &font);
	InsertRenderLabel(rect, dc, state, StrWPrintf(L"%04d", string.string + m_gd->GetScriptData()->GetStringStart()), 40);
	font = dc->GetFont().Italic();
	font.SetFamily(wxFONTFAMILY_TELETYPE);
	InsertRenderLabel(rect, dc, state, string_preview, 0, &font);
	return string.end;
}

bool ScriptDataViewRenderer::RenderCutsceneProperties(wxRect& rect, wxDC* dc, int state)
{
	const auto& cutscene = dynamic_cast<ScriptInitiateCutsceneEntry&>(*m_value);
	wxFont font = dc->GetFont().Bold();
	wxColour orig_col = dc->GetTextForeground();
	dc->SetTextForeground(*wxWHITE);
	InsertRenderBubble(rect, dc, state, "PLAY CUTSCENE", *wxBLUE, LABEL_WIDTH, &font);
	dc->SetTextForeground(orig_col);
	InsertRenderLabel(rect, dc, state, _("Cutscene Index: "), COLUMN_WIDTH, &font);
	InsertRenderLabel(rect, dc, state, StrPrintf("%04d", cutscene.cutscene), 40);
	font = dc->GetFont().Italic();
	font.SetFamily(wxFONTFAMILY_TELETYPE);
	InsertRenderLabel(rect, dc, state, ScriptData::GetCutsceneDisplayName(cutscene.cutscene), 0, &font);
	return true;
}

bool ScriptDataViewRenderer::RenderSetItemProperties(wxRect& rect, wxDC* dc, int state)
{
	const auto& item_set = dynamic_cast<ScriptItemLoadEntry&>(*m_value);
	std::wstring item_name = m_gd->GetStringData()->GetItemDisplayName(item_set.item);
	wxFont font = dc->GetFont().Bold();
	wxColour orig_col = dc->GetTextForeground();
	dc->SetTextForeground(*wxWHITE);
	InsertRenderBubble(rect, dc, state, "SET ITEM", wxColour("DARK ORCHID"), LABEL_WIDTH, &font);
	dc->SetTextForeground(orig_col);
	InsertRenderLabel(rect, dc, state, _("Item Slot: "), COLUMN_WIDTH, &font);
	InsertRenderLabel(rect, dc, state, StrPrintf("%01d", item_set.slot + 1), 40);
	InsertRenderLabel(rect, dc, state, _("Item: "), 0, &font);
	InsertRenderLabel(rect, dc, state, StrPrintf("%02d", item_set.item), 40);
	font = dc->GetFont().Italic();
	font.SetFamily(wxFONTFAMILY_TELETYPE);
	InsertRenderLabel(rect, dc, state, item_name, 0, &font);
	return false;
}

bool ScriptDataViewRenderer::RenderSetNumberProperties(wxRect& rect, wxDC* dc, int state)
{
	const auto& num_set = dynamic_cast<ScriptNumLoadEntry&>(*m_value);
	wxFont font = dc->GetFont().Bold();
	wxColour orig_col = dc->GetTextForeground();
	dc->SetTextForeground(*wxWHITE);
	InsertRenderBubble(rect, dc, state, "SET NUMBER", wxColour("MAROON"), LABEL_WIDTH, &font);
	dc->SetTextForeground(orig_col);
	InsertRenderLabel(rect, dc, state, _("Number: "), COLUMN_WIDTH, &font);
	InsertRenderLabel(rect, dc, state, StrPrintf("%d", num_set.num), 40);
	return false;
}

bool ScriptDataViewRenderer::RenderGiveItemProperties(wxRect& rect, wxDC* dc, int state)
{
	wxFont font = dc->GetFont().Bold();
	wxColour orig_col = dc->GetTextForeground();
	dc->SetTextForeground(*wxWHITE);
	InsertRenderBubble(rect, dc, state, "GIVE ITEM TO PLAYER", wxColour("FOREST GREEN"), LABEL_WIDTH, &font);
	dc->SetTextForeground(orig_col);
	return false;
}

bool ScriptDataViewRenderer::RenderGiveMoneyProperties(wxRect& rect, wxDC* dc, int state)
{
	wxFont font = dc->GetFont().Bold();
	wxColour orig_col = dc->GetTextForeground();
	dc->SetTextForeground(*wxWHITE);
	InsertRenderBubble(rect, dc, state, "GIVE MONEY TO PLAYER", wxColour("DARK SLATE BLUE"), LABEL_WIDTH, &font);
	dc->SetTextForeground(orig_col);
	return false;
}

bool ScriptDataViewRenderer::RenderSetFlagProperties(wxRect& rect, wxDC* dc, int state)
{
	const auto& flag_set = dynamic_cast<ScriptSetFlagEntry&>(*m_value);
	wxFont font = dc->GetFont().Bold();
	wxColour orig_col = dc->GetTextForeground();
	InsertRenderBubble(rect, dc, state, "SET FLAG", wxColour("MEDIUM SPRING GREEN"), LABEL_WIDTH, &font);
	InsertRenderLabel(rect, dc, state, _("Flag: "), COLUMN_WIDTH, &font);
	InsertRenderLabel(rect, dc, state, StrPrintf("%04d", flag_set.flag), 0);
	font = dc->GetFont().Italic();
	font.SetFamily(wxFONTFAMILY_TELETYPE);
	InsertRenderLabel(rect, dc, state, ScriptData::GetFlagDisplayName(flag_set.flag), 0, &font);
	return false;
}

bool ScriptDataViewRenderer::RenderPlayBGMProperties(wxRect& rect, wxDC* dc, int state)
{
	const auto& bgm = dynamic_cast<ScriptPlayBgmEntry&>(*m_value);
	wxFont font = dc->GetFont().Bold();
	wxColour orig_col = dc->GetTextForeground();
	InsertRenderBubble(rect, dc, state, "PLAY BGM", wxColour("LIGHT STEEL BLUE"), LABEL_WIDTH, &font);
	InsertRenderLabel(rect, dc, state, _("BGM: "), COLUMN_WIDTH, &font);
	InsertRenderLabel(rect, dc, state, StrPrintf("%01d", bgm.bgm), 40);
	if (bgm.bgm < ScriptPlayBgmEntry::BGMS.size() && Labels::Get(Labels::C_SOUNDS, ScriptPlayBgmEntry::BGMS.at(bgm.bgm)))
	{
		font = dc->GetFont().Italic();
		font.SetFamily(wxFONTFAMILY_TELETYPE);
		InsertRenderLabel(rect, dc, state, *Labels::Get(Labels::C_SOUNDS, ScriptPlayBgmEntry::BGMS.at(bgm.bgm)), 40, &font);
	}
	return false;
}

bool ScriptDataViewRenderer::RenderSetSpeakerProperties(wxRect& rect, wxDC* dc, int state)
{
	const auto& speaker = dynamic_cast<ScriptSetSpeakerEntry&>(*m_value);
	wxFont font = dc->GetFont().Bold();
	wxColour orig_col = dc->GetTextForeground();
	dc->SetTextForeground(*wxWHITE);
	InsertRenderBubble(rect, dc, state, "SET SPEAKER", wxColour("DARK GREEN"), LABEL_WIDTH, &font);
	dc->SetTextForeground(orig_col);
	InsertRenderLabel(rect, dc, state, _("Character: "), COLUMN_WIDTH, &font);
	InsertRenderLabel(rect, dc, state, StrPrintf("%03d", speaker.chr), 40);
	font = dc->GetFont().Italic();
	font.SetFamily(wxFONTFAMILY_TELETYPE);
	InsertRenderLabel(rect, dc, state, m_gd->GetStringData()->GetCharacterDisplayName(speaker.chr), 100, &font);
	return false;
}

bool ScriptDataViewRenderer::RenderSetGlobalSpeakerProperties(wxRect& rect, wxDC* dc, int state)
{
	const auto& speaker = dynamic_cast<ScriptSetGlobalSpeakerEntry&>(*m_value);
	wxFont font = dc->GetFont().Bold();
	wxColour orig_col = dc->GetTextForeground();
	InsertRenderBubble(rect, dc, state, "SET GLOBAL SPEAKER", wxColour("ORANGE"), LABEL_WIDTH, &font);
	InsertRenderLabel(rect, dc, state, _("Speaker: "), COLUMN_WIDTH, &font);
	InsertRenderLabel(rect, dc, state, StrPrintf("%03d", speaker.chr), 40);
	font = dc->GetFont().Italic();
	font.SetFamily(wxFONTFAMILY_TELETYPE);
	InsertRenderLabel(rect, dc, state, m_gd->GetStringData()->GetGlobalCharacterDisplayName(speaker.chr), 100, &font);
	return false;
}

bool ScriptDataViewRenderer::RenderLoadGlobalSpeakerProperties(wxRect& rect, wxDC* dc, int state)
{
	const auto& chr = dynamic_cast<ScriptGlobalCharLoadEntry&>(*m_value);
	wxFont font = dc->GetFont().Bold();
	wxColour orig_col = dc->GetTextForeground();
	InsertRenderBubble(rect, dc, state, "LOAD GLOBAL CHAR", wxColour("GOLD"), LABEL_WIDTH, &font);
	InsertRenderLabel(rect, dc, state, _("Slot: "), COLUMN_WIDTH, &font);
	InsertRenderLabel(rect, dc, state, StrPrintf("%01d", chr.slot + 1), 40);
	InsertRenderLabel(rect, dc, state, _("Character: "), 0, &font);
	InsertRenderLabel(rect, dc, state, StrPrintf("%02d", chr.chr), 40);
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
	return { GetOwner()->GetOwner()->GetSize().GetWidth(), GetTextExtent(m_value->ToString(m_gd)).GetHeight() };
}

bool ScriptDataViewRenderer::SetValue(const wxVariant& value)
{
	m_value = ScriptTableEntry::FromBytes(m_gd->GetScriptData()->GetScript()->GetScriptLine(static_cast<uint16_t>(value.GetLong())).ToBytes());
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
	return new ScriptDataViewEditorControl(parent, labelRect, static_cast<uint16_t>(value.GetLong()), m_gd);
}

bool ScriptDataViewRenderer::GetValueFromEditorCtrl(wxWindow* ctrl, wxVariant& value)
{
	value = static_cast<long>(static_cast<ScriptDataViewEditorControl*>(ctrl)->GetValue());
	return true;
}
