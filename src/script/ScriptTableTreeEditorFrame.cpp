#include <script/ScriptTableTreeEditorFrame.h>

#include <wx/propgrid/advprops.h>

#include <fstream>

enum MENU_IDS
{
    ID_FILE_EXPORT_FUNCTIONS_YML = 20000,
    ID_FILE_IMPORT_FUNCTIONS_YML,
    ID_FILE_EXPORT_TABLE_YML,
    ID_FILE_IMPORT_TABLE_YML,
    ID_FILE_EXPORT_TABLE_ASM,
    ID_FILE_IMPORT_TABLE_ASM,
    ID_FILE_EXPORT_ASM,
    ID_FILE_IMPORT_ASM
};

ScriptTableTreeEditorFrame::ScriptTableTreeEditorFrame(wxWindow* parent, ImageList* imglst)
    : EditorFrame(parent, wxID_ANY, imglst)
{
    m_mgr.SetManagedWindow(this);
    m_editor = new ScriptTableTreeEditorCtrl(this, imglst);
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
        // All four categories share this frame. When it is already visible, Show() does not
        // run again to refresh the category-specific File menu.
        if (IsShown())
        {
            FireEvent(EVT_MENU_INIT);
        }
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
    // No frame-level toolbar items to enable/disable - ScriptTableTreeEditorCtrl's own
    // UpdateEditButtons() drives its embedded buttons' enabled state directly.
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

void ScriptTableTreeEditorFrame::InitMenu(wxMenuBar& menu, ImageList& /*ilist*/) const
{
    // No frame-level toolbar - ScriptTableTreeEditorCtrl's own embedded buttons (both the entry
    // list's Add/Remove/Move Up/Down and the script tree's Add Child/Add Sibling/Remove/Move Up/
    // Down) already cover these actions directly on the controls they act on.
    ClearMenu(menu);
    auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
    int position = 0;
    if (m_category == ScriptTableTreeCategory::CHARACTER ||
        m_category == ScriptTableTreeCategory::CUTSCENE)
    {
        AddMenuItem(fileMenu, position++, ID_FILE_EXPORT_TABLE_YML, "Export Script Table as YAML...");
        AddMenuItem(fileMenu, position++, ID_FILE_IMPORT_TABLE_YML, "Import Script Table from YAML...");
        AddMenuItem(fileMenu, position++, ID_FILE_EXPORT_TABLE_ASM, "Export Script Table as ASM...");
        AddMenuItem(fileMenu, position++, ID_FILE_IMPORT_TABLE_ASM, "Import Script Table from ASM...");
    }
    AddMenuItem(fileMenu, position++, ID_FILE_EXPORT_FUNCTIONS_YML, "Export Script Functions as YAML...");
    AddMenuItem(fileMenu, position++, ID_FILE_IMPORT_FUNCTIONS_YML, "Import Script Functions from YAML...");
    AddMenuItem(fileMenu, position++, ID_FILE_EXPORT_ASM, "Export Script Functions as ASM...");
    AddMenuItem(fileMenu, position, ID_FILE_IMPORT_ASM, "Import Script Functions from ASM...");

    m_mgr.Update();
    UpdateUI();
}

void ScriptTableTreeEditorFrame::OnMenuClick(wxMenuEvent& evt)
{
    switch (evt.GetId())
    {
    case ID_FILE_EXPORT_FUNCTIONS_YML:
        m_editor->ExportScript(true);
        break;
    case ID_FILE_IMPORT_FUNCTIONS_YML:
        m_editor->ImportScript(true);
        break;
    case ID_FILE_EXPORT_TABLE_YML:
        m_editor->ExportTableYaml();
        break;
    case ID_FILE_IMPORT_TABLE_YML:
        m_editor->ImportTableYaml();
        break;
    case ID_FILE_EXPORT_TABLE_ASM:
        m_editor->ExportTableAsm();
        break;
    case ID_FILE_IMPORT_TABLE_ASM:
        m_editor->ImportTableAsm();
        break;
    case ID_FILE_EXPORT_ASM:
        m_editor->ExportScript(false);
        break;
    case ID_FILE_IMPORT_ASM:
        m_editor->ImportScript(false);
        break;
    }
    UpdateUI();
}

void ScriptTableTreeEditorFrame::ClearMenu(wxMenuBar& menu) const
{
    EditorFrame::ClearMenu(menu);
}
