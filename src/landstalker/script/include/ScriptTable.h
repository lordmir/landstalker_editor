#ifndef _SCRIPT_TABLE_
#define _SCRIPT_TABLE_

#include <cstdint>
#include <string>
#include <variant>
#include <array>
#include <vector>
#include <map>
#include <optional>
#include <memory>
#include <filesystem>

class GameData;

namespace ScriptTable
{
	using Action = std::variant<uint16_t, std::string>;

	struct Shop
	{
		Shop() : room(0), markup(0), lifestock_markup(0) {}
		uint16_t room;
		uint8_t markup;
		uint8_t lifestock_markup;
		std::array<Action, 5> actions;

		bool operator==(const Shop& rhs) const
		{
			return (this->room == rhs.room && this->markup == rhs.markup &&
				this->lifestock_markup == rhs.lifestock_markup && this->actions == rhs.actions);
		}
		bool operator!=(const Shop& rhs) const
		{
			return !(*this == rhs);
		}
	};
	struct Item
	{
		Item() : item(0), shop(0xFFFF), other(std::nullopt) {}
		uint8_t item;
		uint16_t shop;
		std::optional<uint16_t> other;
		std::vector<Action> actions;

		bool operator==(const Item& rhs) const
		{
			return (this->item == rhs.item && this->shop == rhs.shop &&
				this->other == rhs.other && this->actions == rhs.actions);
		}
		bool operator!=(const Item& rhs) const
		{
			return !(*this == rhs);
		}
	};

	std::shared_ptr<std::vector<Action>> ReadTable(const std::string& path, const std::map<std::string, std::string>& defines = {});
	std::shared_ptr<std::vector<Shop>> ReadShopTable(const std::string& path, const std::map<std::string, std::string>& defines = {});
	std::shared_ptr<std::vector<Item>> ReadItemTable(const std::string& path, const std::map<std::string, std::string>& defines = {});
	bool WriteTable(const std::filesystem::path& prefix, const std::filesystem::path& path, const std::string& description, std::shared_ptr<std::vector<Action>> table);
	bool WriteShopTable(const std::filesystem::path& prefix, const std::filesystem::path& path, const std::string& description, std::shared_ptr<std::vector<Shop>> table);
	bool WriteItemTable(const std::filesystem::path& prefix, const std::filesystem::path& path, const std::string& description, std::shared_ptr<std::vector<Item>> table);

	std::string TableToYaml(std::shared_ptr<std::vector<Action>> table);
	std::string TableToYaml(std::shared_ptr<std::vector<Shop>> table);
	std::string TableToYaml(std::shared_ptr<std::vector<Item>> table);
	std::vector<Action> TableFromYaml(const std::string& yaml);
	std::vector<Shop>  ShopTableFromYaml(const std::string& yaml);
	std::vector<Item> ItemTableFromYaml(const std::string& yaml);

	std::wstring GetActionDescription(const Action& action, std::shared_ptr<GameData> gd);
	std::wstring GetActionSummary(const Action& action, std::shared_ptr<GameData> gd);
	std::string ToString(const Action& action);
	Action FromString(const std::string& string);
}

#endif // _SCRIPT_TABLE_
