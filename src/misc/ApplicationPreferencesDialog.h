#ifndef APPLICATION_PREFERENCES_DIALOG_H
#define APPLICATION_PREFERENCES_DIALOG_H

#include <wx/dialog.h>
#include <wx/config.h>
#include <rooms/gpu/GLCanvasInputTypes.h>

class wxChoice;
class wxCommandEvent;
class ApplicationPreferencesDialog : public wxDialog
{
public:
    ApplicationPreferencesDialog(wxWindow* parent, wxConfig* config);

    GLCanvasDirectionInputMode GetDirectionInputMode() const;
    static GLCanvasDirectionInputMode LoadDirectionInputMode(wxConfig* config);

private:
    static wxString ConfigValue(GLCanvasDirectionInputMode mode);
    static int ChoiceIndex(GLCanvasDirectionInputMode mode);
    void OnOK(wxCommandEvent& event);

    wxConfig* m_config;
    wxChoice* m_control_scheme;
};

#endif  // APPLICATION_PREFERENCES_DIALOG_H
