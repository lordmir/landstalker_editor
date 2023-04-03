#include "StringEditorFrame.h"
#include <codecvt>
#include <locale>

#include <wx/msw/msvcrt.h>  

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
      m_type(StringData::Type::MAIN)
{
	m_mgr.SetManagedWindow(this);

	m_stringView = new wxDataViewCtrl(this, wxID_ANY);
	m_mgr.AddPane(m_stringView, wxAuiPaneInfo().CenterPane());

	// tell the manager to "commit" all the changes just made
	m_mgr.Update();
	UpdateUI();
}

StringEditorFrame::~StringEditorFrame()
{
}

void StringEditorFrame::SetMode(StringData::Type type)
{
    m_type = type;
    
    m_stringView->ClearColumns();
    m_model = new StringDataViewModel(type, m_gd->GetStringData());
    m_stringView->AssociateModel(m_model);
    m_model->DecRef();
    auto font = m_stringView->GetFont();
    if (type != StringData::Type::MAIN)
    {
        font.SetFamily(wxFONTFAMILY_TELETYPE);
    }
    else
    {
        font.SetFamily(wxFONTFAMILY_DEFAULT);
    }
    m_stringView->SetFont(font);
    m_stringView->InsertColumn(0, new wxDataViewColumn(m_model->GetColumnHeader(0),
        new wxDataViewTextRenderer("long"), 0, 50, wxALIGN_RIGHT));
    if (type == StringData::Type::INTRO)
    {
        m_stringView->InsertColumn(1, new wxDataViewColumn(m_model->GetColumnHeader(1),
            new wxDataViewSpinRenderer(0, 65535, wxDATAVIEW_CELL_EDITABLE), 1, 80, wxALIGN_CENTER));
        m_stringView->InsertColumn(2, new wxDataViewColumn(m_model->GetColumnHeader(2),
            new wxDataViewSpinRenderer(0, 65535, wxDATAVIEW_CELL_EDITABLE), 2, 80, wxALIGN_CENTER));
        m_stringView->InsertColumn(3, new wxDataViewColumn(m_model->GetColumnHeader(3),
            new wxDataViewSpinRenderer(0, 65535, wxDATAVIEW_CELL_EDITABLE), 3, 80, wxALIGN_CENTER));
        m_stringView->InsertColumn(4, new wxDataViewColumn(m_model->GetColumnHeader(4),
            new wxDataViewSpinRenderer(0, 65535, wxDATAVIEW_CELL_EDITABLE), 4, 80, wxALIGN_CENTER));
        m_stringView->InsertColumn(5, new wxDataViewColumn(m_model->GetColumnHeader(5),
            new wxDataViewSpinRenderer(0, 65535, wxDATAVIEW_CELL_EDITABLE), 5, 80, wxALIGN_CENTER));
        m_stringView->InsertColumn(6, new wxDataViewColumn(m_model->GetColumnHeader(6),
            new wxDataViewTextRenderer("string", wxDATAVIEW_CELL_EDITABLE), 6, 160, wxALIGN_LEFT));
        m_stringView->InsertColumn(7, new wxDataViewColumn(m_model->GetColumnHeader(7),
            new wxDataViewTextRenderer("string", wxDATAVIEW_CELL_EDITABLE), 7, -1, wxALIGN_LEFT));
    }
    else if (type == StringData::Type::END_CREDITS)
    {
        m_stringView->InsertColumn(1, new wxDataViewColumn(m_model->GetColumnHeader(1),
            new wxDataViewSpinRenderer(-128, 127, wxDATAVIEW_CELL_EDITABLE), 1, 80, wxALIGN_CENTER));
        m_stringView->InsertColumn(2, new wxDataViewColumn(m_model->GetColumnHeader(2),
            new wxDataViewSpinRenderer(-128, 127, wxDATAVIEW_CELL_EDITABLE), 2, 80, wxALIGN_CENTER));
        m_stringView->InsertColumn(3, new wxDataViewColumn(m_model->GetColumnHeader(3),
            new wxDataViewTextRenderer("string", wxDATAVIEW_CELL_EDITABLE), 3, -1, wxALIGN_LEFT));
    }
    else
    {
        m_stringView->InsertColumn(1, new wxDataViewColumn(m_model->GetColumnHeader(1),
            new wxDataViewTextRenderer("string", wxDATAVIEW_CELL_EDITABLE), 1, -1, wxALIGN_LEFT));
    }
	Update();
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
    std::string title = "Import " + MODE_DESCRIPTORS[static_cast<int>(m_type)] + " Strings";
    std::string filter = "Text File (*.txt)|*.txt|All Files (*.*)|*.*";
    if (m_type == StringData::Type::INTRO || m_type == StringData::Type::END_CREDITS)
    {
        filter = "CSV File (*.csv)|*.csv|All Files (*.*)|*.*";
    }
    wxFileDialog fd(this, title, "", "", filter,
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (fd.ShowModal() != wxID_CANCEL)
    {
        std::string path = fd.GetPath().ToStdString();
        ImportStrings(path, m_type);

        Refresh();
    }
}

void StringEditorFrame::OnMenuExport()
{
    std::string title = "Export " + MODE_DESCRIPTORS[static_cast<int>(m_type)] + " Strings";
    std::string filter = "Text File (*.txt)|*.txt|All Files (*.*)|*.*";
    if (m_type == StringData::Type::INTRO || m_type == StringData::Type::END_CREDITS)
    {
        filter = "CSV File (*.csv)|*.csv|All Files (*.*)|*.*";
    }
    wxFileDialog fd(this, title, "", "", filter,
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (fd.ShowModal() != wxID_CANCEL)
    {
        std::string path = fd.GetPath().ToStdString();
        ExportStrings(path, m_type);
        Update();
    }
}

void StringEditorFrame::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
    m_type = StringData::Type::MAIN;
    Update();
}

void StringEditorFrame::ClearGameData()
{
	m_gd = nullptr;
    m_type = StringData::Type::MAIN;
    Update();
}

bool StringEditorFrame::ExportStrings(const filesystem::path& filename, StringData::Type mode)
{
    bool retval = true;

    auto sd = m_gd->GetStringData();
    std::wstring_convert<std::codecvt_utf8<LSString::StringType::value_type>> utf8_conv;
    std::ofstream fs(filename.str(), std::ios::out | std::ios::trunc);

    switch (m_type)
    {
    case StringData::Type::INTRO:
        fs << utf8_conv.to_bytes(sd->GetIntroString(0).GetHeaderRow()) << std::endl;
        for (int i = 0; i < sd->GetIntroStringCount(); ++i)
        {
            const auto& str = sd->GetIntroString(i);
            fs << utf8_conv.to_bytes(str.Serialise()) << std::endl;
        }
        break;
    case StringData::Type::END_CREDITS:
        fs << utf8_conv.to_bytes(sd->GetEndCreditString(0).GetHeaderRow()) << std::endl;
        for (int i = 0; i < sd->GetEndCreditStringCount(); ++i)
        {
            const auto& str = sd->GetEndCreditString(i);
            fs << utf8_conv.to_bytes(str.Serialise()) << std::endl;
        }
        break;
    default:
        for (int i = 0; i < sd->GetStringCount(m_type); ++i)
        {
            fs << sd->GetString(m_type, i) << std::endl;
        }
        break;
    }

    return retval;
}

bool StringEditorFrame::ImportStrings(const filesystem::path& filename, StringData::Type mode)
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
        if (mode == StringData::Type::INTRO || mode == StringData::Type::END_CREDITS)
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
                case StringData::Type::INTRO:
                    sd->SetIntroString(line_count++, IntroString(wline));
                    break;
                case StringData::Type::END_CREDITS:
                    sd->SetEndCreditString(line_count++, EndCreditString(wline));
                    break;
                default:
                    sd->SetString(mode, line_count++, wline);
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
