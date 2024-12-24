#ifndef _DATA_MANAGER_H_
#define _DATA_MANAGER_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <landstalker/main/include/Rom.h>
#include <landstalker/main/include/AsmFile.h>
#include <filesystem>
#include <atomic>
#include <mutex>

using ByteVector = std::vector<uint8_t>;
using ByteVectorPtr = std::shared_ptr<ByteVector>;
using ConstByteVectorPtr = std::shared_ptr<const ByteVector>;
using PendingWrites = std::vector<std::pair<std::string, ConstByteVectorPtr>>;

class DataManager
{
public:
	template<class T>
	class Entry
	{
	public:
		Entry(DataManager* owner, const ByteVector& b, const std::string& name, const std::filesystem::path& filename);
		Entry(DataManager* owner, const std::string& name, const std::filesystem::path& filename);

		virtual bool Serialise(const std::shared_ptr<T> in, ByteVectorPtr out) = 0;
		virtual bool Deserialise(const ByteVectorPtr in, std::shared_ptr<T>& out) = 0;

		virtual void Initialise();
		virtual void Commit();
		virtual void AbandonChanges();
		virtual bool HasDataChanged() const;
		virtual bool HasSavedDataChanged() const;
		virtual bool Save(const std::filesystem::path& dir);

		DataManager* GetOwner() { return m_owner; }

		std::shared_ptr<T> GetData();
		std::shared_ptr<const T> GetData() const;
		std::shared_ptr<const T> GetOrigData() const;
		std::shared_ptr<const ByteVector> GetBytes();
		std::shared_ptr<const ByteVector> GetOrigBytes() const;
		std::string GetName() const;
		void SetName(const std::string& val);
		std::filesystem::path GetFilename() const;
		void SetFilename(const std::filesystem::path& val);
		uint32_t GetStartAddress() const;
		uint32_t GetDataLength();
		uint32_t GetEndAddress();
		uint32_t GetOrigDataLength() const;
		uint32_t GetOrigEndAddress() const;
		void SetStartAddress(uint32_t addr);
	private:
		std::shared_ptr<T> m_data;
		std::shared_ptr<T> m_orig_data;
		std::shared_ptr<T> m_saved_data;
		uint32_t m_begin_address;
		std::string m_name;
		std::filesystem::path m_filename;
		ByteVectorPtr m_raw_data;
		ByteVectorPtr m_cached_raw_data;
		DataManager* m_owner;
	};

	DataManager(const std::string& content_description, const std::filesystem::path& asm_file) : m_ready(false), m_status("Waiting"), m_progress(0.0), m_asm_filename(asm_file),
		m_base_path(asm_file.parent_path()), m_content_description(content_description) {}
	DataManager(const std::string& content_description, const Rom&) : m_ready(false), m_status("Waiting"), m_progress(0.0), m_content_description(content_description) {}
	DataManager(const std::string& content_description) : m_ready(false), m_status("Waiting"), m_progress(0.0), m_content_description(content_description) {}
	DataManager() : m_ready(false), m_status("Waiting"), m_progress(0.0) {}

	virtual bool Open(const std::filesystem::path& asm_file)
	{
		m_asm_filename = asm_file;
		m_base_path = asm_file.parent_path();
		return true;
	}
	virtual bool Open(const Rom&) { return true; }

	virtual ~DataManager() {}

	virtual PendingWrites GetPendingWrites() const;
	virtual bool WillFitInRom(const Rom& rom) const;
	virtual bool HasBeenModified() const;
	virtual bool InjectIntoRom(Rom& rom);
	virtual bool AbandomRomInjection();
	virtual bool Save(const std::filesystem::path& dir);
	virtual bool Save();
	virtual void RefreshPendingWrites(const Rom& rom);
	const std::string& GetContentDescription() const { return m_content_description; }

	bool IsReady() const { return m_ready; }
	std::pair<std::string, double> GetProgress() const
	{
		std::lock_guard<std::mutex> guard(m_status_lock);
		return std::pair{ std::string(m_status), double(m_progress) };
	}

	std::filesystem::path GetBasePath() const { return m_base_path; }
	std::filesystem::path GetAsmFilename() const { return m_asm_filename; }
protected:
	void SetProgress(const std::string& status, double progress) const
	{
		std::lock_guard<std::mutex> guard(m_status_lock);
		m_status = status;
		m_progress = progress;
	}
	mutable std::mutex m_busy_lock;
	std::atomic<bool> m_ready;

	virtual void CommitAllChanges();
	
	virtual bool GetFilenameFromAsm(AsmFile& file, const std::string& label, std::filesystem::path& path);


	mutable PendingWrites m_pending_writes;
private:
	mutable std::mutex m_status_lock;
	mutable std::string m_status;
	mutable double m_progress;
	std::filesystem::path m_asm_filename;
	std::filesystem::path m_base_path;
	std::string m_content_description;
};

template<class T>
inline DataManager::Entry<T>::Entry(DataManager* owner, const ByteVector& b, const std::string& name, const std::filesystem::path& filename)
	: m_data(std::make_shared<T>()),
	  m_orig_data(std::make_shared<T>()),
	  m_saved_data(nullptr),
	  m_begin_address(0),
	  m_name(name),
	  m_filename(filename),
	  m_raw_data(std::make_shared<ByteVector>(b)),
	  m_cached_raw_data(std::make_shared<ByteVector>()),
	  m_owner(owner)
{
}

template<class T>
inline DataManager::Entry<T>::Entry(DataManager* owner, const std::string& name, const std::filesystem::path& filename)
  : m_data(std::make_shared<T>()),
	m_orig_data(std::make_shared<T>()),
	m_saved_data(std::make_shared<T>()),
	m_begin_address(0),
	m_name(name),
	m_filename(filename),
	m_raw_data(std::make_shared<ByteVector>()),
	m_cached_raw_data(std::make_shared<ByteVector>()),
	m_owner(owner)
{
}

template<class T>
inline void DataManager::Entry<T>::Initialise()
{
	Deserialise(m_raw_data, m_orig_data);
	m_data = std::make_shared<T>(*m_orig_data);
	m_saved_data = std::make_shared<T>(*m_orig_data);
}

template<class T>
inline void DataManager::Entry<T>::Commit()
{
	if (HasDataChanged())
	{
		Serialise(m_data, m_cached_raw_data);
		*m_saved_data = *m_data;
		*m_raw_data = *m_cached_raw_data;
	}
}

template<class T>
inline void DataManager::Entry<T>::AbandonChanges()
{
	*m_data = *m_saved_data;
	m_cached_raw_data->clear();
}

template<class T>
inline bool DataManager::Entry<T>::HasDataChanged() const
{
	return *m_orig_data != *m_data;
}

template<class T>
inline bool DataManager::Entry<T>::HasSavedDataChanged() const
{
	return m_saved_data != nullptr && *m_saved_data != *m_data;
}

template<class T>
inline std::shared_ptr<T> DataManager::Entry<T>::GetData()
{
	return m_data;
}

template<class T>
inline std::shared_ptr<const T> DataManager::Entry<T>::GetData() const
{
	return GetData();
}

template<class T>
inline std::shared_ptr<const T> DataManager::Entry<T>::GetOrigData() const
{
	return m_orig_data;
}

template<class T>
inline std::shared_ptr<const ByteVector> DataManager::Entry<T>::GetBytes()
{
	if (HasDataChanged())
	{
		Serialise(m_data, m_cached_raw_data);
		return m_cached_raw_data;
	}
	return m_raw_data;
}

template<class T>
inline std::shared_ptr<const ByteVector> DataManager::Entry<T>::GetOrigBytes() const
{
	return m_raw_data;
}

template<class T>
inline std::string DataManager::Entry<T>::GetName() const
{
	return m_name;
}

template<class T>
inline void DataManager::Entry<T>::SetName(const std::string& val)
{
	m_name = val;
}

template<class T>
inline std::filesystem::path DataManager::Entry<T>::GetFilename() const
{
	return m_filename;
}

template<class T>
inline void DataManager::Entry<T>::SetFilename(const std::filesystem::path& val)
{
	m_filename = val;
}

template<class T>
inline uint32_t DataManager::Entry<T>::GetStartAddress() const
{
	return m_begin_address;
}

template<class T>
inline uint32_t DataManager::Entry<T>::GetDataLength()
{
	if (HasDataChanged())
	{
		Serialise(m_data, m_cached_raw_data);
		return m_cached_raw_data->size();
	}
	return GetOrigDataLength();
}

template<class T>
inline uint32_t DataManager::Entry<T>::GetEndAddress()
{
	return m_begin_address + GetDataLength();
}

template<class T>
inline uint32_t DataManager::Entry<T>::GetOrigDataLength() const
{
	return m_raw_data->size();
}

template<class T>
inline uint32_t DataManager::Entry<T>::GetOrigEndAddress() const
{
	return m_begin_address + GetOrigDataLength();
}

template<class T>
inline void DataManager::Entry<T>::SetStartAddress(uint32_t addr)
{
	m_begin_address = addr;
}

template<class T>
inline bool DataManager::Entry<T>::Save(const std::filesystem::path& dir)
{
	auto bytes = GetBytes();
	auto fname = dir / m_filename;
	CreateDirectoryTree(fname);
	WriteBytes(*bytes, fname);
	return true;
}

#endif // _DATA_MANAGER_H_
