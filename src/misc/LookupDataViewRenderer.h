#ifndef _LOOKUP_DATA_VIEW_RENDERER_H_
#define _LOOKUP_DATA_VIEW_RENDERER_H_

#include <wx/dataview.h>

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
