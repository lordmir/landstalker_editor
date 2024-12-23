#ifndef _PROGRESS_FLAGS_H_
#define _PROGRESS_FLAGS_H_

#include <landstalker/script/include/ScriptFunction.h>

class ProgressFlags
{
public:
	using Flags = std::map<uint8_t, std::vector<uint16_t>>;

	static Flags GetFlags(ScriptFunctionTable& table);
	static ScriptFunctionTable MakeAsm(const Flags& flags);

	static std::string ToYaml(ScriptFunctionTable& table);
	static ScriptFunctionTable FromYaml(const std::string& flags);
};

#endif // _PROGRESS_FLAGS_H_
