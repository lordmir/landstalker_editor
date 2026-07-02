#include <misc/LookupDataViewRenderer.h>
#include <misc/SearchableComboUtils.h>

#include <algorithm>
#include <wx/combobox.h>
#include <wx/odcombo.h>

LookupDataViewRenderer::LookupDataViewRenderer(wxDataViewCellMode mode, wxArrayString choices)
	: wxDataViewCustomRenderer("long", mode, wxALIGN_LEFT),
	  m_choices(std::move(choices)),
	  m_value(0),
	  m_size_cached(false),
	  m_cached_size(80, 18)
{
}

bool LookupDataViewRenderer::Render(wxRect rect, wxDC* dc, int state)
{
	RenderText(FormatLabel(m_value), 2, rect, dc, state);
	return true;
}

bool LookupDataViewRenderer::ActivateCell(const wxRect& /*cell*/, wxDataViewModel* /*model*/, const wxDataViewItem& /*item*/,
	unsigned int /*col*/, const wxMouseEvent* /*mouseEvent*/)
{
	return false;
}

wxSize LookupDataViewRenderer::GetSize() const
{
	if (!m_size_cached)
	{
		int max_width = 0;
		int max_height = 0;

		if (m_choices.IsEmpty())
		{
			const wxSize sz = GetTextExtent("[0000] ???");
			max_width = sz.GetWidth();
			max_height = sz.GetHeight();
		}
		else
		{
			for (const auto& choice : m_choices)
			{
				const wxSize sz = GetTextExtent(choice);
				max_width = std::max(max_width, sz.GetWidth());
				max_height = std::max(max_height, sz.GetHeight());
			}
		}

		m_cached_size = { std::max(max_width + 6, 80), std::max(max_height + 2, 18) };
		m_size_cached = true;
	}

	return m_cached_size;
}

bool LookupDataViewRenderer::SetValue(const wxVariant& value)
{
	m_value = value.GetLong();
	return true;
}

bool LookupDataViewRenderer::GetValue(wxVariant& value) const
{
	value = m_value;
	return true;
}

bool LookupDataViewRenderer::HasEditorCtrl() const
{
	return true;
}

wxWindow* LookupDataViewRenderer::CreateEditorCtrl(wxWindow* parent, wxRect labelRect, const wxVariant& value)
{
	m_value = value.GetLong();
	auto* combo = new wxOwnerDrawnComboBox(parent, wxID_ANY, FormatLabel(m_value), labelRect.GetPosition(), labelRect.GetSize(), m_choices,
		wxCB_DROPDOWN | wxTE_PROCESS_ENTER);
	SearchableComboUtils::SetupSearchableOwnerDrawnCombo(combo, m_choices, 480, 24);
	const long text_len = static_cast<long>(combo->GetValue().Length());
	combo->SetSelection(0, text_len);
	combo->SetInsertionPoint(0);
	return combo;
}

bool LookupDataViewRenderer::GetValueFromEditorCtrl(wxWindow* ctrl, wxVariant& value)
{
	auto* combo = dynamic_cast<wxOwnerDrawnComboBox*>(ctrl);
	if (!combo)
	{
		return false;
	}
	value = ParseValue(combo->GetValue());
	return true;
}

wxString LookupDataViewRenderer::FormatLabel(long index) const
{
	if (IsValidIndex(index))
	{
		return m_choices[static_cast<std::size_t>(index)];
	}
	return wxString::Format("[%04ld] ???", index);
}

long LookupDataViewRenderer::ParseValue(const wxString& text) const
{
	wxString t = text;
	t.Trim(true);
	t.Trim(false);

	if (t.empty())
	{
		return m_value;
	}

	if (t.StartsWith("["))
	{
		const int close = t.Find(']');
		if (close != wxNOT_FOUND)
		{
			wxString idx = t.SubString(1, close - 1);
			long parsed = 0;
			if (idx.ToLong(&parsed) && IsValidIndex(parsed))
			{
				return parsed;
			}
		}
	}

	long parsed = 0;
	if (t.ToLong(&parsed) && IsValidIndex(parsed))
	{
		return parsed;
	}

	const wxString lower = t.Lower();
	for (std::size_t i = 0; i < m_choices.GetCount(); ++i)
	{
		if (m_choices[i].Lower() == lower)
		{
			return static_cast<long>(i);
		}
	}

	for (std::size_t i = 0; i < m_choices.GetCount(); ++i)
	{
		if (m_choices[i].Lower().Find(lower) != wxNOT_FOUND)
		{
			return static_cast<long>(i);
		}
	}

	return m_value;
}

bool LookupDataViewRenderer::IsValidIndex(long index) const
{
	return index >= 0 && index < static_cast<long>(m_choices.GetCount());
}
