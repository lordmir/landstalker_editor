#ifndef _SCRIPT_TABLE_ENTRY_H_
#define _SCRIPT_TABLE_ENTRY_H_

#include <vector>
#include <memory>
#include <string>
#include <cstdint>
#include <unordered_map>

class GameData;

enum class ScriptTableEntryType
{
	INVALID,
	STRING,
	ITEM_LOAD,
	GLOBAL_CHAR_LOAD,
	NUMBER_LOAD,
	SET_FLAG,
	GIVE_ITEM,
	GIVE_MONEY,
	PLAY_BGM,
	SET_SPEAKER,
	SET_GLOBAL_SPEAKER,
	PLAY_CUTSCENE
};


class ScriptTableEntry
{
public:
	virtual ~ScriptTableEntry() = default;
	virtual uint16_t ToBytes() const = 0;
	virtual std::wstring ToString(std::shared_ptr<const GameData> gd) const = 0;
	virtual std::wstring ToYaml(std::shared_ptr<const GameData> gd) const = 0;
	ScriptTableEntryType GetType() const { return type; }
	std::string GetName() const;
	std::wstring GetWName() const;

	static std::unique_ptr<ScriptTableEntry> MakeEntry(const ScriptTableEntryType& ntype);
	static std::unique_ptr<ScriptTableEntry> MakeEntry(const std::string& ntype);
	static std::unique_ptr<ScriptTableEntry> FromBytes(uint16_t word);
protected:
	ScriptTableEntryType type = ScriptTableEntryType::INVALID;
private:
	static const std::unordered_map<ScriptTableEntryType, std::string> ENTRY_NAMES;
};

class ScriptStringEntry : public ScriptTableEntry
{
public:
	ScriptStringEntry(uint16_t p_string, bool p_clear_box, bool p_end) : string(p_string), clear_box(p_clear_box), end(p_end)
	{
		type = ScriptTableEntryType::STRING;
	}

	virtual uint16_t ToBytes() const override;
	virtual std::wstring ToString(std::shared_ptr<const GameData> gd) const override;
	virtual std::wstring ToYaml(std::shared_ptr<const GameData> gd) const override;

	uint16_t string;
	bool clear_box;
	bool end;
};

class ScriptItemLoadEntry : public ScriptTableEntry
{
public:
	ScriptItemLoadEntry(uint8_t p_item, uint8_t p_slot) : item(p_item), slot(p_slot)
	{
		type = ScriptTableEntryType::ITEM_LOAD;
	}

	virtual uint16_t ToBytes() const override;
	virtual std::wstring ToString(std::shared_ptr<const GameData> gd) const override;
	virtual std::wstring ToYaml(std::shared_ptr<const GameData> gd) const override;

	uint8_t item;
	uint8_t slot;
};

class ScriptGlobalCharLoadEntry : public ScriptTableEntry
{
public:
	ScriptGlobalCharLoadEntry(uint8_t p_chr, uint8_t p_slot) : chr(p_chr), slot(p_slot)
	{
		type = ScriptTableEntryType::GLOBAL_CHAR_LOAD;
	}

	virtual uint16_t ToBytes() const override;
	virtual std::wstring ToString(std::shared_ptr<const GameData> gd) const override;
	virtual std::wstring ToYaml(std::shared_ptr<const GameData> gd) const override;

	uint8_t chr;
	uint8_t slot;
};

class ScriptNumLoadEntry : public ScriptTableEntry
{
public:
	ScriptNumLoadEntry(uint16_t p_num) : num(p_num)
	{
		type = ScriptTableEntryType::NUMBER_LOAD;
	}

	virtual uint16_t ToBytes() const override;
	virtual std::wstring ToString(std::shared_ptr<const GameData> gd) const override;
	virtual std::wstring ToYaml(std::shared_ptr<const GameData> gd) const override;

	uint16_t num;
};

class ScriptSetFlagEntry : public ScriptTableEntry
{
public:
	ScriptSetFlagEntry(uint16_t p_flag) : flag(p_flag)
	{
		type = ScriptTableEntryType::SET_FLAG;
	}

	virtual uint16_t ToBytes() const override;
	virtual std::wstring ToString(std::shared_ptr<const GameData> gd) const override;
	virtual std::wstring ToYaml(std::shared_ptr<const GameData> gd) const override;

	uint16_t flag;
};

class ScriptGiveItemEntry : public ScriptTableEntry
{
public:
	ScriptGiveItemEntry()
	{
		type = ScriptTableEntryType::GIVE_ITEM;
	}

	virtual uint16_t ToBytes() const override;
	virtual std::wstring ToString(std::shared_ptr<const GameData> gd) const override;
	virtual std::wstring ToYaml(std::shared_ptr<const GameData> gd) const override;
};

class ScriptGiveMoneyEntry : public ScriptTableEntry
{
public:
	ScriptGiveMoneyEntry()
	{
		type = ScriptTableEntryType::GIVE_MONEY;
	}

	virtual uint16_t ToBytes() const override;
	virtual std::wstring ToString(std::shared_ptr<const GameData> gd) const override;
	virtual std::wstring ToYaml(std::shared_ptr<const GameData> gd) const override;
};

class ScriptPlayBgmEntry : public ScriptTableEntry
{
public:
	ScriptPlayBgmEntry(uint8_t p_bgm) : bgm(p_bgm)
	{
		type = ScriptTableEntryType::PLAY_BGM;
	}

	virtual uint16_t ToBytes() const override;
	virtual std::wstring ToString(std::shared_ptr<const GameData> gd) const override;
	virtual std::wstring ToYaml(std::shared_ptr<const GameData> gd) const override;

	uint8_t bgm;

	static const std::array<int, 2> BGMS;
};

class ScriptSetSpeakerEntry : public ScriptTableEntry
{
public:
	ScriptSetSpeakerEntry(uint16_t p_chr) : chr(p_chr)
	{
		type = ScriptTableEntryType::SET_SPEAKER;
	}

	virtual uint16_t ToBytes() const override;
	virtual std::wstring ToString(std::shared_ptr<const GameData> gd) const override;
	virtual std::wstring ToYaml(std::shared_ptr<const GameData> gd) const override;

	uint16_t chr;
};

class ScriptSetGlobalSpeakerEntry : public ScriptTableEntry
{
public:
	ScriptSetGlobalSpeakerEntry(uint8_t p_chr) : chr(p_chr)
	{
		type = ScriptTableEntryType::SET_GLOBAL_SPEAKER;
	}

	virtual uint16_t ToBytes() const override;
	virtual std::wstring ToString(std::shared_ptr<const GameData> gd) const override;
	virtual std::wstring ToYaml(std::shared_ptr<const GameData> gd) const override;

	uint8_t chr;
};

class ScriptInitiateCutsceneEntry : public ScriptTableEntry
{
public:
	ScriptInitiateCutsceneEntry(uint16_t p_cutscene) : cutscene(p_cutscene)
	{
		type = ScriptTableEntryType::PLAY_CUTSCENE;
	}

	virtual uint16_t ToBytes() const override;
	virtual std::wstring ToString(std::shared_ptr<const GameData> gd) const override;
	virtual std::wstring ToYaml(std::shared_ptr<const GameData> gd) const override;

	uint16_t cutscene;
};

class ScriptInvalidEntry : public ScriptTableEntry
{
public:
	ScriptInvalidEntry(uint16_t p_bits) : bits(p_bits)
	{
		type = ScriptTableEntryType::INVALID;
	}

	virtual uint16_t ToBytes() const override;
	virtual std::wstring ToString(std::shared_ptr<const GameData> gd) const override;
	virtual std::wstring ToYaml(std::shared_ptr<const GameData> gd) const override;

	uint16_t bits;
};

#endif // _SCRIPT_TABLE_ENTRY_H_
