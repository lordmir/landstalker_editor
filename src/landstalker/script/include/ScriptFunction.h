#ifndef _SCRIPT_FUNCTION_H_
#define _SCRIPT_FUNCTION_H_

#include <landstalker/main/include/AsmFile.h>
#include <sstream>
#include <map>
#include <set>
#include <variant>
#include <yaml-cpp/yaml.h>

struct Statement
{
	virtual void ToAsm(AsmFile& file) const = 0;
	virtual std::string ToYaml(int indent = 0) const = 0;
	virtual std::string Print(int indent = 0) const = 0;
	virtual bool IsEndOfFunction() const = 0;
};

struct Action;
struct YesNoPrompt;
struct SetFlagOnTalk;
struct IsFlagSet;
struct PlaySound;
struct CustomItemScript;
struct Sleep;
struct DisplayPrice;
struct Branch;
struct ShopInteraction;
struct ChurchInteraction;
struct Rts;
struct CustomAsm;
struct ProgressList;
struct ProgressFlagMapping;
struct ActionTable;

class ScriptFunction
{
public:
	// Possible statement types

	using ScriptStatement = std::variant
	<
		Action, CustomAsm, ProgressList, YesNoPrompt, SetFlagOnTalk, IsFlagSet, PlaySound, CustomItemScript, ActionTable, Sleep, DisplayPrice, Branch,
		ShopInteraction, ChurchInteraction, ProgressFlagMapping, Rts
	>;

	ScriptFunction(const std::string& name, const std::vector<ScriptStatement>& statements = {});
	ScriptFunction(AsmFile& file);
	ScriptFunction(const YAML::Node::const_iterator& it);
	void ToAsm(AsmFile& file) const;
	std::string ToYaml(int indent = 0) const;
	std::string Print(int indent = 0) const;
	bool ProcessScriptFunction(const AsmFile::Instruction& ins, AsmFile& file);
	bool ProcessScriptTrap(const AsmFile::Instruction& ins, AsmFile& file);
	bool ProcessScriptMiscInstructions(const AsmFile::Instruction& ins);

	std::string name;
	std::vector<ScriptStatement> statements;
};

class ScriptFunctionTable
{
public:
	ScriptFunctionTable() {}
	ScriptFunctionTable(AsmFile& file);
	ScriptFunctionTable(const std::string& yaml);
	bool ReadAsm(AsmFile& file);
	bool WriteAsm(AsmFile& file);
	std::string Print() const;
	std::string ToYaml(const std::string& name = std::string("Script")) const;
private:
	void Consolidate();
	void Unconsolidate();

	std::map<std::string, ScriptFunction> function_mapping;
	std::vector<std::string> funcnames;
};

struct Action : public Statement
{
	Action() = default;
	Action(const Action&) = default;
	Action(Action&&) = default;
	Action(const AsmFile::ScriptAction& scriptaction);
	Action(const ScriptFunction& func) : action(func) {}
	Action(AsmFile& file);
	Action(const YAML::Node::const_iterator& it);
	Action& operator=(const Action&) = default;
	Action& operator=(Action&&) = default;
	operator AsmFile::ScriptAction() const;
	virtual void ToAsm(AsmFile& file) const override;
	void ActionToAsm(AsmFile& file, int offset = 0) const;
	virtual std::string ToYaml(int indent = 0) const override;
	virtual std::string Print(int indent = 0) const override;
	virtual bool IsEndOfFunction() const override;
	std::optional<std::variant<AsmFile::ScriptId, AsmFile::ScriptJump, ScriptFunction>> action;
	std::size_t offset = 0;
};

struct YesNoPrompt : public Statement
{
	YesNoPrompt(AsmFile& file);
	YesNoPrompt(const YAML::Node::const_iterator& it);
	virtual void ToAsm(AsmFile& file) const override;
	virtual std::string ToYaml(int indent = 0) const override;
	virtual std::string Print(int indent = 0) const override;
	virtual bool IsEndOfFunction() const override;
	Action prompt;
	Action on_yes;
	Action on_no;
};

struct SetFlagOnTalk : public Statement
{
	SetFlagOnTalk(AsmFile& file);
	SetFlagOnTalk(const YAML::Node::const_iterator& it);
	virtual void ToAsm(AsmFile& file) const override;
	virtual std::string ToYaml(int indent = 0) const override;
	virtual std::string Print(int indent = 0) const override;
	virtual bool IsEndOfFunction() const override;
	uint16_t flag;
	Action on_clear;
	Action on_set;
};

struct IsFlagSet : public Statement
{
	IsFlagSet(AsmFile& file);
	IsFlagSet(const YAML::Node::const_iterator& it);
	virtual void ToAsm(AsmFile& file) const override;
	virtual std::string ToYaml(int indent = 0) const override;
	virtual std::string Print(int indent = 0) const override;
	virtual bool IsEndOfFunction() const override;
	uint16_t flag;
	Action on_set;
	Action on_clear;
};

struct PlaySound : public Statement
{
	PlaySound(AsmFile& file);
	PlaySound(const YAML::Node::const_iterator& it);
	virtual void ToAsm(AsmFile& file) const override;
	virtual std::string ToYaml(int indent = 0) const override;
	virtual std::string Print(int indent = 0) const override;
	virtual bool IsEndOfFunction() const override;
	uint16_t sound;
};

struct CustomItemScript : public Statement
{
	CustomItemScript(AsmFile& file);
	CustomItemScript(const YAML::Node::const_iterator& it);
	virtual void ToAsm(AsmFile& file) const override;
	virtual std::string ToYaml(int indent = 0) const override;
	virtual std::string Print(int indent = 0) const override;
	virtual bool IsEndOfFunction() const override;
	uint16_t custom_item_script;
};

struct Sleep : public Statement
{
	Sleep(AsmFile& file);
	Sleep(const YAML::Node::const_iterator& it);
	virtual void ToAsm(AsmFile& file) const override;
	virtual std::string ToYaml(int indent = 0) const override;
	virtual std::string Print(int indent = 0) const override;
	virtual bool IsEndOfFunction() const override;
	uint16_t ticks;
};

struct DisplayPrice : public Statement
{
	DisplayPrice(AsmFile& file);
	DisplayPrice(const YAML::Node::const_iterator& it);
	virtual void ToAsm(AsmFile& file) const override;
	virtual std::string ToYaml(int indent = 0) const override;
	virtual std::string Print(int indent = 0) const override;
	virtual bool IsEndOfFunction() const override;
	Action display_price;
};

struct Branch : public Statement
{
	Branch(const AsmFile::Instruction& file);
	Branch(const YAML::Node::const_iterator& it);
	virtual void ToAsm(AsmFile& file) const override;
	virtual std::string ToYaml(int indent = 0) const override;
	virtual std::string Print(int indent = 0) const override;
	virtual bool IsEndOfFunction() const override;
	std::string label;
	bool wide;
};

struct ShopInteraction : public Statement
{
	ShopInteraction(AsmFile& file);
	ShopInteraction(const YAML::Node::const_iterator& it);
	virtual void ToAsm(AsmFile& file) const override;
	virtual std::string ToYaml(int indent = 0) const override;
	virtual std::string Print(int indent = 0) const override;
	virtual bool IsEndOfFunction() const override;
	Action on_sale_prompt;
	Action on_sale_confirm;
	Action on_no_money;
	Action on_sale_decline;
};

struct ChurchInteraction : public Statement
{
	ChurchInteraction(AsmFile& file);
	ChurchInteraction(const YAML::Node::const_iterator& it);
	virtual void ToAsm(AsmFile& file) const override;
	virtual std::string ToYaml(int indent = 0) const override;
	virtual std::string Print(int indent = 0) const override;
	virtual bool IsEndOfFunction() const override;
	Action script_normal_priest;
	Action script_skeleton_priest;
};

struct Rts : public Statement
{
	Rts();
	Rts(const YAML::Node::const_iterator& it);
	virtual void ToAsm(AsmFile& file) const override;
	virtual std::string ToYaml(int indent = 0) const override;
	virtual std::string Print(int indent = 0) const override;
	virtual bool IsEndOfFunction() const override;
};

struct CustomAsm : public Statement
{
	CustomAsm(const AsmFile::Instruction& ins);
	CustomAsm(const YAML::Node::const_iterator& it);
	void Append(const AsmFile::Instruction& ins);
	virtual void ToAsm(AsmFile& file) const override;
	virtual std::string ToYaml(int indent = 0) const override;
	virtual std::string Print(int indent = 0) const override;
	virtual bool IsEndOfFunction() const override;
	std::vector<AsmFile::Instruction> instructions;
};

struct ProgressList : public Statement
{
	ProgressList(AsmFile& file);
	ProgressList(const YAML::Node::const_iterator& it);
	virtual void ToAsm(AsmFile& file) const override;
	virtual std::string ToYaml(int indent = 0) const override;
	virtual std::string Print(int indent = 0) const override;
	virtual bool IsEndOfFunction() const override;
	struct Flag
	{
		Flag(uint8_t p_quest, uint8_t p_progress) : quest(p_quest), progress(p_progress) {}
		
		uint8_t quest;
		uint8_t progress;

		bool operator<(const Flag& rhs) const
		{
			if (this->quest == rhs.quest)
			{
				return this->progress < rhs.progress;
			}
			else
			{
				return this->quest < rhs.quest;
			}
		}
	};
	std::map<Flag, Action> progress;
};

struct ProgressFlagMapping : public Statement
{
	ProgressFlagMapping(AsmFile& file);
	ProgressFlagMapping(const YAML::Node::const_iterator& it);
	virtual void ToAsm(AsmFile& file) const override;
	virtual std::string ToYaml(int indent = 0) const override;
	virtual std::string Print(int indent = 0) const override;
	virtual bool IsEndOfFunction() const override;
	std::map<uint8_t, uint16_t, std::greater<uint8_t>> mapping;
};

struct ActionTable : public Statement
{
	ActionTable(AsmFile& file);
	ActionTable(const YAML::Node::const_iterator& it);
	virtual void ToAsm(AsmFile& file) const override;
	virtual std::string ToYaml(int indent = 0) const override;
	virtual std::string Print(int indent = 0) const override;
	virtual bool IsEndOfFunction() const override;
	std::vector<Action> actions;
};

std::ostream& operator<<(std::ostream& lhs, const Statement& rhs);
std::ostream& operator<<(std::ostream& lhs, const ScriptFunction& rhs);
std::ostream& operator<<(std::ostream& lhs, const ScriptFunctionTable& rhs);

#endif // _SCRIPT_FUNCTION_H_
