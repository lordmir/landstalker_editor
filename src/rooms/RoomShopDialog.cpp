#include <rooms/RoomShopDialog.h>

#include <algorithm>
#include <cmath>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/listbox.h>
#include <wx/msgdlg.h>
#include <wx/numdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>

#include <landstalker/main/RoomData.h>
#include <landstalker/main/ScriptData.h>
#include <landstalker/main/StringData.h>
#include <landstalker/misc/Utils.h>
#include <landstalker/script/ScriptTable.h>
#include <misc/LookupChoiceControl.h>
#include <misc/SpinCtrlSize.h>

using Landstalker::ScriptTable::Shop;
using Landstalker::ScriptTable::Item;

namespace
{
	// The shop markup byte encodes a percentage in 1/16 steps offset by -100% (16 == 0%). These
	// mirror the conversions the main script-table editor uses.
	double MarkupToPercent(uint8_t markup) { return static_cast<double>(markup) / 0.16 - 100.0; }
	uint8_t PercentToMarkup(double percent)
	{
		const long v = std::lround((percent + 100.0) * 0.16);
		return static_cast<uint8_t>(std::clamp<long>(v, 0, 255));
	}

	wxString ItemLabel(const std::shared_ptr<Landstalker::GameData>& gd, int item_id)
	{
		wxString name;
		if (gd && gd->GetStringData())
		{
			name = wxString::FromUTF8(Landstalker::wstr_to_utf8(gd->GetStringData()->GetItemDisplayName(item_id)));
		}
		return wxString::Format("Item %02X: %s", static_cast<unsigned>(item_id), name);
	}
}

RoomShopDialog::RoomShopDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd, int room)
	: wxDialog(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(820, 560),
		wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	m_gd(std::move(gd)),
	m_room(room)
{
	wxString name;
	if (m_gd && m_gd->GetRoomData() && room >= 0 && room < static_cast<int>(m_gd->GetRoomData()->GetRoomCount()))
	{
		name = wxString::FromUTF8(Landstalker::wstr_to_utf8(
			m_gd->GetRoomData()->GetRoomDisplayName(static_cast<uint16_t>(room))));
	}
	SetTitle(wxString::Format("Shop - Room %d %s", room, name));

	auto* sizer = new wxBoxSizer(wxVERTICAL);
	auto* top = new wxBoxSizer(wxHORIZONTAL);

	// Left: the room's shop + custom item entries, with add/remove.
	auto* left = new wxBoxSizer(wxVERTICAL);
	left->Add(new wxStaticText(this, wxID_ANY, "Entries:"), 0, wxBOTTOM, 2);
	m_list = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(210, -1));
	left->Add(m_list, 1, wxEXPAND);
	m_add_shop = new wxButton(this, wxID_ANY, "Add Shop");
	m_add_item = new wxButton(this, wxID_ANY, "Add Item...");
	m_remove = new wxButton(this, wxID_ANY, "Remove");
	left->Add(m_add_shop, 0, wxEXPAND | wxTOP, 3);
	left->Add(m_add_item, 0, wxEXPAND | wxTOP, 2);
	left->Add(m_remove, 0, wxEXPAND | wxTOP, 2);
	top->Add(left, 0, wxEXPAND | wxRIGHT, 6);

	// Right: context controls above the shared action-script tree editor.
	auto* right = new wxBoxSizer(wxVERTICAL);

	m_shop_panel = new wxPanel(this, wxID_ANY);
	auto* shop_sizer = new wxBoxSizer(wxHORIZONTAL);
	shop_sizer->Add(new wxStaticText(m_shop_panel, wxID_ANY, "Markup %:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 3);
	m_markup = new wxSpinCtrlDouble(m_shop_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, SpinCtrlSize(90),
		wxSP_ARROW_KEYS, -100.0, 1493.0, 0.0, 6.25);
	shop_sizer->Add(m_markup, 0, wxRIGHT, 12);
	shop_sizer->Add(new wxStaticText(m_shop_panel, wxID_ANY, "Lifestock Markup %:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 3);
	m_lifestock_markup = new wxSpinCtrlDouble(m_shop_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, SpinCtrlSize(90),
		wxSP_ARROW_KEYS, -100.0, 1493.0, 0.0, 6.25);
	shop_sizer->Add(m_lifestock_markup, 0);
	m_shop_panel->SetSizer(shop_sizer);
	right->Add(m_shop_panel, 0, wxEXPAND | wxBOTTOM, 4);

	m_item_panel = new wxPanel(this, wxID_ANY);
	auto* item_sizer = new wxBoxSizer(wxHORIZONTAL);
	m_item_label = new wxStaticText(m_item_panel, wxID_ANY, wxEmptyString);
	item_sizer->Add(m_item_label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);
	m_extra_check = new wxCheckBox(m_item_panel, wxID_ANY, "Additional data word:");
	item_sizer->Add(m_extra_check, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 3);
	m_extra_data = new wxSpinCtrl(m_item_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, SpinCtrlSize(80),
		wxSP_ARROW_KEYS, 0, 0xFFFF, 0);
	m_extra_data->SetBase(16);
	item_sizer->Add(m_extra_data, 0);
	m_item_panel->SetSizer(item_sizer);
	right->Add(m_item_panel, 0, wxEXPAND | wxBOTTOM, 4);

	m_tree = new ScriptTableTreeEditorCtrl(this, nullptr, false);
	m_tree->SetGameData(m_gd);
	right->Add(m_tree, 1, wxEXPAND);
	top->Add(right, 1, wxEXPAND);

	sizer->Add(top, 1, wxEXPAND | wxALL, 6);
	sizer->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 6);
	SetSizer(sizer);

	m_list->Bind(wxEVT_LISTBOX, [this](wxCommandEvent&) { OnListSelect(); });
	m_add_shop->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnAddShop(); });
	m_add_item->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnAddItem(); });
	m_remove->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnRemove(); });
	m_markup->Bind(wxEVT_SPINCTRLDOUBLE, [this](wxSpinDoubleEvent&) { OnMarkupChange(); });
	m_lifestock_markup->Bind(wxEVT_SPINCTRLDOUBLE, [this](wxSpinDoubleEvent&) { OnMarkupChange(); });
	m_extra_check->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { OnExtraToggle(); });
	m_extra_data->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { OnExtraDataChange(); });
	Bind(wxEVT_BUTTON, &RoomShopDialog::OnOk, this, wxID_OK);

	Rebuild();
}

int RoomShopDialog::RoomShopIndex() const
{
	auto sd = m_gd ? m_gd->GetScriptData() : nullptr;
	auto shops = sd ? sd->GetShopTable() : nullptr;
	if (shops)
	{
		for (int i = 0; i < static_cast<int>(shops->size()); ++i)
		{
			if (shops->at(i).room == m_room) return i;
		}
	}
	return -1;
}

bool RoomShopDialog::HasItem(int item_id) const
{
	auto sd = m_gd ? m_gd->GetScriptData() : nullptr;
	auto items = sd ? sd->GetItemTable() : nullptr;
	if (items)
	{
		for (const auto& it : *items)
		{
			if (static_cast<int>(it.shop) == m_room && static_cast<int>(it.item) == item_id) return true;
		}
	}
	return false;
}

void RoomShopDialog::Rebuild(int select_row)
{
	m_entries.clear();
	m_list->Clear();
	auto sd = m_gd ? m_gd->GetScriptData() : nullptr;
	if (sd)
	{
		const int shop_idx = RoomShopIndex();
		if (shop_idx >= 0)
		{
			m_entries.push_back({ ScriptTableTreeCategory::SHOP, shop_idx });
			m_list->Append(wxString::Format("Shop (Room %d)", m_room));
		}
		auto items = sd->GetItemTable();
		if (items)
		{
			for (int i = 0; i < static_cast<int>(items->size()); ++i)
			{
				if (static_cast<int>(items->at(i).shop) == m_room)
				{
					m_entries.push_back({ ScriptTableTreeCategory::ITEM, i });
					m_list->Append(ItemLabel(m_gd, items->at(i).item));
				}
			}
		}
	}
	const bool any = !m_entries.empty();
	m_tree->Show(any);
	m_shop_panel->Show(false);
	m_item_panel->Show(false);
	if (any)
	{
		m_list->SetSelection(std::clamp(select_row, 0, static_cast<int>(m_entries.size()) - 1));
		ShowSelected();
	}
	UpdateButtons();
	Layout();
}

void RoomShopDialog::ShowSelected()
{
	const int row = m_list->GetSelection();
	if (row == wxNOT_FOUND || row >= static_cast<int>(m_entries.size()))
	{
		return;
	}
	const Entry e = m_entries[static_cast<std::size_t>(row)];
	m_tree->Open(e.category, e.index);
	auto sd = m_gd->GetScriptData();
	if (e.category == ScriptTableTreeCategory::SHOP)
	{
		const auto& shop = sd->GetShopTable()->at(e.index);
		m_markup->SetValue(MarkupToPercent(shop.markup));
		m_lifestock_markup->SetValue(MarkupToPercent(shop.lifestock_markup));
		m_shop_panel->Show(true);
		m_item_panel->Show(false);
	}
	else
	{
		const auto& item = sd->GetItemTable()->at(e.index);
		m_item_label->SetLabel(ItemLabel(m_gd, item.item));
		m_extra_check->SetValue(item.other.has_value());
		m_extra_data->Enable(item.other.has_value());
		m_extra_data->SetValue(static_cast<int>(item.other.value_or(0)));
		m_shop_panel->Show(false);
		m_item_panel->Show(true);
	}
	Layout();
}

void RoomShopDialog::UpdateButtons()
{
	m_add_shop->Enable(m_gd && m_gd->GetScriptData() && RoomShopIndex() < 0);
	m_add_item->Enable(m_gd && m_gd->GetScriptData() != nullptr);
	m_remove->Enable(m_list->GetSelection() != wxNOT_FOUND);
}

void RoomShopDialog::OnListSelect()
{
	m_tree->CommitTreeEditing();
	ShowSelected();
	UpdateButtons();
}

void RoomShopDialog::OnAddShop()
{
	auto sd = m_gd ? m_gd->GetScriptData() : nullptr;
	if (!sd || RoomShopIndex() >= 0)
	{
		return;
	}
	Shop shop;
	shop.room = static_cast<uint16_t>(m_room);
	shop.markup = 16;           // 0% by default
	shop.lifestock_markup = 16;
	sd->GetShopTable()->push_back(shop);
	Rebuild(0); // the shop is always the first row
}

void RoomShopDialog::OnAddItem()
{
	auto sd = m_gd ? m_gd->GetScriptData() : nullptr;
	if (!sd)
	{
		return;
	}
	// Pick the item from a searchable name dropdown (choice index == item id).
	wxArrayString choices;
	auto strd = m_gd->GetStringData();
	for (std::size_t i = 0; strd && i < strd->GetItemNameCount(); ++i)
	{
		choices.Add(wxString::Format("%02X: %s", static_cast<unsigned>(i),
			wxString::FromUTF8(Landstalker::wstr_to_utf8(strd->GetItemDisplayName(static_cast<int>(i))))));
	}
	if (choices.IsEmpty())
	{
		return;
	}
	wxDialog picker(this, wxID_ANY, "Add Custom Item", wxDefaultPosition, wxSize(320, -1));
	auto* ps = new wxBoxSizer(wxVERTICAL);
	ps->Add(new wxStaticText(&picker, wxID_ANY, "Item:"), 0, wxLEFT | wxTOP, 8);
	auto* pick = new LookupChoiceControl(&picker, wxID_ANY, choices[0], choices);
	ps->Add(pick, 0, wxEXPAND | wxALL, 8);
	ps->Add(picker.CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 8);
	picker.SetSizerAndFit(ps);
	if (picker.ShowModal() != wxID_OK)
	{
		return;
	}
	pick->CommitPendingSelection();
	const long id = pick->GetSelection();
	if (id < 0)
	{
		return;
	}
	if (HasItem(static_cast<int>(id)))
	{
		wxMessageBox("This room already has a custom item script for that item.", "Add Custom Item",
			wxOK | wxICON_INFORMATION, this);
		return;
	}
	Item item;
	item.item = static_cast<uint8_t>(id);
	item.shop = static_cast<uint16_t>(m_room);
	sd->GetItemTable()->push_back(item);
	Rebuild(static_cast<int>(m_entries.size())); // clamps to the new (last) row
}

void RoomShopDialog::OnRemove()
{
	const int row = m_list->GetSelection();
	if (row == wxNOT_FOUND || row >= static_cast<int>(m_entries.size()))
	{
		return;
	}
	if (wxMessageBox("Remove the selected entry? This cannot be undone.", "Remove Entry",
		wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION, this) != wxYES)
	{
		return;
	}
	const Entry e = m_entries[static_cast<std::size_t>(row)];
	m_tree->CommitTreeEditing();
	auto sd = m_gd->GetScriptData();
	if (e.category == ScriptTableTreeCategory::SHOP)
	{
		sd->GetShopTable()->erase(sd->GetShopTable()->begin() + e.index);
	}
	else
	{
		sd->GetItemTable()->erase(sd->GetItemTable()->begin() + e.index);
	}
	Rebuild(std::max(0, row - 1));
}

void RoomShopDialog::OnMarkupChange()
{
	const int row = m_list->GetSelection();
	if (row == wxNOT_FOUND || m_entries[static_cast<std::size_t>(row)].category != ScriptTableTreeCategory::SHOP)
	{
		return;
	}
	auto& shop = m_gd->GetScriptData()->GetShopTable()->at(m_entries[static_cast<std::size_t>(row)].index);
	shop.markup = PercentToMarkup(m_markup->GetValue());
	shop.lifestock_markup = PercentToMarkup(m_lifestock_markup->GetValue());
}

void RoomShopDialog::OnExtraToggle()
{
	const int row = m_list->GetSelection();
	if (row == wxNOT_FOUND || m_entries[static_cast<std::size_t>(row)].category != ScriptTableTreeCategory::ITEM)
	{
		return;
	}
	auto& item = m_gd->GetScriptData()->GetItemTable()->at(m_entries[static_cast<std::size_t>(row)].index);
	const bool on = m_extra_check->GetValue();
	m_extra_data->Enable(on);
	item.other = on ? std::optional<uint16_t>(static_cast<uint16_t>(m_extra_data->GetValue())) : std::nullopt;
}

void RoomShopDialog::OnExtraDataChange()
{
	const int row = m_list->GetSelection();
	if (row == wxNOT_FOUND || m_entries[static_cast<std::size_t>(row)].category != ScriptTableTreeCategory::ITEM)
	{
		return;
	}
	auto& item = m_gd->GetScriptData()->GetItemTable()->at(m_entries[static_cast<std::size_t>(row)].index);
	if (m_extra_check->GetValue())
	{
		item.other = static_cast<uint16_t>(m_extra_data->GetValue());
	}
}

void RoomShopDialog::OnOk(wxCommandEvent& evt)
{
	m_tree->CommitTreeEditing();
	evt.Skip();
}
