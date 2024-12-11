#include <landstalker/script/include/ScriptTable.h>
#include <landstalker/main/include/AsmFile.h>
#include <landstalker/misc/include/Literals.h>
#include <landstalker/misc/include/Utils.h>

#include <yaml-cpp/yaml.h>

static std::vector<ScriptTable::Action> ParseAsmBody(AsmFile& file)
{
	AsmFile::ScriptAction action;
	std::vector<ScriptTable::Action> actions;
	try
	{
		while (file.IsGood() && file.Read(action))
		{
			std::visit([&](const auto& arg)
				{
					using T = std::decay_t<decltype(arg)>;
					if constexpr (std::is_same_v<T, AsmFile::ScriptJump>)
					{
						actions.push_back(arg.func);
					}
					else if constexpr (std::is_same_v<T, AsmFile::ScriptId>)
					{
						actions.push_back(static_cast<uint16_t>(arg.script_id));
					}
				}, *action);
		}
	}
	catch (const std::exception&)
	{
	}
	return actions;
}

template <class Iter>
static void WriteScriptBody(AsmFile& file, Iter it, Iter end, std::size_t& offset)
{
	for (; it != end; ++it)
	{
		std::visit([&](const auto& arg)
			{
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, uint16_t>)
				{
					file << AsmFile::ScriptId(arg, offset++);
				}
				else if constexpr (std::is_same_v<T, std::string>)
				{
					file << AsmFile::ScriptJump(arg, offset++);
				}
			}, *it);
	}
}

std::shared_ptr<std::vector<ScriptTable::Action>> ScriptTable::ReadTable(const std::string& path)
{
	AsmFile file(path);
	return std::make_shared<std::vector<ScriptTable::Action>>(ParseAsmBody(file));
}

std::shared_ptr<std::vector<ScriptTable::Shop>> ScriptTable::ReadShopTable(const std::string& path)
{
	AsmFile file(path);
	std::shared_ptr<std::vector<ScriptTable::Shop>> shops = std::make_shared<std::vector<ScriptTable::Shop>>();
	try
	{
		while (file.IsGood())
		{
			Shop shop;
			file >> shop.room;
			if (shop.room == 0xFFFF)
			{
				break;
			}
			file >> shop.markup >> shop.lifestock_markup;
			auto actions = ParseAsmBody(file);
			assert(actions.size() == shop.actions.size());
			std::copy(actions.begin(), actions.end(), shop.actions.begin());
			shops->push_back(shop);
		}
	}
	catch (const std::exception&)
	{
	}
	return shops;
}

std::shared_ptr<std::vector<ScriptTable::Item>> ScriptTable::ReadItemTable(const std::string& path)
{
	AsmFile file(path);
	std::shared_ptr<std::vector<ScriptTable::Item>> items = std::make_shared<std::vector<ScriptTable::Item>>();
	try
	{
		while (file.IsGood())
		{
			Item item;
			uint8_t len;
			uint16_t extra;
			file >> item.item >> len;
			if (item.item == 0xFF || len == 0xFF)
			{
				break;
			}
			file >> item.shop;
			item.actions = ParseAsmBody(file);
			if ((item.actions.size() * 2 + 4) < len && file.Read(extra))
			{
				item.other = extra;
			}
			items->push_back(item);
		}
	}
	catch (const std::exception&)
	{
	}
	return items;
}

bool ScriptTable::WriteTable(const std::filesystem::path& prefix, const std::filesystem::path& path, const std::string& description, std::shared_ptr<std::vector<ScriptTable::Action>> table)
{
	std::size_t offset = 0;
	AsmFile file;
	file.WriteFileHeader(path, description);
	WriteScriptBody(file, table->begin(), table->end(), offset);
	return file.WriteFile(prefix / path);
}

bool ScriptTable::WriteShopTable(const std::filesystem::path& prefix, const std::filesystem::path& path, const std::string& description, std::shared_ptr<std::vector<ScriptTable::Shop>> table)
{
	std::size_t offset = 0;
	AsmFile file;
	file.WriteFileHeader(path, description);
	for (const auto& shop : *table)
	{
		file << shop.room << shop.markup << shop.lifestock_markup;
		offset += 2;
		WriteScriptBody(file, shop.actions.begin(), shop.actions.end(), offset);
	}
	file << 0xFFFF_u16;
	return file.WriteFile(prefix / path);
}

bool ScriptTable::WriteItemTable(const std::filesystem::path& prefix, const std::filesystem::path& path, const std::string& description, std::shared_ptr<std::vector<ScriptTable::Item>> table)
{
	AsmFile file;
	file.WriteFileHeader(path, description);
	for (const auto& item : *table)
	{
		std::size_t offset = 2;
		uint8_t len = static_cast<uint8_t>(4 + item.actions.size() * 2 + (item.other ? 2 : 0));
		file << item.item << len << item.shop;
		WriteScriptBody(file, item.actions.begin(), item.actions.end(), offset);
		if (item.other)
		{
			file << *item.other;
		}
	}
	file << 0xFFFF_u16;
	return file.WriteFile(prefix / path);
}

static std::ostringstream& ActionsToYaml(std::ostringstream& ss, const std::vector<ScriptTable::Action>& actions, std::size_t indent = 1)
{
	for (const auto& action : actions)
	{
		std::visit([&](const auto& arg)
			{
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, uint16_t>)
				{
					ss << std::setw(indent * 2) << "- " << "ScriptID:   " << Hex(arg) << std::endl;
				}
				else if constexpr (std::is_same_v<T, std::string>)
				{
					ss << std::setw(indent * 2) << "- " << "ScriptJump: " << arg << std::endl;
				}
			}, action);
	}
	return ss;
}

std::string ScriptTable::TableToYaml(std::shared_ptr<std::vector<Action>> table)
{
	std::ostringstream ss;
	ActionsToYaml(ss, *table);
	return ss.str();
}

std::string ScriptTable::TableToYaml(std::shared_ptr<std::vector<ScriptTable::Shop>> table)
{
	std::ostringstream ss;
	for (const auto& shop : *table)
	{
		ss << "- Shop:                   " << Hex(shop.room) << std::endl;
		ss << "  ItemMarkupPercent:      " << (shop.markup * 6.25 - 100.0) << std::endl;
		ss << "  LifestockMarkupPercent: " << (shop.lifestock_markup * 6.25 - 100.0) << std::endl;
		ss << "  Script: " << std::endl;
		ActionsToYaml(ss, std::vector<ScriptTable::Action>(shop.actions.cbegin(), shop.actions.cend()), 2) << std::endl;
	}
	return ss.str();
}

std::string ScriptTable::TableToYaml(std::shared_ptr<std::vector<ScriptTable::Item>> table)
{
	std::ostringstream ss;
	for (const auto& item : *table)
	{
		ss << "- Item:      " << Hex(item.item) << std::endl;
		if (item.shop != 0xFFFF)
		{
			ss << "  Shop:      " << Hex(item.shop) << std::endl;
		}
		if (item.other.has_value())
		{
			ss << "  ExtraData: " << Hex(*item.other) << std::endl;
		}
		ss << "  Script:    " << std::endl;
		ActionsToYaml(ss, item.actions, 2) << std::endl;
	}
	return ss.str();
}

static void ParseTable(const YAML::Node& node, std::vector<ScriptTable::Action>& actions, std::size_t& counter)
{
	try
	{
		if (node.IsSequence())
		{
			for (YAML::const_iterator it = node.begin(); it != node.end(); ++it)
			{
				if (it->IsMap() && it->size() >= 1 && it->begin()->first.IsScalar())
				{
					const std::string label = it->begin()->first.as<std::string>();
					if (label == "ScriptID")
					{
						actions.push_back(it->begin()->second.as<uint16_t>());
					}
					else if (label == "ScriptJump")
					{
						actions.push_back(it->begin()->second.as<std::string>());
					}
					else
					{
						throw std::runtime_error(StrPrintf("Bad element \"%s\" on line %d.", label.c_str(), counter));
					}
				}
				else
				{
					throw std::runtime_error(StrPrintf("Unexpected Type on line %d.", counter));
				}
			}
			counter++;
		}
		else
		{
			throw std::runtime_error("Expected Sequence.");
		}
	}
	catch (const std::exception& e)
	{
		throw std::runtime_error(StrPrintf("Malformed YAML: Element %d: %s", counter, e.what()));
	}
}

std::vector<ScriptTable::Action> ScriptTable::TableFromYaml(const std::string& yaml)
{
	std::vector<ScriptTable::Action> actions{};
	YAML::Node node = YAML::Load(yaml);
	std::size_t counter = 1;
	ParseTable(node, actions, counter);
	return actions;
}

std::vector<ScriptTable::Shop> ScriptTable::ShopTableFromYaml(const std::string& yaml)
{
	std::vector<Shop> shops{};
	std::size_t counter = 1;
	try
	{
		YAML::Node node = YAML::Load(yaml);
		if (node.IsSequence())
		{
			for (YAML::const_iterator it = node.begin(); it != node.end(); ++it)
			{
				if (it->IsMap() && it->size() >= 1 && it->begin()->first.IsScalar())
				{
					Shop shop;
					for (YAML::const_iterator it2 = it->begin(); it2 != it->end(); ++it2)
					{
						const std::string label = it2->first.as<std::string>();
						if (label == "Shop")
						{
							shop.room = it2->second.as<uint16_t>();
						}
						if (label == "ItemMarkupPercent")
						{
							shop.markup = static_cast<uint8_t>(it2->second.as<double>() * 0.16) + 16;
						}
						if (label == "LifestockMarkupPercent")
						{
							shop.lifestock_markup = static_cast<uint8_t>(it2->second.as<double>() * 0.16) + 16;
						}
						if (label == "Script")
						{
							std::vector<Action> actions;
							ParseTable(it2->second, actions, counter);
							if (actions.size() == shop.actions.size())
							{
								std::copy(actions.cbegin(), actions.cend(), shop.actions.begin());
							}
						}
						++counter;
					}
					shops.push_back(shop);
				}
			}
		}
	}
	catch (const std::exception& e)
	{
		throw std::runtime_error(StrPrintf("Malformed YAML: Element %d: %s", counter, e.what()));
	}
	return shops;
}

std::vector<ScriptTable::Item> ScriptTable::ItemTableFromYaml(const std::string& yaml)
{
	std::vector<ScriptTable::Item> items{};
	std::size_t counter = 1;
	try
	{
		YAML::Node node = YAML::Load(yaml);
		if (node.IsSequence())
		{
			for (YAML::const_iterator it = node.begin(); it != node.end(); ++it)
			{
				if (it->IsMap() && it->size() >= 1 && it->begin()->first.IsScalar())
				{
					Item item;
					for (YAML::const_iterator it2 = it->begin(); it2 != it->end(); ++it2)
					{
						const std::string label = it2->first.as<std::string>();
						if (label == "Item")
						{
							item.item = it2->second.as<uint8_t>();
						}
						if (label == "Shop")
						{
							item.shop = it2->second.as<uint16_t>();
						}
						if (label == "ExtraData")
						{
							item.other = it2->second.as<uint16_t>();
						}
						if (label == "Script")
						{
							ParseTable(it2->second, item.actions, counter);
						}
						++counter;
					}
					items.push_back(item);
				}
			}
		}
	}
	catch (const std::exception& e)
	{
		throw std::runtime_error(StrPrintf("Malformed YAML: Element %d: %s", counter, e.what()));
	}
	return items;
}
