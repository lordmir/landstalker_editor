#include <script/AsmSymbolHarvester.h>

#include <algorithm>
#include <fstream>
#include <mutex>
#include <regex>
#include <set>

namespace
{
	std::string Trim(const std::string& s)
	{
		const auto b = s.find_first_not_of(" \t\r\n");
		if (b == std::string::npos)
		{
			return {};
		}
		const auto e = s.find_last_not_of(" \t\r\n");
		return s.substr(b, e - b + 1);
	}

	// Recursively read `full`, following `include "..."` directives (resolved relative to base_path,
	// as AsmFile does), gathering equ constants (with a description) and column-0 labels. `visited`
	// guards include cycles.
	void Walk(const std::filesystem::path& full, const std::filesystem::path& base_path,
		std::set<std::string>& constants, std::set<std::string>& labels,
		std::map<std::string, std::string>& descriptions, std::set<std::filesystem::path>& visited)
	{
		std::error_code ec;
		auto canonical = std::filesystem::weakly_canonical(full, ec);
		if (ec)
		{
			canonical = full;
		}
		if (!visited.insert(canonical).second)
		{
			return; // already parsed
		}
		std::ifstream ifs(full);
		if (!ifs.good())
		{
			return;
		}
		// Same shapes AsmFile recognises: `NAME: equ VALUE`, an include, and (new here) a bare
		// column-0 `NAME:` label. equ is tested first so a define never counts as a plain label.
		// The include pattern is NOT a raw string (it contains )" which would close R"(...)") and
		// allows a leading `Label:` because the top-level asm writes e.g. `Defines: include "..."`.
		static const std::regex equ_re("^(\\w+):\\s+equ\\s+(\\S+)", std::regex::icase);
		static const std::regex label_re("^(\\w+):");
		static const std::regex inc_re("^\\s*(?:\\w+:\\s*)?include\\s+\"([^\"]+)\"", std::regex::icase);
		std::string line;
		std::smatch m;
		while (std::getline(ifs, line))
		{
			// Follow includes FIRST and independently: the top-level file writes them behind a label
			// (`Defines: include "..."`), which also matches label_re - so this must not be an
			// else-if after the label branch, or the whole constants subtree is never descended into.
			if (std::regex_search(line, m, inc_re))
			{
				std::string rel = m[1].str();
				std::replace(rel.begin(), rel.end(), '\\', '/');
				const std::filesystem::path child = base_path.empty()
					? full.parent_path() / rel
					: base_path / rel;
				Walk(child, base_path, constants, labels, descriptions, visited);
			}
			// Then classify any symbol the line defines (a labelled include still yields its label).
			if (std::regex_search(line, m, equ_re))
			{
				const std::string name = m[1].str();
				constants.insert(name);
				// Prefer the trailing ; comment (what the flag means); fall back to the value.
				const auto semi = line.find(';');
				const std::string comment = (semi == std::string::npos) ? std::string() : Trim(line.substr(semi + 1));
				descriptions[name] = comment.empty() ? m[2].str() : comment;
			}
			else if (std::regex_search(line, m, label_re))
			{
				const std::string name = m[1].str();
				// Locals ('_'-prefixed) are file-scoped, so they are noise in a global picker.
				if (!name.empty() && name.front() != '_')
				{
					labels.insert(name);
				}
			}
		}
	}

	std::mutex g_mtx;
	std::map<std::string, AsmSymbols> g_cache;

	std::string CacheKey(const std::filesystem::path& top, const std::filesystem::path& base)
	{
		return top.string() + "|" + base.string();
	}
}

namespace AsmSymbolHarvester
{
	const AsmSymbols& Get(const std::filesystem::path& top_level, const std::filesystem::path& base_path)
	{
		std::lock_guard<std::mutex> lock(g_mtx);
		const std::string key = CacheKey(top_level, base_path);
		const auto it = g_cache.find(key);
		if (it != g_cache.end())
		{
			return it->second;
		}
		std::set<std::string> constants;
		std::set<std::string> labels;
		std::map<std::string, std::string> descriptions;
		std::set<std::filesystem::path> visited;
		Walk(top_level, base_path, constants, labels, descriptions, visited);
		AsmSymbols syms;
		syms.constants.assign(constants.begin(), constants.end());
		syms.labels.assign(labels.begin(), labels.end());
		syms.descriptions = std::move(descriptions);
		return g_cache.emplace(key, std::move(syms)).first->second;
	}

	void Prewarm(const std::filesystem::path& top_level, const std::filesystem::path& base_path)
	{
		Get(top_level, base_path); // fills the cache; result is intentionally discarded
	}
}
