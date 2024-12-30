#ifndef _SCRIPT_FUNCTION_H_
#define _SCRIPT_FUNCTION_H_

#include <landstalker/main/include/AsmFile.h>
#include <sstream>
#include <map>
#include <set>
#include <variant>
#include <yaml-cpp/yaml.h>

namespace Statements
{
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
}

class ScriptFunction
{
public:
	// Possible statement types

	using ScriptStatement = std::variant
	<
		Statements::Action, Statements::CustomAsm, Statements::ProgressList, Statements::YesNoPrompt, Statements::SetFlagOnTalk,
		Statements::IsFlagSet, Statements::PlaySound, Statements::CustomItemScript, Statements::ActionTable, Statements::Sleep,
		Statements::DisplayPrice, Statements::Branch, Statements::ShopInteraction, Statements::ChurchInteraction,
		Statements::ProgressFlagMapping, Statements::Rts
	>;

	ScriptFunction(const std::string& name, const std::vector<ScriptStatement>& statements = {});
	ScriptFunction(AsmFile& file);
	ScriptFunction(const YAML::Node::const_iterator& it);

	bool operator== (const ScriptFunction& rhs) const;
	bool operator!= (const ScriptFunction& rhs) const;

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
	ScriptFunctionTable(AsmFile&& file);
	ScriptFunctionTable(const std::string& yaml);

	bool operator== (const ScriptFunctionTable& rhs) const;
	bool operator!= (const ScriptFunctionTable& rhs) const;

	bool ReadAsm(AsmFile& file);
	bool WriteAsm(AsmFile& file);
	std::string Print() const;
	std::string ToYaml(const std::string& name = std::string("Script")) const;

	const std::vector<std::string>& GetFunctionNames() const;
	const ScriptFunction* GetMapping(const std::string& funcname) const;
	ScriptFunction* GetMapping(const std::string& funcname);
	ScriptFunction* GetMapping(std::size_t index);

	bool AddFunction(ScriptFunction&& func);
private:
	void Consolidate();
	void Unconsolidate();

	std::map<std::string, ScriptFunction> function_mapping;
	std::vector<std::string> funcnames;
};

namespace Statements
{
	struct Action : public Statement
	{
		Action() = default;
		Action(const Action&) = default;
		Action(Action&&) = default;
		Action(const AsmFile::ScriptAction& scriptaction);
		Action(const ScriptFunction& func) : action(func) {}
		Action(AsmFile& file);
		Action(const YAML::Node::const_iterator& it);
		Action& operator=(const Action& rhs) = default;
		Action& operator=(Action&&) = default;
		bool operator== (const Action& rhs) const;
		bool operator!= (const Action& rhs) const;
		operator AsmFile::ScriptAction() const;
		virtual void ToAsm(AsmFile& file) const override;
		void ActionToAsm(AsmFile& file, int offset = 0) const;
		virtual std::string ToYaml(int indent = 0) const override;
		std::string ActionToYaml(int indent = 0) const;
		virtual std::string Print(int indent = 0) const override;
		virtual bool IsEndOfFunction() const override;
		std::optional<std::variant<AsmFile::ScriptId, AsmFile::ScriptJump, ScriptFunction>> action;
		std::size_t offset = 0;
	};

	struct YesNoPrompt : public Statement
	{
		YesNoPrompt(AsmFile& file);
		YesNoPrompt(const YAML::Node::const_iterator& it);

		bool operator== (const YesNoPrompt& rhs) const;
		bool operator!= (const YesNoPrompt& rhs) const;

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

		bool operator== (const SetFlagOnTalk& rhs) const;
		bool operator!= (const SetFlagOnTalk& rhs) const;

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

		bool operator== (const IsFlagSet& rhs) const;
		bool operator!= (const IsFlagSet& rhs) const;

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

		bool operator== (const PlaySound& rhs) const;
		bool operator!= (const PlaySound& rhs) const;

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

		bool operator== (const CustomItemScript& rhs) const;
		bool operator!= (const CustomItemScript& rhs) const;

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

		bool operator== (const Sleep& rhs) const;
		bool operator!= (const Sleep& rhs) const;

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

		bool operator== (const DisplayPrice& rhs) const;
		bool operator!= (const DisplayPrice& rhs) const;

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

		bool operator== (const Branch& rhs) const;
		bool operator!= (const Branch& rhs) const;

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

		bool operator== (const ShopInteraction& rhs) const;
		bool operator!= (const ShopInteraction& rhs) const;

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

		bool operator== (const ChurchInteraction& rhs) const;
		bool operator!= (const ChurchInteraction& rhs) const;

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

		bool operator== (const Rts& rhs) const;
		bool operator!= (const Rts& rhs) const;

		virtual void ToAsm(AsmFile& file) const override;
		virtual std::string ToYaml(int indent = 0) const override;
		virtual std::string Print(int indent = 0) const override;
		virtual bool IsEndOfFunction() const override;
	};

	struct CustomAsm : public Statement
	{
		CustomAsm(const AsmFile::Instruction& ins);
		CustomAsm(const YAML::Node::const_iterator& it);
		CustomAsm(const std::string& inst);
		CustomAsm(const std::vector<AsmFile::Instruction>& inst);

		bool operator== (const CustomAsm& rhs) const;
		bool operator!= (const CustomAsm& rhs) const;

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

		bool operator== (const ProgressList& rhs) const;
		bool operator!= (const ProgressList& rhs) const;

		virtual void ToAsm(AsmFile& file) const override;
		virtual std::string ToYaml(int indent = 0) const override;
		virtual std::string Print(int indent = 0) const override;
		virtual bool IsEndOfFunction() const override;
		struct QuestProgress
		{
			QuestProgress(uint8_t p_quest, uint8_t p_progress) : quest(p_quest), progress(p_progress) {}

			QuestProgress() = delete;
			QuestProgress(const QuestProgress&) = default;
			QuestProgress(QuestProgress&&) = default;
			QuestProgress& operator= (const QuestProgress&) = default;
			QuestProgress& operator= (QuestProgress&&) = default;

			bool operator==(const QuestProgress& rhs) const
			{
				return this->quest == rhs.quest && this->progress == rhs.progress;
			}
			bool operator!=(const QuestProgress& rhs) const
			{
				return !(*this == rhs);
			}
			bool operator<(const QuestProgress& rhs) const
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

			uint8_t quest;
			uint8_t progress;
		};
		std::map<QuestProgress, Action> progress;
	};

	struct ProgressFlagMapping : public Statement
	{
		ProgressFlagMapping(AsmFile& file);
		ProgressFlagMapping(const YAML::Node::const_iterator& it);
		ProgressFlagMapping(std::map<uint8_t, uint16_t, std::greater<uint8_t>>&& p_mapping) : mapping(p_mapping) {}

		bool operator== (const ProgressFlagMapping& rhs) const;
		bool operator!= (const ProgressFlagMapping& rhs) const;

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

		bool operator== (const ActionTable& rhs) const;
		bool operator!= (const ActionTable& rhs) const;

		virtual void ToAsm(AsmFile& file) const override;
		virtual std::string ToYaml(int indent = 0) const override;
		virtual std::string Print(int indent = 0) const override;
		virtual bool IsEndOfFunction() const override;
		std::vector<Action> actions;
	};
}

std::ostream& operator<<(std::ostream& lhs, const Statements::Statement& rhs);
std::ostream& operator<<(std::ostream& lhs, const ScriptFunction& rhs);
std::ostream& operator<<(std::ostream& lhs, const ScriptFunctionTable& rhs);

#endif // _SCRIPT_FUNCTION_H_
