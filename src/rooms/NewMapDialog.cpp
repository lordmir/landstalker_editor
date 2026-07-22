#include <rooms/NewMapDialog.h>

#include <wx/textdlg.h>

#include <landstalker/misc/Utils.h>

namespace
{

// New and imported maps start out at this size; an import then resizes to suit the file.
constexpr uint8_t DEFAULT_MAP_SIZE = 16;

uint8_t SpinValue(const wxSpinCtrl* ctrl)
{
	return static_cast<uint8_t>(ctrl->GetValue());
}

}

NewMapDialog::NewMapDialog(wxWindow* parent, const std::string& suggested_name)
	: wxDialog(parent, wxID_ANY, "Add Map")
{
	auto* outer = new wxBoxSizer(wxVERTICAL);
	auto* fields = new wxFlexGridSizer(2, 6, 6);
	fields->AddGrowableCol(1, 1);

	m_name = new wxTextCtrl(this, wxID_ANY, wxString::FromUTF8(suggested_name));
	m_name->SetMaxLength(30);
	fields->Add(new wxStaticText(this, wxID_ANY, "Name"), 0, wxALIGN_CENTER_VERTICAL);
	fields->Add(m_name, 1, wxEXPAND);

	m_map_width = AddSpin(fields, "Map width", DEFAULT_MAP_SIZE, 1, 64);
	m_map_height = AddSpin(fields, "Map height", DEFAULT_MAP_SIZE, 1, 64);
	m_heightmap_width = AddSpin(fields, "Heightmap width", DEFAULT_MAP_SIZE, 1, 64);
	m_heightmap_height = AddSpin(fields, "Heightmap height", DEFAULT_MAP_SIZE, 1, 64);
	m_heightmap_left = AddSpin(fields, "Heightmap left", 0, 0, 63);
	m_heightmap_top = AddSpin(fields, "Heightmap top", 0, 0, 63);

	outer->Add(fields, 1, wxALL | wxEXPAND, 10);
	outer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL), 0, wxALL | wxEXPAND, 10);
	SetSizerAndFit(outer);
	SetMinSize(wxSize(360, -1));
	CentreOnParent();
}

std::string NewMapDialog::GetMapName() const { return m_name->GetValue().ToStdString(); }
uint8_t NewMapDialog::GetMapWidth() const { return SpinValue(m_map_width); }
uint8_t NewMapDialog::GetMapHeight() const { return SpinValue(m_map_height); }
uint8_t NewMapDialog::GetHeightmapWidth() const { return SpinValue(m_heightmap_width); }
uint8_t NewMapDialog::GetHeightmapHeight() const { return SpinValue(m_heightmap_height); }
uint8_t NewMapDialog::GetHeightmapLeft() const { return SpinValue(m_heightmap_left); }
uint8_t NewMapDialog::GetHeightmapTop() const { return SpinValue(m_heightmap_top); }

wxSpinCtrl* NewMapDialog::AddSpin(wxFlexGridSizer* fields, const char* label, int value, int minimum, int maximum)
{
	fields->Add(new wxStaticText(this, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
	auto* control = new wxSpinCtrl(this, wxID_ANY);
	control->SetRange(minimum, maximum);
	control->SetValue(value);
	fields->Add(control, 1, wxEXPAND);
	return control;
}

std::string SuggestMapName(const Landstalker::RoomData& rooms)
{
	const auto& maps = rooms.GetMaps();
	for (unsigned int i = 1; ; ++i)
	{
		const auto candidate = Landstalker::StrPrintf("Map%03u", i);
		if (maps.count(candidate) == 0)
		{
			return candidate;
		}
	}
}

std::string SuggestMapCopyName(const Landstalker::RoomData& rooms, const std::string& source)
{
	const auto& maps = rooms.GetMaps();
	// Truncate before appending, so repeatedly copying a long name cannot creep past the
	// 30 character limit and start failing validation.
	std::string base = source.size() > 24 ? source.substr(0, 24) : source;
	for (unsigned int i = 1; ; ++i)
	{
		const auto candidate = i == 1 ? base + "Copy" : base + "Copy" + std::to_string(i);
		if (maps.count(candidate) == 0)
		{
			return candidate;
		}
	}
}

bool PromptForMapName(wxWindow* parent, const wxString& title, const Landstalker::RoomData& rooms,
	const std::string& initial, std::string& name)
{
	wxTextEntryDialog dialog(parent, "Enter a unique assembly label for the map.", title,
		wxString::FromUTF8(initial));
	dialog.SetMaxLength(30);
	while (dialog.ShowModal() == wxID_OK)
	{
		const auto candidate = dialog.GetValue().ToStdString();
		if (Landstalker::RoomData::IsValidMapName(candidate) && rooms.GetMaps().count(candidate) == 0)
		{
			name = candidate;
			return true;
		}
		wxMessageBox("The name must start with a letter, only contain A-Z, a-z, 0-9 and _, and be at most 30 characters.",
			title, wxOK | wxICON_ERROR, parent);
	}
	return false;
}

bool DuplicateMap(const std::shared_ptr<Landstalker::GameData>& gd,
	const std::string& source, const std::string& new_name)
{
	if (!gd)
	{
		return false;
	}
	const auto room_data = gd->GetRoomData();
	// GetMap dereferences an end iterator for an unknown name, so look before leaping.
	if (room_data->GetMaps().count(source) == 0)
	{
		return false;
	}
	const auto original = room_data->GetMap(source);
	if (!original || !original->GetData())
	{
		return false;
	}

	const auto& map = *original->GetData();
	const auto copy = room_data->CreateMap(new_name, map.GetWidth(), map.GetHeight(),
		map.GetHeightmapWidth(), map.GetHeightmapHeight(), map.GetLeft(), map.GetTop());
	if (!copy || !copy->GetData())
	{
		return false;
	}
	// Tilemap3D is a plain value type, so this carries the tilemap layers and heightmap
	// across as well as the dimensions CreateMap already set.
	*copy->GetData() = map;
	return true;
}

std::string PromptCreateMap(wxWindow* parent, const std::shared_ptr<Landstalker::GameData>& gd)
{
	if (!gd)
	{
		return std::string();
	}
	const auto room_data = gd->GetRoomData();
	NewMapDialog dialog(parent, SuggestMapName(*room_data));
	while (dialog.ShowModal() == wxID_OK)
	{
		const auto name = dialog.GetMapName();
		if (room_data->CreateMap(name,
			dialog.GetMapWidth(), dialog.GetMapHeight(),
			dialog.GetHeightmapWidth(), dialog.GetHeightmapHeight(),
			dialog.GetHeightmapLeft(), dialog.GetHeightmapTop()))
		{
			return name;
		}
		wxMessageBox("The name must be unique, start with a letter, only contain A-Z, a-z, 0-9 and _, "
			"and be at most 30 characters. All sizes must be valid.",
			"Add Map", wxOK | wxICON_ERROR, parent);
	}
	return std::string();
}
