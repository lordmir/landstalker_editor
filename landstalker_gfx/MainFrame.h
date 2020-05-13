#ifndef MAINFRAME_H
#define MAINFRAME_H
#include "wxcrafter.h"
#include <cstdint>
#include <vector>
#include <memory>
#include <wx/dcmemory.h>
#include "BigTile.h"
#include "Tileset.h"
#include "Palette.h"
#include "LSTilemapCmp.h"
#include "Rom.h"
#include "SpriteGraphic.h"
#include "SpriteFrame.h"
#include "Sprite.h"
#include "ImageBuffer.h"

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
    virtual void OnLayerOpacityChange(wxScrollEvent& event);
    virtual void OnLayerSelect(wxCommandEvent& event);
    virtual void OnLayerVisibilityChange(wxCommandEvent& event);
    virtual void OnPaint(wxPaintEvent& event);
    virtual void OnButton1(wxCommandEvent& event);
    virtual void OnButton2(wxCommandEvent& event);
    virtual void OnButton3(wxCommandEvent& event);
    virtual void OnButton4(wxCommandEvent& event);
    virtual void OnButton5(wxCommandEvent& event);
    virtual void OnButton6(wxCommandEvent& event);
    virtual void OnButton7(wxCommandEvent& event);
    virtual void OnButton8(wxCommandEvent& event);
    virtual void OnButton9(wxCommandEvent& event);
    virtual void OnButton10(wxCommandEvent& event);
    virtual void OnMousewheel(wxMouseEvent& event);
    virtual void OnKeyDown(wxKeyEvent& event);
    virtual void OnKeyUp(wxKeyEvent& event);
    virtual void OnOpen(wxCommandEvent& event);
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
private:
    struct RoomData
    {
        uint32_t offset;
        uint8_t tileset;
        uint8_t priBigTileset;
        uint8_t secBigTileset;
        uint8_t bigTilesetIdx;
        uint8_t roomPalette;
        uint8_t backgroundMusic;
        uint8_t unknownParam1;
        uint8_t unknownParam2;
        uint8_t unknownParam3;

        RoomData(const uint8_t* src)
        :   offset((src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3]),
            tileset(src[4] & 0x1F),
            priBigTileset((src[4] >> 5) & 0x01),
            secBigTileset((src[7] >> 5) & 0x07),
            bigTilesetIdx(priBigTileset << 5 | tileset),
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
            NODE_TILESET,
            NODE_ANIM_TILESET,
            NODE_BIG_TILES,
            NODE_ROOM_PAL,
            NODE_ROOM,
            NODE_ROOM_HEIGHTMAP,
            NODE_SPRITE,
            NODE_SPRITE_FRAME
        };
        TreeNodeData(NodeType nodeType = NODE_BASE, size_t value = 0) : m_nodeType(nodeType), m_value(value) {}
        size_t GetValue() const { return m_value; }
        NodeType GetNodeType() const { return m_nodeType; }
    private:
        const NodeType m_nodeType;
        const size_t m_value;
    };
    enum Mode
    {
        MODE_NONE,
        MODE_TILESET,
        MODE_BLOCKSET,
        MODE_PALETTE,
        MODE_ROOMMAP,
        MODE_SPRITE
    };
    void DrawTiles(size_t row_width = -1, size_t scale = 1, uint8_t pal = 0);
    void DrawBigTiles(size_t row_width = -1, size_t scale = 1, uint8_t pal = 0);
    void DrawTilemap(size_t scale, uint8_t pal);
    void DrawHeightmap(size_t scale, uint16_t room);
    void DrawSprite(const SpriteFrame& sprite, uint8_t pal_idx, size_t scale = 4);
    void ForceRepaint();
    void PaintNow(wxDC& dc, size_t scale = 1);
    void InitPals(const wxTreeItemId& node);
    void LoadTileset(size_t offset);
    void LoadTilemap(size_t offset);
    void LoadBigTiles(size_t offset);
    void OpenRomFile(const wxString& path);
    void InitRoom(uint16_t room);
    void PopulateRoomProperties(uint16_t room, const RoomTilemap& tm);
    void EnableLayerControls(bool state);
    void SetMode(const Mode& mode);
    void Refresh();
    
    RoomTilemap m_tilemap;
    Rom m_rom;
    uint8_t m_gfxBuffer[65536];
    size_t m_gfxSize;
    wxMemoryDC memDc;
    std::shared_ptr<wxBitmap> bmp;
    std::vector<RoomData> m_rooms;
    std::vector<Palette> m_pal2;
    std::vector<Palette> m_palette;
    Tileset m_tilebmps;
    ImageBuffer m_imgbuf;
    wxImage m_img;
    size_t m_scale;
    uint8_t m_rpalidx;
    uint8_t m_tsidx;
    uint8_t m_bs1;
    uint8_t m_bs2;
    uint16_t m_roomnum;
    uint16_t m_sprite_idx;
    uint16_t m_sprite_anim;
    uint16_t m_sprite_frame;
    Mode m_mode;
    bool m_layer_controls_enabled;
    std::vector<uint32_t> m_tilesetOffsets;
    std::vector<std::vector<uint32_t>> m_bigTileOffsets;
    std::vector<BigTile> m_bigTiles;
    std::vector<SpriteFrame> m_spriteFrames;
    std::vector<SpriteGraphic> m_spriteGraphics;
    std::map<uint8_t, Sprite> m_sprites;
    uint16_t m_pal[54][15];
    ImgLst* m_imgs;
};
#endif // MAINFRAME_H
