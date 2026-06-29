#include <wx/app.h>
#include <wx/event.h>
#include <main/MainFrame.h>
#include <landstalker/misc/Labels.h>
#include <wx/image.h>
#include <wx/cmdline.h>
#include <cstdlib>
#include <cstring>
#include <string>

// Define the MainApp
class MainApp : public wxApp
{
public:
    MainApp() {}
    virtual ~MainApp() {}

    virtual bool OnInit() {
        wxInitAllImageHandlers();

        std::string romFile("");
        if (this->argc == 2)
        {
            romFile = this->argv[1];
        }

        MainFrame* mainFrame = new MainFrame(NULL, romFile);
        SetTopWindow(mainFrame);
        return GetTopWindow()->Show();
    }
};

DECLARE_APP(MainApp)

#ifdef __linux__
// This is needed to work around a GLEW issue where it fails to initialize properly on Wayland sessions.
wxIMPLEMENT_APP_NO_MAIN(MainApp);

void PreferX11BackendForGlew()
{
    const char* display = std::getenv("DISPLAY");
    const char* session_type = std::getenv("XDG_SESSION_TYPE");
    if (!display || display[0] == '\0' || !session_type || std::strcmp(session_type, "wayland") != 0) {
        return;
    }

    setenv("GDK_BACKEND", "x11", 1);
    setenv("XDG_SESSION_TYPE", "x11", 1);
    unsetenv("WAYLAND_DISPLAY");
}

int main(int argc, char** argv)
{
    PreferX11BackendForGlew();
    return wxEntry(argc, argv);
}

#else
wxIMPLEMENT_APP(MainApp);
#endif
