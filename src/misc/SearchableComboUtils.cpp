#include <misc/SearchableComboUtils.h>

#include <algorithm>
#include <memory>
#include <wx/textcompleter.h>
#include <wx/textctrl.h>

namespace SearchableComboUtils
{
namespace
{
class ContainsTextCompleter : public wxTextCompleter
{
public:
	explicit ContainsTextCompleter(const wxArrayString& choices)
		: m_choices(choices),
		  m_match_idx(0)
	{
	}

	virtual bool Start(const wxString& prefix) override
	{
		m_matches.Clear();
		m_match_idx = 0;

		const wxString needle = prefix.Lower();
		for (const auto& choice : m_choices)
		{
			if (needle.IsEmpty() || choice.Lower().Find(needle) != wxNOT_FOUND)
			{
				m_matches.Add(choice);
			}
		}

		return !m_matches.IsEmpty();
	}

	virtual wxString GetNext() override
	{
		if (m_match_idx >= m_matches.GetCount())
		{
			return wxString();
		}
		return m_matches[m_match_idx++];
	}

private:
	wxArrayString m_choices;
	wxArrayString m_matches;
	unsigned int m_match_idx;
};
}

void AttachContainsAutocomplete(wxOwnerDrawnComboBox* combo, const wxArrayString& choices)
{
	auto in_update = std::make_shared<bool>(false);
	auto first_match = std::make_shared<int>(wxNOT_FOUND);
	if (auto* text = combo->GetTextCtrl())
	{
		text->AutoComplete(new ContainsTextCompleter(choices));
	}
	else
	{
		combo->AutoComplete(new ContainsTextCompleter(choices));
	}
	combo->Bind(wxEVT_TEXT, [combo, choices, in_update, first_match](wxCommandEvent& evt)
	{
		if (*in_update)
		{
			evt.Skip();
			return;
		}

		const wxString input = combo->GetValue();
		const wxString needle = input.Lower();
		*first_match = wxNOT_FOUND;
		bool has_matches = needle.IsEmpty();

		for (unsigned int i = 0; i < choices.GetCount(); ++i)
		{
			const wxString candidate = choices[i];
			if (needle.IsEmpty() || candidate.Lower().Find(needle) != wxNOT_FOUND)
			{
				has_matches = true;
				if (*first_match == wxNOT_FOUND)
				{
					*first_match = static_cast<int>(i);
				}
			}
		}

		if (!has_matches && combo->IsPopupShown())
		{
			combo->HidePopup();
		}

		evt.Skip();
	});

	auto on_key = [combo, choices, in_update, first_match](wxKeyEvent& evt)
	{
		if (evt.GetKeyCode() == WXK_ESCAPE && combo->IsPopupShown())
		{
			combo->HidePopup();
			return;
		}

		if (evt.GetKeyCode() == WXK_DOWN)
		{
			if (!combo->IsPopupShown())
			{
				if (*first_match != wxNOT_FOUND)
				{
					combo->Popup();
					if (*first_match != wxNOT_FOUND)
					{
						combo->SetSelection(*first_match);
					}
				}
				return;
			}
			evt.Skip();
			return;
		}

		if ((evt.GetKeyCode() == WXK_RETURN || evt.GetKeyCode() == WXK_NUMPAD_ENTER) && combo->IsPopupShown())
		{
			int sel = combo->GetSelection();
			if (sel == wxNOT_FOUND)
			{
				sel = *first_match;
			}

			if (sel != wxNOT_FOUND && sel >= 0 && sel < static_cast<int>(choices.GetCount()))
			{
				const wxString selected = combo->GetString(sel);
				*in_update = true;
				combo->ChangeValue(selected);
				combo->SetInsertionPointEnd();
				combo->SetSelection(selected.Length(), selected.Length());
				*in_update = false;
			}

			combo->HidePopup();
			return;
		}

		evt.Skip();
	};

	if (auto* text = combo->GetTextCtrl())
	{
		text->Bind(wxEVT_CHAR_HOOK, on_key);
	}
	else
	{
		combo->Bind(wxEVT_CHAR_HOOK, on_key);
	}
}

void ConfigurePopup(wxOwnerDrawnComboBox* combo, const wxArrayString& choices, int popupMaxHeight, int popupExtraWidth)
{
	int max_width = 0;
	for (const auto& entry : choices)
	{
		const wxSize sz = combo->GetTextExtent(entry);
		max_width = std::max(max_width, sz.GetWidth());
	}

	combo->SetPopupMinWidth(std::max(combo->GetSize().GetWidth(), max_width + popupExtraWidth));
	combo->SetPopupMaxHeight(popupMaxHeight);
}

void SetupSearchableOwnerDrawnCombo(wxOwnerDrawnComboBox* combo, const wxArrayString& choices, int popupMaxHeight, int popupExtraWidth)
{
	ConfigurePopup(combo, choices, popupMaxHeight, popupExtraWidth);
	AttachContainsAutocomplete(combo, choices);
}
}
