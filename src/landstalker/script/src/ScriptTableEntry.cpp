#include <landstalker/script/include/ScriptTableEntry.h>
#include <landstalker/main/include/GameData.h>

#include <codecvt>

std::unique_ptr<ScriptTableEntry> ScriptTableEntry::FromBytes(uint16_t word)
{
	if ((word & 0x8000) > 0)
	{
		return std::make_unique<ScriptStringEntry>(static_cast<uint16_t>(word & 0x7FF), (word & 0x4000) > 0, (word & 0x2000) > 0);
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
	return 0x8000 | (clear_box ? 0x4000 : 0) | (end ? 0x2000 : 0) | string & 0x7FF;
}

std::wstring ScriptStringEntry::ToString(std::shared_ptr<const GameData> gd) const
{
	LSString::StringType formatted_string = StrWPrintf(L"Message %04d %s %s", string, clear_box ? L"[Clear]" : L"", end ? L"[End]" : L"");
	if (gd)
	{
		formatted_string += std::wstring(L": \"");
		formatted_string += gd->GetStringData()->GetString(StringData::Type::MAIN, gd->GetScriptData()->GetStringStart() + string).c_str();
		formatted_string += L"\"";
	}
	return formatted_string;
}

uint16_t ScriptItemLoadEntry::ToBytes() const
{
	return (slot << 10) | item;
}

std::wstring ScriptItemLoadEntry::ToString(std::shared_ptr<const GameData> gd) const
{
	return StrWPrintf(L"Load item %02d (%s) into Slot %01d", item, gd->GetStringData()->GetItemName(item).c_str(), slot);
}

uint16_t ScriptGlobalCharLoadEntry::ToBytes() const
{
	return (slot << 10) | (chr + 1000);
}

std::wstring ScriptGlobalCharLoadEntry::ToString(std::shared_ptr<const GameData> gd) const
{
	return StrWPrintf(L"Load global character %02d (%s) into Slot %01d", chr, gd->GetStringData()->GetSpecialCharName(chr).c_str(), slot);
}

uint16_t ScriptNumLoadEntry::ToBytes() const
{
	return (4 << 10) | num;
}

std::wstring ScriptNumLoadEntry::ToString(std::shared_ptr<const GameData> gd) const
{
	return StrWPrintf(L"Load number % 5d into Slot 0", num);
}

uint16_t ScriptSetFlagEntry::ToBytes() const
{
	return (5 << 10) | flag;
}

std::wstring ScriptSetFlagEntry::ToString(std::shared_ptr<const GameData> gd) const
{
	return StrWPrintf(L"Set flag %03d", flag);;
}

uint16_t ScriptGiveItemEntry::ToBytes() const
{
	return (5 << 10) | 1000;
}

std::wstring ScriptGiveItemEntry::ToString(std::shared_ptr<const GameData> gd) const
{
	return L"Give item in Slot 0 to player";
}

uint16_t ScriptGiveMoneyEntry::ToBytes() const
{
	return (5 << 10) | 1001;
}

std::wstring ScriptGiveMoneyEntry::ToString(std::shared_ptr<const GameData> gd) const
{
	return L"Give money amount in Slot 0 to player";
}

uint16_t ScriptPlayBgmEntry::ToBytes() const
{
	return (5 << 10) | 1002 + bgm;
}

std::wstring ScriptPlayBgmEntry::ToString(std::shared_ptr<const GameData> gd) const
{
	static const std::array<std::wstring, 2> BGMS{ L"Heh Heh, I Think I Will Disrupt This Good Cheer", L"Black Market" };
	return StrWPrintf(L"Play BGM track %01d (\"%s\").", bgm, BGMS[bgm % BGMS.size()].c_str());
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

uint16_t ScriptInvalidEntry::ToBytes() const
{
	return bits;
}

std::wstring ScriptInvalidEntry::ToString(std::shared_ptr<const GameData> gd) const
{
	return StrWPrintf(L"Invalid Entry %04X", bits);
}

uint16_t ScriptInitiateCutsceneEntry::ToBytes() const
{
	return (7 << 10) | cutscene;
}

std::wstring ScriptInitiateCutsceneEntry::ToString(std::shared_ptr<const GameData> gd) const
{
	return StrWPrintf(L"Initiate cutscene %03d", cutscene);
}
