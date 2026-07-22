#include <rooms/RoomConstantsCtrl.h>
#include <misc/DataViewModelAssociate.h>

RoomConstantsEditorCtrl::RoomConstantsEditorCtrl(wxWindow* parent, ImageList* imglst)
	: wxPanel(parent),
	  m_model(nullptr)
{
	const int plus_img = imglst->GetIdx("plus");
	const int minus_img = imglst->GetIdx("minus");

	wxBoxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
	this->SetSizer(vsizer);

	m_dvc_ctrl = new wxDataViewCtrl(this, wxID_ANY);
	vsizer->Add(m_dvc_ctrl, 1, wxALL | wxEXPAND, 5);

	wxBoxSizer* btnsizer = new wxBoxSizer(wxHORIZONTAL);
	vsizer->Add(btnsizer, 0, wxEXPAND, 5);
	m_ctrl_add = new wxBitmapButton(this, wxID_ADD, imglst->GetBitmap(plus_img), wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW);
	m_ctrl_delete = new wxBitmapButton(this, wxID_DELETE, imglst->GetBitmap(minus_img), wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW);
	m_ctrl_add->SetToolTip(_("Add a new room constant"));
	m_ctrl_delete->SetToolTip(_("Delete the selected room constant"));
	btnsizer->Add(m_ctrl_add, 0, wxLEFT | wxBOTTOM | wxALIGN_CENTER_VERTICAL, 5);
	btnsizer->Add(m_ctrl_delete, 0, wxLEFT | wxBOTTOM | wxALIGN_CENTER_VERTICAL, 5);

	GetSizer()->Fit(this);

	m_ctrl_add->Connect(wxEVT_BUTTON, wxCommandEventHandler(RoomConstantsEditorCtrl::OnAdd), nullptr, this);
	m_ctrl_delete->Connect(wxEVT_BUTTON, wxCommandEventHandler(RoomConstantsEditorCtrl::OnDelete), nullptr, this);
	m_dvc_ctrl->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler(RoomConstantsEditorCtrl::OnSelectionChange), nullptr, this);
}

RoomConstantsEditorCtrl::~RoomConstantsEditorCtrl()
{
	m_ctrl_add->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(RoomConstantsEditorCtrl::OnAdd), nullptr, this);
	m_ctrl_delete->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(RoomConstantsEditorCtrl::OnDelete), nullptr, this);
	m_dvc_ctrl->Disconnect(wxEVT_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler(RoomConstantsEditorCtrl::OnSelectionChange), nullptr, this);
}

void RoomConstantsEditorCtrl::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	m_gd = gd;
	m_dvc_ctrl->ClearColumns();
	AssociateDataViewModel(m_dvc_ctrl, nullptr);
	m_model = new RoomConstantsDataViewModel(gd);

	m_model->Initialise();
	AssociateDataViewModel(m_dvc_ctrl, m_model);
	m_model->DecRef();
	m_model->InitControl(m_dvc_ctrl);
	m_dvc_ctrl->SetSelections({});
	UpdateUI();
}

void RoomConstantsEditorCtrl::ClearGameData()
{
	m_gd.reset();
	m_dvc_ctrl->ClearColumns();
	AssociateDataViewModel(m_dvc_ctrl, nullptr);
	m_model = nullptr;
	UpdateUI();
}

void RoomConstantsEditorCtrl::Open(int row)
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
		if (row >= 0)
		{
			m_dvc_ctrl->EnsureVisible(wxDataViewItem(reinterpret_cast<void*>(static_cast<intptr_t>(row + 1))));
		}
	}
}

void RoomConstantsEditorCtrl::RefreshData()
{
	if (m_model)
	{
		// The room list may have grown since the page was last shown, so rebuild the
		// choices rather than just resetting the row count.
		m_model->Initialise();
		UpdateUI();
	}
}

void RoomConstantsEditorCtrl::UpdateUI()
{
	const bool has_data = m_gd != nullptr && m_model != nullptr;
	m_ctrl_add->Enable(has_data);
	m_ctrl_delete->Enable(has_data && m_dvc_ctrl->HasSelection());
}

void RoomConstantsEditorCtrl::OnAdd(wxCommandEvent& evt)
{
	if (m_model)
	{
		const unsigned int row = m_model->GetRowCount();
		if (m_model->AddRow(row))
		{
			// Deferred: the control has not processed the row insertion yet.
			CallAfter([this, row]()
			{
				const auto item = wxDataViewItem(reinterpret_cast<void*>(static_cast<intptr_t>(row + 1)));
				m_dvc_ctrl->Select(item);
				m_dvc_ctrl->EnsureVisible(item);
				UpdateUI();
			});
		}
	}
	evt.Skip();
}

void RoomConstantsEditorCtrl::OnDelete(wxCommandEvent& evt)
{
	if (m_model && m_dvc_ctrl->HasSelection())
	{
		const auto selected_id = reinterpret_cast<intptr_t>(m_dvc_ctrl->GetSelection().GetID());
		if (selected_id > 0)
		{
			const unsigned int row = static_cast<unsigned int>(selected_id - 1);
			if (m_model->DeleteRow(row))
			{
				const unsigned int row_count = m_model->GetRowCount();
				CallAfter([this, row, row_count]()
				{
					if (row_count == 0)
					{
						m_dvc_ctrl->UnselectAll();
					}
					else
					{
						const auto next = row < row_count ? row : row_count - 1;
						m_dvc_ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(static_cast<intptr_t>(next + 1))));
					}
					UpdateUI();
				});
			}
		}
	}
	evt.Skip();
}

void RoomConstantsEditorCtrl::OnSelectionChange(wxDataViewEvent& evt)
{
	UpdateUI();
	evt.Skip();
}
