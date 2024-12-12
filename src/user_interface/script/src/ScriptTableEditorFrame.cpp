#include <user_interface/script/include/ScriptTableEditorFrame.h>
#include <user_interface/script/include/ScriptTableEditorCtrl.h>
#include <user_interface/main/include/BrowserTreeCtrl.h>

#include <wx/propgrid/advprops.h>

enum TOOL_IDS
{
	ID_APPEND = 30000,
	ID_INSERT,
	ID_DELETE,
	ID_MOVE_UP,
	ID_MOVE_DOWN,
	ID_NEW_COLLECTION,
	ID_DELETE_COLLECTION
};

enum MENU_IDS
{
	ID_FILE_EXPORT_YML = 20000,
	ID_FILE_IMPORT_YML
};

ScriptTableEditorFrame::ScriptTableEditorFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst)
{
	m_mgr.SetManagedWindow(this);
	m_editor = new ScriptTableEditorCtrl(this);

	// add the panes to the manager
	m_mgr.AddPane(m_editor, wxAuiPaneInfo().CenterPane());

	// tell the manager to "commit" all the changes just made
	m_mgr.Update();
}

ScriptTableEditorFrame::~ScriptTableEditorFrame()
{
}

bool ScriptTableEditorFrame::Open(const ScriptTableDataViewModel::Mode& mode, unsigned int index, int row)
{
	if (m_gd)
	{
		m_editor->Open(mode, index, row);
		m_mode = mode;
		m_index = index;
		m_reset_props = true;
		FireEvent(EVT_PROPERTIES_UPDATE);
		UpdateUI();
		return true;
	}
	return false;
}

void ScriptTableEditorFrame::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
	m_editor->SetGameData(gd);
}

void ScriptTableEditorFrame::ClearGameData()
{
	m_editor->ClearGameData();
	m_gd.reset();
	m_reset_props = true;
	FireEvent(EVT_PROPERTIES_UPDATE);
	UpdateUI();
}

void ScriptTableEditorFrame::UpdateUI() const
{
	EnableToolbarItem("Script", ID_APPEND, m_gd != nullptr && m_mode != ScriptTableDataViewModel::Mode::SHOP);
	EnableToolbarItem("Script", ID_INSERT, m_gd != nullptr && m_mode != ScriptTableDataViewModel::Mode::SHOP);
	EnableToolbarItem("Script", ID_DELETE, m_gd != nullptr && m_mode != ScriptTableDataViewModel::Mode::SHOP && m_editor->IsRowSelected());
	EnableToolbarItem("Script", ID_MOVE_UP, m_gd != nullptr && m_editor->IsRowSelected() && !m_editor->IsSelTop());
	EnableToolbarItem("Script", ID_MOVE_DOWN, m_gd != nullptr && m_editor->IsRowSelected() && !m_editor->IsSelBottom());
	EnableToolbarItem("Script", ID_NEW_COLLECTION, m_gd != nullptr && (m_mode == ScriptTableDataViewModel::Mode::SHOP || m_mode == ScriptTableDataViewModel::Mode::ITEM));
	EnableToolbarItem("Script", ID_DELETE_COLLECTION, m_gd != nullptr && 
		((m_mode == ScriptTableDataViewModel::Mode::SHOP && m_index < m_gd->GetScriptData()->GetShopTable()->size() && m_gd->GetScriptData()->GetShopTable()->size() > 1)
		|| (m_mode == ScriptTableDataViewModel::Mode::ITEM && m_index < m_gd->GetScriptData()->GetItemTable()->size() && m_gd->GetScriptData()->GetItemTable()->size() > 1)));
}

void ScriptTableEditorFrame::InitProperties(wxPropertyGridManager& props) const
{
	if (m_gd && ArePropsInitialised() == false)
	{
		RefreshLists();
		props.GetGrid()->Clear();

		auto shop_props = props.Append(new wxPropertyCategory("Shop", "ShopC"));
		props.Append(new wxIntProperty("Table Index", "Table IndexS", m_index))->Enable(false);
		props.Append(new wxEnumProperty("Room", "Room", m_rooms, 0));
		auto markup = new wxFloatProperty("Discount/Markup", "Discount/Markup", 0.0);
		markup->SetAttribute(wxPG_ATTR_MIN, -100.0);
		markup->SetAttribute(wxPG_ATTR_MAX, 1493.75);
		markup->SetAttribute(wxPG_ATTR_SPINCTRL_STEP, 6.25);
		markup->SetAttribute(wxPG_ATTR_UNITS, "%");
		markup->SetEditor(wxPGEditor_SpinCtrl);
		props.Append(markup);
		auto lifestock_markup = new wxFloatProperty("Lifestock Discount/Markup", "Lifestock Discount/Markup", 0.0);
		lifestock_markup->SetAttribute(wxPG_ATTR_MIN, -100.0);
		lifestock_markup->SetAttribute(wxPG_ATTR_MAX, 1493.75);
		lifestock_markup->SetAttribute(wxPG_ATTR_SPINCTRL_STEP, 6.25);
		lifestock_markup->SetAttribute(wxPG_ATTR_UNITS, "%");
		lifestock_markup->SetEditor(wxPGEditor_SpinCtrl);
		props.Append(lifestock_markup);
		shop_props->Hide(true);

		auto item_props = props.Append(new wxPropertyCategory("Item", "ItemC"));
		props.Append(new wxIntProperty("Table Index", "Table IndexI", m_index))->Enable(false);
		props.Append(new wxEnumProperty("Item", "Item", m_items, 0));
		props.Append(new wxEnumProperty("Shop", "Shop", m_rooms_plus_empty, 0xFFFF));
		auto has_extra_data = new wxBoolProperty("Extra", "Extra", false);
		has_extra_data->SetAttribute(wxPG_BOOL_USE_CHECKBOX, true);
		props.Append(has_extra_data);
		auto extra_data = new wxUIntProperty("Extra Data", "Extra Data", 0);
		extra_data->SetAttribute(wxPG_ATTR_MIN, 0);
		extra_data->SetAttribute(wxPG_ATTR_MAX, 0xFFFF);
		extra_data->SetAttribute(wxPG_ATTR_SPINCTRL_STEP, 1);
		extra_data->SetEditor(wxPGEditor_SpinCtrl);
		props.Append(extra_data)->Enable(false);
		item_props->Hide(true);

		RefreshProperties(props);
		EditorFrame::InitProperties(props);
	}
}

void ScriptTableEditorFrame::RefreshLists() const
{
	if (m_gd)
	{
		m_rooms.Clear();
		m_rooms_plus_empty.Clear();
		m_rooms_plus_empty.Add("<ALL>", 0xFFFF);
		m_items.Clear();
		for (std::size_t i = 0; i < m_gd->GetRoomData()->GetRoomCount(); ++i)
		{
			m_rooms.Add(_(m_gd->GetRoomData()->GetRoomDisplayName(i)), i);
			m_rooms_plus_empty.Add(_(m_gd->GetRoomData()->GetRoomDisplayName(i)), i);
		}
		for (std::size_t i = 0; i < 64; ++i)
		{
			m_items.Add(_(m_gd->GetStringData()->GetItemDisplayName(i)), i);
		}
	}
}

void ScriptTableEditorFrame::UpdateProperties(wxPropertyGridManager& props) const
{
	EditorFrame::UpdateProperties(props);
	if (ArePropsInitialised() == true)
	{
		if (m_reset_props)
		{
			props.GetGrid()->ClearModifiedStatus();
			m_reset_props = false;
		}
		RefreshProperties(props);
	}
}

void ScriptTableEditorFrame::RefreshProperties(wxPropertyGridManager& props) const
{
	if (m_gd != nullptr && m_gd->GetScriptData()->HasTables())
	{
		bool valid = true;
		if (m_mode == ScriptTableDataViewModel::Mode::SHOP && m_index >= m_gd->GetScriptData()->GetShopTable()->size())
		{
			valid = false;
		}
		if (m_mode == ScriptTableDataViewModel::Mode::ITEM && m_index >= m_gd->GetScriptData()->GetItemTable()->size())
		{
			valid = false;
		}
		props.GetGrid()->Freeze();
		props.GetGrid()->GetProperty("ShopC")->Hide(!valid || m_mode != ScriptTableDataViewModel::Mode::SHOP);
		props.GetGrid()->GetProperty("ItemC")->Hide(!valid || m_mode != ScriptTableDataViewModel::Mode::ITEM);
		if (valid && m_mode == ScriptTableDataViewModel::Mode::SHOP)
		{
			const auto& shop = m_gd->GetScriptData()->GetShopTable()->at(m_index);
			props.GetGrid()->SetPropertyValue("Table IndexS", static_cast<int>(m_index));
			props.GetGrid()->SetPropertyValue("Room", static_cast<int>(shop.room));
			props.GetGrid()->SetPropertyValue("Discount/Markup", static_cast<double>(shop.markup) / 0.16 - 100.0);
			props.GetGrid()->SetPropertyValue("Lifestock Discount/Markup", static_cast<double>(shop.lifestock_markup) / 0.16 - 100.0);

		}
		else if (valid && m_mode == ScriptTableDataViewModel::Mode::ITEM)
		{
			const auto& item = m_gd->GetScriptData()->GetItemTable()->at(m_index);
			props.GetGrid()->SetPropertyValue("Table IndexI", static_cast<int>(m_index));
			props.GetGrid()->SetPropertyValue("Shop", static_cast<int>(item.shop));
			props.GetGrid()->SetPropertyValue("Item", static_cast<int>(item.item));
			props.GetGrid()->SetPropertyValue("Extra", item.other.has_value());
			props.GetGrid()->GetProperty("Extra Data")->Enable(item.other.has_value());
			props.GetGrid()->SetPropertyValue("Extra Data", static_cast<int>(item.other.value_or(0)));
		}
		props.GetGrid()->Thaw();
	}
}

void ScriptTableEditorFrame::OnPropertyChange(wxPropertyGridEvent& evt)
{
	auto* ctrl = static_cast<wxPropertyGridManager*>(evt.GetEventObject());
	wxPGProperty* property = evt.GetProperty();
	if (property == nullptr || m_gd == nullptr)
	{
		return;
	}
	ctrl->GetGrid()->Freeze();
	if (m_mode == ScriptTableDataViewModel::Mode::SHOP && m_index < m_gd->GetScriptData()->GetShopTable()->size())
	{
		auto& shop = m_gd->GetScriptData()->GetShopTable()->at(m_index);
		if (property->GetName() == "Room")
		{
			shop.room = static_cast<uint16_t>(property->GetValue().GetLong());
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
		else if (property->GetName() == "Discount/Markup")
		{
			shop.markup = static_cast<uint8_t>((property->GetValue().GetDouble() + 100.0) * 0.16);
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
		else if (property->GetName() == "Lifestock Discount/Markup")
		{
			shop.lifestock_markup = static_cast<uint8_t>((property->GetValue().GetDouble() + 100.0) * 0.16);
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
	}
	else if (m_mode == ScriptTableDataViewModel::Mode::ITEM && m_index < m_gd->GetScriptData()->GetItemTable()->size())
	{
		auto& item = m_gd->GetScriptData()->GetItemTable()->at(m_index);
		if (property->GetName() == "Shop")
		{
			item.shop = static_cast<uint16_t>(property->GetValue().GetLong());
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
		else if (property->GetName() == "Item")
		{
			item.item = static_cast<uint8_t>(property->GetValue().GetLong());
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
		else if (property->GetName() == "Extra" || property->GetName() == "Extra Data")
		{
			uint16_t extra_data = static_cast<uint16_t>(ctrl->GetGrid()->GetPropertyByName("Extra Data")->GetValue().GetLong());
			if (ctrl->GetGrid()->GetPropertyByName("Extra")->GetValue().GetBool())
			{
				item.other = extra_data;
				ctrl->GetGrid()->GetPropertyByName("Extra Data")->Enable();
			}
			else
			{
				item.other = std::nullopt;
				ctrl->GetGrid()->GetPropertyByName("Extra Data")->Enable(false);
			}
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
	}
	ctrl->GetGrid()->Thaw();
}

void ScriptTableEditorFrame::InitMenu(wxMenuBar& menu, ImageList& ilist) const
{
	auto* parent = m_mgr.GetManagedWindow();

	ClearMenu(menu);
	auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
	AddMenuItem(fileMenu, 0, ID_FILE_EXPORT_YML, "Export Script as YAML...");
	AddMenuItem(fileMenu, 1, ID_FILE_IMPORT_YML, "Import Script from YAML...");

	wxAuiToolBar* script_tb = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORIZONTAL);
	script_tb->AddTool(ID_APPEND, "Append Entry", ilist.GetImage("append_tile"), "Append Entry");
	script_tb->AddTool(ID_INSERT, "Insert Entry", ilist.GetImage("plus"), "Insert Entry");
	script_tb->AddTool(ID_DELETE, "Delete Entry", ilist.GetImage("minus"), "Delete Entry");
	script_tb->AddSeparator();
	script_tb->AddTool(ID_MOVE_UP, "Move Up", ilist.GetImage("up"), "Move Up");
	script_tb->AddTool(ID_MOVE_DOWN, "Move Down", ilist.GetImage("down"), "Move Down");
	script_tb->AddSeparator();
	script_tb->AddTool(ID_NEW_COLLECTION, "New", ilist.GetImage("new"), "New Element");
	script_tb->AddTool(ID_DELETE_COLLECTION, "Delete", ilist.GetImage("delete"), "Delete Element");
	AddToolbar(m_mgr, *script_tb, "Script", "Script Tools", wxAuiPaneInfo().ToolbarPane().Top().Row(1).Position(1).CloseButton(false).Movable(false).DockFixed(true));

	m_mgr.Update();
	UpdateUI();
}

void ScriptTableEditorFrame::OnMenuClick(wxMenuEvent& evt)
{
	switch (evt.GetId())
	{
	case ID_FILE_EXPORT_YML:
		OnExportYml();
		break;
	case ID_FILE_IMPORT_YML:
		OnImportYml();
		break;
	case ID_APPEND:
		OnAppend();
		break;
	case ID_INSERT:
		OnInsert();
		break;
	case ID_DELETE:
		OnDelete();
		break;
	case ID_MOVE_UP:
		OnMoveUp();
		break;
	case ID_MOVE_DOWN:
		OnMoveDown();
		break;
	case ID_NEW_COLLECTION:
		OnNewCollection();
		break;
	case ID_DELETE_COLLECTION:
		OnDeleteCollection();
		break;
	}
	UpdateUI();
}

void ScriptTableEditorFrame::ClearMenu(wxMenuBar& menu) const
{
	EditorFrame::ClearMenu(menu);
}

void ScriptTableEditorFrame::OnExportYml()
{
	const std::array<wxString, 4> DEFAULT_FILENAMES{ "cutscene_table.yaml", "character_table.yaml", "shop_table.yaml", "item_table.yaml" };
	if (m_gd && m_gd->GetScriptData()->HasTables())
	{
		wxFileDialog fd(this, _("Export Script as YAML"), "", DEFAULT_FILENAMES.at(static_cast<std::size_t>(m_mode)),
			"YAML file (*.yaml, *.yml)|*.yaml;*.yml|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		if (fd.ShowModal() != wxID_CANCEL)
		{
			std::ofstream ofs(fd.GetPath().ToStdString(), std::ios::binary | std::ios::out);
			switch (m_mode)
			{
			case ScriptTableDataViewModel::Mode::CUTSCENE:
				ofs << ScriptTable::TableToYaml(m_gd->GetScriptData()->GetCutsceneTable());
				break;
			case ScriptTableDataViewModel::Mode::CHARACTER:
				ofs << ScriptTable::TableToYaml(m_gd->GetScriptData()->GetCharTable());
				break;
			case ScriptTableDataViewModel::Mode::SHOP:
				ofs << ScriptTable::TableToYaml(m_gd->GetScriptData()->GetShopTable());
				break;
			case ScriptTableDataViewModel::Mode::ITEM:
				ofs << ScriptTable::TableToYaml(m_gd->GetScriptData()->GetItemTable());
				break;
			}
		}
	}
}

void ScriptTableEditorFrame::OnImportYml()
{
	if (m_gd)
	{
		wxFileDialog fd(this, _("Import Script from YAML"), "", "", "YAML Files (*.yml, *.yaml)|*.yml;*.yaml|All Files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
		if (fd.ShowModal() != wxID_CANCEL)
		{
			try
			{
				std::ifstream ifs(fd.GetPath().ToStdString());
				std::stringstream yaml;
				yaml << ifs.rdbuf();
				switch (m_mode)
				{
				case ScriptTableDataViewModel::Mode::CUTSCENE:
					*m_gd->GetScriptData()->GetCutsceneTable() = ScriptTable::TableFromYaml(yaml.str());
					break;
				case ScriptTableDataViewModel::Mode::CHARACTER:
					*m_gd->GetScriptData()->GetCharTable() = ScriptTable::TableFromYaml(yaml.str());
					break;
				case ScriptTableDataViewModel::Mode::SHOP:
					*m_gd->GetScriptData()->GetShopTable() = ScriptTable::ShopTableFromYaml(yaml.str());
					break;
				case ScriptTableDataViewModel::Mode::ITEM:
					*m_gd->GetScriptData()->GetItemTable() = ScriptTable::ItemTableFromYaml(yaml.str());
					break;
				}
				FireEvent(EVT_PROPERTIES_UPDATE);
				m_editor->RefreshData();
			}
			catch (std::exception& e)
			{
				wxMessageBox(std::string("Error when parsing YAML:\n") + e.what());
			}
		}
	}
}

void ScriptTableEditorFrame::OnAppend()
{
	m_editor->AppendRow();
}

void ScriptTableEditorFrame::OnInsert()
{
	m_editor->InsertRow();
}

void ScriptTableEditorFrame::OnDelete()
{
	m_editor->DeleteRow();
}

void ScriptTableEditorFrame::OnMoveUp()
{
	m_editor->MoveRowUp();
}

void ScriptTableEditorFrame::OnMoveDown()
{
	m_editor->MoveRowDown();
}

void ScriptTableEditorFrame::OnNewCollection()
{
	if (m_mode == ScriptTableDataViewModel::Mode::SHOP)
	{
		ScriptTable::Shop shop;
		std::string new_item = StrPrintf("Script/Script Tables/Shop Tables/ShopTable%d", m_gd->GetScriptData()->GetShopTable()->size());
		FireEvent(EVT_ADD_NAV_ITEM, new_item, (static_cast<uint32_t>(m_mode) << 16) | m_gd->GetScriptData()->GetShopTable()->size(),
			m_imglst->GetIdx("script"), static_cast<uint32_t>(TreeNodeData::Node::SCRIPT_TABLE));
		m_gd->GetScriptData()->GetShopTable()->push_back(shop);
		FireEvent(EVT_GO_TO_NAV_ITEM, new_item);
	}
	else if (m_mode == ScriptTableDataViewModel::Mode::ITEM)
	{
		ScriptTable::Item item;
		std::string new_item = StrPrintf("Script/Script Tables/Item Tables/ItemTable%d", m_gd->GetScriptData()->GetItemTable()->size());
		FireEvent(EVT_ADD_NAV_ITEM, new_item, (static_cast<uint32_t>(m_mode) << 16) | m_gd->GetScriptData()->GetItemTable()->size(),
			m_imglst->GetIdx("script"), static_cast<uint32_t>(TreeNodeData::Node::SCRIPT_TABLE));
		m_gd->GetScriptData()->GetItemTable()->push_back(item);
		FireEvent(EVT_GO_TO_NAV_ITEM, new_item);
	}
}

void ScriptTableEditorFrame::OnDeleteCollection()
{
	if (m_mode == ScriptTableDataViewModel::Mode::SHOP)
	{
		std::shared_ptr<std::vector<ScriptTable::Shop>> shop_table = m_gd->GetScriptData()->GetShopTable();
		if (shop_table->size() == 0)
		{
			return;
		}
		std::string item_to_remove = StrPrintf("Script/Script Tables/Shop Tables/ShopTable%d", shop_table->size() - 1);
		std::string item_to_select = "Script/Script Tables/Character Table";
		if (m_index < shop_table->size() - 1)
		{
			item_to_select = StrPrintf("Script/Script Tables/Shop Tables/ShopTable%d", m_index);
		}
		else if (m_index > 0)
		{
			item_to_select = StrPrintf("Script/Script Tables/Shop Tables/ShopTable%d", m_index - 1);
		}
		shop_table->erase(shop_table->begin() + m_index);
		FireEvent(EVT_DELETE_NAV_ITEM, item_to_remove);
		FireEvent(EVT_GO_TO_NAV_ITEM, item_to_select);
	}
	else if (m_mode == ScriptTableDataViewModel::Mode::ITEM)
	{
		std::shared_ptr<std::vector<ScriptTable::Item>> item_table = m_gd->GetScriptData()->GetItemTable();
		if (item_table->size() == 0)
		{
			return;
		}
		std::string item_to_remove = StrPrintf("Script/Script Tables/Item Tables/ItemTable%d", item_table->size() - 1);
		std::string item_to_select = "Script/Script Tables/Character Table";
		if (m_index < item_table->size() - 1)
		{
			item_to_select = StrPrintf("Script/Script Tables/Item Tables/ItemTable%d", m_index);
		}
		else if (m_index > 0)
		{
			item_to_select = StrPrintf("Script/Script Tables/Item Tables/ItemTable%d", m_index - 1);
		}
		item_table->erase(item_table->begin() + m_index);
		FireEvent(EVT_DELETE_NAV_ITEM, item_to_remove);
		FireEvent(EVT_GO_TO_NAV_ITEM, item_to_select);
	}
}
