#ifndef _SCRIPT_H_
#define _SCRIPT_H_

#include <vector>
#include <string>
#include <cstdint>

#include <landstalker/script/include/ScriptTableEntry.h>

class Script
{
public:
	Script();
	Script(const std::vector<uint8_t>& bytes);
	Script(const Script&);

	Script& operator=(const Script&);
	bool operator==(const Script& rhs) const;
	bool operator!=(const Script& rhs) const;

	std::vector<uint8_t> ToBytes() const;

	std::size_t GetScriptLineCount() const;
	const ScriptTableEntry& GetScriptLine(std::size_t line) const;
	void SetScriptLine(std::size_t line, std::unique_ptr<ScriptTableEntry> content);
	void SetScriptLineClear(std::size_t line, bool clear);
	void SetScriptLineEnd(std::size_t line, bool end);
	void SetScriptLineData(std::size_t line, uint16_t data);
	bool GetScriptLineClear(std::size_t line) const;
	bool GetScriptLineEnd(std::size_t line) const;
	uint16_t GetScriptLineData(std::size_t line) const;
	void AddScriptLineBefore(std::size_t line, std::unique_ptr<ScriptTableEntry> content);
	void DeleteScriptLine(std::size_t line);
	void SwapScriptLines(std::size_t line1, std::size_t line2);

	std::wstring GetScriptString(std::size_t line, std::shared_ptr<GameData> gd) const;
	std::wstring GetScriptSummaryAtLine(std::size_t line, std::shared_ptr<GameData> gd) const;
	std::wstring GetScriptAtLine(std::size_t line, std::shared_ptr<GameData> gd) const;
	std::wstring GetAllScriptStrings(std::shared_ptr<GameData> gd) const;

	std::wstring ToYaml(std::shared_ptr<GameData> gd) const;
	bool FromYaml(std::shared_ptr<GameData> gd, const std::string& yaml);

private:
	void Copy(const Script& rhs);
	std::vector<std::unique_ptr<ScriptTableEntry>> m_table;
};

#endif // _SCRIPT_H_
