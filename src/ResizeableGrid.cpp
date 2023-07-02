#include "ResizeableGrid.h"
#include "wx/textctrl.h"

IMPLEMENT_DYNAMIC_CLASS(ResizeableGrid, wxGrid);

BEGIN_EVENT_TABLE(ResizeableGrid, wxGrid)
EVT_SIZE(ResizeableGrid::OnSize)
END_EVENT_TABLE()

ResizeableGrid::ResizeableGrid(wxWindow* parent, wxWindowID id,
    const wxPoint& pos, const wxSize& size,
    long style, const wxString& name)
    : wxGrid(parent, id, pos, size, style, name)
{
}

void ResizeableGrid::SetColumnWidths(const std::vector<int>& arg_widths)
{
    m_columnWidths = arg_widths;
}

void ResizeableGrid::SizeColumns()
{
    unsigned int num_cols = GetNumberCols();
    for (; m_columnWidths.size() < num_cols; )
    {
        m_columnWidths.push_back(0);
    }

    int side_width = GetRowLabelSize();
    int width = 0;
    int prop_count = 0;

    // First, set and add up the fixed width columns as well as the total
    // proportional count
    for (unsigned int col = 0; col < num_cols; col++)
    {
        if (m_columnWidths[col] > 0)
        {
            SetColSize(col, m_columnWidths[col]);
            width += GetColWidth(col);
        }
        else if (m_columnWidths[col] == 0)
        {
            AutoSizeColumn(col);
            m_columnWidths[col] = -GetColSize(col);
        }
        else
        {
            prop_count += -m_columnWidths[col];
        }
    }

    // Calculate remaining area
    int leftover_width = GetClientSize().GetWidth() - GetRowLabelSize() -
        width - 3;

    float round_error = 0;
    // Second, set the proportional columns' widths
    for (unsigned int col = 0; col < num_cols; col++)
    {
        if (m_columnWidths[col] < 0)
        {
            float width = ((float)leftover_width) / prop_count *
                -m_columnWidths[col];
            int int_width = int(width);
            round_error += width - int_width;

            SetColSize(col, int_width + int(round_error));
            if (round_error > 1)
                round_error -= 1;
        }
    }
}

void ResizeableGrid::OnSize(wxSizeEvent& event)
{
    SizeColumns();
    event.Skip();
}

void ResizeableGrid::ResizeGrid(int new_rows, int new_cols)
{
    if (!m_created)
    {
        CreateGrid(new_rows, new_cols);
        return;
    }
    ClearGrid();
    if (new_rows < m_numRows)
    {
        DeleteRows(0, m_numRows - new_rows);
    }
    if (new_rows > m_numRows)
    {
        AppendRows(new_rows - m_numRows);
    }
    if (new_cols < m_numCols)
    {
        DeleteCols(0, m_numCols - new_cols);
    }
    if (new_cols > m_numCols)
    {
        AppendCols(new_cols - m_numCols);
    }
    SizeColumns();
    Update();
}

void CellTextEditor::BeginEdit(int row, int col, wxGrid* grid)
{
    Text()->SetBackgroundColour(wxColour(247, 240, 213));
    Text()->SetValue(grid->GetTable()->GetValue(row, col));
    Text()->SetInsertionPointEnd();
    Text()->SetFocus();
}
