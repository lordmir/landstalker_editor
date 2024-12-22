#ifndef _PROGRESS_FLAGS_H_
#define _PROGRESS_FLAGS_H_

#include <landstalker/script/include/ScriptFunction.h>

class ProgressFlags
{
public:
	using Flags = std::map<Statements::ProgressList::QuestProgress, uint16_t>;

	static Flags GetFlags(ScriptFunctionTable& table);
	static ScriptFunctionTable MakeAsm(const Flags& flags);
};

#endif // _PROGRESS_FLAGS_H_
