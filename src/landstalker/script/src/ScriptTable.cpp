#include <landstalker/script/include/ScriptTable.h>
#include <landstalker/main/include/AsmFile.h>
#include <landstalker/misc/include/Literals.h>
#include <landstalker/misc/include/Utils.h>

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
