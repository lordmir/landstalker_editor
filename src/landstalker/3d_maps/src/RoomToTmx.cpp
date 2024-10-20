#include <landstalker/3d_maps/include/RoomToTmx.h>
#include <landstalker/3d_maps/include/MapToTmx.h>
#include <landstalker/rooms/include/Room.h>
#include <landstalker/rooms/include/WarpList.h>
#include <sstream>
#include <iomanip>
#include <wx/filename.h>
#include <wx/xml/xml.h>

bool RoomToTmx::ExportToTmx(const std::string& fname, int roomnum, std::shared_ptr<RoomData> roomData, const std::string& blockset_filename)
{
	wxXmlDocument tmx = MapToTmx::GenerateXmlDocument(fname, *(roomData->GetMapForRoom(roomnum)->GetData()), blockset_filename);

	// Properties
	auto room = roomData->GetRoom(roomnum);
	auto properties = new wxXmlNode(wxXML_ELEMENT_NODE, "properties");

	// Room Properties
	auto name_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
	name_property->AddAttribute("name", "RoomName");
	name_property->AddAttribute("value", room->name);
	properties->AddChild(name_property);

	auto room_number_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
	room_number_property->AddAttribute("name", "RoomNumber");
	room_number_property->AddAttribute("value", std::to_string(roomnum));
	properties->AddChild(room_number_property);

	auto tileset_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
	tileset_property->AddAttribute("name", "RoomTileset");
	tileset_property->AddAttribute("value", std::to_string(room->tileset));
	properties->AddChild(tileset_property);

	auto palette_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
	palette_property->AddAttribute("name", "RoomPalette");
	palette_property->AddAttribute("value", std::to_string(room->room_palette));
	properties->AddChild(palette_property);

	auto pri_blockset_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
	pri_blockset_property->AddAttribute("name", "RoomPrimaryBlockset");
	pri_blockset_property->AddAttribute("value", std::to_string(room->pri_blockset));
	properties->AddChild(pri_blockset_property);

	auto sec_blockset_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
	sec_blockset_property->AddAttribute("name", "RoomSecondaryBlockset");
	sec_blockset_property->AddAttribute("value", std::to_string(room->sec_blockset));
	properties->AddChild(sec_blockset_property);

	auto bgm_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
	bgm_property->AddAttribute("name", "RoomBGM");
	bgm_property->AddAttribute("value", std::to_string(room->bgm));
	properties->AddChild(bgm_property);

	auto map_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
	map_property->AddAttribute("name", "RoomMap");
	map_property->AddAttribute("value", room->map);
	properties->AddChild(map_property);

	auto unknown_param1_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
	unknown_param1_property->AddAttribute("name", "RoomUnknownParam1");
	unknown_param1_property->AddAttribute("value", std::to_string(room->unknown_param1));
	properties->AddChild(unknown_param1_property);

	auto unknown_param2_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
	unknown_param2_property->AddAttribute("name", "RoomUnknownParam2");
	unknown_param2_property->AddAttribute("value", std::to_string(room->unknown_param2));
	properties->AddChild(unknown_param2_property);

	auto z_begin_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
	z_begin_property->AddAttribute("name", "RoomZBegin");
	z_begin_property->AddAttribute("value", std::to_string(room->room_z_begin));
	properties->AddChild(z_begin_property);

	auto z_end_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
	z_end_property->AddAttribute("name", "RoomZEnd");
	z_end_property->AddAttribute("value", std::to_string(room->room_z_end));
	properties->AddChild(z_end_property);

	// Warps Properties
	auto fall_destination_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
	fall_destination_property->AddAttribute("name", "WarpFallDestination");
	fall_destination_property->AddAttribute("type", "int");
	fall_destination_property->AddAttribute("value", std::to_string(roomData->GetFallDestination(roomnum)));
	properties->AddChild(fall_destination_property);

	auto climb_destination_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
	climb_destination_property->AddAttribute("name", "WarpClimbDestination");
	climb_destination_property->AddAttribute("type", "int");
	climb_destination_property->AddAttribute("value", std::to_string(roomData->GetClimbDestination(roomnum)));
	properties->AddChild(climb_destination_property);

	// Flags Properties
	auto lantern_room_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
	lantern_room_property->AddAttribute("name", "FlagHasLantern");
	lantern_room_property->AddAttribute("type", "bool");
	lantern_room_property->AddAttribute("value", roomData->HasLanternFlag(roomnum) ? "true" : "false");
	properties->AddChild(lantern_room_property);

	// Misc Properties
	auto shop_room_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
	shop_room_property->AddAttribute("name", "FlagIsShopChurchInn");
	shop_room_property->AddAttribute("type", "bool");
	shop_room_property->AddAttribute("value", roomData->IsShop(roomnum) ? "true" : "false");
	properties->AddChild(shop_room_property);

	auto lifestock_for_sale_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
	lifestock_for_sale_property->AddAttribute("name", "MiscLifestockForSale");
	lifestock_for_sale_property->AddAttribute("type", "bool");
	lifestock_for_sale_property->AddAttribute("value", roomData->HasLifestockSaleFlag(roomnum) ? "true" : "false");
	properties->AddChild(lifestock_for_sale_property);

	auto lifestock_sale_flags_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
	lifestock_sale_flags_property->AddAttribute("name", "MiscLifestockSaleFlag");
	lifestock_sale_flags_property->AddAttribute("type", "int");
	lifestock_sale_flags_property->AddAttribute("value", std::to_string(roomData->GetLifestockSaleFlag(roomnum)));
	properties->AddChild(lifestock_sale_flags_property);

	tmx.GetRoot()->AddChild(properties);


	// Warps object
	auto warps_objectgroup = new wxXmlNode(wxXML_ELEMENT_NODE, "objectgroup");
	warps_objectgroup->AddAttribute("id", "3");
	warps_objectgroup->AddAttribute("name", "Warps");

	int warp_id = 1;
    std::vector<WarpList::Warp> warps = roomData->GetWarpsForRoom(roomnum);
	for (const auto& warp : warps) {
		auto warp_object = new wxXmlNode(wxXML_ELEMENT_NODE, "object");
		warp_object->AddAttribute("id", std::to_string(warp_id));
		warp_object->AddAttribute("visible", "0");
		warp_object->AddAttribute("name", "Warp");
		warp_object->AddAttribute("x", std::to_string(warp.x1));
		warp_object->AddAttribute("y", std::to_string(warp.y1));
		warp_object->AddAttribute("width", std::to_string(warp.x_size));
		warp_object->AddAttribute("height", std::to_string(warp.y_size));
		
		// Adding warp properties
		auto properties = new wxXmlNode(wxXML_ELEMENT_NODE, "properties");
		
		auto room1_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
		room1_property->AddAttribute("name", "room1");
		room1_property->AddAttribute("value", std::to_string(warp.room1));
		properties->AddChild(room1_property);
		
		auto room2_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
		room2_property->AddAttribute("name", "room2");
		room2_property->AddAttribute("value", std::to_string(warp.room2));
		properties->AddChild(room2_property);
		
		auto x2_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
		x2_property->AddAttribute("name", "x2");
		x2_property->AddAttribute("value", std::to_string(warp.x2));
		properties->AddChild(x2_property);
		
		auto y2_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
		y2_property->AddAttribute("name", "y2");
		y2_property->AddAttribute("value", std::to_string(warp.y2));
		properties->AddChild(y2_property);
		
		auto type_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
		type_property->AddAttribute("name", "type");

		// Convert the warp type to a string
		std::string warp_type_str;
		switch (warp.type) {
			case WarpList::Warp::Type::NORMAL:   warp_type_str = "NORMAL"; break;
			case WarpList::Warp::Type::STAIR_SE: warp_type_str = "STAIR_SE"; break;
			case WarpList::Warp::Type::STAIR_SW: warp_type_str = "STAIR_SW"; break;
			default:                   warp_type_str = "UNKNOWN"; break;
		}

		type_property->AddAttribute("value", warp_type_str);
		properties->AddChild(type_property);
		
		warp_object->AddChild(properties);
		warp_id++;
		warps_objectgroup->AddChild(warp_object);
	}


	tmx.GetRoot()->AddChild(warps_objectgroup);

	return tmx.Save(fname);
}
