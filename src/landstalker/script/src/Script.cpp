#include <landstalker/script/include/Script.h>
#include <landstalker/misc/include/Utils.h>
#include <landstalker/main/include/GameData.h>
#include <yaml-cpp/yaml.h>
#include <optional>

Script::Script()
{
}

Script::Script(const std::vector<uint8_t>& bytes)
{
	//m_table.reserve(bytes.size() >> 1);
	for (std::size_t i = 0; i < bytes.size(); i += 2)
	{
		uint16_t word = (bytes.at(i) << 8) | bytes.at(i + 1);
		m_table.push_back(std::move(ScriptTableEntry::FromBytes(word)));
	}
}

Script::Script(const Script& rhs)
{
	Copy(rhs);
}

bool Script::operator==(const Script& rhs) const
{
	if (this->m_table.size() != rhs.m_table.size())
	{
		return false;
	}
	for (std::size_t i = 0; i < this->m_table.size(); ++i)
	{
		if (this->m_table[i]->ToBytes() != rhs.m_table[i]->ToBytes())
		{
			return false;
		}
	}
	return true;
}

Script& Script::operator=(const Script& rhs)
{
	Copy(rhs);
	return *this;
}

bool Script::operator!=(const Script& rhs) const
{
	return !(*this == rhs);
}

std::vector<uint8_t> Script::ToBytes() const
{
	std::vector<uint8_t> bytes;
	bytes.reserve(m_table.size() << 1);
	for (const auto& e : m_table)
	{
		uint16_t word = e->ToBytes();
		bytes.push_back(word >> 8);
		bytes.push_back(word & 0xFF);
	}
	return bytes;
}

std::size_t Script::GetScriptLineCount() const
{
	return m_table.size();
}

const ScriptTableEntry& Script::GetScriptLine(std::size_t line) const
{
	return *m_table.at(line);
}

void Script::SetScriptLine(std::size_t line, std::unique_ptr<ScriptTableEntry> content)
{
	if (line < m_table.size())
	{
		m_table[line] = std::move(content);
	}
}

void Script::SetScriptLineClear(std::size_t line, bool clear)
{
	m_table[line]->clear_box = clear;
}

void Script::SetScriptLineEnd(std::size_t line, bool end)
{
	m_table[line]->end = end;
}

void Script::SetScriptLineData(std::size_t line, uint16_t data)
{
	m_table[line]->SetData(data);
}

bool Script::GetScriptLineClear(std::size_t line) const
{
	return m_table.at(line)->GetClear();
}

bool Script::GetScriptLineEnd(std::size_t line) const
{
	return m_table.at(line)->GetEnd();
}

uint16_t Script::GetScriptLineData(std::size_t line) const
{
	return m_table.at(line)->GetData();
}

void Script::AddScriptLineBefore(std::size_t line, std::unique_ptr<ScriptTableEntry> content)
{
	m_table.insert(m_table.cbegin() + line, std::move(content));
}

void Script::DeleteScriptLine(std::size_t line)
{
	m_table.erase(m_table.cbegin() + line);
}

void Script::SwapScriptLines(std::size_t line1, std::size_t line2)
{
	if (line1 < m_table.size() && line2 < m_table.size() && line1 != line2)
	{
		std::iter_swap(m_table.begin() + line1, m_table.begin() + line2);
	}
}

std::wstring Script::GetScriptString(std::size_t line, std::shared_ptr<GameData> gd) const
{
	return m_table.at(line)->ToString(gd);
}

std::wstring Script::GetScriptAtLine(std::size_t line, std::shared_ptr<GameData> gd) const
{
	std::wstring script;
	if (line >= m_table.size())
	{
		return StrWPrintf(L"Invalid Script ID: %d", line);
	}
	for (; line < m_table.size(); ++line)
	{
		if (!script.empty())
		{
			script += L"\n";
		}
		script += GetScriptString(line, gd);
		if (GetScriptLine(line).GetType() == ScriptTableEntryType::PLAY_CUTSCENE)
		{
			break;
		}
		if (GetScriptLine(line).GetType() == ScriptTableEntryType::STRING)
		{
			if (static_cast<const ScriptStringEntry&>(GetScriptLine(line)).end)
			{
				break;
			}
		}
	}
	return script;
}

std::wstring Script::GetScriptSummaryAtLine(std::size_t line, std::shared_ptr<GameData> gd) const
{
	std::wstring script;
	if (line >= m_table.size())
	{
		return StrWPrintf(L"Invalid Script ID: %d", line);
	}
	for (; line < m_table.size(); ++line)
	{
		if (!script.empty())
		{
			script += L", ";
		}
		script += GetScriptString(line, gd);
		if (script.size() > 120)
		{
			return script.substr(0, 117) + L"...";
		}
		if (GetScriptLine(line).GetType() == ScriptTableEntryType::PLAY_CUTSCENE)
		{
			break;
		}
		if (GetScriptLine(line).GetType() == ScriptTableEntryType::STRING)
		{
			if (static_cast<const ScriptStringEntry&>(GetScriptLine(line)).end)
			{
				break;
			}
		}
	}
	return script;
}

std::wstring Script::GetAllScriptStrings(std::shared_ptr<GameData> gd) const
{
	std::wstring lines;
	for (const auto& line : m_table)
	{
		lines += line->ToString(gd);
		lines += L"\n";
	}

	return lines;
}

std::wstring Script::ToYaml(std::shared_ptr<GameData> gd) const
{
	std::wstring lines;
	std::size_t counter = 0;
	for (const auto& line : m_table)
	{
		lines += StrWPrintf(L"# ID: % 4d\n", counter++);
		lines += line->ToYaml(gd);
		lines += L"\n";
	}

	return lines;
}

static std::unique_ptr<ScriptTableEntry> DecodeYamlEntry(std::shared_ptr<GameData> gd, const std::string& type, std::optional<YAML::const_iterator> it = std::nullopt)
{
	std::unique_ptr<ScriptTableEntry> entry = ScriptTableEntry::MakeEntry(type);
	if (entry)
	{
		if (it && (*it)->IsMap())
		{
			const auto& node = **it;
			switch (entry->GetType())
			{
			case ScriptTableEntryType::STRING:
				{
					auto& string_entry = dynamic_cast<ScriptStringEntry&>(*entry);
					string_entry.string = static_cast<uint16_t>(node[type.c_str()].as<uint16_t>() - gd->GetScriptData()->GetStringStart()) & 0x1FFF_u16;
				}
				break;
			case ScriptTableEntryType::ITEM_LOAD:
				{
					auto& item_entry = dynamic_cast<ScriptItemLoadEntry&>(*entry);
					item_entry.item = std::clamp<uint8_t>(node[type.c_str()].as<uint8_t>(), 0_u8, 0x3F_u8);
					if (node["Slot"] && node["Slot"].IsScalar())
					{
						item_entry.slot = std::clamp<uint8_t>(node["Slot"].as<uint8_t>() - 1, 0_u8, 3_u8);
					}
				}
				break;
			case ScriptTableEntryType::NUMBER_LOAD:
				{
					auto& num_entry = dynamic_cast<ScriptNumLoadEntry&>(*entry);
					num_entry.num = std::clamp<uint16_t>(node[type.c_str()].as<uint16_t>(), 0_u16, 999_u16);
				}
				break;
			case ScriptTableEntryType::GLOBAL_CHAR_LOAD:
				{
					auto& chr_entry = dynamic_cast<ScriptGlobalCharLoadEntry&>(*entry);
					chr_entry.chr = std::clamp<uint8_t>(node[type.c_str()].as<uint8_t>(), 0_u8, 23_u8);
					if (node["Slot"] && node["Slot"].IsScalar())
					{
						chr_entry.slot = std::clamp<uint8_t>(node["Slot"].as<uint8_t>() - 1, 0_u8, 3_u8);
					}
				}
				break;
			case ScriptTableEntryType::SET_FLAG:
				{
					auto& flag_entry = dynamic_cast<ScriptSetFlagEntry&>(*entry);
					flag_entry.flag = std::clamp<uint16_t>(node[type.c_str()].as<uint16_t>(), 0_u16, 999_u16);
				}
				break;
			case ScriptTableEntryType::SET_SPEAKER:
				{
					auto& chr_entry = dynamic_cast<ScriptSetSpeakerEntry&>(*entry);
					chr_entry.chr = std::clamp<uint16_t>(node[type.c_str()].as<uint16_t>(), 0_u16, 999_u16);
				}
				break;
			case ScriptTableEntryType::SET_GLOBAL_SPEAKER:
				{
					auto& chr_entry = dynamic_cast<ScriptSetGlobalSpeakerEntry&>(*entry);
					chr_entry.chr = std::clamp<uint8_t>(node[type.c_str()].as<uint8_t>(), 0_u8, 23_u8);
				}
				break;
			case ScriptTableEntryType::PLAY_CUTSCENE:
				{
					auto& cs_entry = dynamic_cast<ScriptInitiateCutsceneEntry&>(*entry);
					cs_entry.cutscene = std::clamp<uint16_t>(node[type.c_str()].as<uint16_t>(), 0_u16, 0x3FF_u16);
				}
				break;
			case ScriptTableEntryType::PLAY_BGM:
				{
					auto& bgm_entry = dynamic_cast<ScriptPlayBgmEntry&>(*entry);
					bgm_entry.bgm = std::clamp<uint8_t>(node[type.c_str()].as<uint8_t>(), 0_u8, static_cast<uint8_t>(ScriptPlayBgmEntry::BGMS.size()));
				}
				break;
			case ScriptTableEntryType::INVALID:
				{
					auto& unk_entry = dynamic_cast<ScriptInvalidEntry&>(*entry);
					unk_entry.bits = static_cast<uint8_t>(node[type.c_str()].as<uint8_t>());
				}
				break;
			case ScriptTableEntryType::GIVE_ITEM:
			case ScriptTableEntryType::GIVE_MONEY:
			default:
				break;
			}
			if (node["Clear"])
			{
				if (node["Clear"].IsScalar())
				{
					entry->clear_box = node["Clear"].as<bool>();
				}
				else
				{
					entry->clear_box = true;
				}
			}
			if (node["End"])
			{
				if (node["End"].IsScalar())
				{
					entry->end = node["End"].as<bool>();
				}
				else
				{
					entry->end = true;
				}
			}
		}
	}
	else
	{
		throw std::runtime_error(StrPrintf("%s: Unknown Command", type.c_str()));
	}
	return entry;
}

bool Script::FromYaml(std::shared_ptr<GameData> gd, const std::string& yaml)
{
	YAML::Node node = YAML::Load(yaml);
	std::vector<std::unique_ptr<ScriptTableEntry>> entries;
	std::size_t counter = 0;
	try
	{
		if (node.IsSequence())
		{
			for (YAML::const_iterator it = node.begin(); it != node.end(); ++it)
			{
				if (it->IsMap() && it->size() >= 1 && it->begin()->first.IsScalar())
				{
					entries.push_back(DecodeYamlEntry(gd, it->begin()->first.as<std::string>(), it));
				}
				else if (it->IsScalar())
				{
					entries.push_back(DecodeYamlEntry(gd, it->as<std::string>()));
				}
				else
				{
					throw std::runtime_error(StrPrintf("Unexpected Type", counter));
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

	m_table.clear();
	m_table.reserve(entries.size());
	for (const auto& e : entries)
	{
		m_table.push_back(std::move(ScriptTableEntry::FromBytes(e->ToBytes())));
	}
	
	return true;
}

void Script::Copy(const Script& rhs)
{
	m_table.clear();
	m_table.reserve(rhs.m_table.size());
	for (const auto& e : rhs.m_table)
	{
		m_table.push_back(std::move(ScriptTableEntry::FromBytes(e->ToBytes())));
	}
}
