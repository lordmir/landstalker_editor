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

private:
	void Copy(const Script& rhs);
	std::vector<std::unique_ptr<ScriptTableEntry>> m_table;
};

#endif // _SCRIPT_H_
