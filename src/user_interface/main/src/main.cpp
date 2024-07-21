#include <wx/app.h>
#include <wx/event.h>
#include <user_interface/main/include/MainFrame.h>
#include <wx/image.h>
#include <wx/cmdline.h>
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
IMPLEMENT_APP(MainApp)
