#ifndef _SEARCHABLE_COMBO_UTILS_H_
#define _SEARCHABLE_COMBO_UTILS_H_

#include <wx/arrstr.h>
#include <wx/odcombo.h>

namespace SearchableComboUtils
{
	void AttachContainsAutocomplete(wxOwnerDrawnComboBox* combo, const wxArrayString& choices);
	void ConfigurePopup(wxOwnerDrawnComboBox* combo, const wxArrayString& choices, int popupMaxHeight = 480, int popupExtraWidth = 24);
	void SetupSearchableOwnerDrawnCombo(wxOwnerDrawnComboBox* combo, const wxArrayString& choices, int popupMaxHeight = 480, int popupExtraWidth = 24);
}

#endif // _SEARCHABLE_COMBO_UTILS_H_