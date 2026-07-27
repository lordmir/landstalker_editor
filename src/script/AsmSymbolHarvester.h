#ifndef _ASM_SYMBOL_HARVESTER_H_
#define _ASM_SYMBOL_HARVESTER_H_

#include <filesystem>
#include <map>
#include <string>
#include <vector>

// Harvests the symbol names defined across a disassembly's whole include tree, for editor
// autocomplete and annotations. Starting at a top-level .asm and following every `include "..."`
// (resolved relative to base_path, mirroring AsmFile's rules, and allowing a leading `Label:` on
// the include line as the top-level file uses), it collects:
//   - constants: the names of `NAME: equ VALUE` definitions (this includes the FLAG_* flag ids)
//   - labels:    column-0 `NAME:` definitions that are not `equ` and not local (leading '_')
//   - descriptions: for each constant, its trailing `; comment` (or its value when uncommented) -
//                   used to annotate e.g. a SetFlag FLAG_X line with what the flag means.
// Results are sorted, de-duplicated and cached per (top_level, base_path) so repeated lookups are
// cheap. See [[cutscene-two-layer-architecture]].
struct AsmSymbols
{
	std::vector<std::string> constants;
	std::vector<std::string> labels;
	std::map<std::string, std::string> descriptions;
};

namespace AsmSymbolHarvester
{
	// The (cached) symbols reachable from top_level. Blocks on first call for a given (top,base)
	// while the tree is read; call Prewarm from a worker thread to hide that latency. A tree that
	// cannot be read yields an empty result rather than an error.
	const AsmSymbols& Get(const std::filesystem::path& top_level, const std::filesystem::path& base_path);

	// Populate the cache for (top_level, base_path) if it is not already present. Safe to call from a
	// background thread (the cache is mutex-guarded); does nothing but warm the cache.
	void Prewarm(const std::filesystem::path& top_level, const std::filesystem::path& base_path);
}

#endif // _ASM_SYMBOL_HARVESTER_H_
