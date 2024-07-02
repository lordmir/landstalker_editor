#include <EntityViewerCtrl.h>
#include <wx/dcbuffer.h>

wxBEGIN_EVENT_TABLE(EntityViewerCtrl, wxHVScrolledWindow)
EVT_PAINT(EntityViewerCtrl::OnPaint)
EVT_SIZE(EntityViewerCtrl::OnSize)
wxEND_EVENT_TABLE()

EntityViewerCtrl::EntityViewerCtrl(wxWindow* parent)
	: wxHVScrolledWindow(parent)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetRowColumnCount(0, 0);
}

EntityViewerCtrl::~EntityViewerCtrl()
{
}

wxCoord EntityViewerCtrl::OnGetRowHeight(size_t row) const
{
	return 1;
}

wxCoord EntityViewerCtrl::OnGetColumnWidth(size_t column) const
{
	return 1;
}

void EntityViewerCtrl::OnDraw(wxDC& dc)
{
	dc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE)));
	dc.Clear();

	wxPosition s = GetVisibleBegin();
	wxPosition e = GetVisibleEnd();

	dc.SetPen(*wxGREEN_PEN);
	dc.DrawLine(0, 0, 100, 100);
}

void EntityViewerCtrl::OnPaint(wxPaintEvent& evt)
{
	wxBufferedPaintDC dc(this);
	this->PrepareDC(dc);
	this->OnDraw(dc);
}

void EntityViewerCtrl::OnSize(wxSizeEvent& evt)
{
	wxVarHScrollHelper::HandleOnSize(evt);
	wxVarVScrollHelper::HandleOnSize(evt);
	evt.Skip();
}
