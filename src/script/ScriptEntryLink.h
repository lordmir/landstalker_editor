#ifndef _SCRIPT_ENTRY_LINK_H_
#define _SCRIPT_ENTRY_LINK_H_

#include <memory>
#include <optional>

#include <landstalker/main/GameData.h>
#include <landstalker/main/ScriptData.h>
#include <landstalker/script/AsmFunctionTable.h>
#include <landstalker/script/ScriptTableEntry.h>

// Shared resolution of "does this script table entry reference another entry's own script
// tree?" - used by the Script Function Editor's preview rows and the Main Script Editor's rows
// alike, both to style cutscene / (non-global) character references as hyperlinks and to open
// the right popup when one is clicked, so styling and click behaviour can never diverge.
namespace ScriptEntryLink
{
struct Target
{
	bool is_cutscene; // otherwise a (non-global) character
	int entry;        // index into the cutscene/character script table
};

inline std::optional<Target> Resolve(const std::shared_ptr<Landstalker::GameData>& gd,
	const Landstalker::ScriptTableEntry& entry)
{
	if (!gd || !gd->GetScriptData())
	{
		return std::nullopt;
	}
	if (entry.GetType() == Landstalker::ScriptTableEntryType::PLAY_CUTSCENE)
	{
		// A cutscene index is a dialogueactions handler slot (layer 1) - the real target of the
		// index - so validate it against that table. Fall back to the script-VM cutscene table's
		// size when the asm handlers are unavailable (e.g. a ROM-loaded project) so links still
		// render there.
		const int cutscene = entry.GetData();
		const auto actions = gd->GetScriptData()->GetCutsceneActions();
		if (actions && actions->IsValid())
		{
			if (cutscene < static_cast<int>(actions->SlotCount()))
			{
				return Target{ true, cutscene };
			}
		}
		else
		{
			const auto table = gd->GetScriptData()->GetCutsceneTable();
			if (table && cutscene < static_cast<int>(table->size()))
			{
				return Target{ true, cutscene };
			}
		}
	}
	else if (entry.GetType() == Landstalker::ScriptTableEntryType::SET_SPEAKER)
	{
		// Non-global characters only - global characters (SET_GLOBAL_SPEAKER) have no entry in
		// the character script table to link to.
		const int chr = entry.GetData();
		const auto table = gd->GetScriptData()->GetCharTable();
		if (table && chr < static_cast<int>(table->size()))
		{
			return Target{ false, chr };
		}
	}
	return std::nullopt;
}
}

#endif // _SCRIPT_ENTRY_LINK_H_
