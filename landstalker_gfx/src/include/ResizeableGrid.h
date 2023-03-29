#ifndef _RESIZEABLE_GRID_H_
#define _RESIZEABLE_GRID_H_

#include <wx/grid.h>

#include <vector>

class ResizeableGrid : public wxGrid
{
public:
	DECLARE_DYNAMIC_CLASS(ResizeableGrid);

	ResizeableGrid() { }
	ResizeableGrid(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize, long style = wxWANTS_CHARS,
		const wxString& name = wxPanelNameStr);

	void SetColumnWidths(const std::vector<int>& arg_widths);

	void SizeColumns();
	void OnSize(wxSizeEvent& event);

	void ResizeGrid(int new_rows, int new_cols);

private:

	std::vector<int> m_columnWidths;

	DECLARE_EVENT_TABLE();
};

class CellTextEditor : public wxGridCellTextEditor
{
public:
	explicit CellTextEditor(size_t maxChars = 0)
		: wxGridCellTextEditor(maxChars)
	{
	}

	virtual void BeginEdit(int row, int col, wxGrid* grid) override;
};

#endif // _RESIZEABLE_GRID_H_
