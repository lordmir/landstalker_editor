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
	RenderText(ScriptTable::ToString(m_value), 2, { rect.GetPosition(), wxSize{150, rect.GetHeight()} }, dc, state);
	RenderText(ScriptTable::GetActionSummary(m_value, m_gd), 152, rect, dc, state);
	return true;
}

bool DataViewScriptActionRenderer::ActivateCell(const wxRect& /*cell*/, wxDataViewModel* /*model*/, const wxDataViewItem& /*item*/, unsigned int /*col*/, const wxMouseEvent* /*mouseEvent*/)
{
	return false;
}

wxSize DataViewScriptActionRenderer::GetSize() const
{
	return { GetOwner()->GetOwner()->GetSize().GetWidth(), GetTextExtent(ScriptTable::ToString(m_value)).GetHeight() };
}

bool DataViewScriptActionRenderer::SetValue(const wxVariant& value)
{
	m_value = ScriptTable::FromString(value.GetString().ToStdString());
	return true;
}

bool DataViewScriptActionRenderer::GetValue(wxVariant& value) const
{
	value = ScriptTable::ToString(m_value);
	return true;
}

bool DataViewScriptActionRenderer::HasEditorCtrl() const
{
	return true;
}

wxWindow* DataViewScriptActionRenderer::CreateEditorCtrl(wxWindow* parent, wxRect labelRect, const wxVariant& value)
{
	m_value = ScriptTable::FromString(value.GetString().ToStdString());
	return new DataViewScriptActionEditorControl(parent, labelRect, m_value, {}, m_gd);
}

bool DataViewScriptActionRenderer::GetValueFromEditorCtrl(wxWindow* ctrl, wxVariant& value)
{
	ScriptTable::Action action = (static_cast<DataViewScriptActionEditorControl*>(ctrl)->GetValue());
	value = ScriptTable::ToString(action);
	return true;
}
