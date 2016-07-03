#ifndef MAINFRAME_H
#define MAINFRAME_H
#include "wxcrafter.h"
#include <cstdint>
#include <vector>
#include <wx/dcmemory.h>
#include "BigTile.h"
#include "TileBitmap.h"
#include "Palette.h"
#include "LSTilemapCmp.h"

class wxImage;

class MainFrame : public MainFrameBaseClass
{
public:
    MainFrame(wxWindow* parent);
    virtual ~MainFrame();

    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
protected:
    virtual void OnTreectrl101TreeItemActivated(wxTreeEvent& event);
    virtual void OnAuimgr127Paint(wxPaintEvent& event);
    virtual void OnMenuitem109MenuSelected(wxCommandEvent& event);
    virtual void OnButton51ButtonClicked(wxCommandEvent& event);
    virtual void OnScrollwin27Paint(wxPaintEvent& event);
    virtual void OnPaint(wxPaintEvent& event);
    virtual void OnButton41ButtonClicked(wxCommandEvent& event);
private:
    struct RoomData
    {
        RoomData(const uint8_t* src)
        {
            offset = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
            params[0] = src[4];
            params[1] = src[5];
            params[2] = src[6];
            params[3] = src[7];
        }
        uint32_t offset;
        uint8_t params[4];
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
            NODE_ROOM
        };
        TreeNodeData(NodeType nodeType = NODE_BASE, size_t value = 0) : m_nodeType(nodeType), m_value(value) {}
        size_t GetValue() const { return m_value; }
        NodeType GetNodeType() const { return m_nodeType; }
    private:
        const NodeType m_nodeType;
        const size_t m_value;
    };
    void DrawTiles(size_t row_width = -1, size_t scale = 1, uint8_t pal = 0);
    void DrawBigTiles(size_t row_width = -1, size_t scale = 1, uint8_t pal = 0);
    void DrawTilemap(size_t scale, uint8_t pal);
    void PaintNow(wxDC& dc, size_t scale = 1);
    void InitPals(const wxTreeItemId& node);
    void LoadTileset(size_t offset);
    void LoadTilemap(size_t offset);
    void LoadBigTiles(size_t offset);
    void OpenRomFile(const wxString& path);
    
    Tilemap m_tilemap;
    uint8_t m_rom[2*1024*1024];
    uint8_t m_gfxBuffer[65536];
    size_t m_gfxSize;
    wxMemoryDC memDc;
    wxBitmap bmp;
    std::vector<RoomData> m_rooms;
    std::vector<Palette> m_pal2;
    TileBitmap m_tilebmps;
    wxImage m_img;
    size_t m_scale;
    uint8_t m_rpalidx;
    uint8_t m_tsidx;
    uint16_t m_roomnum;
    std::vector<uint32_t> m_tilesetOffsets;
    std::vector<std::vector<uint32_t>> m_bigTileOffsets;
    std::vector<BigTile> m_bigTiles;
    uint16_t m_pal[54][15];
    ImgLst* m_imgs;
};
#endif // MAINFRAME_H
