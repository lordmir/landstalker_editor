#include <user_interface/script/include/DataViewScriptActionRenderer.h>
#include <user_interface/script/include/DataViewScriptActionEditorControl.h>

DataViewScriptActionRenderer::DataViewScriptActionRenderer(wxDataViewCellMode mode, std::shared_ptr<GameData> gd)
	: wxDataViewCustomRenderer("string", mode, wxALIGN_LEFT),
	m_index(-1),
	m_gd(gd)
{
}

bool DataViewScriptActionRenderer::Render(wxRect rect, wxDC* dc, int state)
{
	RenderText(ToString(m_value), 2, { rect.GetPosition(), wxSize{150, rect.GetHeight()} }, dc, state);
	RenderText(std::visit([this](const auto& arg)
		{
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, uint16_t>)
			{
				if (arg < m_gd->GetScriptData()->GetScript()->GetScriptLineCount())
				{
					return _(m_gd->GetScriptData()->GetScript()->GetScriptLine(arg).ToString(m_gd));
				}
				else
				{
					return _("Invalid Script ID");
				}
			}
			else if constexpr (std::is_same_v<T, std::string>)
			{
				return _(StrPrintf("Jump to function %s", arg.c_str()));
			}
		}, m_value), 152, rect, dc, state);
	return true;
}

bool DataViewScriptActionRenderer::ActivateCell(const wxRect& /*cell*/, wxDataViewModel* /*model*/, const wxDataViewItem& /*item*/, unsigned int /*col*/, const wxMouseEvent* /*mouseEvent*/)
{
	return false;
}

wxSize DataViewScriptActionRenderer::GetSize() const
{
	return { GetOwner()->GetOwner()->GetSize().GetWidth(), GetTextExtent(ToString(m_value)).GetHeight() };
}

bool DataViewScriptActionRenderer::SetValue(const wxVariant& value)
{
	m_value = FromString(value.GetString().ToStdString());
	return true;
}

bool DataViewScriptActionRenderer::GetValue(wxVariant& value) const
{
	value = ToString(m_value);
	return true;
}

bool DataViewScriptActionRenderer::HasEditorCtrl() const
{
	return true;
}

wxWindow* DataViewScriptActionRenderer::CreateEditorCtrl(wxWindow* parent, wxRect labelRect, const wxVariant& value)
{
	m_value = FromString(value.GetString());
	return new DataViewScriptActionEditorControl(parent, labelRect, m_value, {}, m_gd);
}

bool DataViewScriptActionRenderer::GetValueFromEditorCtrl(wxWindow* ctrl, wxVariant& value)
{
	ScriptTable::Action action = (static_cast<DataViewScriptActionEditorControl*>(ctrl)->GetValue());
	value = ToString(action);
	return true;
}

std::string DataViewScriptActionRenderer::ToString(const ScriptTable::Action& action) const
{
	return std::visit([](const auto& arg) {
		using T = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<T, uint16_t>)
		{
			return std::to_string(arg);
		}
		else if constexpr (std::is_same_v<T, std::string>)
		{
			return arg;
		}
	}, action);
}

ScriptTable::Action DataViewScriptActionRenderer::FromString(const wxString& string) const
{
	unsigned long val = 0;
	if (string.ToULong(&val, 0))
	{
		return static_cast<uint16_t>(val);
	}
	return string.ToStdString();
}
