#ifndef MAINFRAME_H
#define MAINFRAME_H
#include <cstdint>
#include <vector>
#include <memory>
#include <optional>
#include <wx/dcmemory.h>
#include <wx/dataview.h>
#include <landstalker/main/include/Rom.h>
#include <landstalker/main/include/ImageBuffer.h>
#include <user_interface/main/include/ImageList.h>
#include <user_interface/wxresource/include/wxcrafter.h>
#include <user_interface/tileset/include/TilesetEditorFrame.h>
#include <user_interface/text/include/StringEditorFrame.h>
#include <user_interface/palettes/include/PaletteListFrame.h>
#include <user_interface/rooms/include/RoomViewerFrame.h>
#include <user_interface/2d_maps/include/Map2DEditorFrame.h>
#include <user_interface/blockset/include/BlocksetEditorFrame.h>
#include <user_interface/sprites/include/SpriteEditorFrame.h>
#include <user_interface/sprites/include/EntityViewerFrame.h>
#include <user_interface/behaviours/include/BehaviourScriptEditorFrame.h>
#include <user_interface/script/include/ScriptEditorFrame.h>
#include <user_interface/script/include/ScriptTableEditorFrame.h>
#include <user_interface/script/include/ProgressFlagsFrame.h>
#include <user_interface/script/include/CharacterSfxFrame.h>
#include <landstalker/main/include/GameData.h>
#include <landstalker/misc/include/Labels.h>

#ifdef _WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

class wxImage;

class MainFrame : public MainFrameBaseClass
{
public:
    MainFrame(wxWindow* parent, const std::string& filename);
    virtual ~MainFrame();
protected:
    enum class ReturnCode
    {
        OK,
        ERR,
        CANCELLED
    };
    virtual void OnClose(wxCloseEvent& event);
    virtual void OnOpen(wxCommandEvent& event);
    virtual void OnSaveAsAsm(wxCommandEvent& event);
    virtual void OnSaveToRom(wxCommandEvent& event);
    virtual void OnSave(wxCommandEvent& event);
    virtual void OnBuildAsm(wxCommandEvent& event);
    virtual void OnRunEmulator(wxCommandEvent& event);
    virtual void OnPreferences(wxCommandEvent& event);
    virtual void OnMRUFile(wxCommandEvent& event);
    virtual void OnExit(wxCommandEvent& event);
    virtual void OnAbout(wxCommandEvent& event);
    virtual void OnBrowserSelect(wxTreeEvent& event);
private:
    enum class Mode
    {
        NONE,
        STRING,
        IMAGE,
        TILESET,
        ANIMATED_TILESET,
        BLOCKSET,
        PALETTE,
        ROOMMAP,
        SPRITE,
        ENTITY,
        BEHAVIOUR_SCRIPT,
        SCRIPT,
        SCRIPT_TABLE,
        SCRIPT_FUNCTIONS,
        PROGRESS_FLAGS,
        CHARACTER_SFX,
        ROM_DATA,
        CHARSET,
        INVENTORY_LAYOUT,
        FRIDAY_ANIMATION
    };

    enum class EditorType
    {
        TILESET,
        STRING,
        PALETTE,
        MAP_VIEW,
        MAP_2D,
        MAP_3D,
        BLOCKSET,
        SPRITE,
        ENTITY,
        BEHAVIOUR_SCRIPT,
        SCRIPT,
        SCRIPT_TABLE,
        SCRIPT_FUNCTIONS,
        PROGRESS_FLAGS,
        CHARACTER_SFX,
        ROM_DATA,
        CHARSET,
        INVENTORY_LAYOUT,
        FRIDAY_ANIMATION,
        NONE
    };
	void OnStatusBarInit(wxCommandEvent& event);
	void OnStatusBarUpdate(wxCommandEvent& event);
	void OnStatusBarClear(wxCommandEvent& event);
	void OnPropertiesInit(wxCommandEvent& event);
	void OnPropertiesUpdate(wxCommandEvent& event);
	void OnPropertiesClear(wxCommandEvent& event);
    void OnPropertyChange(wxPropertyGridEvent& event);
	void OnMenuInit(wxCommandEvent& event);
	void OnMenuClear(wxCommandEvent& event);
	void OnMenuClick(wxMenuEvent& event);
	void OnPaneClose(wxAuiManagerEvent& event);
    void OnGoToNavItem(wxCommandEvent& event);
    void OnRenameNavItem(wxCommandEvent& event);
    void OnDeleteNavItem(wxCommandEvent& event);
    void OnAddNavItem(wxCommandEvent& event);
    std::optional<wxTreeItemId> FindNavItem(const std::wstring& path);
    std::optional<wxTreeItemId> InsertNavItem(const std::wstring& path, int img = -1, const TreeNodeData::Node& type = TreeNodeData::Node::BASE, int value = 0, bool no_delete = true);
    void SortNavItems(const wxTreeItemId& parent);
    bool RemoveNavItem(const std::wstring& path);
    void GoToNavItem(const std::wstring& path, int data = 0);
    bool RenameNavItem(const std::wstring& old_path, const std::wstring& new_path);
    bool AddNavItem(const std::wstring& path, int image = -1, const TreeNodeData::Node& type = TreeNodeData::Node::BASE, int value = 0, bool no_delete = true);
    bool DeleteNavItem(const std::wstring& path);
    std::wstring GetNavItemParent(const std::wstring& path);
    void ShowEditor(EditorType editor);
    void HideAllEditors();
    ReturnCode CloseFiles(bool force = false);
    bool CheckForFileChanges();
    void OpenFile(const wxString& path);
    void OpenLabelsFile(std::string path);
    void OpenRomFile(const wxString& path);
    void OpenAsmFile(const wxString& path);
    void InitUI();
    void InitConfig();
    ReturnCode Save();
    ReturnCode SaveLabelsFile(std::string path = std::string());
    ReturnCode SaveAsAsm(std::string path = std::string());
    ReturnCode SaveToRom(std::string path = std::string());
    void SetMode(const Mode& mode);
    void RefreshEditor();
	ImageList& GetImageList();
    void ProcessSelectedBrowserItem(const wxTreeItemId& item, int data = 0);
    TilesetEditorFrame* GetTilesetEditor();
    StringEditorFrame* GetStringEditor();
    PaletteListFrame* GetPaletteEditor();
    RoomViewerFrame* GetRoomEditor();
    Map2DEditorFrame* GetMap2DEditor();
    BlocksetEditorFrame* GetBlocksetEditor();
    SpriteEditorFrame* GetSpriteEditor();
    EntityViewerFrame* GetEntityViewer();
    BehaviourScriptEditorFrame* GetBehaviourScriptEditor();
    ScriptEditorFrame* GetScriptEditor();
    ScriptTableEditorFrame* GetScriptTableEditor();
    ProgressFlagsEditorFrame* GetProgressFlagsEditorFrame();
    CharacterSfxEditorFrame* GetCharacterSfxEditorFrame();
    
    Mode m_mode;
    ImageList* m_imgs;
    ImageList* m_imgs32;
    EditorFrame* m_activeEditor;
    std::map<EditorType, EditorFrame*> m_editors;

    Rom m_rom;
    bool m_asmfile;
    std::shared_ptr<GameData> m_g;

    wxString m_last;
    bool m_last_was_asm;
    wxString m_last_asm;
    wxString m_last_rom;
    wxString m_built_rom;

    std::string m_selname;
    int m_seldata = 0;
    int m_extradata = 0;
};
#endif // MAINFRAME_H
