#pragma once

#include "VGA_esp32.h"

struct tile_struct{
    uint8_t*    buf = nullptr;
    uint32_t*   offset = nullptr;

    uint8_t tilesCount = 0;
    uint8_t maxTile = 0;

    uint8_t bpp = 0;    
    uint8_t width = 0, height = 0;
    int     maxX = 0, maxY = 0;   
    int     lineSize = 0;
    int     size = 0, fullSize = 0; 
    
    bool    loaded = false;
};
 
struct map_struct{
    uint8_t* buf = nullptr;

    uint32_t width = 0, height = 0;
    uint32_t maxX = 0, maxY = 0;

    uint8_t visX = 0, visY = 0;

    // Viewport
    int x0, x1;
    int y0, y1;

    bool loaded = false;
};

class Tiles{
    public:
        Tiles(VGA_esp32& vga);
        ~Tiles();

        bool loadTiles(const uint8_t* data, uint8_t blockWidth, uint8_t blockHeight, uint8_t offsetX = 0, uint8_t offsetY = 0);
        void putTile(int x, int y, uint8_t num);
        void destroy();

        bool loadMap(const uint8_t* data);
        void drawMap(int camX, int camY);

    private:
        tile_struct _t;
        map_struct  _m;
        VGA_esp32&  _vga;  
};