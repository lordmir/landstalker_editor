#include <user_interface/script/include/ScriptDataViewRenderer.h>
#include <user_interface/script/include/ScriptDataViewEditorControl.h>
#include <wx/dc.h>
#include <wx/renderer.h>
#include <landstalker/main/include/GameData.h>

ScriptDataViewRenderer::ScriptDataViewRenderer(wxDataViewCellMode mode, std::shared_ptr<GameData> gd)
	: wxDataViewCustomRenderer("long", mode, wxALIGN_LEFT),
	  m_gd(gd)
{
}

bool ScriptDataViewRenderer::Render(wxRect rect, wxDC* dc, int state)
{
	auto brush = wxBrush();
	if (m_value)
	{
		switch (m_value->GetType())
		{
		case ScriptTableEntryType::INVALID:
			brush.SetColour(*wxRED);
			break;
		case ScriptTableEntryType::STRING:
			return RenderString(rect, dc, state);
		case ScriptTableEntryType::ITEM_LOAD:
			brush.SetColour(*wxGREEN);
			break;
		case ScriptTableEntryType::NUMBER_LOAD:
			brush.SetColour(*wxYELLOW);
			break;
		case ScriptTableEntryType::GIVE_ITEM:
			brush.SetColour(wxColour(255,0,255));
			break;
		case ScriptTableEntryType::GIVE_MONEY:
			brush.SetColour(wxColour(128,255,128));
			break;
		case ScriptTableEntryType::SET_FLAG:
			brush.SetColour(wxColour(128, 128, 255));
			break;
		case ScriptTableEntryType::PLAY_BGM:
			brush.SetColour(wxColour(255, 128, 255));
			break;
		case ScriptTableEntryType::PLAY_CUTSCENE:
			brush.SetColour(wxColour(255, 128, 128));
			break;
		case ScriptTableEntryType::SET_SPEAKER:
			brush.SetColour(wxColour(255, 255, 128));
			break;
		case ScriptTableEntryType::SET_GLOBAL_SPEAKER:
			brush.SetColour(wxColour(255, 192, 128));
			break;
		case ScriptTableEntryType::GLOBAL_CHAR_LOAD:
			brush.SetColour(wxColour(255, 255, 64));
			break;
		default:
			brush.SetColour(wxColour(100, 100, 100));
			break;
		}
	}
	dc->SetBrush(brush);
	rect.width += 2;
	rect.y += 2;
	dc->DrawRoundedRectangle(rect, 3);
	if (m_value && m_gd)
	{
		RenderText(_(m_value->ToString(m_gd)), 2, rect, dc, state);
	}
	return true;
}

bool ScriptDataViewRenderer::RenderString(wxRect rect, wxDC* dc, int state)
{
	const wxString CLEAR_LABEL = "Clear: ";
	const wxString END_LABEL = "End: ";
	const wxString STRING_LABEL = "String: ";
	const auto& string = dynamic_cast<ScriptStringEntry&>(*m_value);
	const wxSize DEFAULT_CHECKBOX_SIZE = wxRendererNative::Get().GetCheckBoxSize(GetView());
	std::wstring string_preview = !m_gd ? L"" : m_gd->GetStringData()->GetString(StringData::Type::MAIN, m_gd->GetScriptData()->GetStringStart() + string.string);

	wxRendererNative& renderer = wxRendererNative::Get();
	wxWindow* const win = GetOwner()->GetOwner();

	wxSize size = rect.GetSize();
	size.IncTo(GetSize());
	rect.SetSize(size);
	wxFont font = dc->GetFont();

	wxRect clear_check_rect(DEFAULT_CHECKBOX_SIZE), clear_label_rect(rect),
		end_check_rect(DEFAULT_CHECKBOX_SIZE), end_label_rect(rect),
		string_label_rect(rect), string_id_rect(rect), string_preview_rect(rect);
	
	dc->SetFont(string.clear_box ? font.Bold() : font);
	clear_label_rect.width = GetTextExtent(CLEAR_LABEL).GetWidth() + 10;
	RenderText(CLEAR_LABEL, 0, clear_label_rect, dc, state);
	clear_check_rect.x = clear_label_rect.GetRight();
	clear_check_rect.y = rect.y + (clear_check_rect.height < rect.height ? (rect.height - clear_check_rect.height) / 2 : 0);
	renderer.DrawCheckBox(win, *dc, clear_check_rect, string.clear_box ? wxCONTROL_CHECKED : 0);

	dc->SetFont(string.end ? font.Bold() : font);
	end_label_rect.width = GetTextExtent(END_LABEL).GetWidth() + 10;
	end_label_rect.x = clear_check_rect.GetRight() + clear_check_rect.GetWidth();
	RenderText(END_LABEL, 0, end_label_rect, dc, state);
	end_check_rect.x = end_label_rect.GetRight();
	end_check_rect.y = clear_check_rect.y;
	renderer.DrawCheckBox(win, *dc, end_check_rect, string.end ? wxCONTROL_CHECKED : 0);

	dc->SetFont(font);
	string_label_rect.width = GetTextExtent(STRING_LABEL).GetWidth() + 10;
	string_label_rect.x = end_check_rect.GetRight() + end_check_rect.GetWidth();
	RenderText(STRING_LABEL, 0, string_label_rect, dc, state);
	string_id_rect.width = GetTextExtent("00000").GetWidth();
	string_id_rect.x = string_label_rect.GetRight() + 10;
	RenderText(StrWPrintf(L"%04d", string.string + m_gd->GetScriptData()->GetStringStart()), 0, string_id_rect, dc, state);
	dc->SetFont(font.Italic());
	string_preview_rect.width = GetTextExtent(string_preview).GetWidth();
	string_preview_rect.x = string_id_rect.GetRight() + 20;
	dc->SetBrush(*wxCYAN_BRUSH);
	string_preview_rect.width += 4;
	dc->DrawRoundedRectangle(string_preview_rect, 3);
	RenderText(string_preview, 2, string_preview_rect, dc, state);
	dc->SetFont(font);

	if (string.end)
	{
		dc->SetPen(wxPen(wxColour(128, 128, 128), 1, wxPENSTYLE_SHORT_DASH));
		dc->DrawLine(rect.GetLeft(), rect.GetBottom() + 4, rect.GetLeft() + win->GetSize().GetWidth(), rect.GetBottom() + 4);
		dc->SetPen(*wxTRANSPARENT_PEN);
	}

	return true;
}

bool ScriptDataViewRenderer::ActivateCell(const wxRect& /*cell*/, wxDataViewModel* /*model*/, const wxDataViewItem& /*item*/, unsigned int /*col*/, const wxMouseEvent* /*mouseEvent*/)
{
	return false;
}

wxSize ScriptDataViewRenderer::GetSize() const
{
	return GetTextExtent(m_value->ToString(m_gd));
}

bool ScriptDataViewRenderer::SetValue(const wxVariant& value)
{
	m_value = ScriptTableEntry::FromBytes(static_cast<uint16_t>(value.GetLong()));
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
