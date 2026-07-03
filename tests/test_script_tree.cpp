// Editor-side tests for the tree-based script table editor: the ScriptTreeNode builders and
// ScriptTreeDataViewModel edit paths, ported from the scripttbl proof-of-concept's headless
// harness. These run against a real disassembly project: set LANDSTALKER_DISASM_PATH to the
// root of a landstalker_disasm checkout, otherwise the whole suite is skipped.
//
// The models are exercised headless (no windows are created); only the wxDataViewModel API
// itself is used, which needs no running wxApp.

#include <gtest/gtest.h>

#include <script/ScriptTreeNode.h>
#include <script/ScriptTreeDataViewModel.h>

#include <landstalker/main/GameData.h>
#include <landstalker/misc/Labels.h>

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <vector>

using namespace Landstalker;

namespace
{

std::shared_ptr<GameData> LoadDisasm()
{
    const char* root = std::getenv("LANDSTALKER_DISASM_PATH");
    if (root == nullptr)
    {
        return nullptr;
    }
    const auto root_path = std::filesystem::path(root);
    const auto labels = root_path / "landstalker_labels.yaml";
    if (std::filesystem::exists(labels))
    {
        Labels::LoadData(labels.string());
    }
    const auto asm_file = root_path / "landstalker_us.asm";
    if (!std::filesystem::exists(asm_file))
    {
        return nullptr;
    }
    auto gd = std::make_shared<GameData>();
    if (!gd->Open(asm_file.string()) || !gd->IsReady())
    {
        return nullptr;
    }
    return gd;
}

wxDataViewItem FindItemByPrefix(ScriptTreeDataViewModel* model, const wxDataViewItem& item, const wxString& prefix)
{
    wxDataViewItemArray children;
    model->GetChildren(item, children);
    for (const auto& child : children)
    {
        wxVariant value;
        model->GetValue(value, child, 0);
        if (value.GetString().StartsWith(prefix))
        {
            return child;
        }
        wxDataViewItem found = FindItemByPrefix(model, child, prefix);
        if (found.IsOk())
        {
            return found;
        }
    }
    return wxDataViewItem();
}

wxString RowText(ScriptTreeDataViewModel* model, const wxDataViewItem& item)
{
    wxVariant value;
    model->GetValue(value, item, 0);
    return value.GetString();
}

} // namespace

class ScriptTreeEditorTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        s_gd = LoadDisasm();
    }

    static void TearDownTestSuite()
    {
        s_gd.reset();
    }

    void SetUp() override
    {
        if (!s_gd)
        {
            GTEST_SKIP() << "LANDSTALKER_DISASM_PATH not set or disassembly could not be loaded";
        }
    }

    std::vector<std::shared_ptr<ScriptFunctionTable>> FullPool() const
    {
        auto sd = s_gd->GetScriptData();
        return { sd->GetCharFuncs(), sd->GetCutsceneFuncs(), sd->GetShopFuncs(), sd->GetItemFuncs() };
    }

    static std::shared_ptr<GameData> s_gd;
};

std::shared_ptr<GameData> ScriptTreeEditorTest::s_gd;

// Functions defined in one file but referenced by the other table must resolve (display with
// a body, not a red bare reference) in the category whose table references them, and must NOT
// show up as loose orphans in the category that owns their file. Shop_08 lives in the
// custom-item file but is referenced by the shop table; ShopMap_07 lives in the shop file but
// is referenced by the custom-item table.
TEST_F(ScriptTreeEditorTest, CrossTablePoolResolution)
{
    auto has_bodied_function = [](const ScriptTreeNode& entry_root, const std::string& name)
    {
        std::set<std::string> displayed;
        CollectDisplayFunctionNames(entry_root, displayed);
        return displayed.find(name) != displayed.end();
    };
    auto in_other_functions = [](const ScriptTreeNode& tab_root, const std::string& name)
    {
        for (const auto& entry : tab_root.children)
        {
            if (entry.name != "Other Functions")
            {
                continue;
            }
            for (const auto& f : entry.children)
            {
                if (f.type == ScriptTreeNodeType::FUNCTION && f.text_value == name)
                {
                    return true;
                }
            }
        }
        return false;
    };
    auto displayed_anywhere = [&](const ScriptTreeNode& tab_root, const std::string& name)
    {
        for (const auto& entry : tab_root.children)
        {
            if (has_bodied_function(entry, name))
            {
                return true;
            }
        }
        return false;
    };

    ScriptTreeNode shops_tab = ScriptTreeNode::BuildCategoryTree(s_gd, 0);
    ScriptTreeNode items_tab = ScriptTreeNode::BuildCategoryTree(s_gd, 1);
    EXPECT_TRUE(displayed_anywhere(shops_tab, "Shop_08"));
    EXPECT_FALSE(in_other_functions(items_tab, "Shop_08"));
    EXPECT_TRUE(displayed_anywhere(items_tab, "ShopMap_07"));
    EXPECT_FALSE(in_other_functions(shops_tab, "ShopMap_07"));
}

// YAML round trip of the loaded tables: export -> parse must reproduce the same set of
// functions with identical statements. The parse ctor un-nests (Unconsolidate()), which can
// legitimately move a nested single-use function to a different table position, so the first
// trip checks content only; a second trip must then be fully stable, order included. The
// top-level name deliberately contains ": ", which the emitter must quote.
TEST_F(ScriptTreeEditorTest, YamlRoundTripLoadedTables)
{
    auto round_trip = [](const char* label, std::shared_ptr<ScriptFunctionTable> table)
    {
        SCOPED_TRACE(label);
        const std::string yaml = table->ToYaml("Test : " + std::string(label));
        ScriptFunctionTable parsed(yaml);

        const std::set<std::string> before(table->GetFunctionNames().cbegin(), table->GetFunctionNames().cend());
        const std::set<std::string> after(parsed.GetFunctionNames().cbegin(), parsed.GetFunctionNames().cend());
        EXPECT_EQ(before, after);
        if (before == after)
        {
            for (const auto& name : before)
            {
                EXPECT_EQ(*table->GetMapping(name), *parsed.GetMapping(name)) << name << " differs";
            }
        }

        ScriptFunctionTable reparsed(parsed.ToYaml());
        EXPECT_EQ(parsed, reparsed) << "second trip not stable";
    };
    round_trip("shops", s_gd->GetScriptData()->GetShopFuncs());
    round_trip("items", s_gd->GetScriptData()->GetItemFuncs());
    round_trip("characters", s_gd->GetScriptData()->GetCharFuncs());
    round_trip("cutscenes", s_gd->GetScriptData()->GetCutsceneFuncs());
}

// Function-order stability: an edit inside an entry that displays related "Function:" headers
// must not shuffle the underlying function table's order (order drives fallthrough detection,
// inline-embed eligibility and the characters category's related-function search - a reorder
// makes other entries' subtrees change/disappear).
TEST_F(ScriptTreeEditorTest, ValueEditPreservesFunctionOrder)
{
    ScriptTreeNode chr_cat = ScriptTreeNode::BuildCategoryTree(s_gd, 2);
    ScriptTreeNode* chr_entry = nullptr;
    for (auto& e : chr_cat.children)
    {
        for (auto& c : e.children)
        {
            if (c.type == ScriptTreeNodeType::FUNCTION)
            {
                chr_entry = &e;
                break;
            }
        }
        if (chr_entry)
        {
            break;
        }
    }
    ASSERT_NE(chr_entry, nullptr) << "no character entry with a related function found";

    auto char_funcs = s_gd->GetScriptData()->GetCharFuncs();
    const std::vector<std::string> before_order = char_funcs->GetFunctionNames();
    wxObjectDataPtr<ScriptTreeDataViewModel> model(
        new ScriptTreeDataViewModel(*chr_entry, char_funcs, s_gd, FullPool()));
    wxDataViewItem slot = FindItemByPrefix(model.get(), wxDataViewItem(), "On Talk:");
    ASSERT_TRUE(slot.IsOk());
    wxDataViewItem snd = model->AddChild(slot, { ScriptTreeNodeType::PLAY_SOUND, "Play Sound", "Play Sound: 0" });
    ASSERT_TRUE(snd.IsOk());
    EXPECT_TRUE(model->SetValue(wxVariant(wxString("3")), snd, 0));
    EXPECT_EQ(before_order, char_funcs->GetFunctionNames());
}

// The main edit-path flow, ported from the proof-of-concept harness. Steps are cumulative on
// the shared GameData, matching the way the harness ran them; ASSERTs guard the state later
// steps depend on.
TEST_F(ScriptTreeEditorTest, ShopModelEditFlow)
{
    ScriptTreeNode cat = ScriptTreeNode::BuildCategoryTree(s_gd, 0);
    ASSERT_FALSE(cat.children.empty()) << "no shop entries";

    auto shop_funcs = s_gd->GetScriptData()->GetShopFuncs();
    wxObjectDataPtr<ScriptTreeDataViewModel> model(
        new ScriptTreeDataViewModel(cat.children.front(), shop_funcs, s_gd, FullPool()));

    // Point a script action at a brand-new function: the function must be created in the
    // shop table and its body displayed in place.
    wxDataViewItem target = FindItemByPrefix(model.get(), wxDataViewItem(), "Display Item Price:");
    ASSERT_TRUE(target.IsOk()) << "no 'Display Item Price:' node found in shop 0";
    EXPECT_TRUE(model->SetValue(wxVariant(wxString("New Function: zztestfunc")), target, 0));
    EXPECT_NE(shop_funcs->GetMapping("zztestfunc"), nullptr);

    // Point the same action at a function whose body is already displayed elsewhere in this
    // entry (Shop_01, embedded at the "On Pay:" slot): a bare reference, no embedded copy.
    target = FindItemByPrefix(model.get(), wxDataViewItem(), "Display Item Price:");
    ASSERT_TRUE(target.IsOk());
    EXPECT_TRUE(model->SetValue(wxVariant(wxString("Function: Shop_01")), target, 0));

    // Recursive reference: point the action inside ShopPrice_01's displayed body at
    // ShopPrice_01 itself - the edit must persist into the function table (a stale embedded
    // copy previously synced last and silently reverted it).
    target = FindItemByPrefix(model.get(), wxDataViewItem(), "Display Item Price:");
    ASSERT_TRUE(target.IsOk());
    EXPECT_TRUE(model->SetValue(wxVariant(wxString("Function: ShopPrice_01")), target, 0));

    // Numeric parameter editors: the committed value must be reflected in the row text.
    wxDataViewItem on_pay = FindItemByPrefix(model.get(), wxDataViewItem(), "On Pay:");
    ASSERT_TRUE(on_pay.IsOk());
    wxDataViewItem sound = model->AddChild(on_pay, { ScriptTreeNodeType::PLAY_SOUND, "Play Sound", "Play Sound: 0" });
    ASSERT_TRUE(sound.IsOk());
    EXPECT_TRUE(model->SetValue(wxVariant(wxString("105")), sound, 0));
    EXPECT_NE(RowText(model.get(), sound).Find("105"), wxNOT_FOUND);

    wxDataViewItem flag = model->AddChild(on_pay, { ScriptTreeNodeType::SET_FLAG, "Set Flag on Talk", "Set Flag 0000 on Talk:" });
    ASSERT_TRUE(flag.IsOk());
    EXPECT_TRUE(model->SetValue(wxVariant(wxString("89")), flag, 0));

    // Custom ASM block: whole-block replacement; blank-only text and script-structure
    // instructions (rts) must be rejected without changing the block.
    wxDataViewItem custom_asm = model->AddChild(on_pay, { ScriptTreeNodeType::CUSTOM_ASM, "Custom ASM Block", "Custom ASM" });
    ASSERT_TRUE(custom_asm.IsOk());
    EXPECT_TRUE(model->ApplyCustomAsm(custom_asm, wxString("  move.w #$0001,d0  \n\n  nop\n")));
    EXPECT_FALSE(model->ApplyCustomAsm(custom_asm, wxString("   \n  \n")));
    EXPECT_FALSE(model->ApplyCustomAsm(custom_asm, wxString("nop\nrts\n")));

    // Adding two new functions in a row must generate unique default names - a second
    // "NewFunction" would otherwise silently overwrite the first one's body on sync.
    wxDataViewItem f1 = model->AddSibling(on_pay, { ScriptTreeNodeType::FUNCTION, "Function", "Function: NewFunction" });
    wxDataViewItem f2 = model->AddSibling(on_pay, { ScriptTreeNodeType::FUNCTION, "Function", "Function: NewFunction" });
    ASSERT_TRUE(f1.IsOk());
    ASSERT_TRUE(f2.IsOk());
    EXPECT_EQ(RowText(model.get(), f1), "Function: NewFunction");
    EXPECT_EQ(RowText(model.get(), f2), "Function: NewFunction2");

    // Branch editor commit path: "<label> W" marks the branch wide; labels with whitespace or
    // invalid characters are rejected.
    wxDataViewItem branch = model->AddChild(f1, { ScriptTreeNodeType::BRANCH, "Branch", "Branch: Label" });
    ASSERT_TRUE(branch.IsOk());
    EXPECT_TRUE(model->SetValue(wxVariant(wxString("ShopPrice_01 W")), branch, 0));
    EXPECT_NE(RowText(model.get(), branch).Find("ShopPrice_01"), wxNOT_FOUND);
    EXPECT_FALSE(model->SetValue(wxVariant(wxString("bad label")), branch, 0));
    EXPECT_FALSE(model->SetValue(wxVariant(wxString("[bad")), branch, 0));
    target = FindItemByPrefix(model.get(), wxDataViewItem(), "Display Item Price:");
    ASSERT_TRUE(target.IsOk());
    EXPECT_FALSE(model->SetValue(wxVariant(wxString("Function: [bad")), target, 0));

    // Progress Entry editor: quest/progress value pair.
    wxDataViewItem prog_table = model->AddChild(f2, { ScriptTreeNodeType::PROG_DEP_TABLE, "Progress Dependent List", "Progress Dependent:" });
    ASSERT_TRUE(prog_table.IsOk());
    wxDataViewItem prog = model->AddChild(prog_table, { ScriptTreeNodeType::PROG_DEP_ACTION, "Progress Entry", "On Quest 0, Progress 0:" });
    ASSERT_TRUE(prog.IsOk());
    EXPECT_TRUE(model->SetValue(wxVariant(wxString("5 12")), prog, 0));
    EXPECT_NE(RowText(model.get(), prog).Find("5"), wxNOT_FOUND);

    // The app rebuilds the category after every committed edit - the edits must survive a
    // rebuild with the entry still present.
    ScriptTreeNode rebuilt = ScriptTreeNode::BuildCategoryTree(s_gd, 0);
    EXPECT_FALSE(rebuilt.children.empty());
    std::set<std::string> displayed;
    CollectDisplayFunctionNames(rebuilt, displayed);
    EXPECT_NE(displayed.find("zztestfunc"), displayed.end());
    EXPECT_NE(displayed.find("NewFunction"), displayed.end());
    EXPECT_NE(displayed.find("NewFunction2"), displayed.end());
}
