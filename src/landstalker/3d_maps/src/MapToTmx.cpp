#include <landstalker/3d_maps/include/MapToTmx.h>
#include <sstream>
#include <iomanip>
#include <wx/filename.h>

std::string GetData(const Tilemap3D& map, Tilemap3D::Layer layer)
{
	std::ostringstream ss;
	int i = 0;
	for (int y = 0; y < map.GetHeight(); ++y)
	{
		for (int x = 0; x < map.GetWidth(); ++x, ++i)
		{
			ss << std::setw(4) << std::setfill('0') << (map.GetBlock({ x, y }, layer) + 1);
			if (i < map.GetWidth() * map.GetHeight() - 1)
			{
				ss << ",";
			}
		}
		ss << std::endl;
	}
	return ss.str();
}

std::vector<uint16_t> ReadData(int width, int height, const wxString& csv)
{
	std::istringstream ss(csv.ToStdString());
	std::vector<uint16_t> retval(width * height);
	int x = 0, y = 0;
    std::string row, cell;
	while(std::getline(ss, row) && y < height)
	{
		std::istringstream rss(row);
		while(std::getline(rss, cell, ',') && x < width)
		{
			retval[x + y * width] = (std::stoi(cell) - 1) & 0x03FF;
			x++;
		}
		x = 0;
		y++;
	}
	return retval;
}

bool MapToTmx::ImportFromTmx(const std::string& fname, Tilemap3D& map)
{
	wxXmlDocument tmx(fname);
	int width = std::stoi(tmx.GetRoot()->GetAttribute("width").ToStdString()) - 1;
	int height = std::stoi(tmx.GetRoot()->GetAttribute("height").ToStdString());
	std::vector<uint16_t> fg, bg;
	auto child = tmx.GetRoot()->GetChildren();
	while (child)
	{
		if (child->GetName() == "layer")
		{
			auto grandchild = child->GetChildren();
			if (grandchild && grandchild->GetName() == "data" && grandchild->GetChildren())
			{
				if(child->GetAttribute("id") == "1")
				{
					bg = ReadData(width, height, grandchild->GetChildren()->GetContent().ToStdString());
				}
				else if (child->GetAttribute("id") == "2")
				{
					fg = ReadData(width, height, grandchild->GetChildren()->GetContent().ToStdString());
				}
			}
		}
		child = child->GetNext();
	}

	if (width > 0 && width < 64 &&
	    height > 0 && height < 64 &&
		fg.size() == static_cast<std::size_t>(width * height) &&
		bg.size() == static_cast<std::size_t>(width * height))
	{
		map.Resize(width, height);
		int i = 0;
		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x, ++i)
			{
				map.SetBlock({ fg[i], {x, y} }, Tilemap3D::Layer::FG);
				map.SetBlock({ bg[i], {x, y} }, Tilemap3D::Layer::BG);
			}
		}
		return true;
	}

	return false;
}

wxXmlDocument MapToTmx::GenerateXmlDocument(const std::string& fname, const Tilemap3D& map, const std::string& blockset_filename)
{
	wxXmlDocument tmx;
	wxFileName fn(fname);
	wxFileName bsfn(blockset_filename);
	tmx.SetRoot(new wxXmlNode(wxXML_ELEMENT_NODE, "map"));
	tmx.GetRoot()->AddAttribute("version", "1.10");
	tmx.GetRoot()->AddAttribute("tiledversion", "1.10.1");
	tmx.GetRoot()->AddAttribute("class", fn.GetName());
	tmx.GetRoot()->AddAttribute("orientation", "isometric");
	tmx.GetRoot()->AddAttribute("renderorder", "left-down");
	tmx.GetRoot()->AddAttribute("width", std::to_string(map.GetWidth() + 1));
	tmx.GetRoot()->AddAttribute("height", std::to_string(map.GetHeight()));
	tmx.GetRoot()->AddAttribute("tilewidth", "32");
	tmx.GetRoot()->AddAttribute("tileheight", "16");
	tmx.GetRoot()->AddAttribute("infinite", "0");
	tmx.GetRoot()->AddAttribute("backgroundcolor", "#181818");
	tmx.GetRoot()->AddAttribute("nextlayerid", "3");
	tmx.GetRoot()->AddAttribute("nextobjectid", "1");

	auto tileset = new wxXmlNode(wxXML_ELEMENT_NODE, "tileset");
	tileset->AddAttribute("firstgid", "1");
	tileset->AddAttribute("name", bsfn.GetName());
	tileset->AddAttribute("tilewidth", "16");
	tileset->AddAttribute("tileheight", "16");
	tileset->AddAttribute("tilecount", "1024");
	tileset->AddAttribute("columns", "16");

	auto image = new wxXmlNode(wxXML_ELEMENT_NODE, "image");
	image->AddAttribute("source", blockset_filename);
	image->AddAttribute("width", "256");
	image->AddAttribute("height", "1024");
	tileset->AddChild(image);
	tmx.GetRoot()->AddChild(tileset);

	auto bg_layer = new wxXmlNode(wxXML_ELEMENT_NODE, "layer");
	bg_layer->AddAttribute("id", "1");
	bg_layer->AddAttribute("name", "Background");
	bg_layer->AddAttribute("width", std::to_string(map.GetWidth()));
	bg_layer->AddAttribute("height", std::to_string(map.GetHeight()));
	bg_layer->AddAttribute("offsetx", "16");
	bg_layer->AddAttribute("offsety", "0");
	auto bg_data = new wxXmlNode(wxXML_ELEMENT_NODE, "data");
	bg_data->AddAttribute("encoding", "csv");
	bg_data->AddChild(new wxXmlNode(wxXML_TEXT_NODE, "", GetData(map, Tilemap3D::Layer::BG)));
	bg_layer->AddChild(bg_data);

	auto fg_layer = new wxXmlNode(wxXML_ELEMENT_NODE, "layer");
	fg_layer->AddAttribute("id", "2");
	fg_layer->AddAttribute("name", "Foreground");
	fg_layer->AddAttribute("width", std::to_string(map.GetWidth()));
	fg_layer->AddAttribute("height", std::to_string(map.GetHeight()));
	fg_layer->AddAttribute("offsetx", "0");
	fg_layer->AddAttribute("offsety", "0");
	auto fg_data = new wxXmlNode(wxXML_ELEMENT_NODE, "data");
	fg_data->AddAttribute("encoding", "csv");
	fg_data->AddChild(new wxXmlNode(wxXML_TEXT_NODE, "", GetData(map, Tilemap3D::Layer::FG)));
	fg_layer->AddChild(fg_data);

	tmx.GetRoot()->AddChild(bg_layer);
	tmx.GetRoot()->AddChild(fg_layer);

	return tmx;
}

bool MapToTmx::ExportToTmx(const std::string& fname, const Tilemap3D& map, const std::string& blockset_filename)
{
	wxXmlDocument tmx = MapToTmx::GenerateXmlDocument(fname, map, blockset_filename);
	return tmx.Save(fname);
}
