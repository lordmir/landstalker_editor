#ifndef MAINFRAME_H
#define MAINFRAME_H
#include "wxcrafter.h"
#include <cstdint>
#include <vector>
#include <memory>
#include <wx/dcmemory.h>
#include <wx/dataview.h>
#include "Rom.h"
#include "ImageBuffer.h"
#include "ImageList.h"
#include "TilesetEditorFrame.h"
#include "StringEditorFrame.h"
#include "PaletteListFrame.h"
#include "RoomViewerFrame.h"
#include "Map2DEditorFrame.h"
#include "GameData.h"

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
    virtual void OnExport(wxCommandEvent& event);
    virtual void OnMRUFile(wxCommandEvent& event);
    virtual void OnExit(wxCommandEvent& event);
    virtual void OnAbout(wxCommandEvent& event);
    virtual void OnBrowserSelect(wxTreeEvent& event);
    virtual void OnScrollWindowPaint(wxPaintEvent& event);
private:
    class TreeNodeData : public wxTreeItemData
    {
    public:
        enum NodeType {
            NODE_BASE,
            NODE_STRING,
            NODE_IMAGE,
            NODE_TILESET,
            NODE_ANIM_TILESET,
            NODE_BLOCKSET,
            NODE_PALETTE,
            NODE_ROOM,
            NODE_SPRITE,
            NODE_SPRITE_FRAME
        };
        TreeNodeData(NodeType nodeType = NODE_BASE, std::size_t value = 0) : m_nodeType(nodeType), m_value(value) {}
        std::size_t GetValue() const { return m_value; }
        NodeType GetNodeType() const { return m_nodeType; }
    private:
        const NodeType m_nodeType;
        const std::size_t m_value;
    };
    enum Mode
    {
        MODE_NONE,
        MODE_STRING,
        MODE_IMAGE,
        MODE_TILESET,
        MODE_ANIMATED_TILESET,
        MODE_BLOCKSET,
        MODE_PALETTE,
        MODE_ROOMMAP,
        MODE_SPRITE
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
    void GoToNavItem(const std::string& path);
    void DrawBlocks(const std::string& name, std::size_t row_width = -1, std::size_t scale = 1, uint8_t pal = 0);
    void DrawSprite(const std::string& name, int data, std::size_t scale = 4);
    void ShowBitmap();
    void ShowEditor(EditorType editor);
    void HideAllEditors();
    void ForceRepaint();
    void ClearScreen();
    void PaintNow(wxDC& dc, std::size_t scale = 1);
    ReturnCode CloseFiles(bool force = false);
    bool CheckForFileChanges();
    void OpenFile(const wxString& path);
    void OpenRomFile(const wxString& path);
    void OpenAsmFile(const wxString& path);
    void InitUI();
    ReturnCode Save();
    ReturnCode SaveAsAsm(std::string path = std::string());
    ReturnCode SaveToRom(std::string path = std::string());
    void SetMode(const Mode& mode);
    void Refresh();
	ImageList& GetImageList();
    void ProcessSelectedBrowserItem(const wxTreeItemId& item);
    TilesetEditorFrame* GetTilesetEditor();
    StringEditorFrame* GetStringEditor();
    PaletteListFrame* GetPaletteEditor();
    RoomViewerFrame* GetRoomEditor();
    Map2DEditorFrame* GetMap2DEditor();
    
    wxMemoryDC memDc;
    std::shared_ptr<wxBitmap> bmp;
    ImageBuffer m_imgbuf;
    wxImage m_img;
    std::size_t m_scale;
    Mode m_mode;
	ImageList* m_imgs;
    EditorFrame* m_activeEditor;
    std::map<EditorType, EditorFrame*> m_editors;
    wxScrolledCanvas* m_canvas;

    Rom m_rom;
    bool m_asmfile;
    std::shared_ptr<GameData> m_g;

    std::string m_selname;
    int m_seldata;
};
#endif // MAINFRAME_H
