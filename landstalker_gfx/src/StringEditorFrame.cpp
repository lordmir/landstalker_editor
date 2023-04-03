#include "StringEditorFrame.h"
#include <codecvt>
#include <locale>

enum MENU_IDS
{
    ID_FILE_EXPORT = 22000,
    ID_FILE_IMPORT
};

static const std::string MODE_DESCRIPTORS[] =
{
    "Main",
    "Character Name",
    "Special Character Name",
    "Default Character Name",
    "Item Name",
    "Menu",
    "Intro",
    "End Credit",
    "System"
};

StringEditorFrame::StringEditorFrame(wxWindow* parent)
	: EditorFrame(parent, wxID_ANY),
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
            m_stringView->SetCellEditor(i, 5, new wxGridCellTextEditor(16));
            m_stringView->SetCellEditor(i, 6, new wxGridCellTextEditor(16));
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
            m_stringView->SetCellEditor(i, 2, new wxGridCellTextEditor(256));
        }
    }
    else
    {
        m_stringView->SetColLabelValue(0, "Strings");
        for (std::size_t i = 0; i < m_stringView->GetNumberRows(); ++i)
        {
            m_stringView->SetCellEditor(i, 0, new wxGridCellTextEditor(256));
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
    const auto& intro_string_orig = m_gd->GetStringData()->GetOrigIntroString(idx);
    m_stringView->SetCellValue(idx, 0, wxString::Format("%u", intro_string.GetDisplayTime()));
    m_stringView->SetCellValue(idx, 1, wxString::Format("%u", intro_string.GetLine1X()));
    m_stringView->SetCellValue(idx, 2, wxString::Format("%u", intro_string.GetLine1Y()));
    m_stringView->SetCellValue(idx, 3, wxString::Format("%u", intro_string.GetLine2X()));
    m_stringView->SetCellValue(idx, 4, wxString::Format("%u", intro_string.GetLine2Y()));
    m_stringView->SetCellValue(idx, 5, intro_string.GetLine(0));
    m_stringView->SetCellValue(idx, 6, intro_string.GetLine(1));
    m_stringView->SetCellTextColour(idx, 0, intro_string.GetDisplayTime() != intro_string_orig.GetDisplayTime() ? *wxRED : *wxBLACK);
    m_stringView->SetCellTextColour(idx, 1, intro_string.GetLine1X() != intro_string_orig.GetLine1X() ? *wxRED : *wxBLACK);
    m_stringView->SetCellTextColour(idx, 2, intro_string.GetLine1Y() != intro_string_orig.GetLine1Y() ? *wxRED : *wxBLACK);
    m_stringView->SetCellTextColour(idx, 3, intro_string.GetLine2X() != intro_string_orig.GetLine2X() ? *wxRED : *wxBLACK);
    m_stringView->SetCellTextColour(idx, 4, intro_string.GetLine2Y() != intro_string_orig.GetLine2Y() ? *wxRED : *wxBLACK);
    m_stringView->SetCellTextColour(idx, 5, intro_string.GetLine(0) != intro_string_orig.GetLine(0) ? *wxRED : *wxBLACK);
    m_stringView->SetCellTextColour(idx, 6, intro_string.GetLine(1) != intro_string_orig.GetLine(1) ? *wxRED : *wxBLACK);
}

void StringEditorFrame::RefreshEndingString(int idx)
{
    const auto& end_string = m_gd->GetStringData()->GetEndCreditString(idx);
    const auto& end_string_orig = m_gd->GetStringData()->GetOrigEndCreditString(idx);
    m_stringView->SetCellValue(idx, 0, wxString::Format("%d", end_string.GetColumn()));
    m_stringView->SetCellValue(idx, 1, wxString::Format("%d", end_string.GetHeight()));
    m_stringView->SetCellValue(idx, 2, end_string.Str());
    m_stringView->SetCellTextColour(idx, 0, end_string.GetColumn() != end_string_orig.GetColumn() ? *wxRED : *wxBLACK);
    m_stringView->SetCellTextColour(idx, 1, end_string.GetHeight() != end_string_orig.GetHeight() ? *wxRED : *wxBLACK);
    m_stringView->SetCellTextColour(idx, 2, end_string.Str() != end_string_orig.Str() ? *wxRED : *wxBLACK);
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

void StringEditorFrame::OnMenuClick(wxMenuEvent& evt)
{
    const auto id = evt.GetId();
    if ((id >= 22000) && (id < 31000))
    {
        switch (id)
        {
        case ID_FILE_EXPORT:
            OnMenuExport();
            break;
        case ID_FILE_IMPORT:
            OnMenuImport();
            break;
        default:
            wxMessageBox(wxString::Format("Unrecognised Event %d", evt.GetId()));
        }
        UpdateUI();
    }
}

void StringEditorFrame::OnMenuImport()
{
    std::string title = "Import " + MODE_DESCRIPTORS[static_cast<int>(m_mode)] + " Strings";
    std::string filter = "Text File (*.txt)|*.txt|All Files (*.*)|*.*";
    if (m_mode == Mode::MODE_INTRO || m_mode == Mode::MODE_END_CREDITS)
    {
        filter = "CSV File (*.csv)|*.csv|All Files (*.*)|*.*";
    }
    wxFileDialog fd(this, title, "", "", filter,
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (fd.ShowModal() != wxID_CANCEL)
    {
        std::string path = fd.GetPath().ToStdString();
        ImportStrings(path, m_mode);

        Refresh();
    }
}

void StringEditorFrame::OnMenuExport()
{
    std::string title = "Export " + MODE_DESCRIPTORS[static_cast<int>(m_mode)] + " Strings";
    std::string filter = "Text File (*.txt)|*.txt|All Files (*.*)|*.*";
    if (m_mode == Mode::MODE_INTRO || m_mode == Mode::MODE_END_CREDITS)
    {
        filter = "CSV File (*.csv)|*.csv|All Files (*.*)|*.*";
    }
    wxFileDialog fd(this, title, "", "", filter,
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (fd.ShowModal() != wxID_CANCEL)
    {
        std::string path = fd.GetPath().ToStdString();
        ExportStrings(path, m_mode);
        Update();
    }
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

bool StringEditorFrame::ExportStrings(const filesystem::path& filename, Mode mode)
{
    bool retval = true;

    auto sd = m_gd->GetStringData();
    std::wstring_convert<std::codecvt_utf8<LSString::StringType::value_type>> utf8_conv;
    std::ofstream fs(filename.str(), std::ios::out | std::ios::trunc);

    switch (m_mode)
    {
    case Mode::MODE_MAIN:
        for (int i = 0; i < sd->GetMainStringCount(); ++i)
        {
            fs << utf8_conv.to_bytes(sd->GetMainString(i)) << std::endl;
        }
        break;
    case Mode::MODE_CHARS:
        for (int i = 0; i < sd->GetCharNameCount(); ++i)
        {
            fs << utf8_conv.to_bytes(sd->GetCharName(i)) << std::endl;
        }
        break;
    case Mode::MODE_SPECIAL_CHARS:
        for (int i = 0; i < sd->GetSpecialCharNameCount(); ++i)
        {
            fs << utf8_conv.to_bytes(sd->GetSpecialCharName(i)) << std::endl;
        }
        break;
    case Mode::MODE_DEFAULT_CHAR:
        fs << utf8_conv.to_bytes(sd->GetDefaultCharName()) << std::endl;
        break;
    case Mode::MODE_ITEMS:
        for (int i = 0; i < sd->GetItemNameCount(); ++i)
        {
            fs << utf8_conv.to_bytes(sd->GetItemName(i)) << std::endl;
        }
        break;
    case Mode::MODE_MENU:
        for (int i = 0; i < sd->GetMenuStrCount(); ++i)
        {
            fs << utf8_conv.to_bytes(sd->GetMenuStr(i)) << std::endl;
        }
        break;
    case Mode::MODE_SYSTEM:
        for (int i = 0; i < sd->GetSystemStringCount(); ++i)
        {
            fs << sd->GetSystemString(i) << std::endl;
        }
        break;
    case Mode::MODE_INTRO:
        fs << utf8_conv.to_bytes(sd->GetIntroString(0).GetHeaderRow()) << std::endl;
        for (int i = 0; i < sd->GetIntroStringCount(); ++i)
        {
            const auto& str = sd->GetIntroString(i);
            fs << utf8_conv.to_bytes(str.Serialise()) << std::endl;
        }
        break;
    case Mode::MODE_END_CREDITS:
        fs << utf8_conv.to_bytes(sd->GetEndCreditString(0).GetHeaderRow()) << std::endl;
        for (int i = 0; i < sd->GetEndCreditStringCount(); ++i)
        {
            const auto& str = sd->GetEndCreditString(i);
            fs << utf8_conv.to_bytes(str.Serialise()) << std::endl;
        }
        break;
    }

    return retval;
}

bool StringEditorFrame::ImportStrings(const filesystem::path& filename, Mode mode)
{
    bool retval = true;
    std::ifstream fs;
    std::ostringstream errorss;
    std::vector<LSString> lines;
    std::wstring_convert<std::codecvt_utf8<LSString::StringType::value_type>> utf8_conv;
    auto sd = m_gd->GetStringData();

    fs.open(filename.str(), std::ios::in);
    if (fs.good() == false)
    {
        errorss << "Unable to open file \"" << filename << "\" for reading." << std::endl;
        retval = false;
    }
    else
    {
        std::string line;
        std::wstring wline;
        int line_count = 0;
        if (mode == Mode::MODE_INTRO || mode == Mode::MODE_END_CREDITS)
        {
            std::getline(fs, line); // Discard header row
        }
        while (fs.eof() == false)
        {
            std::getline(fs, line);
            wline = utf8_conv.from_bytes(line);
            if (line.empty() == false)
            {
                switch (mode)
                {
                case Mode::MODE_INTRO:
                    sd->SetIntroString(line_count++, IntroString(wline));
                    break;
                case Mode::MODE_END_CREDITS:
                    sd->SetEndCreditString(line_count++, EndCreditString(wline));
                    break;
                case Mode::MODE_MAIN:
                    sd->SetMainString(line_count++, wline);
                    break;
                case Mode::MODE_CHARS:
                    sd->SetCharName(line_count++, wline);
                    break;
                case Mode::MODE_SPECIAL_CHARS:
                    sd->SetSpecialCharName(line_count++, wline);
                    break;
                case Mode::MODE_DEFAULT_CHAR:
                    sd->SetDefaultCharName(wline);
                    break;
                case Mode::MODE_ITEMS:
                    sd->SetItemName(line_count++, wline);
                    break;
                case Mode::MODE_MENU:
                    sd->SetMenuStr(line_count++, wline);
                    break;
                case Mode::MODE_SYSTEM:
                    sd->SetSystemString(line_count++, line);
                    break;
                }
            }
        }
        Update();
    }

    if (!retval)
    {
        wxMessageBox("Errors during import!\n" + errorss.str(), "Errors during import", wxICON_ERROR);
    }

    return retval;
}

void StringEditorFrame::InitMenu(wxMenuBar& menu, ImageList& ilist) const
{
    auto* parent = m_mgr.GetManagedWindow();

    ClearMenu(menu);
    auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
    AddMenuItem(fileMenu, 0, ID_FILE_EXPORT, "Export Strings...");
    AddMenuItem(fileMenu, 1, ID_FILE_IMPORT, "Import Strings...");
    UpdateUI();
    m_mgr.Update();
}
