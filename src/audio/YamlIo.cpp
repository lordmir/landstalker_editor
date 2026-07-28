#include <audio/YamlIo.h>

#include <fstream>
#include <sstream>
#include <stdexcept>

#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/window.h>

namespace
{
	constexpr const char* YAML_WILDCARD = "YAML file (*.yml;*.yaml)|*.yml;*.yaml|All Files (*.*)|*.*";
}

bool ExportYamlWithDialog(wxWindow* parent, const wxString& title, const wxString& default_filename,
	const std::function<void(YAML::Emitter&)>& emit)
{
	wxFileDialog fd(parent, title, "", default_filename, YAML_WILDCARD, wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return false;
	}
	YAML::Emitter out;
	emit(out);
	if (!out.good())
	{
		wxMessageBox("Failed to build the YAML document.", title, wxOK | wxICON_ERROR, parent);
		return false;
	}
	std::ofstream ofs(fd.GetPath().ToStdString());
	if (!ofs.is_open())
	{
		wxMessageBox("Unable to write to the selected file.", title, wxOK | wxICON_ERROR, parent);
		return false;
	}
	ofs << out.c_str();
	return true;
}

bool ImportYamlWithDialog(wxWindow* parent, const wxString& title,
	const std::function<void(const YAML::Node&)>& apply)
{
	wxFileDialog fd(parent, title, "", "", YAML_WILDCARD, wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return false;
	}
	std::ifstream ifs(fd.GetPath().ToStdString(), std::ios::binary);
	if (!ifs.is_open())
	{
		wxMessageBox("Unable to read the selected file.", title, wxOK | wxICON_ERROR, parent);
		return false;
	}
	std::ostringstream contents;
	contents << ifs.rdbuf();
	try
	{
		apply(YAML::Load(contents.str()));
		return true;
	}
	catch (const std::exception& e)
	{
		wxMessageBox(std::string("Error when parsing YAML:\n") + e.what(), title, wxOK | wxICON_ERROR, parent);
		return false;
	}
}

int ReadYamlInt(const YAML::Node& node, const std::string& context, int min, int max)
{
	try
	{
		const int value = node.as<int>();
		if (value < min || value > max)
		{
			throw std::runtime_error(context + ": value " + std::to_string(value) + " out of range");
		}
		return value;
	}
	catch (const YAML::Exception& e)
	{
		throw std::runtime_error(context + ": " + e.what());
	}
}
