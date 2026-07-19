#include <misc/ApplicationPreferencesDialog.h>

#include <wx/choice.h>
#include <wx/config.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

namespace {
constexpr const char* kControlSchemeConfigPath = "/preferences/control_scheme";
constexpr const char* kUpIsNorthEastConfigValue = "up_is_north_east";
constexpr const char* kUpIsNorthWestConfigValue = "up_is_north_west";
constexpr const char* kLandstalkerConfigValue = "landstalker";
constexpr const char* kDiagonalChordsConfigValue = "diagonal_chords";
}

ApplicationPreferencesDialog::ApplicationPreferencesDialog(wxWindow* parent, wxConfig* config)
    : wxDialog(parent, wxID_ANY, "Preferences", wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_config(config),
      m_control_scheme(new wxChoice(this, wxID_ANY))
{
    m_control_scheme->Append("Isometric (W = North-East)");
    m_control_scheme->Append("Isometric, rotated (W = North-West)");
    m_control_scheme->Append("Landstalker (diagonal chords)");
    m_control_scheme->Append("Diagonal chords only");
    m_control_scheme->SetSelection(ChoiceIndex(LoadDirectionInputMode(m_config)));

    auto* settings = new wxFlexGridSizer(2, 10, 12);
    settings->Add(new wxStaticText(this, wxID_ANY, "Control scheme:"), 0, wxALIGN_CENTER_VERTICAL);
    settings->Add(m_control_scheme, 1, wxEXPAND);
    settings->AddGrowableCol(1, 1);

    auto* root = new wxBoxSizer(wxVERTICAL);
    root->Add(settings, 0, wxEXPAND | wxALL, 12);
    root->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    SetSizerAndFit(root);
    SetMinSize(wxSize(430, GetSize().GetHeight()));
    CentreOnParent();

    Bind(wxEVT_BUTTON, &ApplicationPreferencesDialog::OnOK, this, wxID_OK);
}

GLCanvasDirectionInputMode ApplicationPreferencesDialog::GetDirectionInputMode() const
{
    switch (m_control_scheme->GetSelection()) {
    case 1: return GLCanvasDirectionInputMode::UpIsNorthWest;
    case 2: return GLCanvasDirectionInputMode::Landstalker;
    case 3: return GLCanvasDirectionInputMode::DiagonalChords;
    case 0:
    default: return GLCanvasDirectionInputMode::UpIsNorthEast;
    }
}

GLCanvasDirectionInputMode ApplicationPreferencesDialog::LoadDirectionInputMode(wxConfig* config)
{
    const wxString value = config != nullptr
        ? config->Read(kControlSchemeConfigPath, wxString(kUpIsNorthEastConfigValue))
        : wxString(kUpIsNorthEastConfigValue);
    if (value == kUpIsNorthWestConfigValue) {
        return GLCanvasDirectionInputMode::UpIsNorthWest;
    }
    if (value == kLandstalkerConfigValue) {
        return GLCanvasDirectionInputMode::Landstalker;
    }
    if (value == kDiagonalChordsConfigValue) {
        return GLCanvasDirectionInputMode::DiagonalChords;
    }
    return GLCanvasDirectionInputMode::UpIsNorthEast;
}

wxString ApplicationPreferencesDialog::ConfigValue(GLCanvasDirectionInputMode mode)
{
    switch (mode) {
    case GLCanvasDirectionInputMode::UpIsNorthWest: return kUpIsNorthWestConfigValue;
    case GLCanvasDirectionInputMode::Landstalker: return kLandstalkerConfigValue;
    case GLCanvasDirectionInputMode::DiagonalChords: return kDiagonalChordsConfigValue;
    case GLCanvasDirectionInputMode::UpIsNorthEast:
    default: return kUpIsNorthEastConfigValue;
    }
}

int ApplicationPreferencesDialog::ChoiceIndex(GLCanvasDirectionInputMode mode)
{
    switch (mode) {
    case GLCanvasDirectionInputMode::UpIsNorthWest: return 1;
    case GLCanvasDirectionInputMode::Landstalker: return 2;
    case GLCanvasDirectionInputMode::DiagonalChords: return 3;
    case GLCanvasDirectionInputMode::UpIsNorthEast:
    default: return 0;
    }
}

void ApplicationPreferencesDialog::OnOK(wxCommandEvent& /*event*/)
{
    if (m_config != nullptr) {
        m_config->Write(kControlSchemeConfigPath, ConfigValue(GetDirectionInputMode()));
        m_config->Flush();
    }
    EndModal(wxID_OK);
}
