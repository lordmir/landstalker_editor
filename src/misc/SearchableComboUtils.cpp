#include <misc/SearchableComboUtils.h>

#include <algorithm>
#include <memory>

namespace SearchableComboUtils
{
void AttachContainsAutocomplete(wxOwnerDrawnComboBox* combo, const wxArrayString& choices)
{
	auto in_update = std::make_shared<bool>(false);
	combo->Bind(wxEVT_TEXT, [combo, choices, in_update](wxCommandEvent& evt)
	{
		if (*in_update)
		{
			evt.Skip();
			return;
		}

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
					*in_update = true;
					combo->ChangeValue(candidate);
					combo->SetInsertionPoint(caret);
					combo->SetSelection(caret, static_cast<long>(candidate.Length()));
					*in_update = false;
				}
				break;
			}
		}

		evt.Skip();
	});
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
