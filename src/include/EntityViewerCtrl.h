#ifndef _ENTITY_VIEWER_CTRL_H_
#define _ENTITY_VIEWER_CTRL_H_

#include <wx/wx.h>
#include <wx/vscroll.h>

class EntityViewerCtrl : public wxHVScrolledWindow
{
public:
	EntityViewerCtrl(wxWindow* parent);
	virtual ~EntityViewerCtrl();

private:
	virtual wxCoord OnGetRowHeight(size_t row) const override;
	virtual wxCoord OnGetColumnWidth(size_t column) const override;

	void OnDraw(wxDC& dc);
	void OnPaint(wxPaintEvent& evt);
	void OnSize(wxSizeEvent& evt);

	wxDECLARE_EVENT_TABLE();
};

#endif // _ENTITY_VIEWER_CTRL_H_
