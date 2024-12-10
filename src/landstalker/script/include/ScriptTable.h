#ifndef _SCRIPT_TABLE_
#define _SCRIPT_TABLE_

#include <cstdint>
#include <string>
#include <variant>
#include <array>
#include <vector>
#include <optional>
#include <memory>
#include <filesystem>

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
		Item() : item(0), shop(0), other(std::nullopt) {}
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

	std::shared_ptr<std::vector<Action>> ReadTable(const std::string& path);
	std::shared_ptr<std::vector<Shop>> ReadShopTable(const std::string& path);
	std::shared_ptr<std::vector<Item>> ReadItemTable(const std::string& path);
	bool WriteTable(const std::filesystem::path& prefix, const std::filesystem::path& path, const std::string& description, std::shared_ptr<std::vector<Action>> table);
	bool WriteShopTable(const std::filesystem::path& prefix, const std::filesystem::path& path, const std::string& description, std::shared_ptr<std::vector<Shop>> table);
	bool WriteItemTable(const std::filesystem::path& prefix, const std::filesystem::path& path, const std::string& description, std::shared_ptr<std::vector<Item>> table);
}

#endif // _SCRIPT_TABLE_
