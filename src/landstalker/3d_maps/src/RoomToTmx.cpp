#include <landstalker/3d_maps/include/RoomToTmx.h>
#include <landstalker/3d_maps/include/MapToTmx.h>
#include <landstalker/rooms/include/Room.h>
#include <landstalker/rooms/include/Entity.h>
#include <landstalker/rooms/include/WarpList.h>
#include <sstream>
#include <iomanip>
#include <wx/filename.h>
#include <wx/xml/xml.h>

bool RoomToTmx::ExportToTmx(const std::string& fname, int roomnum, std::shared_ptr<GameData> gameData, const std::string& blockset_filename)
{
	std::shared_ptr<RoomData> roomData = gameData->GetRoomData();
	wxXmlDocument tmx = MapToTmx::GenerateXmlDocument(fname, *(roomData->GetMapForRoom(roomnum)->GetData()), blockset_filename);

	// Properties
	auto room = roomData->GetRoom(roomnum);
	auto properties = new wxXmlNode(wxXML_ELEMENT_NODE, "properties");

	// Room Properties
	auto name_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
	name_property->AddAttribute("name", "RoomName");
	name_property->AddAttribute("value", room->GetDisplayName());
	properties->AddChild(name_property);

	auto label_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
	name_property->AddAttribute("name", "RoomLabel");
	name_property->AddAttribute("value", room->name);
	properties->AddChild(label_property);

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

	auto lantern_flags_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
	lantern_flags_property->AddAttribute("name", "FlagLantern");
	lantern_flags_property->AddAttribute("type", "int");
	lantern_flags_property->AddAttribute("value", std::to_string(roomData->GetLanternFlag(roomnum)));
	properties->AddChild(lantern_flags_property);
	tmx.GetRoot()->AddChild(properties);

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


	// Warp objects
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
		
		// Warp properties
		auto warp_properties = new wxXmlNode(wxXML_ELEMENT_NODE, "properties");
		
		auto room1_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
		room1_property->AddAttribute("name", "room1");
		room1_property->AddAttribute("value", std::to_string(warp.room1));
		warp_properties->AddChild(room1_property);
		
		auto room2_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
		room2_property->AddAttribute("name", "room2");
		room2_property->AddAttribute("value", std::to_string(warp.room2));
		warp_properties->AddChild(room2_property);
		
		auto x2_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
		x2_property->AddAttribute("name", "x2");
		x2_property->AddAttribute("value", std::to_string(warp.x2));
		warp_properties->AddChild(x2_property);
		
		auto y2_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
		y2_property->AddAttribute("name", "y2");
		y2_property->AddAttribute("value", std::to_string(warp.y2));
		warp_properties->AddChild(y2_property);
		
		auto type_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
		type_property->AddAttribute("name", "warpType");

		// Convert the warp type to a string
		std::string warp_type_str;
		switch (warp.type) {
			case WarpList::Warp::Type::NORMAL:   warp_type_str = "NORMAL"; break;
			case WarpList::Warp::Type::STAIR_SE: warp_type_str = "STAIR_SE"; break;
			case WarpList::Warp::Type::STAIR_SW: warp_type_str = "STAIR_SW"; break;
			default:                   warp_type_str = "UNKNOWN"; break;
		}

		type_property->AddAttribute("value", warp_type_str);
		warp_properties->AddChild(type_property);
		
		warp_object->AddChild(warp_properties);
		warp_id++;
		warps_objectgroup->AddChild(warp_object);
	}


	tmx.GetRoot()->AddChild(warps_objectgroup);


	// Entity objects
	auto entities_objectgroup = new wxXmlNode(wxXML_ELEMENT_NODE, "objectgroup");
	entities_objectgroup->AddAttribute("id", "4");
	entities_objectgroup->AddAttribute("name", "Entities");

int entity_id = 1;
std::vector<Entity> entities = gameData->GetSpriteData()->GetRoomEntities(roomnum);

for (const auto& entity : entities) {
    auto entity_object = new wxXmlNode(wxXML_ELEMENT_NODE, "object");
    entity_object->AddAttribute("id", std::to_string(entity_id));
    entity_object->AddAttribute("visible", "0");
    entity_object->AddAttribute("name", entity.GetTypeName());
	entity_object->AddAttribute("class", entity.GetTypeName());
    entities_objectgroup->AddChild(entity_object);

    // Entity properties
    auto entity_properties = new wxXmlNode(wxXML_ELEMENT_NODE, "properties");

    auto type_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
    type_property->AddAttribute("name", "Type");
    type_property->AddAttribute("value", std::to_string(entity.GetType()));
	entity_properties->AddChild(type_property);

    auto x_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
    x_property->AddAttribute("name", "X");
	x_property->AddAttribute("type", "float");
    x_property->AddAttribute("value", std::to_string(entity.GetXDbl()));
	entity_properties->AddChild(x_property);

    auto y_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
    y_property->AddAttribute("name", "Y");
	y_property->AddAttribute("type", "float");
    y_property->AddAttribute("value", std::to_string(entity.GetYDbl()));
	entity_properties->AddChild(y_property);

    auto z_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
    z_property->AddAttribute("name", "Z");
	z_property->AddAttribute("type", "float");
    z_property->AddAttribute("value", std::to_string(entity.GetZDbl()));
	entity_properties->AddChild(z_property);

    auto ent_palette_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
	ent_palette_property->AddAttribute("name", "Palette");
	ent_palette_property->AddAttribute("value", std::to_string(entity.GetPalette()));
	entity_properties->AddChild(ent_palette_property);

    auto behaviour_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
    behaviour_property->AddAttribute("name", "Behaviour");
    behaviour_property->AddAttribute("value", std::to_string(entity.GetBehaviour()));
	entity_properties->AddChild(behaviour_property);

    auto dialogue_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
    dialogue_property->AddAttribute("name", "Dialogue");
    dialogue_property->AddAttribute("value", std::to_string(entity.GetDialogue()));
	entity_properties->AddChild(dialogue_property);

    auto hostile_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
    hostile_property->AddAttribute("name", "Hostile");
	hostile_property->AddAttribute("type", "bool");
    hostile_property->AddAttribute("value", entity.IsHostile() ? "true" : "false");
	entity_properties->AddChild(hostile_property);

    auto no_rotate_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
    no_rotate_property->AddAttribute("name", "NoRotate");
	no_rotate_property->AddAttribute("type", "bool");
    no_rotate_property->AddAttribute("value", entity.NoRotate() ? "true" : "false");
	entity_properties->AddChild(no_rotate_property);

    auto no_pickup_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
    no_pickup_property->AddAttribute("name", "NoPickup");
	no_pickup_property->AddAttribute("type", "bool");
    no_pickup_property->AddAttribute("value", entity.NoPickup() ? "true" : "false");
	entity_properties->AddChild(no_pickup_property);

    auto has_dialogue_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
    has_dialogue_property->AddAttribute("name", "HasDialogue");
	has_dialogue_property->AddAttribute("type", "bool");
    has_dialogue_property->AddAttribute("value", entity.HasDialogue() ? "true" : "false");
	entity_properties->AddChild(has_dialogue_property);

    auto visible_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
    visible_property->AddAttribute("name", "Visible");
	visible_property->AddAttribute("type", "bool");
    visible_property->AddAttribute("value", entity.IsVisible() ? "true" : "false");
	entity_properties->AddChild(visible_property);

    auto solid_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
    solid_property->AddAttribute("name", "Solid");
	solid_property->AddAttribute("type", "bool");
    solid_property->AddAttribute("value", entity.IsSolid() ? "true" : "false");
	entity_properties->AddChild(solid_property);

    auto gravity_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
    gravity_property->AddAttribute("name", "Gravity");
	gravity_property->AddAttribute("type", "bool");
    gravity_property->AddAttribute("value", entity.HasGravity() ? "true" : "false");
	entity_properties->AddChild(gravity_property);

    auto friction_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
    friction_property->AddAttribute("name", "Friction");
	friction_property->AddAttribute("type", "bool");
    friction_property->AddAttribute("value", entity.HasFriction() ? "true" : "false");
	entity_properties->AddChild(friction_property);

    auto speed_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
    speed_property->AddAttribute("name", "Speed");
	speed_property->AddAttribute("type", "int");
    speed_property->AddAttribute("value", std::to_string(entity.GetSpeed()));
	entity_properties->AddChild(speed_property);

    auto orientation_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
    orientation_property->AddAttribute("name", "Orientation");
    orientation_property->AddAttribute("value", entity.GetOrientationName());
	entity_properties->AddChild(orientation_property);

    auto tile_source_property = new wxXmlNode(wxXML_ELEMENT_NODE, "property");
    tile_source_property->AddAttribute("name", "TileSource");
    tile_source_property->AddAttribute("value", std::to_string(entity.GetCopySource()));
	entity_properties->AddChild(tile_source_property);

    entity_object->AddChild(entity_properties);
    
    entity_id++;
}

	tmx.GetRoot()->AddChild(entities_objectgroup);

	return tmx.Save(fname);
}
