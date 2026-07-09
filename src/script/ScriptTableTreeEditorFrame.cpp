#include <script/ScriptTableTreeEditorFrame.h>

#include <wx/propgrid/advprops.h>

#include <fstream>

enum TOOL_IDS
{
    ID_ADD_ENTRY = 30000,
    ID_REMOVE_ENTRY,
    ID_MOVE_ENTRY_UP,
    ID_MOVE_ENTRY_DOWN,
    ID_ADD_CHILD,
    ID_ADD_SIBLING,
    ID_REMOVE_ITEM,
    ID_MOVE_ITEM_UP,
    ID_MOVE_ITEM_DOWN
};

enum MENU_IDS
{
    ID_FILE_EXPORT_YML = 20000,
    ID_FILE_IMPORT_YML,
    ID_FILE_EXPORT_ASM,
    ID_FILE_IMPORT_ASM
};

ScriptTableTreeEditorFrame::ScriptTableTreeEditorFrame(wxWindow* parent, ImageList* imglst)
    : EditorFrame(parent, wxID_ANY, imglst)
{
    m_mgr.SetManagedWindow(this);
    m_editor = new ScriptTableTreeEditorCtrl(this);
    m_editor->SetStateChangeCallback([this]()
    {
        UpdateUI();
        FireEvent(EVT_PROPERTIES_UPDATE);
    });

    m_mgr.AddPane(m_editor, wxAuiPaneInfo().CenterPane());
    m_mgr.Update();
}

ScriptTableTreeEditorFrame::~ScriptTableTreeEditorFrame()
{
}

bool ScriptTableTreeEditorFrame::Open(ScriptTableTreeCategory category)
{
    if (m_gd)
    {
        m_category = category;
        m_editor->Open(category);
        m_reset_props = true;
        FireEvent(EVT_PROPERTIES_UPDATE);
        UpdateUI();
        return true;
    }
    return false;
}

void ScriptTableTreeEditorFrame::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
    m_gd = gd;
    m_editor->SetGameData(gd);
}

void ScriptTableTreeEditorFrame::ClearGameData()
{
    m_editor->ClearGameData();
    m_gd.reset();
    m_reset_props = true;
    FireEvent(EVT_PROPERTIES_UPDATE);
    UpdateUI();
}

void ScriptTableTreeEditorFrame::CommitPendingEdits()
{
    if (m_editor)
    {
        m_editor->CommitTreeEditing();
    }
}

void ScriptTableTreeEditorFrame::UpdateUI() const
{
    const bool loaded = m_gd != nullptr && m_gd->GetScriptData()->HasTables();
    EnableToolbarItem("Script", ID_ADD_ENTRY, loaded);
    EnableToolbarItem("Script", ID_REMOVE_ENTRY, loaded && m_editor->CanRemoveEntry());
    EnableToolbarItem("Script", ID_MOVE_ENTRY_UP, loaded && m_editor->CanMoveEntryUp());
    EnableToolbarItem("Script", ID_MOVE_ENTRY_DOWN, loaded && m_editor->CanMoveEntryDown());
    EnableToolbarItem("Script", ID_ADD_CHILD, loaded && m_editor->CanAddChild());
    EnableToolbarItem("Script", ID_ADD_SIBLING, loaded && m_editor->CanAddSibling());
    EnableToolbarItem("Script", ID_REMOVE_ITEM, loaded && m_editor->CanRemoveItem());
    EnableToolbarItem("Script", ID_MOVE_ITEM_UP, loaded && m_editor->CanMoveItemUp());
    EnableToolbarItem("Script", ID_MOVE_ITEM_DOWN, loaded && m_editor->CanMoveItemDown());
}

int ScriptTableTreeEditorFrame::GetSelectedTableIndex() const
{
    if (!m_gd || !m_gd->GetScriptData()->HasTables())
    {
        return -1;
    }
    const int index = m_editor->GetSelectedEntry();
    std::size_t table_size = 0;
    switch (m_category)
    {
    case ScriptTableTreeCategory::SHOP: table_size = m_gd->GetScriptData()->GetShopTable()->size(); break;
    case ScriptTableTreeCategory::ITEM: table_size = m_gd->GetScriptData()->GetItemTable()->size(); break;
    case ScriptTableTreeCategory::CHARACTER: table_size = m_gd->GetScriptData()->GetCharTable()->size(); break;
    case ScriptTableTreeCategory::CUTSCENE: table_size = m_gd->GetScriptData()->GetCutsceneTable()->size(); break;
    default: break;
    }
    // The synthesised "Other Functions" entry sits past the end of the real table.
    if (index < 0 || static_cast<std::size_t>(index) >= table_size)
    {
        return -1;
    }
    return index;
}

void ScriptTableTreeEditorFrame::InitProperties(wxPropertyGridManager& props) const
{
    if (m_gd && ArePropsInitialised() == false)
    {
        RefreshLists();
        props.GetGrid()->Clear();

        auto shop_props = props.Append(new wxPropertyCategory("Shop", "ShopC"));
        props.Append(new wxIntProperty("Table Index", "Table IndexS", 0))->Enable(false);
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
        props.Append(new wxIntProperty("Table Index", "Table IndexI", 0))->Enable(false);
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

void ScriptTableTreeEditorFrame::RefreshLists() const
{
    if (m_gd)
    {
        m_rooms.Clear();
        m_rooms_plus_empty.Clear();
        m_rooms_plus_empty.Add("<ALL>", 0xFFFF);
        m_items.Clear();
        for (std::size_t i = 0; i < m_gd->GetRoomData()->GetRoomCount(); ++i)
        {
            m_rooms.Add(wxString(m_gd->GetRoomData()->GetRoomDisplayName(i)), i);
            m_rooms_plus_empty.Add(wxString(m_gd->GetRoomData()->GetRoomDisplayName(i)), i);
        }
        for (std::size_t i = 0; i < 64; ++i)
        {
            m_items.Add(wxString(m_gd->GetStringData()->GetItemDisplayName(i)), i);
        }
    }
}

void ScriptTableTreeEditorFrame::UpdateProperties(wxPropertyGridManager& props) const
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

void ScriptTableTreeEditorFrame::RefreshProperties(wxPropertyGridManager& props) const
{
    if (m_gd != nullptr && m_gd->GetScriptData()->HasTables())
    {
        const int index = GetSelectedTableIndex();
        const bool valid = index >= 0;
        props.GetGrid()->Freeze();
        props.GetGrid()->GetProperty("ShopC")->Hide(!valid || m_category != ScriptTableTreeCategory::SHOP);
        props.GetGrid()->GetProperty("ItemC")->Hide(!valid || m_category != ScriptTableTreeCategory::ITEM);
        if (valid && m_category == ScriptTableTreeCategory::SHOP)
        {
            const auto& shop = m_gd->GetScriptData()->GetShopTable()->at(index);
            props.GetGrid()->SetPropertyValue("Table IndexS", index);
            props.GetGrid()->SetPropertyValue("Room", static_cast<int>(shop.room));
            props.GetGrid()->SetPropertyValue("Discount/Markup", static_cast<double>(shop.markup) / 0.16 - 100.0);
            props.GetGrid()->SetPropertyValue("Lifestock Discount/Markup", static_cast<double>(shop.lifestock_markup) / 0.16 - 100.0);
        }
        else if (valid && m_category == ScriptTableTreeCategory::ITEM)
        {
            const auto& item = m_gd->GetScriptData()->GetItemTable()->at(index);
            props.GetGrid()->SetPropertyValue("Table IndexI", index);
            props.GetGrid()->SetPropertyValue("Shop", static_cast<int>(item.shop));
            props.GetGrid()->SetPropertyValue("Item", static_cast<int>(item.item));
            props.GetGrid()->SetPropertyValue("Extra", item.other.has_value());
            props.GetGrid()->GetProperty("Extra Data")->Enable(item.other.has_value());
            props.GetGrid()->SetPropertyValue("Extra Data", static_cast<int>(item.other.value_or(0)));
        }
        props.GetGrid()->Thaw();
    }
}

void ScriptTableTreeEditorFrame::OnPropertyChange(wxPropertyGridEvent& evt)
{
    auto* ctrl = static_cast<wxPropertyGridManager*>(evt.GetEventObject());
    wxPGProperty* property = evt.GetProperty();
    const int index = GetSelectedTableIndex();
    if (property == nullptr || m_gd == nullptr || index < 0)
    {
        return;
    }
    ctrl->GetGrid()->Freeze();
    if (m_category == ScriptTableTreeCategory::SHOP)
    {
        auto& shop = m_gd->GetScriptData()->GetShopTable()->at(index);
        if (property->GetName() == "Room")
        {
            shop.room = static_cast<uint16_t>(property->GetValue().GetLong());
            // The room is part of the entry's display name and location anchors - rebuild.
            m_editor->RebuildCategory(index);
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
    else if (m_category == ScriptTableTreeCategory::ITEM)
    {
        auto& item = m_gd->GetScriptData()->GetItemTable()->at(index);
        if (property->GetName() == "Shop")
        {
            item.shop = static_cast<uint16_t>(property->GetValue().GetLong());
            m_editor->RebuildCategory(index);
            FireEvent(EVT_PROPERTIES_UPDATE);
        }
        else if (property->GetName() == "Item")
        {
            item.item = static_cast<uint8_t>(property->GetValue().GetLong());
            m_editor->RebuildCategory(index);
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

void ScriptTableTreeEditorFrame::InitMenu(wxMenuBar& menu, ImageList& ilist) const
{
    auto* parent = m_mgr.GetManagedWindow();

    ClearMenu(menu);
    auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
    AddMenuItem(fileMenu, 0, ID_FILE_EXPORT_YML, "Export Script as YAML...");
    AddMenuItem(fileMenu, 1, ID_FILE_IMPORT_YML, "Import Script from YAML...");
    AddMenuItem(fileMenu, 2, ID_FILE_EXPORT_ASM, "Export Script as ASM...");
    AddMenuItem(fileMenu, 3, ID_FILE_IMPORT_ASM, "Import Script from ASM...");

    wxAuiToolBar* script_tb = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORIZONTAL);
    script_tb->AddTool(ID_ADD_ENTRY, "Add Entry", ilist.GetImage("append_tile"), "Add Entry");
    script_tb->AddTool(ID_REMOVE_ENTRY, "Remove Entry", ilist.GetImage("delete"), "Remove Entry");
    script_tb->AddTool(ID_MOVE_ENTRY_UP, "Move Entry Up", ilist.GetImage("up"), "Move Entry Up");
    script_tb->AddTool(ID_MOVE_ENTRY_DOWN, "Move Entry Down", ilist.GetImage("down"), "Move Entry Down");
    script_tb->AddSeparator();
    script_tb->AddTool(ID_ADD_CHILD, "Add Child", ilist.GetImage("new"), "Add a child script item to the selected row");
    script_tb->AddTool(ID_ADD_SIBLING, "Add Sibling", ilist.GetImage("plus"), "Add a script item next to the selected row");
    script_tb->AddTool(ID_REMOVE_ITEM, "Remove", ilist.GetImage("minus"), "Remove the selected script item");
    script_tb->AddTool(ID_MOVE_ITEM_UP, "Move Up", ilist.GetImage("up"), "Move the selected script item up");
    script_tb->AddTool(ID_MOVE_ITEM_DOWN, "Move Down", ilist.GetImage("down"), "Move the selected script item down");
    AddToolbar(m_mgr, *script_tb, "Script", "Script Tools", wxAuiPaneInfo().ToolbarPane().Top().Row(1).Position(1).CloseButton(false).Movable(false).DockFixed(true));

    m_mgr.Update();
    UpdateUI();
}

void ScriptTableTreeEditorFrame::OnMenuClick(wxMenuEvent& evt)
{
    switch (evt.GetId())
    {
    case ID_FILE_EXPORT_YML:
        m_editor->ExportScript(true);
        break;
    case ID_FILE_IMPORT_YML:
        m_editor->ImportScript(true);
        break;
    case ID_FILE_EXPORT_ASM:
        m_editor->ExportScript(false);
        break;
    case ID_FILE_IMPORT_ASM:
        m_editor->ImportScript(false);
        break;
    case ID_ADD_ENTRY:
        m_editor->AddEntry();
        break;
    case ID_REMOVE_ENTRY:
        m_editor->RemoveEntry();
        break;
    case ID_MOVE_ENTRY_UP:
        m_editor->MoveEntryUp();
        break;
    case ID_MOVE_ENTRY_DOWN:
        m_editor->MoveEntryDown();
        break;
    case ID_ADD_CHILD:
        m_editor->AddChildItem();
        break;
    case ID_ADD_SIBLING:
        m_editor->AddSiblingItem();
        break;
    case ID_REMOVE_ITEM:
        m_editor->RemoveSelectedItem();
        break;
    case ID_MOVE_ITEM_UP:
        m_editor->MoveSelectedItemUp();
        break;
    case ID_MOVE_ITEM_DOWN:
        m_editor->MoveSelectedItemDown();
        break;
    }
    UpdateUI();
}

void ScriptTableTreeEditorFrame::ClearMenu(wxMenuBar& menu) const
{
    EditorFrame::ClearMenu(menu);
}
