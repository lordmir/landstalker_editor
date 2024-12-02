#include <landstalker/script/include/ScriptTableEntry.h>
#include <landstalker/main/include/GameData.h>

#include <codecvt>

const std::unordered_map<ScriptTableEntryType, std::string> ScriptTableEntry::ENTRY_NAMES{
	{ScriptTableEntryType::STRING, "String"},
	{ScriptTableEntryType::GIVE_ITEM, "GiveItem"},
	{ScriptTableEntryType::GIVE_MONEY, "GiveMoney"},
	{ScriptTableEntryType::GLOBAL_CHAR_LOAD, "LoadGlobalChar"},
	{ScriptTableEntryType::ITEM_LOAD, "LoadItem"},
	{ScriptTableEntryType::NUMBER_LOAD, "LoadNumber"},
	{ScriptTableEntryType::PLAY_BGM, "PlayBGM"},
	{ScriptTableEntryType::PLAY_CUTSCENE, "Cutscene"},
	{ScriptTableEntryType::SET_FLAG, "SetFlag"},
	{ScriptTableEntryType::SET_GLOBAL_SPEAKER, "SetGlobalSpeaker"},
	{ScriptTableEntryType::SET_SPEAKER, "SetSpeaker"},
	{ScriptTableEntryType::INVALID, "Unknown"}
};

const std::array<std::wstring, 2> ScriptPlayBgmEntry::BGMS{ L"Heh Heh, I Think I Will Disrupt This Good Cheer", L"Black Market" };

std::unique_ptr<ScriptTableEntry> ScriptTableEntry::MakeEntry(const ScriptTableEntryType& ntype)
{
	switch (ntype)
	{
	case ScriptTableEntryType::STRING:
		return std::make_unique<ScriptStringEntry>(0_u16, false, false);
	case ScriptTableEntryType::GIVE_ITEM:
		return std::make_unique<ScriptGiveItemEntry>();
	case ScriptTableEntryType::GIVE_MONEY:
		return std::make_unique<ScriptGiveMoneyEntry>();
	case ScriptTableEntryType::GLOBAL_CHAR_LOAD:
		return std::make_unique<ScriptGlobalCharLoadEntry>(0_u8, 0_u8);
	case ScriptTableEntryType::ITEM_LOAD:
		return std::make_unique<ScriptItemLoadEntry>(0_u8, 0_u8);
	case ScriptTableEntryType::NUMBER_LOAD:
		return std::make_unique<ScriptNumLoadEntry>(0_u8);
	case ScriptTableEntryType::PLAY_BGM:
		return std::make_unique<ScriptPlayBgmEntry>(0_u8);
	case ScriptTableEntryType::PLAY_CUTSCENE:
		return std::make_unique<ScriptInitiateCutsceneEntry>(0_u16);
	case ScriptTableEntryType::SET_FLAG:
		return std::make_unique<ScriptSetFlagEntry>(0_u16);
	case ScriptTableEntryType::SET_GLOBAL_SPEAKER:
		return std::make_unique<ScriptSetGlobalSpeakerEntry>(0_u8);
	case ScriptTableEntryType::SET_SPEAKER:
		return std::make_unique<ScriptSetSpeakerEntry>(0_u16);
	case ScriptTableEntryType::INVALID:
	default:
		return std::make_unique<ScriptInvalidEntry>(0x77FF_u16);
	}
}

std::unique_ptr<ScriptTableEntry> ScriptTableEntry::MakeEntry(const std::string& ntype)
{
	auto result = FindMapKey(ENTRY_NAMES, ntype);
	if (result != ENTRY_NAMES.cend())
	{
		return MakeEntry(FindMapKey(ENTRY_NAMES, ntype)->first);
	}
	else
	{
		return nullptr;
	}
}

std::unique_ptr<ScriptTableEntry> ScriptTableEntry::FromBytes(uint16_t word)
{
	if ((word & 0x8000) > 0)
	{
		return std::make_unique<ScriptStringEntry>(static_cast<uint16_t>(word & 0x1FFF), (word & 0x4000) > 0, (word & 0x2000) > 0);
	}
	else
	{
		uint8_t cmd = (word & 0x1C00) >> 10;
		uint16_t param = word & 0x3FF;
		switch (cmd)
		{
		case 0:
		case 1:
		case 2:
		case 3:
			if (param < 64)
			{
				return std::make_unique<ScriptItemLoadEntry>(static_cast<uint8_t>(param), cmd);
			}
			else if (param >= 1000)
			{
				return std::make_unique<ScriptGlobalCharLoadEntry>(static_cast<uint8_t>(param - 1000), cmd);
			}
			else
			{
				return std::make_unique<ScriptInvalidEntry>(word);
			}
			break;
		case 4:
			return std::make_unique<ScriptNumLoadEntry>(param);
		case 5:
			if (param < 1000)
			{
				return std::make_unique<ScriptSetFlagEntry>(param);
			}
			else if (param == 1000)
			{
				return std::make_unique<ScriptGiveItemEntry>();
			}
			else if (param == 1001)
			{
				return std::make_unique<ScriptGiveMoneyEntry>();
			}
			else if (param >= 1002 && param < 1004)
			{
				return std::make_unique<ScriptPlayBgmEntry>(static_cast<uint8_t>(param - 1002));
			}
			else
			{
				return std::make_unique<ScriptInvalidEntry>(word);
			}
			break;
		case 6:
			if (param < 1000)
			{
				return std::make_unique<ScriptSetSpeakerEntry>(param);
			}
			else if (param >= 1000)
			{
				return std::make_unique<ScriptSetGlobalSpeakerEntry>(static_cast<uint8_t>(param - 1000));
			}
			else
			{
				return std::make_unique<ScriptInvalidEntry>(word);
			}
			break;
		case 7:
			return std::make_unique<ScriptInitiateCutsceneEntry>(param);
		default:
			return std::make_unique<ScriptInvalidEntry>(word);
		}
	}
}

uint16_t ScriptStringEntry::ToBytes() const
{
	return 0x8000 | (clear_box ? 0x4000 : 0) | (end ? 0x2000 : 0) | (string & 0x1FFF);
}

std::wstring ScriptStringEntry::ToString(std::shared_ptr<const GameData> gd) const
{
	LSString::StringType formatted_string = StrWPrintf(L"Message %04d %s %s", string, clear_box ? L"[Clear]" : L"       ", end ? L"[End]" : L"     ");
	if (gd)
	{
		std::size_t string_idx = gd->GetScriptData()->GetStringStart() + string;
		formatted_string += std::wstring(L": \"");
		if (string_idx < gd->GetStringData()->GetStringCount(StringData::Type::MAIN))
		{
			formatted_string += gd->GetStringData()->GetString(StringData::Type::MAIN, string_idx).c_str();
		}
		else
		{
			formatted_string += L"<INVALID>";
		}
		formatted_string += L"\"";
	}
	return formatted_string;
}

std::wstring ScriptStringEntry::ToYaml(std::shared_ptr<const GameData> gd) const
{
	std::wstring str_preview;
	std::size_t string_idx = string;
	if (gd)
	{
		str_preview = L"  # ";
		string_idx += gd->GetScriptData()->GetStringStart();
		if (string_idx < gd->GetStringData()->GetStringCount(StringData::Type::MAIN))
		{
			str_preview += gd->GetStringData()->GetString(StringData::Type::MAIN, string_idx).c_str();
		}
		else
		{
			str_preview += L"<INVALID>";
		}
	}
	std::wstring yaml(StrWPrintf(
		L"- %s: % 4d%s%s%s",
		GetWName().c_str(), string_idx, str_preview.c_str(), clear_box ? L"\n  Clear: true" : L"", end ? L"\n  End: true" : L""
	));
	
	if (end)
	{
		yaml += L"\n\n##############################\n";
	}
	
	return yaml;
}

uint16_t ScriptItemLoadEntry::ToBytes() const
{
	return (slot << 10) | item;
}

std::wstring ScriptItemLoadEntry::ToString(std::shared_ptr<const GameData> gd) const
{
	return StrWPrintf(L"Load item %02d (%s) into Slot %01d", item, gd->GetStringData()->GetItemName(item).c_str(), slot + 1);
}

std::wstring ScriptItemLoadEntry::ToYaml(std::shared_ptr<const GameData> gd) const
{
	std::wstring str_preview;
	if (gd)
	{
		str_preview = L"  # ";
		if (item < gd->GetStringData()->GetStringCount(StringData::Type::ITEM_NAMES))
		{
			str_preview += gd->GetStringData()->GetString(StringData::Type::ITEM_NAMES, item).c_str();
		}
		else
		{
			str_preview += L"<INVALID>";
		}
	}
	std::wstring yaml(StrWPrintf(
		L"- %s: % 2d%s\n"
		L"  Slot: %d",
		GetWName().c_str(), item, str_preview.c_str(), slot + 1
	));

	return yaml;
}

uint16_t ScriptGlobalCharLoadEntry::ToBytes() const
{
	return (slot << 10) | (chr + 1000);
}

std::wstring ScriptGlobalCharLoadEntry::ToString(std::shared_ptr<const GameData> gd) const
{
	std::wstring char_name = gd->GetStringData()->GetDefaultCharName();
	if (chr < gd->GetStringData()->GetSpecialCharNameCount())
	{
		char_name = gd->GetStringData()->GetSpecialCharName(chr);
	}
	return StrWPrintf(L"Load global character %02d (%s) into Slot %01d", chr, char_name.c_str(), slot);
}

std::wstring ScriptGlobalCharLoadEntry::ToYaml(std::shared_ptr<const GameData> gd) const
{
	std::wstring str_preview;
	if (gd)
	{
		str_preview = L"  # ";
		if (chr < gd->GetStringData()->GetStringCount(StringData::Type::SPECIAL_NAMES))
		{
			str_preview += gd->GetStringData()->GetString(StringData::Type::SPECIAL_NAMES, chr).c_str();
		}
		else
		{
			str_preview += L"<INVALID>";
		}
	}
	std::wstring yaml(StrWPrintf(
		L"- %s: %3d%s\n"
		L"  Slot: %d",
		GetWName().c_str(), chr, str_preview.c_str(), slot + 1
	));

	return yaml;
}

uint16_t ScriptNumLoadEntry::ToBytes() const
{
	return (4 << 10) | num;
}

std::wstring ScriptNumLoadEntry::ToString(std::shared_ptr<const GameData> /*gd*/) const
{
	return StrWPrintf(L"Load number % 5d into Slot 0", num);
}

std::wstring ScriptNumLoadEntry::ToYaml(std::shared_ptr<const GameData> /*gd*/) const
{
	std::wstring yaml(StrWPrintf(
		L"- %s: % 3d",
		GetWName().c_str(), num
	));

	return yaml;
}

uint16_t ScriptSetFlagEntry::ToBytes() const
{
	return (5 << 10) | flag;
}

std::wstring ScriptSetFlagEntry::ToString(std::shared_ptr<const GameData> /*gd*/) const
{
	return StrWPrintf(L"Set flag %03d", flag);;
}

std::wstring ScriptSetFlagEntry::ToYaml(std::shared_ptr<const GameData> /*gd*/) const
{
	std::wstring yaml(StrWPrintf(
		L"- %s: % 3d",
		GetWName().c_str(), flag
	));

	return yaml;
}

uint16_t ScriptGiveItemEntry::ToBytes() const
{
	return (5 << 10) | 1000;
}

std::wstring ScriptGiveItemEntry::ToString(std::shared_ptr<const GameData> /*gd*/) const
{
	return L"Give item in Slot 0 to player";
}

std::wstring ScriptGiveItemEntry::ToYaml(std::shared_ptr<const GameData> /*gd*/) const
{
	std::wstring yaml(StrWPrintf(
		L"- %s",
		GetWName().c_str()
	));

	return yaml;
}

uint16_t ScriptGiveMoneyEntry::ToBytes() const
{
	return (5 << 10) | 1001;
}

std::wstring ScriptGiveMoneyEntry::ToString(std::shared_ptr<const GameData> /*gd*/) const
{
	return L"Give money amount in Slot 0 to player";
}

std::wstring ScriptGiveMoneyEntry::ToYaml(std::shared_ptr<const GameData> /*gd*/) const
{
	std::wstring yaml(StrWPrintf(
		L"- %s",
		GetWName().c_str()
	));

	return yaml;
}

uint16_t ScriptPlayBgmEntry::ToBytes() const
{
	return (5 << 10) | (1002 + bgm);
}

std::wstring ScriptPlayBgmEntry::ToString(std::shared_ptr<const GameData> /*gd*/) const
{
	return StrWPrintf(L"Play BGM track %01d (\"%s\").", bgm, BGMS[bgm % BGMS.size()].c_str());
}

std::wstring ScriptPlayBgmEntry::ToYaml(std::shared_ptr<const GameData> /*gd*/) const
{
	std::wstring str_preview;
	str_preview = L"  # ";
	if (bgm < BGMS.size())
	{
		str_preview += BGMS.at(bgm);
	}
	else
	{
		str_preview += L"<INVALID>";
	}
	std::wstring yaml(StrWPrintf(
		L"- %s: %1d%s",
		GetWName().c_str(), bgm, str_preview.c_str()
	));

	return yaml;
}

uint16_t ScriptSetSpeakerEntry::ToBytes() const
{
	return (6 << 10) | chr;
}

std::wstring ScriptSetSpeakerEntry::ToString(std::shared_ptr<const GameData> gd) const
{
	std::wstring char_name = gd->GetStringData()->GetDefaultCharName();
	if (chr < gd->GetStringData()->GetCharNameCount())
	{
		char_name = gd->GetStringData()->GetCharName(chr);
	}
	return StrWPrintf(L"Set talking character to %03d (%s)", chr, char_name.c_str());
}

std::wstring ScriptSetSpeakerEntry::ToYaml(std::shared_ptr<const GameData> gd) const
{
	std::wstring str_preview;
	if (gd)
	{
		str_preview = L"  # ";
		if (chr < gd->GetStringData()->GetStringCount(StringData::Type::NAMES))
		{
			str_preview += gd->GetStringData()->GetString(StringData::Type::NAMES, chr).c_str();
		}
		else
		{
			str_preview += gd->GetStringData()->GetString(StringData::Type::DEFAULT_NAME, 0).c_str();
		}
	}
	std::wstring yaml(StrWPrintf(
		L"- %s: %3d%s",
		GetWName().c_str(), chr, str_preview.c_str()
	));

	return yaml;
}

uint16_t ScriptSetGlobalSpeakerEntry::ToBytes() const
{
	return (6 << 10) | (chr + 1000);
}

std::wstring ScriptSetGlobalSpeakerEntry::ToString(std::shared_ptr<const GameData> gd) const
{
	std::wstring char_name = gd->GetStringData()->GetDefaultCharName();
	if (chr < gd->GetStringData()->GetSpecialCharNameCount())
	{
		char_name = gd->GetStringData()->GetSpecialCharName(chr);
	}
	return StrWPrintf(L"Set talking character to global character %03d (%s)", chr, char_name.c_str());
}

std::wstring ScriptSetGlobalSpeakerEntry::ToYaml(std::shared_ptr<const GameData> gd) const
{
	std::wstring str_preview;
	if (gd)
	{
		str_preview = L"  # ";
		if (chr < gd->GetStringData()->GetStringCount(StringData::Type::SPECIAL_NAMES))
		{
			str_preview += gd->GetStringData()->GetString(StringData::Type::SPECIAL_NAMES, chr).c_str();
		}
		else
		{
			str_preview += gd->GetStringData()->GetString(StringData::Type::DEFAULT_NAME, 0).c_str();
		}
	}
	std::wstring yaml(StrWPrintf(
		L"- %s: % 3d%s",
		GetWName().c_str(), chr, str_preview.c_str()
	));

	return yaml;
}

uint16_t ScriptInvalidEntry::ToBytes() const
{
	return bits;
}

std::wstring ScriptInvalidEntry::ToString(std::shared_ptr<const GameData> /*gd*/) const
{
	return StrWPrintf(L"Invalid Entry %04X", bits);
}

std::wstring ScriptInvalidEntry::ToYaml(std::shared_ptr<const GameData> /*gd*/) const
{
	std::wstring yaml(StrWPrintf(
		L"- %s: 0x%04X",
		GetWName().c_str(), bits
	));

	return yaml;
}

uint16_t ScriptInitiateCutsceneEntry::ToBytes() const
{
	return (7 << 10) | cutscene;
}

std::wstring ScriptInitiateCutsceneEntry::ToString(std::shared_ptr<const GameData> /*gd*/) const
{
	return StrWPrintf(L"Initiate cutscene %03d", cutscene);
}

std::wstring ScriptInitiateCutsceneEntry::ToYaml(std::shared_ptr<const GameData> /*gd*/) const
{
	std::wstring yaml(StrWPrintf(
		L"- %s: % 3d\n\n"
		L"##############################\n",
		GetWName().c_str(), cutscene
	));

	return yaml;
}

std::string ScriptTableEntry::GetName() const
{
	if (ENTRY_NAMES.find(GetType()) != ENTRY_NAMES.cend())
	{
		return ENTRY_NAMES.at(GetType());
	}
	return ENTRY_NAMES.at(ScriptTableEntryType::INVALID);
}

std::wstring ScriptTableEntry::GetWName() const
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> cvt;
	return cvt.from_bytes(GetName());
}
