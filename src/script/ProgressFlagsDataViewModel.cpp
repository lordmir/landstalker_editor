#include <script/ProgressFlagsDataViewModel.h>
#include <misc/LookupDataViewRenderer.h>

#include <numeric>

namespace
{
// Progress flags store the raw flag ID directly (uint16_t, 0xFFFF sentinel for "none") with no
// clamp in ProgressFlags.cpp, unlike e.g. SET_FLAG's [0, 999] - matches the broader 0-2047 range
// FlagDataViewModel::GetSharedFlagChoices() uses elsewhere. "<NONE>" is appended past the end
// (index 2048) so a bare typed flag number still resolves directly to its own index.
const wxArrayString& GetProgressFlagChoices(const std::shared_ptr<Landstalker::GameData>& gd)
{
	static const Landstalker::GameData* s_cached_gd = nullptr;
	static wxArrayString s_choices;

	if (gd.get() != s_cached_gd || s_choices.GetCount() != 2049)
	{
		s_cached_gd = gd.get();
		s_choices.Clear();
		s_choices.Alloc(2049);
		for (int i = 0; i < 2048; ++i)
		{
			s_choices.Add(wxString(gd->GetScriptData()->GetFlagDisplayName(i)));
		}
		s_choices.Add(_("<NONE>"));
	}

	return s_choices;
}
}

ProgressFlagsDataViewModel::ProgressFlagsDataViewModel(std::shared_ptr<Landstalker::GameData> gd)
	: BaseDataViewModel(),
	  m_gd(gd)
{
	Initialise();
}

ProgressFlagsDataViewModel::~ProgressFlagsDataViewModel()
{
}

void ProgressFlagsDataViewModel::Initialise()
{
	if (m_gd && m_gd->GetScriptData()->HasTables())
	{
		m_flags = Landstalker::ProgressFlags::GetFlags(*m_gd->GetScriptData()->GetProgressFlagsFuncs());
	}
	else
	{
		m_flags.clear();
	}
	Reset(GetRowCount());
}

void ProgressFlagsDataViewModel::CommitData()
{
	if (m_gd && m_gd->GetScriptData()->HasTables())
	{
		m_gd->GetScriptData()->SetProgressFlagsFuncs(Landstalker::ProgressFlags::MakeAsm(m_flags));
	}
}

unsigned int ProgressFlagsDataViewModel::GetColumnCount() const
{
	return 3;
}

unsigned int ProgressFlagsDataViewModel::GetRowCount() const
{
	return std::accumulate(m_flags.cbegin(), m_flags.cend(), 0U, [](unsigned int sum, const auto& q) {return sum + q.second.size(); });
}

std::pair<int, int> ProgressFlagsDataViewModel::GetQuestProgressFromRow(int row) const
{
	int quest = -1;
	int progress = -1;
	for (const auto& qflag : m_flags)
	{
		quest = qflag.first;
		if (row < static_cast<int>(qflag.second.size()))
		{
			progress = row;
			break;
		}
		row -= qflag.second.size();
	}
	return { quest, progress };
}

int ProgressFlagsDataViewModel::GetTotalQuests() const
{
	return m_flags.size();
}

int ProgressFlagsDataViewModel::GetMaxQuest() const
{
	return m_flags.empty() ? -1 : m_flags.rbegin()->first;
}

int ProgressFlagsDataViewModel::GetNextFreeQuest() const
{
	if (m_flags.empty())
	{
		return 0;
	}
	else
	{
		for (int i = 0; i < 256; ++i)
		{
			if (m_flags.count(i) == 0)
			{
				return i;
			}
		}
		return -1;
	}
}

int ProgressFlagsDataViewModel::GetRowFromQuestProgress(int quest, int progress) const
{
	int row = 0;
	if (m_flags.count(quest) == 0 || m_flags.at(quest).size() <= static_cast<std::size_t>(progress))
	{
		return -1;
	}
	for (int q = 0; q < quest; ++q)
	{
		if (m_flags.count(q) == 0)
		{
			continue;
		}
		row += m_flags.at(q).size();
	}
	return row + progress;
}

int ProgressFlagsDataViewModel::GetTotalProgressInQuest(int quest) const
{
	if (m_flags.count(quest) == 0)
	{
		return -1;
	}
	return  m_flags.at(quest).size();
}

wxString ProgressFlagsDataViewModel::GetColumnHeader(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return _("Quest");
	case 1:
		return _("Progress");
	case 2:
		return _("Flag");
	default:
		return "???";
	}
}

wxArrayString ProgressFlagsDataViewModel::GetColumnChoices(unsigned int col) const
{
	if (col == 2)
	{
		return GetProgressFlagChoices(m_gd);
	}
	return wxArrayString();
}

wxString ProgressFlagsDataViewModel::GetColumnType(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "long";
	case 1:
		return "string";
	case 2:
		return "long";
	default:
		return "???";
	}
}

void ProgressFlagsDataViewModel::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const
{
	auto [quest, progress] = GetQuestProgressFromRow(row);
	if (quest == -1)
	{
		return;
	}
	long flag = m_flags.at(quest).at(progress);
	switch (col)
	{
	case 0:
		variant = static_cast<long>(quest);
		break;
	case 1:
		variant = Landstalker::StrPrintf("%d/%d (%d%%)", progress + 1, GetTotalProgressInQuest(quest),
			static_cast<int>(100.0 * (progress + 1) / GetTotalProgressInQuest(quest)));
		break;
	case 2:
		variant = static_cast<long>(flag == 0xFFFF ? 2048 : flag);
		break;
	default:
		break;
	}
}

bool ProgressFlagsDataViewModel::GetAttrByRow(unsigned int /*row*/, unsigned int /*col*/, wxDataViewItemAttr& /*attr*/) const
{
	return false;
}

bool ProgressFlagsDataViewModel::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col)
{
	auto [quest, progress] = GetQuestProgressFromRow(row);
	if (quest == -1)
	{
		return false;
	}
	switch (col)
	{
	case 2:
	{
		const long idx = variant.GetLong();
		m_flags[quest][progress] = (idx < 0 || idx >= 2048) ? 0xFFFF : static_cast<uint16_t>(idx);
		break;
	}
	default:
		return false;
	}
	CommitData();
	return true;
}

bool ProgressFlagsDataViewModel::DeleteRow(unsigned int row)
{
	auto [q, p] = GetQuestProgressFromRow(row);
	if (q < 0 || (m_flags.size() == 1 && m_flags.at(q).size() == 1))
	{
		return false;
	}
	auto& vec = m_flags.at(q);
	vec.erase(vec.begin() + p);
	if (vec.size() == 0)
	{
		m_flags.erase(q);
	}
	RowDeleted(row);
	CommitData();
	return false;
}

bool ProgressFlagsDataViewModel::AddRow(unsigned int row)
{
	auto [q, p] = GetQuestProgressFromRow(row);
	if (q < 0)
	{
		return false;
	}
	auto& vec = m_flags.at(q);
	vec.insert(vec.begin() + p, 0xFFFF);
	RowInserted(row);
	CommitData();
	return false;
}

bool ProgressFlagsDataViewModel::AppendRow(unsigned int row)
{
	auto [q, p] = GetQuestProgressFromRow(row);
	if (q < 0)
	{
		return false;
	}
	m_flags.at(q).push_back(0xFFFF);
	RowInserted(GetRowFromQuestProgress(q, m_flags.at(q).size() - 1));
	CommitData();
	return false;
}

bool ProgressFlagsDataViewModel::DeleteQuest(unsigned int row)
{
	if (GetTotalQuests() < 1)
	{
		return false;
	}
	auto [q, p] = GetQuestProgressFromRow(row);
	if (q >= 0)
	{
		m_flags.erase(q);
		CommitData();
		Reset(GetRowCount());
		return true;
	}

	return false;
}

bool ProgressFlagsDataViewModel::AddQuest()
{
	int next_quest = GetNextFreeQuest();
	if (next_quest == -1)
	{
		return false;
	}
	m_flags.emplace(next_quest, std::vector<uint16_t>{ 0xFFFF });
	CommitData();
	RowInserted(GetRowCount() - 1);

	return true;
}

bool ProgressFlagsDataViewModel::SwapRows(unsigned int r1, unsigned int r2)
{
	auto [q1, p1] = GetQuestProgressFromRow(r1);
	auto [q2, p2] = GetQuestProgressFromRow(r2);
	if (q1 != q2 || p1 == p2 || q1 < 0)
	{
		return false;
	}
	std::swap(m_flags[q1][p1], m_flags[q2][p2]);
	RowChanged(r1);
	RowChanged(r2);
	CommitData();
	return true;
}

void ProgressFlagsDataViewModel::InitControl(wxDataViewCtrl* ctrl) const
{
	// Quest
	ctrl->InsertColumn(0, new wxDataViewColumn(this->GetColumnHeader(0),
		new wxDataViewTextRenderer(GetColumnType(0)), 0, 80, wxALIGN_LEFT));
	// Progress
	ctrl->InsertColumn(1, new wxDataViewColumn(this->GetColumnHeader(1),
		new wxDataViewTextRenderer(GetColumnType(1)), 1, 120, wxALIGN_LEFT));
	// Flag (searchable name combo - replaces the old separate Flag spin + Description text columns)
	ctrl->InsertColumn(2, new wxDataViewColumn(this->GetColumnHeader(2),
		new LookupDataViewRenderer(wxDATAVIEW_CELL_EDITABLE, GetProgressFlagChoices(m_gd)), 2, 320, wxALIGN_LEFT));
}
