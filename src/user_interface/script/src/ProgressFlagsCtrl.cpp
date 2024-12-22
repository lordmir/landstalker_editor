#include <user_interface/script/include/ProgressFlagsCtrl.h>
#include <user_interface/script/include/ProgressFlagsFrame.h>

wxBEGIN_EVENT_TABLE(ProgressFlagsEditorCtrl, wxPanel)
EVT_DATAVIEW_SELECTION_CHANGED(wxID_ANY, ProgressFlagsEditorCtrl::OnSelectionChange)
wxEND_EVENT_TABLE()

ProgressFlagsEditorCtrl::ProgressFlagsEditorCtrl(wxWindow* parent)
	: wxPanel(parent),
	  m_model(nullptr)
{
	wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
	this->SetSizer(hsizer);

	m_dvc_ctrl = new wxDataViewCtrl(this, wxID_ANY);
	hsizer->Add(m_dvc_ctrl, 1, wxALL | wxEXPAND, 5);

	GetSizer()->Fit(this);
}

ProgressFlagsEditorCtrl::~ProgressFlagsEditorCtrl()
{
}

void ProgressFlagsEditorCtrl::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
	m_dvc_ctrl->ClearColumns();
	m_dvc_ctrl->AssociateModel(nullptr);
	m_model = new ProgressFlagsDataViewModel(gd);

	m_model->Initialise();
	m_dvc_ctrl->AssociateModel(m_model);
	m_model->DecRef();
	m_model->InitControl(m_dvc_ctrl);
	m_dvc_ctrl->SetSelections({});
	UpdateUI();
}

void ProgressFlagsEditorCtrl::ClearGameData()
{
	m_gd.reset();
	m_dvc_ctrl->ClearColumns();
}

void ProgressFlagsEditorCtrl::Open(int /*quest*/, int /*prog*/)
{
	if (m_model)
	{
		Freeze();
		for (unsigned int i = 0; i < m_dvc_ctrl->GetColumnCount(); ++i)
		{
			m_dvc_ctrl->GetColumn(i)->SetTitle(m_model->GetColumnHeader(i));
		}
		Thaw();
		UpdateUI();
	}
}

void ProgressFlagsEditorCtrl::RefreshData()
{
	m_model->Reset(m_model->GetRowCount());
	UpdateUI();
}

void ProgressFlagsEditorCtrl::AppendRow()
{
	m_model->AddRow(m_model->GetRowCount());
	UpdateUI();
	m_dvc_ctrl->SetCurrentItem(wxDataViewItem(reinterpret_cast<void*>(m_model->GetRowCount())));
	m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
}

void ProgressFlagsEditorCtrl::InsertRow()
{
	if (IsRowSelected())
	{
		m_model->AddRow(reinterpret_cast<std::intptr_t>(m_dvc_ctrl->GetSelection().GetID()) - 1);
	}
	else
	{
		m_model->AddRow(0);
		m_dvc_ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(1)));
		m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
	}
	UpdateUI();
}

void ProgressFlagsEditorCtrl::DeleteRow()
{
	if (IsRowSelected())
	{
		std::size_t sel = reinterpret_cast<std::intptr_t>(m_dvc_ctrl->GetSelection().GetID()) - 1;
		m_model->DeleteRow(sel);
		if (m_model->GetRowCount() > sel)
		{
			m_dvc_ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(sel + 1)));
			m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
		}
		else if (m_model->GetRowCount() != 0)
		{
			m_dvc_ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(m_model->GetRowCount())));
			m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
		}
	}
	UpdateUI();
}

void ProgressFlagsEditorCtrl::MoveRowUp()
{
	if (IsRowSelected() && m_model->GetRowCount() >= 2)
	{
		std::size_t sel = reinterpret_cast<std::intptr_t>(m_dvc_ctrl->GetSelection().GetID()) - 1;
		if (sel > 0)
		{
			m_model->SwapRows(sel - 1, sel);
			m_dvc_ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(sel)));
			m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
		}
	}
	UpdateUI();
}

void ProgressFlagsEditorCtrl::MoveRowDown()
{
	if (IsRowSelected() && m_model->GetRowCount() >= 2)
	{
		std::size_t sel = reinterpret_cast<std::intptr_t>(m_dvc_ctrl->GetSelection().GetID()) - 1;
		if (sel < m_model->GetRowCount() - 1)
		{
			m_model->SwapRows(sel, sel + 1);
			m_dvc_ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(sel + 2)));
			m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
		}
	}
	UpdateUI();
}

bool ProgressFlagsEditorCtrl::IsRowSelected() const
{
	return m_dvc_ctrl->HasSelection();
}

bool ProgressFlagsEditorCtrl::IsSelTop() const
{
	return reinterpret_cast<std::intptr_t>(m_dvc_ctrl->GetSelection().GetID()) < 2;
}

bool ProgressFlagsEditorCtrl::IsSelBottom() const
{
	return reinterpret_cast<std::intptr_t>(m_dvc_ctrl->GetSelection().GetID()) >= static_cast<intptr_t>(m_model->GetRowCount());
}

void ProgressFlagsEditorCtrl::OnSelectionChange(wxDataViewEvent& evt)
{
	UpdateUI();
	evt.Skip();
}

void ProgressFlagsEditorCtrl::UpdateUI()
{
	m_model->CommitData();
	static_cast<ProgressFlagsEditorFrame*>(GetParent())->UpdateUI();
}

void ProgressFlagsEditorCtrl::FireEvent(const wxEventType& e, const wxString& data, long numeric_data, long extra_numeric_data, long extra_extra_numeric_data)
{
	wxCommandEvent evt(e);
	evt.SetString(data);
	evt.SetInt(numeric_data);
	evt.SetExtraLong(extra_numeric_data);
	evt.SetId(extra_extra_numeric_data);
	evt.SetClientData(this);
	wxPostEvent(this->GetParent(), evt);
}
