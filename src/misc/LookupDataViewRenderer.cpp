#include <misc/LookupDataViewRenderer.h>

#include <algorithm>
#include <wx/combobox.h>
#include <wx/odcombo.h>

namespace
{
void AttachContainsAutocomplete(wxOwnerDrawnComboBox* combo, const wxArrayString& choices)
{
	combo->Bind(wxEVT_TEXT, [combo, choices](wxCommandEvent& evt)
	{
		const wxString input = combo->GetValue();
		const long caret = combo->GetInsertionPoint();
		if (input.empty())
		{
			evt.Skip();
			return;
		}

		const wxString needle = input.Lower();
		for (unsigned int i = 0; i < choices.GetCount(); ++i)
		{
			const wxString candidate = choices[i];
			if (candidate.Lower().Find(needle) != wxNOT_FOUND)
			{
				if (candidate != input)
				{
					combo->ChangeValue(candidate);
					combo->SetInsertionPoint(caret);
					combo->SetSelection(caret, static_cast<long>(candidate.Length()));
				}
				break;
			}
		}

		evt.Skip();
	});
}
}

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
		wxCB_DROPDOWN | wxTE_PROCESS_ENTER | wxWANTS_CHARS);
	combo->SetPopupMaxHeight(480);
	AttachContainsAutocomplete(combo, m_choices);
	combo->SetInsertionPointEnd();
	combo->SelectAll();
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
