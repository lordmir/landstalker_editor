#ifndef _AUDIO_YAML_IO_H_
#define _AUDIO_YAML_IO_H_

#include <functional>
#include <string>

#include <wx/string.h>
#include <yaml-cpp/yaml.h>

class wxWindow;

// The file-dialog + read/write + error-reporting boilerplate shared by every audio editor's
// "Export ... as YAML" / "Import ... from YAML" menu action. The emit callback streams the
// document; the apply callback parses it (throwing std::runtime_error / YAML::Exception on bad
// input, which is reported to the user as a message box). Both return false when the user
// cancelled or something failed - callers only need to refresh their UI on a successful import.
bool ExportYamlWithDialog(wxWindow* parent, const wxString& title, const wxString& default_filename,
    const std::function<void(YAML::Emitter&)>& emit);
bool ImportYamlWithDialog(wxWindow* parent, const wxString& title,
    const std::function<void(const YAML::Node&)>& apply);

// Reads a scalar int field, rethrowing yaml-cpp's error (or an out-of-range value) as a
// std::runtime_error prefixed with `context` - the shared field reader for the audio YAML codecs.
int ReadYamlInt(const YAML::Node& node, const std::string& context, int min, int max);

#endif // _AUDIO_YAML_IO_H_
