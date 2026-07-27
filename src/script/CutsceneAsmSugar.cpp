#include <script/CutsceneAsmSugar.h>

#include <regex>
#include <sstream>
#include <iomanip>
#include <vector>

namespace CutsceneSugar
{
namespace
{
	// The shape a token collapses/expands to. Call = "load an id into d0, then call a subroutine";
	// Store = "move an immediate id straight into an absolute memory symbol".
	enum class Form { Call, Store };

	// One collapsible idiom. A token of this kind round-trips to exactly this shape; the move's size
	// (and, for Store, the absolute-address size) must match on collapse so the expansion is
	// byte-reversible (`.w` vs `.l` absolute addressing assemble to different encodings).
	struct Spec
	{
		std::string kind;
		Form form;
		char size;          // 'w' or 'b': the move.<size> immediate
		bool has_wait;      // Call: a `move.w #wait,d1` sits between the d0-move and the call
		bool tail;          // Call: the call is `bra` (tail) rather than `bsr`
		std::string target; // Call: the subroutine label. Store: the absolute destination symbol.
		char addr_size;     // Store: the (symbol).<addr_size> suffix (typically 'l'); unused for Call.
	};

	const std::vector<Spec>& Specs()
	{
		static const std::vector<Spec> SPECS = {
			{ "PlayCutscene",          Form::Call,  'w', false, false, "LoadCutsceneDialogue",        0   },
			{ "PlayCutsceneAndReturn", Form::Call,  'w', false, true,  "LoadCutsceneDialogue",        0   },
			{ "PlayCutsceneAndWait",   Form::Call,  'w', true,  false, "ShowCutsceneDialogueAndWait", 0   },
			{ "Playback",              Form::Call,  'b', false, false, "PlaybackInput",               0   },
			{ "ShowIntroString",       Form::Store, 'b', false, false, "g_IntroStringToDisplay",      'l' },
		};
		return SPECS;
	}

	const Spec* FindSpec(const std::string& kind)
	{
		for (const auto& s : Specs())
		{
			if (s.kind == kind) return &s;
		}
		return nullptr;
	}

	const Spec* FindCallSpec(const std::string& target, bool tail, bool has_wait, char size)
	{
		for (const auto& s : Specs())
		{
			if (s.form == Form::Call && s.target == target && s.tail == tail
				&& s.has_wait == has_wait && s.size == size)
			{
				return &s;
			}
		}
		return nullptr;
	}

	const Spec* FindStoreSpec(const std::string& symbol, char addr_size, char size)
	{
		for (const auto& s : Specs())
		{
			if (s.form == Form::Store && s.target == symbol && s.addr_size == addr_size && s.size == size)
			{
				return &s;
			}
		}
		return nullptr;
	}

	// Split into lines without their terminators; remember whether a trailing newline was present.
	std::vector<std::string> SplitLines(const std::string& s, bool& trailing_newline)
	{
		std::vector<std::string> lines;
		std::string cur;
		for (char c : s)
		{
			if (c == '\n')
			{
				lines.push_back(cur);
				cur.clear();
			}
			else
			{
				cur += c;
			}
		}
		trailing_newline = s.empty() || s.back() == '\n';
		if (!cur.empty() || (!s.empty() && s.back() != '\n'))
		{
			lines.push_back(cur);
		}
		return lines;
	}

	std::string Join(const std::vector<std::string>& lines, bool trailing_newline)
	{
		std::string out;
		for (std::size_t i = 0; i < lines.size(); ++i)
		{
			out += lines[i];
			if (i + 1 < lines.size() || trailing_newline)
			{
				out += '\n';
			}
		}
		return out;
	}

	// The code part of a line: text before any ';' comment, trimmed of surrounding whitespace.
	std::string Code(const std::string& line)
	{
		std::string s = line.substr(0, line.find(';'));
		std::size_t b = 0;
		std::size_t e = s.size();
		while (b < e && (s[b] == ' ' || s[b] == '\t')) ++b;
		while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r' || s[e - 1] == '\n')) --e;
		return s.substr(b, e - b);
	}

	int ParseImm(const std::string& tok)
	{
		return tok.size() > 1 && tok[0] == '$'
			? static_cast<int>(std::stol(tok.substr(1), nullptr, 16))
			: static_cast<int>(std::stol(tok, nullptr, 10));
	}

	struct MoveMatch { char size; int id; };

	// `move[.bwl]  #<imm> , <reg>` - returns the size and immediate if `reg` matches, else nullopt.
	// A bare `move` (no size) is treated as `.w`. moveq is intentionally excluded (the idioms always
	// use a sized move).
	bool MoveImm(const std::string& code, const std::string& reg, MoveMatch& out)
	{
		static const std::regex re(R"(^move(?:\.([bwl]))?\s+#(\$?[0-9A-Fa-f]+)\s*,\s*(d[0-7])$)",
			std::regex::icase);
		std::smatch m;
		if (!std::regex_match(code, m, re) || m[3].str() != reg)
		{
			return false;
		}
		out.size = m[1].matched ? static_cast<char>(std::tolower(m[1].str()[0])) : 'w';
		try { out.id = ParseImm(m[2].str()); }
		catch (const std::exception&) { return false; }
		return true;
	}

	// `bra|bsr[.bsw]  <label>` - returns {is_bsr, target, size}. `size` is the branch's size suffix
	// ('s'/'w'/'b'), or 'w' when absent (a bare bra/bsr assembles as word). The idioms only ever expand
	// to `.w`, so the caller must reject short/byte branches - collapsing a `.s` call and re-emitting it
	// as `.w` would change 2 bytes to 4 and corrupt the assembled binary (CSA_0001.. do use `bra.s`).
	bool Branch(const std::string& code, bool& is_bsr, std::string& target, char& size)
	{
		static const std::regex re(R"(^(bra|bsr)(?:\.([bsw]))?\s+(\w+)$)", std::regex::icase);
		std::smatch m;
		if (!std::regex_match(code, m, re))
		{
			return false;
		}
		std::string mn = m[1].str();
		for (auto& c : mn) c = static_cast<char>(std::tolower(c));
		is_bsr = (mn == "bsr");
		size = m[2].matched ? static_cast<char>(std::tolower(m[2].str()[0])) : 'w';
		target = m[3].str();
		return true;
	}

	struct StoreMatch { char size; int id; char addr_size; std::string symbol; };

	// `move[.bwl]  #<imm> , ( <symbol> ) . [bwl]` - an immediate stored into an absolute address.
	// Returns the move size, the immediate, the address size and the symbol. A bare `move` is `.w`.
	bool StoreImm(const std::string& code, StoreMatch& out)
	{
		static const std::regex re(
			R"(^move(?:\.([bwl]))?\s+#(\$?[0-9A-Fa-f]+)\s*,\s*\(\s*(\w+)\s*\)\.([bwl])$)",
			std::regex::icase);
		std::smatch m;
		if (!std::regex_match(code, m, re))
		{
			return false;
		}
		out.size = m[1].matched ? static_cast<char>(std::tolower(m[1].str()[0])) : 'w';
		out.addr_size = static_cast<char>(std::tolower(m[4].str()[0]));
		out.symbol = m[3].str();
		try { out.id = ParseImm(m[2].str()); }
		catch (const std::exception&) { return false; }
		return true;
	}

	// The comment text of a line (after ';', trimmed), or "" if none.
	std::string CommentOf(const std::string& line)
	{
		const std::size_t semi = line.find(';');
		if (semi == std::string::npos)
		{
			return {};
		}
		std::string c = line.substr(semi + 1);
		std::size_t b = 0;
		std::size_t e = c.size();
		while (b < e && (c[b] == ' ' || c[b] == '\t')) ++b;
		while (e > b && (c[e - 1] == ' ' || c[e - 1] == '\t' || c[e - 1] == '\r')) --e;
		return c.substr(b, e - b);
	}

	std::string RenderToken(const std::string& kind, int id, int extra)
	{
		std::ostringstream ss;
		ss << "\t\t<" << kind << " $" << std::hex << std::uppercase << std::setw(3) << std::setfill('0') << id;
		if (extra >= 0)
		{
			ss << std::dec << ", " << extra;
		}
		ss << ">";
		return ss.str();
	}

	// Scan for the collapsible idioms. For each match, `sink(token, comment)` is invoked and the
	// scanner advances past it; unmatched lines invoke `passthrough(line)`. Shared by ToSugar (which
	// emits tokens) and CallComments (which collects (kind,id) -> comment).
	template <typename Sink, typename Passthrough>
	void ScanIdioms(const std::vector<std::string>& lines, Sink sink, Passthrough passthrough)
	{
		for (std::size_t i = 0; i < lines.size();)
		{
			// Single-line Store idiom (e.g. <ShowIntroString>): move #id into an absolute symbol.
			StoreMatch st;
			if (StoreImm(Code(lines[i]), st))
			{
				if (const Spec* sp = FindStoreSpec(st.symbol, st.addr_size, st.size))
				{
					sink(Token{ sp->kind, st.id, -1 }, CommentOf(lines[i]));
					++i;
					continue;
				}
			}
			MoveMatch d0;
			bool matched = false;
			if (MoveImm(Code(lines[i]), "d0", d0) && i + 1 < lines.size())
			{
				const std::string comment = CommentOf(lines[i]);
				bool is_bsr = false;
				std::string target;
				char bsize = 'w';
				// Only word-sized branches collapse: the expansion always emits `.w`, so a `.s`/`.b`
				// call must stay verbatim (it still round-trips exactly, just without the sugar).
				if (Branch(Code(lines[i + 1]), is_bsr, target, bsize) && bsize == 'w')
				{
					if (const Spec* sp = FindCallSpec(target, !is_bsr, false, d0.size))
					{
						sink(Token{ sp->kind, d0.id, -1 }, comment);
						i += 2;
						matched = true;
					}
				}
				if (!matched)
				{
					MoveMatch d1;
					if (MoveImm(Code(lines[i + 1]), "d1", d1) && d1.size == 'w' && i + 2 < lines.size()
						&& Branch(Code(lines[i + 2]), is_bsr, target, bsize) && bsize == 'w')
					{
						if (const Spec* sp = FindCallSpec(target, !is_bsr, true, d0.size))
						{
							sink(Token{ sp->kind, d0.id, d1.id }, comment);
							i += 3;
							matched = true;
						}
					}
				}
			}
			if (!matched)
			{
				passthrough(lines[i]);
				++i;
			}
		}
	}
}

const std::vector<std::string>& Kinds()
{
	static const std::vector<std::string> kinds = []
	{
		std::vector<std::string> k;
		for (const auto& s : Specs()) k.push_back(s.kind);
		return k;
	}();
	return kinds;
}

std::string ToSugar(const std::string& asm_text)
{
	bool trailing = false;
	const auto lines = SplitLines(asm_text, trailing);
	std::vector<std::string> out;
	ScanIdioms(lines,
		[&](const Token& t, const std::string&) { out.push_back(RenderToken(t.kind, t.id, t.extra)); },
		[&](const std::string& line) { out.push_back(line); });
	return Join(out, trailing);
}

std::optional<Token> ParseToken(const std::string& sugar_line)
{
	static const std::regex re(R"(^<\s*([A-Za-z]+)\s+(\$?[0-9A-Fa-f]+)\s*(?:,\s*(\d+)\s*)?>$)");
	std::smatch m;
	const std::string code = Code(sugar_line);
	if (!std::regex_match(code, m, re) || !FindSpec(m[1].str()))
	{
		return std::nullopt;
	}
	try
	{
		return Token{ m[1].str(), ParseImm(m[2].str()), m[3].matched ? std::stoi(m[3].str()) : -1 };
	}
	catch (const std::exception&)
	{
		return std::nullopt;
	}
}

std::map<std::pair<std::string, int>, std::string> CallComments(const std::string& asm_text)
{
	bool trailing = false;
	const auto lines = SplitLines(asm_text, trailing);
	std::map<std::pair<std::string, int>, std::string> comments;
	ScanIdioms(lines,
		[&](const Token& t, const std::string& comment)
		{
			if (!comment.empty())
			{
				comments.emplace(std::make_pair(t.kind, t.id), comment); // first authored comment wins
			}
		},
		[](const std::string&) {});
	return comments;
}

std::string ToAsm(const std::string& sugar_text, const std::function<std::string(const Token&)>& describe)
{
	bool trailing = false;
	const auto lines = SplitLines(sugar_text, trailing);
	std::vector<std::string> out;
	for (const auto& line : lines)
	{
		const auto token = ParseToken(line);
		const Spec* sp = token ? FindSpec(token->kind) : nullptr;
		if (!token || !sp)
		{
			out.push_back(line);
			continue;
		}
		// A comment the user typed on the token wins; otherwise re-attach the looked-up description.
		std::string comment = CommentOf(line);
		if (comment.empty() && describe)
		{
			comment = describe(*token);
		}
		// Store idiom: a single `move.<size> #id,(symbol).<addr>` line.
		if (sp->form == Form::Store)
		{
			std::ostringstream st;
			st << "\t\tmove." << sp->size << "\t#$" << std::hex << std::uppercase
				<< std::setw(sp->size == 'b' ? 2 : 4) << std::setfill('0') << token->id
				<< ",(" << sp->target << ")." << sp->addr_size;
			if (!comment.empty())
			{
				st << "\t  ; " << comment;
			}
			out.push_back(st.str());
			continue;
		}
		std::ostringstream d0;
		d0 << "\t\tmove." << sp->size << "\t#$" << std::hex << std::uppercase
			<< std::setw(sp->size == 'b' ? 2 : 4) << std::setfill('0') << token->id << ",d0";
		if (!comment.empty())
		{
			d0 << "\t  ; " << comment;
		}
		out.push_back(d0.str());
		if (sp->has_wait)
		{
			out.push_back("\t\tmove.w\t#" + std::to_string(token->extra >= 0 ? token->extra : 0) + ",d1");
			out.push_back("\t\tbsr.w\t" + sp->target);
		}
		else if (sp->tail)
		{
			out.push_back("\t\tbra.w\t" + sp->target);
		}
		else
		{
			out.push_back("\t\tbsr.w\t" + sp->target);
		}
	}
	return Join(out, trailing);
}

} // namespace CutsceneSugar
