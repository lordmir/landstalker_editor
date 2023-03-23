#include "StringEditorFrame.h"

StringEditorFrame::StringEditorFrame(wxWindow* parent)
	: EditorFrame(parent, wxID_ANY),
	  m_gd(nullptr),
	  m_title(""),
      m_mode(Mode::MODE_MAIN)
{
	m_mgr.SetManagedWindow(this);

	m_stringView = new ResizeableGrid(this, wxID_ANY);
	m_mgr.AddPane(m_stringView, wxAuiPaneInfo().CenterPane());

	// tell the manager to "commit" all the changes just made
	m_mgr.Update();
	UpdateUI();

    this->Connect(wxEVT_GRID_CELL_CHANGED, wxGridEventHandler(StringEditorFrame::OnGridValueChanged), nullptr, this);
}

StringEditorFrame::~StringEditorFrame()
{
    this->Disconnect(wxEVT_GRID_CELL_CHANGED, wxGridEventHandler(StringEditorFrame::OnGridValueChanged), nullptr, this);
}

void StringEditorFrame::SetMode(Mode mode)
{
	m_mode = mode;
	Update();
}

void StringEditorFrame::Update()
{
    if (m_gd != nullptr)
    {
        InitColumns();
        switch (m_mode)
        {
        case Mode::MODE_MAIN:
            ShowMainStrings();
            break;
        case Mode::MODE_CHARS:
            ShowCharacterStrings();
            break;
        case Mode::MODE_SPECIAL_CHARS:
            ShowSpecialCharacterStrings();
            break;
        case Mode::MODE_DEFAULT_CHAR:
            ShowDefaultCharacterString();
            break;
        case Mode::MODE_ITEMS:
            ShowItemStrings();
            break;
        case Mode::MODE_MENU:
            ShowMenuStrings();
            break;
        case Mode::MODE_INTRO:
            ShowIntroStrings();
            break;
        case Mode::MODE_END_CREDITS:
            ShowEndingStrings();
            break;
        case Mode::MODE_SYSTEM:
            ShowSystemStrings();
            break;
        default:
            break;
        }
    }
}

void StringEditorFrame::InitColumns()
{
    m_stringView->SetDefaultCellFont(wxFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL ));
    m_stringView->SetRowLabelSize(60);
    switch (m_mode)
    {
    case Mode::MODE_MAIN:
        m_stringView->SetColumnWidths({ -1 });
        m_stringView->ResizeGrid(m_gd->GetStringData()->GetMainStringCount(), 1);
        break;
    case Mode::MODE_CHARS:
        m_stringView->SetColumnWidths({ -1 });
        m_stringView->ResizeGrid(m_gd->GetStringData()->GetCharNameCount(), 1);
        break;
    case Mode::MODE_SPECIAL_CHARS:
        m_stringView->SetColumnWidths({ -1 });
        m_stringView->ResizeGrid(m_gd->GetStringData()->GetSpecialCharNameCount(), 1);
        break;
    case Mode::MODE_DEFAULT_CHAR:
        m_stringView->SetColumnWidths({ -1 });
        m_stringView->ResizeGrid(1, 1);
        break;
    case Mode::MODE_ITEMS:
        m_stringView->SetColumnWidths({ -1 });
        m_stringView->ResizeGrid(m_gd->GetStringData()->GetItemNameCount(), 1);
        break;
    case Mode::MODE_MENU:
        m_stringView->SetColumnWidths({ -1 });
        m_stringView->ResizeGrid(m_gd->GetStringData()->GetMenuStrCount(), 1);
        break;
    case Mode::MODE_SYSTEM:
        m_stringView->SetColumnWidths({ -1 });
        m_stringView->ResizeGrid(m_gd->GetStringData()->GetSystemStringCount(), 1);
        break;
    case Mode::MODE_INTRO:
        m_stringView->SetColumnWidths({ 100, 100, 100, 100, 100, -1, -1 });
        m_stringView->ResizeGrid(m_gd->GetStringData()->GetIntroStringCount(), 7);
        break;
    case Mode::MODE_END_CREDITS:
        m_stringView->SetColumnWidths({ 100, 100, -1 });
        m_stringView->ResizeGrid(m_gd->GetStringData()->GetEndCreditStringCount(), 3);
        break;
    default:
        return;
    }
    if (m_mode == Mode::MODE_INTRO)
    {
        m_stringView->SetColLabelValue(0, "Display Time");
        m_stringView->SetColLabelValue(1, "Line1 X");
        m_stringView->SetColLabelValue(2, "Line1 Y");
        m_stringView->SetColLabelValue(3, "Line2 X");
        m_stringView->SetColLabelValue(4, "Line2 Y");
        m_stringView->SetColLabelValue(5, "Line1");
        m_stringView->SetColLabelValue(6, "Line2");
        for (std::size_t i = 0; i < m_stringView->GetNumberRows(); ++i)
        {
            m_stringView->SetCellEditor(i, 0, new wxGridCellNumberEditor(0, 65535));
            m_stringView->SetCellEditor(i, 1, new wxGridCellNumberEditor(0, 65535));
            m_stringView->SetCellEditor(i, 2, new wxGridCellNumberEditor(0, 65535));
            m_stringView->SetCellEditor(i, 3, new wxGridCellNumberEditor(0, 65535));
            m_stringView->SetCellEditor(i, 4, new wxGridCellNumberEditor(0, 65535));
            m_stringView->SetCellEditor(i, 5, new CellTextEditor(16));
            m_stringView->SetCellEditor(i, 6, new CellTextEditor(16));
        }
    }
    else if (m_mode == Mode::MODE_END_CREDITS)
    {
        m_stringView->SetColLabelValue(0, "Column");
        m_stringView->SetColLabelValue(1, "Height");
        m_stringView->SetColLabelValue(2, "String");
        for (std::size_t i = 0; i < m_stringView->GetNumberRows(); ++i)
        {
            m_stringView->SetCellEditor(i, 0, new wxGridCellNumberEditor(-128, 127));
            m_stringView->SetCellEditor(i, 1, new wxGridCellNumberEditor(-128, 127));
            m_stringView->SetCellEditor(i, 2, new CellTextEditor(256));
        }
    }
    else
    {
        m_stringView->SetColLabelValue(0, "Strings");
        for (std::size_t i = 0; i < m_stringView->GetNumberRows(); ++i)
        {
            m_stringView->SetCellEditor(i, 0, new CellTextEditor(256));
        }
    }
    m_stringView->SetColLabelAlignment(wxALIGN_LEFT, wxALIGN_CENTER);
    m_stringView->SetRowLabelAlignment(wxALIGN_RIGHT, wxALIGN_CENTER);
    m_stringView->AutoSizeRows();
    m_stringView->DisableDragGridSize();
    m_stringView->DisableDragRowSize();
    m_stringView->DisableDragColSize();
}

void StringEditorFrame::ShowMainStrings()
{
    for (std::size_t i = 0; i < m_stringView->GetNumberRows(); ++i)
    {
        RefreshMainString(i);
    }
}

void StringEditorFrame::ShowCharacterStrings()
{
    for (std::size_t i = 0; i < m_stringView->GetNumberRows(); ++i)
    {
        RefreshCharacterString(i);
    }
}

void StringEditorFrame::ShowSpecialCharacterStrings()
{
    for (std::size_t i = 0; i < m_stringView->GetNumberRows(); ++i)
    {
        RefreshSpecialCharacterString(i);
    }
}

void StringEditorFrame::ShowDefaultCharacterString()
{
    for (std::size_t i = 0; i < m_stringView->GetNumberRows(); ++i)
    {
        RefreshDefaultCharacterString(i);
    }
}

void StringEditorFrame::ShowItemStrings()
{
    for (std::size_t i = 0; i < m_stringView->GetNumberRows(); ++i)
    {
        RefreshItemString(i);
    }
}

void StringEditorFrame::ShowMenuStrings()
{
    for (std::size_t i = 0; i < m_stringView->GetNumberRows(); ++i)
    {
        RefreshMenuString(i);
    }
}

void StringEditorFrame::ShowIntroStrings()
{
    for (std::size_t i = 0; i < m_stringView->GetNumberRows(); ++i)
    {
        RefreshIntroString(i);
    }
} 

void StringEditorFrame::ShowEndingStrings()
{
    for (std::size_t i = 0; i < m_stringView->GetNumberRows(); ++i)
    {
        RefreshEndingString(i);
    }
}

void StringEditorFrame::ShowSystemStrings()
{
    for (std::size_t i = 0; i < m_stringView->GetNumberRows(); ++i)
    {
        RefreshSystemString(i);
    }
}

void StringEditorFrame::RefreshMainString(int idx)
{
    m_stringView->SetCellValue(idx, 0, m_gd->GetStringData()->GetMainString(idx));
    m_stringView->SetCellTextColour(idx, 0, m_gd->GetStringData()->HasMainStringChanged(idx) ? *wxRED : *wxBLACK);
}

void StringEditorFrame::RefreshCharacterString(int idx)
{
    m_stringView->SetCellValue(idx, 0, m_gd->GetStringData()->GetCharName(idx));
    m_stringView->SetCellTextColour(idx, 0, m_gd->GetStringData()->HasCharNameChanged(idx) ? *wxRED : *wxBLACK);
}

void StringEditorFrame::RefreshSpecialCharacterString(int idx)
{
    m_stringView->SetCellValue(idx, 0, m_gd->GetStringData()->GetSpecialCharName(idx));
    m_stringView->SetCellTextColour(idx, 0, m_gd->GetStringData()->HasSpecialCharNameChanged(idx) ? *wxRED : *wxBLACK);
}

void StringEditorFrame::RefreshDefaultCharacterString(int idx)
{
    m_stringView->SetCellValue(idx, 0, m_gd->GetStringData()->GetDefaultCharName());
    m_stringView->SetCellTextColour(idx, 0, m_gd->GetStringData()->HasDefaultCharNameChanged() ? *wxRED : *wxBLACK);
}

void StringEditorFrame::RefreshItemString(int idx)
{
    m_stringView->SetCellValue(idx, 0, m_gd->GetStringData()->GetItemName(idx));
    m_stringView->SetCellTextColour(idx, 0, m_gd->GetStringData()->HasItemNameChanged(idx) ? *wxRED : *wxBLACK);
}

void StringEditorFrame::RefreshMenuString(int idx)
{
    m_stringView->SetCellValue(idx, 0, m_gd->GetStringData()->GetMenuStr(idx));
    m_stringView->SetCellTextColour(idx, 0, m_gd->GetStringData()->HasMenuStrChanged(idx) ? *wxRED : *wxBLACK);
}

void StringEditorFrame::RefreshIntroString(int idx)
{
    const auto& intro_string = m_gd->GetStringData()->GetIntroString(idx);
    m_stringView->SetCellValue(idx, 0, wxString::Format("%u", intro_string.GetDisplayTime()));
    m_stringView->SetCellValue(idx, 1, wxString::Format("%u", intro_string.GetLine1X()));
    m_stringView->SetCellValue(idx, 2, wxString::Format("%u", intro_string.GetLine1Y()));
    m_stringView->SetCellValue(idx, 3, wxString::Format("%u", intro_string.GetLine2X()));
    m_stringView->SetCellValue(idx, 4, wxString::Format("%u", intro_string.GetLine2Y()));
    m_stringView->SetCellValue(idx, 5, intro_string.GetLine(0));
    m_stringView->SetCellValue(idx, 6, intro_string.GetLine(1));
    if (m_gd->GetStringData()->HasIntroStringChanged(idx))
    {
        const auto& intro_string_orig = m_gd->GetStringData()->GetIntroString(idx);
        m_stringView->SetCellTextColour(idx, 0, intro_string.GetDisplayTime() != intro_string_orig.GetDisplayTime() ? *wxRED : *wxBLACK);
        m_stringView->SetCellTextColour(idx, 1, intro_string.GetLine1X() != intro_string_orig.GetLine1X() ? *wxRED : *wxBLACK);
        m_stringView->SetCellTextColour(idx, 2, intro_string.GetLine1Y() != intro_string_orig.GetLine1Y() ? *wxRED : *wxBLACK);
        m_stringView->SetCellTextColour(idx, 3, intro_string.GetLine2X() != intro_string_orig.GetLine2X() ? *wxRED : *wxBLACK);
        m_stringView->SetCellTextColour(idx, 4, intro_string.GetLine2Y() != intro_string_orig.GetLine2Y() ? *wxRED : *wxBLACK);
        m_stringView->SetCellTextColour(idx, 5, intro_string.GetLine(0) != intro_string_orig.GetLine(0) ? *wxRED : *wxBLACK);
        m_stringView->SetCellTextColour(idx, 6, intro_string.GetLine(1) != intro_string_orig.GetLine(1) ? *wxRED : *wxBLACK);
    }
}

void StringEditorFrame::RefreshEndingString(int idx)
{
    const auto& end_string = m_gd->GetStringData()->GetEndCreditString(idx);
    m_stringView->SetCellValue(idx, 0, wxString::Format("%d", end_string.GetColumn()));
    m_stringView->SetCellValue(idx, 1, wxString::Format("%d", end_string.GetHeight()));
    m_stringView->SetCellValue(idx, 2, end_string.Str());
    if (m_gd->GetStringData()->HasEndCreditStringChanged(idx))
    {
        const auto& end_string_orig = m_gd->GetStringData()->GetEndCreditString(idx);
        m_stringView->SetCellTextColour(idx, 0, end_string.GetColumn() != end_string_orig.GetColumn() ? *wxRED : *wxBLACK);
        m_stringView->SetCellTextColour(idx, 1, end_string.GetHeight() != end_string_orig.GetHeight() ? *wxRED : *wxBLACK);
        m_stringView->SetCellTextColour(idx, 2, end_string.Str() != end_string_orig.Str() ? *wxRED : *wxBLACK);
    }
}

void StringEditorFrame::RefreshSystemString(int idx)
{
    m_stringView->SetCellValue(idx, 0, m_gd->GetStringData()->GetSystemString(idx));
    m_stringView->SetCellTextColour(idx, 0, m_gd->GetStringData()->HasSystemStringChanged(idx) ? *wxRED : *wxBLACK);
}

void StringEditorFrame::OnGridValueChanged(wxGridEvent& evt)
{
    if (m_gd != nullptr)
    {
        int row = evt.GetRow();
        int col = evt.GetCol();
        auto wxval = m_stringView->GetCellValue(row, col);
        auto val = wxval.ToStdWstring();
        switch (m_mode)
        {
        case Mode::MODE_MAIN:
            m_gd->GetStringData()->SetMainString(row, val);
            RefreshMainString(row);
            break;
        case Mode::MODE_CHARS:
            m_gd->GetStringData()->SetCharName(row, val);
            RefreshCharacterString(row);
            break;
        case Mode::MODE_SPECIAL_CHARS:
            m_gd->GetStringData()->SetSpecialCharName(row, val);
            RefreshSpecialCharacterString(row);
            break;
        case Mode::MODE_DEFAULT_CHAR:
            m_gd->GetStringData()->SetDefaultCharName(val);
            RefreshDefaultCharacterString(row);
            break;
        case Mode::MODE_ITEMS:
            m_gd->GetStringData()->SetItemName(row, val);
            RefreshItemString(row);
            break;
        case Mode::MODE_MENU:
            m_gd->GetStringData()->SetMenuStr(row, val);
            RefreshMenuString(row);
            break;
        case Mode::MODE_INTRO:
        {
            auto intro_str = m_gd->GetStringData()->GetIntroString(row);
            switch (col)
            {
            case 0: // Display Time
                intro_str.SetDisplayTime(static_cast<uint16_t>(std::stoi(val)));
                break;
            case 1: // Line 1 X
                intro_str.SetLine1X(static_cast<uint16_t>(std::stoi(val)));
                break;
            case 2: // Line 1 Y
                intro_str.SetLine1Y(static_cast<uint16_t>(std::stoi(val)));
                break;
            case 3: // Line 2 X
                intro_str.SetLine2X(static_cast<uint16_t>(std::stoi(val)));
                break;
            case 4: // Line 2 Y
                intro_str.SetLine2Y(static_cast<uint16_t>(std::stoi(val)));
                break;
            case 5: // Line 0
                intro_str.SetLine(0, val);
                break;
            case 6: // Line 1
                intro_str.SetLine(1, val);
                break;
            }
            m_gd->GetStringData()->SetIntroString(row, intro_str);
            RefreshIntroString(row);
            break;
        }
        case Mode::MODE_END_CREDITS:
        {
            auto end_str = m_gd->GetStringData()->GetEndCreditString(row);
            switch (col)
            {
            case 0: // Column
                end_str.SetColumn(static_cast<int8_t>(std::stoi(val)));
                break;
            case 1: // Height
                end_str.SetHeight(static_cast<int8_t>(std::stoi(val)));
                break;
            case 2:
                end_str.SetStr(val);
                break;
            }
            m_gd->GetStringData()->SetEndCreditString(row, end_str);
            RefreshEndingString(row);
            break;
        }
        case Mode::MODE_SYSTEM:
            m_gd->GetStringData()->SetSystemString(row, m_stringView->GetCellValue(row, col).ToStdString());
            RefreshSystemString(row);
            break;
        }
    }
    evt.Skip();
}

void StringEditorFrame::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
    m_mode = Mode::MODE_MAIN;
    Update();
}

void StringEditorFrame::ClearGameData()
{
	m_gd = nullptr;
    m_mode = Mode::MODE_MAIN;
    Update();
}
