#ifndef _LOOKUP_DATA_VIEW_RENDERER_H_
#define _LOOKUP_DATA_VIEW_RENDERER_H_

#include <wx/dataview.h>

// Standalone access to the searchable autocomplete combo widget (text box + drop button + filtered
// popup list) that LookupDataViewRenderer's own cell editor uses internally - for embedding
// directly inside another composite editor rather than as a whole wxDataViewColumn's cell editor.
// `renderer`, if given, is whichever wxDataViewRenderer owns the top-level editor window this
// control ends up nested inside (needed so Escape can call CancelEditing() on it); pass nullptr if
// there is none.
namespace LookupEditor
{
    wxWindow* Create(wxWindow* parent, const wxRect& rect, const wxString& value,
                      const wxArrayString& choices, wxDataViewRenderer* renderer);
    // Pulls out whatever the control currently has selected/typed, resolving any pending (not yet
    // committed) popup-list selection first - call this before GetValueText() when reading the
    // final value back out (mirrors what LookupDataViewRenderer::GetValueFromEditorCtrl() does).
    void CommitPendingSelection(wxWindow* ctrl);
    wxString GetValueText(wxWindow* ctrl);

    wxString FormatLabel(const wxArrayString& choices, long index);
    // Resolves display text back to a choices[] index: exact match, "[NNNN] ..." prefix, or a
    // substring match; falls back to `fallback` (e.g. the value the control was opened with) if
    // nothing matches.
    long ParseValue(const wxArrayString& choices, const wxString& text, long fallback);
    // For combos that accept free-typed text not necessarily in `choices` (e.g. a not-yet-created
    // function name) - lets the caller apply the same red/black validity colouring convention used
    // on plain wxTextCtrl fields elsewhere.
    void SetTextColour(wxWindow* ctrl, const wxColour& colour);
}

class LookupDataViewRenderer : public wxDataViewCustomRenderer
{
public:
	LookupDataViewRenderer(wxDataViewCellMode mode, wxArrayString choices);
	virtual ~LookupDataViewRenderer() {}

	virtual bool Render(wxRect rect, wxDC* dc, int state) override;
	virtual bool ActivateCell(const wxRect& cell, wxDataViewModel* model, const wxDataViewItem& item,
		unsigned int col, const wxMouseEvent* mouseEvent) override;
	virtual wxSize GetSize() const override;
	virtual bool SetValue(const wxVariant& value) override;
	virtual bool GetValue(wxVariant& value) const override;
	virtual bool HasEditorCtrl() const override;
	virtual wxWindow* CreateEditorCtrl(wxWindow* parent, wxRect labelRect, const wxVariant& value) override;
	virtual bool GetValueFromEditorCtrl(wxWindow* ctrl, wxVariant& value) override;

private:
	wxString FormatLabel(long index) const;
	long ParseValue(const wxString& text) const;
	bool IsValidIndex(long index) const;

	wxArrayString m_choices;
	mutable long m_value;
	mutable bool m_size_cached;
	mutable wxSize m_cached_size;
};

#endif // _LOOKUP_DATA_VIEW_RENDERER_H_
