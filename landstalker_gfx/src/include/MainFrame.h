#ifndef MAINFRAME_H
#define MAINFRAME_H
#include "wxcrafter.h"
#include <cstdint>
#include <vector>
#include <memory>
#include <wx/dcmemory.h>
#include <wx/dataview.h>
#include "Block.h"
#include "Tileset.h"
#include "Palette.h"
#include "LSTilemapCmp.h"
#include "Rom.h"
#include "SpriteGraphic.h"
#include "SpriteFrame.h"
#include "Sprite.h"
#include "ImageBuffer.h"
#include "LSString.h"
#include "Images.h"
#include "ImageList.h"
#include "TilesetEditorFrame.h"
#include "TilesetManager.h"
#include "RoomManager.h"

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
    virtual void OnLayerOpacityChange(wxScrollEvent& event);
    virtual void OnLayerSelect(wxCommandEvent& event);
    virtual void OnLayerVisibilityChange(wxCommandEvent& event);
    virtual void OnPaint(wxPaintEvent& event);
    virtual void OnMousewheel(wxMouseEvent& event);
    virtual void OnKeyDown(wxKeyEvent& event);
    virtual void OnKeyUp(wxKeyEvent& event);
    virtual void OnOpen(wxCommandEvent& event);
    virtual void OnSaveAsAsm(wxCommandEvent& event);
    virtual void OnSaveToRom(wxCommandEvent& event);
    virtual void OnExport(wxCommandEvent& event);
    virtual void OnExit(wxCommandEvent& event);
    virtual void OnAbout(wxCommandEvent& event);
    virtual void OnBrowserSelect(wxTreeEvent& event);
    virtual void OnScrollWindowPaint(wxPaintEvent& event);
    virtual void OnScrollWindowMousewheel(wxMouseEvent& event);
    virtual void OnScrollWindowMouseMove(wxMouseEvent& event);
    virtual void OnScrollWindowLeftDown(wxMouseEvent& event);
    virtual void OnScrollWindowLeftUp(wxMouseEvent& event);
    virtual void OnScrollWindowRightDown(wxMouseEvent& event);
    virtual void OnScrollWindowRightUp(wxMouseEvent& event);
    virtual void OnScrollWindowKeyDown(wxKeyEvent& event);
    virtual void OnScrollWindowKeyUp(wxKeyEvent& event);
    virtual void OnScrollWindowResize(wxSizeEvent& event);
private:
    struct RoomData
    {
        uint32_t offset;
        uint8_t tileset;
        uint8_t priBlockset;
        uint8_t secBlockset;
        uint8_t blocksetIdx;
        uint8_t roomPalette;
        uint8_t backgroundMusic;
        uint8_t unknownParam1;
        uint8_t unknownParam2;
        uint8_t unknownParam3;

        RoomData(const uint8_t* src)
        :   offset((src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3]),
            tileset(src[4] & 0x1F),
            priBlockset((src[4] >> 5) & 0x01),
            secBlockset((src[7] >> 5) & 0x07),
            blocksetIdx(priBlockset << 5 | tileset),
            roomPalette(src[5] & 0x3F),
            backgroundMusic(src[7] & 0x1F),
            unknownParam1((src[4] >> 6) & 0x03),
            unknownParam2((src[5] >> 6) & 0x03),
            unknownParam3(src[6])
        {
        }
    };
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
            NODE_ROOM_PAL,
            NODE_ROOM,
            NODE_ROOM_HEIGHTMAP,
            NODE_ROOM_WARPS,
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
        MODE_BLOCKSET,
        MODE_PALETTE,
        MODE_ROOMMAP,
        MODE_SPRITE
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
    void DrawTiles(std::size_t row_width = -1, std::size_t scale = 1, uint8_t pal = 0);
    void DrawBlocks(std::size_t row_width = -1, std::size_t scale = 1, uint8_t pal = 0);
    void DrawTilemap(std::size_t scale, uint8_t pal);
    void DrawHeightmap(std::size_t scale, uint16_t room);
    void DrawWarps(std::size_t scale, uint16_t room);
    void DrawSprite(const Sprite& sprite, std::size_t animation, std::size_t frame, std::size_t scale = 4);
    void DrawImage(const std::string& image, std::size_t scale);
    void DrawWarp(wxGraphicsContext& gc, const WarpList::Warp& warp, std::shared_ptr<RoomManager::MapEntry> tilemap, int tile_width, int tile_height);
    void AddRoomLink(wxGraphicsContext* gc, const std::string& label, uint16_t room, int x, int y);
    void PopulatePalettes();
    void ShowStrings();
    void ShowTileset();
    void ShowBitmap();
    void ForceRepaint();
    void ClearScreen();
    void GoToRoom(uint16_t room);
    void PaintNow(wxDC& dc, std::size_t scale = 1);
    void InitPals(const wxTreeItemId& node);
    void LoadTileset(std::size_t offset);
    void LoadBlocks(std::size_t offset);
    ReturnCode CloseFiles(bool force = false);
    bool CheckForFileChanges();
    void OpenFile(const wxString& path);
    void OpenRomFile(const wxString& path);
    void OpenAsmFile(const wxString& path);
    ReturnCode Save();
    ReturnCode SaveAsAsm(std::string path = std::string());
    ReturnCode SaveToRom(std::string path = std::string());
    void InitRoom(uint16_t room);
    void PopulateRoomProperties(uint16_t room);
    void EnableLayerControls(bool state);
    void SetMode(const Mode& mode);
    void Refresh();
    bool ExportPng(const std::string& filename);
    bool ExportTxt(const std::string& filename);
	ImageList& GetImageList();
    void ProcessSelectedBrowserItem(const wxTreeItemId& item);
    
    Rom m_rom;
    std::vector<uint8_t> m_gfxBuffer;
    std::size_t m_gfxSize;
    wxMemoryDC memDc;
    std::shared_ptr<wxBitmap> bmp;
    std::vector<Palette> m_pal2;
    std::vector<Palette> m_palette;
    Tileset m_tilebmps;
    ImageBuffer m_imgbuf;
    wxImage m_img;
    std::size_t m_scale;
    uint8_t m_rpalidx;
    uint8_t m_tsidx;
    uint8_t m_bs1;
    uint8_t m_bs2;
    uint16_t m_roomnum;
    uint16_t m_sprite_idx;
    uint16_t m_sprite_anim;
    uint16_t m_sprite_frame;
    int m_strtab;
    Mode m_mode;
    bool m_layer_controls_enabled;
    std::vector<std::vector<std::shared_ptr<LSString>>> m_strings;
    std::vector<uint32_t> m_tilesetOffsets;
    std::vector<std::vector<uint32_t>> m_blockOffsets;
    std::vector<MapBlock> m_blocks;
    std::vector<SpriteFrame> m_spriteFrames;
    std::vector<SpriteGraphic> m_spriteGraphics;
    std::map<uint8_t, Sprite> m_sprites;
    std::string m_selImage;
    std::map<std::string, Images::Image> m_images;
    std::shared_ptr<std::map<std::string, Palette>> m_palettes;
    std::string m_selected_palette;
    uint16_t m_pal[54][15];
	ImageList* m_imgs;
    wxDataViewListCtrl* m_stringView;
    TilesetEditorFrame* m_tilesetEditor;
	EditorFrame* m_activeEditor;
    wxScrolledCanvas* m_canvas;
    std::list<std::pair<WarpList::Warp, std::vector<wxPoint2DDouble>>> m_warp_poly;
    std::list<std::pair<uint16_t, std::vector<wxPoint2DDouble>>> m_link_poly;

    bool m_asmfile;
    std::shared_ptr<TilesetManager> m_tsmgr;
    std::shared_ptr<RoomManager> m_rmgr;
    std::string m_tsname;
};
#endif // MAINFRAME_H
