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

const std::array<int, 2> ScriptPlayBgmEntry::BGMS{ 0x24, 0x0E };

std::unique_ptr<ScriptTableEntry> ScriptTableEntry::MakeEntry(const ScriptTableEntryType& ntype)
{
	switch (ntype)
	{
	case ScriptTableEntryType::STRING:
		return std::make_unique<ScriptStringEntry>(0_u16, false, false);
	case ScriptTableEntryType::GIVE_ITEM:
		return std::make_unique<ScriptGiveItemEntry>(false, false);
	case ScriptTableEntryType::GIVE_MONEY:
		return std::make_unique<ScriptGiveMoneyEntry>(false, false);
	case ScriptTableEntryType::GLOBAL_CHAR_LOAD:
		return std::make_unique<ScriptGlobalCharLoadEntry>(0_u8, 0_u8, false, false);
	case ScriptTableEntryType::ITEM_LOAD:
		return std::make_unique<ScriptItemLoadEntry>(0_u8, 0_u8, false, false);
	case ScriptTableEntryType::NUMBER_LOAD:
		return std::make_unique<ScriptNumLoadEntry>(0_u8, false, false);
	case ScriptTableEntryType::PLAY_BGM:
		return std::make_unique<ScriptPlayBgmEntry>(0_u8, false, false);
	case ScriptTableEntryType::PLAY_CUTSCENE:
		return std::make_unique<ScriptInitiateCutsceneEntry>(0_u16, false, false);
	case ScriptTableEntryType::SET_FLAG:
		return std::make_unique<ScriptSetFlagEntry>(0_u16, false, false);
	case ScriptTableEntryType::SET_GLOBAL_SPEAKER:
		return std::make_unique<ScriptSetGlobalSpeakerEntry>(0_u8, false, false);
	case ScriptTableEntryType::SET_SPEAKER:
		return std::make_unique<ScriptSetSpeakerEntry>(0_u16, false, false);
	case ScriptTableEntryType::INVALID:
	default:
		return std::make_unique<ScriptInvalidEntry>(0x17FF_u16, false, false);
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
	bool clear_box = (word & 0x4000) > 0;
	bool end = (word & 0x2000) > 0;
	if ((word & 0x8000) > 0)
	{
		return std::make_unique<ScriptStringEntry>(static_cast<uint16_t>(word & 0x1FFF), clear_box, end);
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
				return std::make_unique<ScriptItemLoadEntry>(static_cast<uint8_t>(param), cmd, clear_box, end);
			}
			else if (param >= 1000)
			{
				return std::make_unique<ScriptGlobalCharLoadEntry>(static_cast<uint8_t>(param - 1000), cmd, clear_box, end);
			}
			else
			{
				return std::make_unique<ScriptInvalidEntry>(word, clear_box, end);
			}
			break;
		case 4:
			return std::make_unique<ScriptNumLoadEntry>(param, clear_box, end);
		case 5:
			if (param < 1000)
			{
				return std::make_unique<ScriptSetFlagEntry>(param, clear_box, end);
			}
			else if (param == 1000)
			{
				return std::make_unique<ScriptGiveItemEntry>(clear_box, end);
			}
			else if (param == 1001)
			{
				return std::make_unique<ScriptGiveMoneyEntry>(clear_box, end);
			}
			else if (param >= 1002 && param < 1004)
			{
				return std::make_unique<ScriptPlayBgmEntry>(static_cast<uint8_t>(param - 1002), clear_box, end);
			}
			else
			{
				return std::make_unique<ScriptInvalidEntry>(word, clear_box, end);
			}
			break;
		case 6:
			if (param < 1000)
			{
				return std::make_unique<ScriptSetSpeakerEntry>(param, clear_box, end);
			}
			else if (param >= 1000)
			{
				return std::make_unique<ScriptSetGlobalSpeakerEntry>(static_cast<uint8_t>(param - 1000), clear_box, end);
			}
			else
			{
				return std::make_unique<ScriptInvalidEntry>(word, clear_box, end);
			}
			break;
		case 7:
			return std::make_unique<ScriptInitiateCutsceneEntry>(param, clear_box, end);
		default:
			return std::make_unique<ScriptInvalidEntry>(word, clear_box, end);
		}
	}
}

uint16_t ScriptStringEntry::ToBytes() const
{
	return 0x8000 | (clear_box ? 0x4000 : 0) | (end ? 0x2000 : 0) | (string & 0x1FFF);
}

std::wstring ScriptStringEntry::ToString(std::shared_ptr<const GameData> gd) const
{
	LSString::StringType formatted_string = StrWPrintf(L"Message %04d %ls%ls", string, clear_box ? L"[Clear] " : L"", end ? L"[End] " : L"");
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
		L"- %ls: % 4d%ls%ls%ls",
		GetWName().c_str(), string_idx, str_preview.c_str(), clear_box ? L"\n  Clear: true" : L"", end ? L"\n  End: true" : L""
	));
	
	if (end)
	{
		yaml += L"\n\n##############################\n";
	}
	
	return yaml;
}

uint16_t ScriptStringEntry::GetData() const
{
	return string & 0x1FFF;
}

void ScriptStringEntry::SetData(uint16_t data)
{
	string = data & 0x1FFF;
}

uint16_t ScriptItemLoadEntry::ToBytes() const
{
	return (clear_box ? 0x4000 : 0) | (end ? 0x2000 : 0) | (slot << 10) | item;
}

std::wstring ScriptItemLoadEntry::ToString(std::shared_ptr<const GameData> gd) const
{
	return StrWPrintf(L"Load item %02d (%ls) into Slot %01d%ls%ls", item, gd->GetStringData()->GetItemName(item).c_str(), slot + 1, clear_box ? L" [Clear]" : L"", end ? L" [End]" : L"");
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
		L"- %ls: % 2d%ls\n"
		L"  Slot: %d%ls%ls",
		GetWName().c_str(), item, str_preview.c_str(), slot + 1, clear_box ? L"\n  Clear: true" : L"", end ? L"\n  End: true" : L""
	));

	if (end)
	{
		yaml += L"\n\n##############################\n";
	}

	return yaml;
}

uint16_t ScriptItemLoadEntry::GetData() const
{
	return (slot << 10) | (item & 0x3F);
}

void ScriptItemLoadEntry::SetData(uint16_t data)
{
	slot = (data >> 10) & 0x03;
	item = data & 0x3F;
}

uint16_t ScriptGlobalCharLoadEntry::ToBytes() const
{
	return (clear_box ? 0x4000 : 0) | (end ? 0x2000 : 0) | (slot << 10) | (chr + 1000);
}

std::wstring ScriptGlobalCharLoadEntry::ToString(std::shared_ptr<const GameData> gd) const
{
	std::wstring char_name = gd->GetStringData()->GetDefaultCharName();
	if (chr < gd->GetStringData()->GetSpecialCharNameCount())
	{
		char_name = gd->GetStringData()->GetSpecialCharName(chr);
	}
	return StrWPrintf(L"Load global character %02d (%ls) into Slot %01d%ls%ls", chr, char_name.c_str(), slot, clear_box ? L" [Clear]" : L"", end ? L" [End]" : L"");
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
		L"- %ls: %3d%ls\n"
		L"  Slot: %d%ls%ls",
		GetWName().c_str(), chr, str_preview.c_str(), slot + 1, clear_box ? L"\n  Clear: true" : L"", end ? L"\n  End: true" : L""
	));

	if (end)
	{
		yaml += L"\n\n##############################\n";
	}

	return yaml;
}

uint16_t ScriptGlobalCharLoadEntry::GetData() const
{
	return (slot << 10) | std::clamp<uint16_t>(chr, 0_u16, 23_u16);
}

void ScriptGlobalCharLoadEntry::SetData(uint16_t data)
{
	slot = (data >> 10) & 0x03;
	chr = std::clamp<uint8_t>(static_cast<uint8_t>(data & 0x3FF), 0_u8, 23_u8);
}

uint16_t ScriptNumLoadEntry::ToBytes() const
{
	return (clear_box ? 0x4000 : 0) | (end ? 0x2000 : 0) | (4 << 10) | num;
}

std::wstring ScriptNumLoadEntry::ToString(std::shared_ptr<const GameData> /*gd*/) const
{
	return StrWPrintf(L"Load number % 5d into Slot 0 %ls%ls", num, clear_box ? L"[Clear] " : L"", end ? L"[End]" : L"");
}

std::wstring ScriptNumLoadEntry::ToYaml(std::shared_ptr<const GameData> /*gd*/) const
{
	std::wstring yaml(StrWPrintf(
		L"- %ls: % 3d%ls%ls",
		GetWName().c_str(), num, clear_box ? L"\n  Clear: true" : L"", end ? L"\n  End: true" : L""
	));

	if (end)
	{
		yaml += L"\n\n##############################\n";
	}

	return yaml;
}

uint16_t ScriptNumLoadEntry::GetData() const
{
	return std::clamp(num, 0_u16, 999_u16);
}

void ScriptNumLoadEntry::SetData(uint16_t data)
{
	num = std::clamp(data, 0_u16, 999_u16);
}

uint16_t ScriptSetFlagEntry::ToBytes() const
{
	return (clear_box ? 0x4000 : 0) | (end ? 0x2000 : 0) | (5 << 10) | flag;
}

std::wstring ScriptSetFlagEntry::ToString(std::shared_ptr<const GameData> /*gd*/) const
{
	return StrWPrintf(L"Set flag %03d%ls%ls", flag, clear_box ? L" [Clear]" : L"", end ? L" [End]" : L"");
}

std::wstring ScriptSetFlagEntry::ToYaml(std::shared_ptr<const GameData> /*gd*/) const
{
	std::wstring yaml(StrWPrintf(
		L"- %ls: % 3d%ls%ls",
		GetWName().c_str(), flag, clear_box ? L"\n  Clear: true" : L"", end ? L"\n  End: true" : L""
	));

	if (end)
	{
		yaml += L"\n\n##############################\n";
	}

	return yaml;
}

uint16_t ScriptSetFlagEntry::GetData() const
{
	return std::clamp(flag, 0_u16, 999_u16);
}

void ScriptSetFlagEntry::SetData(uint16_t data)
{
	flag = std::clamp(data, 0_u16, 999_u16);
}

uint16_t ScriptGiveItemEntry::ToBytes() const
{
	return (clear_box ? 0x4000 : 0) | (end ? 0x2000 : 0) | (5 << 10) | 1000;
}

std::wstring ScriptGiveItemEntry::ToString(std::shared_ptr<const GameData> /*gd*/) const
{
	return StrWPrintf(L"Give item in Slot 0 to player%ls%ls", clear_box ? L" [Clear]" : L"", end ? L" [End]" : L"");
}

std::wstring ScriptGiveItemEntry::ToYaml(std::shared_ptr<const GameData> /*gd*/) const
{
	std::wstring yaml(StrWPrintf(
		L"- %ls%ls%ls",
		GetWName().c_str(), clear_box ? L"\n  Clear: true" : L"", end ? L"\n  End: true" : L""
	));

	if (end)
	{
		yaml += L"\n\n##############################\n";
	}

	return yaml;
}

uint16_t ScriptGiveItemEntry::GetData() const
{
	return 0;
}

void ScriptGiveItemEntry::SetData(uint16_t /*data*/)
{
}

uint16_t ScriptGiveMoneyEntry::ToBytes() const
{
	return (clear_box ? 0x4000 : 0) | (end ? 0x2000 : 0) | (5 << 10) | 1001;
}

std::wstring ScriptGiveMoneyEntry::ToString(std::shared_ptr<const GameData> /*gd*/) const
{
	return StrWPrintf(L"Give money amount in Slot 0 to player%ls%ls", clear_box ? L" [Clear]" : L"", end ? L" [End]" : L"");
}

std::wstring ScriptGiveMoneyEntry::ToYaml(std::shared_ptr<const GameData> /*gd*/) const
{
	std::wstring yaml(StrWPrintf(
		L"- %ls%ls%ls",
		GetWName().c_str(), clear_box ? L"\n  Clear: true" : L"", end ? L"\n  End: true" : L""
	));

	if (end)
	{
		yaml += L"\n\n##############################\n";
	}

	return yaml;
}

uint16_t ScriptGiveMoneyEntry::GetData() const
{
	return 0;
}

void ScriptGiveMoneyEntry::SetData(uint16_t /*data*/)
{
}

uint16_t ScriptPlayBgmEntry::ToBytes() const
{
	return (clear_box ? 0x4000 : 0) | (end ? 0x2000 : 0) | (5 << 10) | (1002 + bgm);
}

std::wstring ScriptPlayBgmEntry::ToString(std::shared_ptr<const GameData> /*gd*/) const
{
	return StrWPrintf(L"Play BGM track %01d (\"%d\")%ls%ls", bgm, BGMS[bgm % BGMS.size()], clear_box ? L" [Clear]" : L"", end ? L" [End]" : L"");
}

std::wstring ScriptPlayBgmEntry::ToYaml(std::shared_ptr<const GameData> /*gd*/) const
{
	std::wstring str_preview;
	str_preview = L"  # ";
	if (bgm < BGMS.size() && Labels::Get(Labels::C_SOUNDS, bgm))
	{
		str_preview += *Labels::Get(Labels::C_SOUNDS , BGMS.at(bgm));
	}
	else
	{
		str_preview += L"<INVALID>";
	}
	std::wstring yaml(StrWPrintf(
		L"- %ls: %1d%ls%ls%ls",
		GetWName().c_str(), bgm, str_preview.c_str(), clear_box ? L"\n  Clear: true" : L"", end ? L"\n  End: true" : L""
	));

	if (end)
	{
		yaml += L"\n\n##############################\n";
	}

	return yaml;
}

uint16_t ScriptPlayBgmEntry::GetData() const
{
	return std::clamp<uint16_t>(bgm, 0, static_cast<uint16_t>(BGMS.size()));
}

void ScriptPlayBgmEntry::SetData(uint16_t data)
{
	bgm = std::clamp<uint8_t>(static_cast<uint8_t>(data), 0, static_cast<uint8_t>(BGMS.size()));
}

uint16_t ScriptSetSpeakerEntry::ToBytes() const
{
	return (clear_box ? 0x4000 : 0) | (end ? 0x2000 : 0) | (6 << 10) | chr;
}

std::wstring ScriptSetSpeakerEntry::ToString(std::shared_ptr<const GameData> gd) const
{
	std::wstring char_name = gd->GetStringData()->GetDefaultCharName();
	if (chr < gd->GetStringData()->GetCharNameCount())
	{
		char_name = gd->GetStringData()->GetCharName(chr);
	}
	return StrWPrintf(L"Set speaker to regular character %03d (%ls)%ls%ls", chr, char_name.c_str(), clear_box ? L" [Clear]" : L"", end ? L" [End]" : L"");
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
		L"- %ls: %3d%ls%ls%ls",
		GetWName().c_str(), chr, str_preview.c_str(), clear_box ? L"\n  Clear: true" : L"", end ? L"\n  End: true" : L""
	));

	if (end)
	{
		yaml += L"\n\n##############################\n";
	}

	return yaml;
}

uint16_t ScriptSetSpeakerEntry::GetData() const
{
	return std::clamp<uint16_t>(chr, 0_u16, 999_u16);
}

void ScriptSetSpeakerEntry::SetData(uint16_t data)
{
	chr = std::clamp<uint16_t>(data, 0_u16, 999_u16);
}

uint16_t ScriptSetGlobalSpeakerEntry::ToBytes() const
{
	return (clear_box ? 0x4000 : 0) | (end ? 0x2000 : 0) | (6 << 10) | (chr + 1000);
}

std::wstring ScriptSetGlobalSpeakerEntry::ToString(std::shared_ptr<const GameData> gd) const
{
	std::wstring char_name = gd->GetStringData()->GetDefaultCharName();
	if (chr < gd->GetStringData()->GetSpecialCharNameCount())
	{
		char_name = gd->GetStringData()->GetSpecialCharName(chr);
	}
	return StrWPrintf(L"Set speaker to global character %03d (%ls)%ls%ls", chr, char_name.c_str(), clear_box ? L" [Clear]" : L"", end ? L" [End]" : L"");
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
		L"- %ls: % 3d%ls%ls%ls",
		GetWName().c_str(), chr, str_preview.c_str(), clear_box ? L"\n  Clear: true" : L"", end ? L"\n  End: true" : L""
	));

	if (end)
	{
		yaml += L"\n\n##############################\n";
	}

	return yaml;
}

uint16_t ScriptSetGlobalSpeakerEntry::GetData() const
{
	return std::clamp<uint16_t>(chr, 0_u16, 23_u16);
}

void ScriptSetGlobalSpeakerEntry::SetData(uint16_t data)
{
	chr = std::clamp<uint8_t>(static_cast<uint8_t>(data), 0_u8, 23_u8);
}

uint16_t ScriptInvalidEntry::ToBytes() const
{
	return (clear_box ? 0x4000 : 0) | (end ? 0x2000 : 0) | bits;
}

std::wstring ScriptInvalidEntry::ToString(std::shared_ptr<const GameData> /*gd*/) const
{
	return StrWPrintf(L"Invalid Entry %04X%ls%ls", bits, clear_box ? L" [Clear]" : L"", end ? L" [End]" : L"");
}

std::wstring ScriptInvalidEntry::ToYaml(std::shared_ptr<const GameData> /*gd*/) const
{
	std::wstring yaml(StrWPrintf(
		L"- %ls: 0x%04X%ls%ls",
		GetWName().c_str(), bits, clear_box ? L"\n  Clear: true" : L"", end ? L"\n  End: true" : L""
	));

	if (end)
	{
		yaml += L"\n\n##############################\n";
	}

	return yaml;
}

uint16_t ScriptInvalidEntry::GetData() const
{
	return bits & 0x9FFF;
}

void ScriptInvalidEntry::SetData(uint16_t data)
{
	bits = data & 0x9FFF;
}

uint16_t ScriptInitiateCutsceneEntry::ToBytes() const
{
	return (clear_box ? 0x4000 : 0) | (end ? 0x2000 : 0) | (7 << 10) | cutscene;
}

std::wstring ScriptInitiateCutsceneEntry::ToString(std::shared_ptr<const GameData> /*gd*/) const
{
	return StrWPrintf(L"Initiate cutscene %03d%ls%ls", cutscene, clear_box ? L" [Clear]" : L"", end ? L" [End]" : L"");
}

std::wstring ScriptInitiateCutsceneEntry::ToYaml(std::shared_ptr<const GameData> /*gd*/) const
{
	std::wstring yaml(StrWPrintf(
		L"- %ls: % 3d%ls%ls",
		GetWName().c_str(), cutscene, clear_box ? L"\n  Clear: true" : L"", end ? L"\n  End: true" : L""
	));

	if (end)
	{
		yaml += L"\n\n##############################\n";
	}

	return yaml;
}

uint16_t ScriptInitiateCutsceneEntry::GetData() const
{
	return cutscene & 0x3FF;
}

void ScriptInitiateCutsceneEntry::SetData(uint16_t data)
{
	cutscene = data & 0x3FF;
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

bool ScriptTableEntry::GetClear() const
{
	return clear_box;
}

void ScriptTableEntry::SetClear(bool p_clear)
{
	clear_box = p_clear;
}

bool ScriptTableEntry::GetEnd() const
{
	return end;
}

void ScriptTableEntry::SetEnd(bool p_end)
{
	end = p_end;
}
