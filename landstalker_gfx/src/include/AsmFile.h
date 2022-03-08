#ifndef _ASM_FILE_H_
#define _ASM_FILE_H_

#include <cstdint>
#include <string>
#include <deque>
#include <vector>
#include <unordered_map>
#include <variant>
#include <map>

class AsmFile
{
public:
	enum FileType
	{
		ASSEMBLER,
		BINARY
	};

	enum Base
	{
		BIN = 2,
		OCT = 8,
		DEC = 10,
		HEX = 16
	};

	struct IncludeFile
	{
		IncludeFile(const std::string& ppath, FileType ptype)
			: path(ppath), type(ptype)
		{}
		IncludeFile() : type(ASSEMBLER) {}

		std::string path;
		FileType type;
	};

	struct Comment
	{
		Comment(const std::string& pcomment) : comment(pcomment) {}
		Comment() {}
		std::string comment;
	};

	struct Label
	{
		Label(const std::string& plabel) : label(plabel) {}
		Label() {}
		std::string label;
	};

	struct Align
	{
		Align(std::size_t pamount) : amount(pamount) {}
		std::size_t amount;
	};

	struct NewLine {};

	AsmFile(const std::string& filename, FileType type = FileType::ASSEMBLER);
	AsmFile(FileType type = FileType::ASSEMBLER);

	template<typename T>
	AsmFile& operator<<(const T& data);
	template<typename T>
	AsmFile& operator>>(T& data);
	template<typename T>
	AsmFile& operator>>(const T& data);

	std::vector<uint8_t> ToBinary();
	std::string ToAssembly();
	void Clear();
	void Reset();
	std::string ReadLabel();
	bool IsLabel();
	bool IsLabel(const std::string& label);

	bool IsGood() const;
	bool ReadFile(const std::string& filename, FileType type = ASSEMBLER);
	bool WriteFile(const std::string& filename, FileType type);
	bool WriteFile(const std::string& filename);
	std::ostream& PrintFile(std::ostream& stream);

	std::size_t GetLineCount() const;
	std::size_t GetByteCount() const;
	std::string GetFilename() const;
	FileType GetFileType() const;

	template<typename T>
	bool Read(T& value);
	bool Read(const Label& label);
	template<template<typename, typename...> class C, typename T, typename... Rest>
	bool Read(C<T, Rest...>& container);
	template<typename T, std::size_t N>
	bool Read(std::array<T, N>& container);
	template<typename T, std::size_t N>
	bool Read(T(&array)[N]);
	template<typename T, typename... Args>
	bool Read(T& value, Args&&... args);

	template<typename T>
	bool Write(const T& data);
	bool Write(const std::string& data);
	template<std::size_t N>
	bool Write(const char(&data)[N]);
	bool Write(const Label& label);
	bool Write(const Comment& comment);
	bool Write(const IncludeFile& file);
	bool Write(const NewLine&);
	bool Write(const Align&);
	template<typename T, typename... Args>
	bool Write(const T& first, Args&&... args);
	template<typename Iter>
	bool Write(Iter begin, Iter end);
	template<typename T, std::size_t N>
	bool Write(const T(&val)[N]);
	template<template <typename, std::size_t> typename C, typename T, std::size_t N>
	bool Write(const C<T, N>& val);
	template<template <typename, typename ...> typename C, typename T, typename... Rest>
	bool Write(const C<T, Rest...>& container);

	friend std::ostream& operator<<(std::ostream& stream, AsmFile& file);
private:
	struct AsmLine
	{
		std::string label;
		std::string instruction;
		std::string width;
		std::string operand;
		std::string comment;
		std::string args;
		bool Empty() const;
		void Clear();
	};

	enum class Inst
	{
		DC,
		DCB,
		INCLUDE,
		INCBIN,
		ALIGN
	};

	bool ParseLine(AsmLine& line, const std::string& str);
	int64_t ParseValue(std::string val);
	int GetWidth(const AsmLine& line);

	template<AsmFile::Inst>
	bool ProcessInst(const AsmFile::AsmLine& line);
	bool ProcessLine(const AsmFile::AsmLine& line);
	std::string ToAsmLine(const AsmFile::AsmLine& line);
	template<typename T>
	std::string ToAsmValue(T value, Base base);
	template<typename T>
	std::string ToAsmValue(const T& value);
	std::string ToAsmValue(const std::string& value);
	std::string ToAsmValue(uint64_t value);
	std::string ToAsmValue(int64_t value);
	std::string ToAsmValue(uint32_t value);
	std::string ToAsmValue(int32_t value);
	std::string ToAsmValue(uint16_t value);
	std::string ToAsmValue(int16_t value);
	std::string ToAsmValue(uint8_t value);
	std::string ToAsmValue(int8_t value);
	template<std::size_t N>
	std::string ToAsmValue(const char(&val)[N]);
	template<typename Iter>
	std::string ToAsmValue(Iter begin, Iter end);
	template<typename T, std::size_t N>
	std::string ToAsmValue(const T(&val)[N]);
	template<template <typename, typename ...> typename C, typename T, typename... Rest>
	std::string ToAsmValue(const C<T, Rest...>& container);
	template<template <typename, std::size_t> typename C, typename T, std::size_t N>
	std::string ToAsmValue(const C<T, N>& val);
	void PushNextLine();

	static const std::unordered_map<std::string, Inst> INSTRUCTIONS;
	static const std::unordered_map<std::string, std::size_t> WIDTHS;
	static const std::size_t MAX_ELEMENTS_ON_LINE = 16;

	bool m_good;
	FileType m_type;
	std::string m_filename;
	std::vector<AsmLine> m_asm;
	AsmLine m_nextline;
	std::vector<std::variant<uint8_t, std::string, IncludeFile>> m_data;
	std::vector<std::variant<uint8_t, std::string, IncludeFile>>::const_iterator m_readptr;
	std::map<std::string, std::size_t> m_labels;
	std::map<std::size_t, std::string> m_labelpos;
};

std::ostream& operator<<(std::ostream& stream, AsmFile& file);

#include <AsmFile_inl.h>

#endif // _ASM_FILE_H_
