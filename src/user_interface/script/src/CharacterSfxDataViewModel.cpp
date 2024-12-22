#include <user_interface/script/include/CharacterSfxDataViewModel.h>

CharacterSfxDataViewModel::CharacterSfxDataViewModel(std::shared_ptr<GameData> gd)
	: BaseDataViewModel(),
	  m_gd(gd)
{
	Initialise();
}

CharacterSfxDataViewModel::~CharacterSfxDataViewModel()
{
}

void CharacterSfxDataViewModel::Initialise()
{
	m_choices.Clear();
	m_choices.reserve(256);
	for (std::size_t i = 0; i < 256; ++i)
	{
		auto name = Labels::Get(Labels::C_SOUNDS, i);
		m_choices.push_back(name.value_or(StrWPrintf("<%02X> ???", i)));
	}
	Reset(GetRowCount());
}

void CharacterSfxDataViewModel::CommitData()
{
}

unsigned int CharacterSfxDataViewModel::GetColumnCount() const
{
	return 2;
}

unsigned int CharacterSfxDataViewModel::GetRowCount() const
{
	if (!m_gd)
	{
		return 0;
	}
	else
	{
		return m_gd->GetStringData()->GetSpecialCharNameCount();
	}
}

wxString CharacterSfxDataViewModel::GetColumnHeader(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return _("Character");
	case 1:
		return _("Sound Effect");
	default:
		return "???";
	}
}

wxArrayString CharacterSfxDataViewModel::GetColumnChoices(unsigned int col) const
{
	switch (col)
	{
	case 1:
		return m_choices;
	default:
		return wxArrayString();
	}
}

wxString CharacterSfxDataViewModel::GetColumnType(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "string";
	case 1:
		return "long";
	default:
		return "???";
	}
}

void CharacterSfxDataViewModel::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const
{
	if (!m_gd || row >= m_gd->GetStringData()->GetSpecialCharNameCount())
	{
		return;
	}
	switch (col)
	{
	case 0:
		variant = StrWPrintf(L"(%02d) %ls", row, m_gd->GetStringData()->GetSpecialCharName(row).c_str());
		break;
	case 1:
		variant = static_cast<long>(m_gd->GetStringData()->GetSpecialCharTalkSfx(row));
		break;
	default:
		break;
	}
}

bool CharacterSfxDataViewModel::GetAttrByRow(unsigned int /*row*/, unsigned int /*col*/, wxDataViewItemAttr& /*attr*/) const
{
	return false;
}

bool CharacterSfxDataViewModel::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col)
{
	if (!m_gd || row >= m_gd->GetStringData()->GetSpecialCharNameCount())
	{
		return false;
	}
	switch (col)
	{
	case 1:
		m_gd->GetStringData()->SetSpecialCharTalkSfx(row, static_cast<uint8_t>(variant.GetLong()));
		return true;
	default:
		break;
	}
	return false;
}

bool CharacterSfxDataViewModel::DeleteRow(unsigned int /*row*/)
{
	return false;
}

bool CharacterSfxDataViewModel::AddRow(unsigned int /*row*/)
{
	return false;
}

bool CharacterSfxDataViewModel::SwapRows(unsigned int /*r1*/, unsigned int /*r2*/)
{
	return false;
}

void CharacterSfxDataViewModel::InitControl(wxDataViewCtrl* ctrl) const
{
	// Character
	ctrl->InsertColumn(0, new wxDataViewColumn(this->GetColumnHeader(0),
		new wxDataViewTextRenderer(GetColumnType(0)), 0, 200, wxALIGN_LEFT));
	// Sound
	ctrl->InsertColumn(1, new wxDataViewColumn(this->GetColumnHeader(1),
		new wxDataViewChoiceByIndexRenderer(GetColumnChoices(1)), 1, 200, wxALIGN_LEFT));
}
