#pragma once

#include "VGA_esp32.h"

class Tiles{
    public:
        Tiles(VGA_esp32& vga);
        ~Tiles();

        bool loadTiles(const uint8_t* data, uint8_t blockSizeX, uint8_t blockSizeY, uint8_t offsetX = 0, uint8_t offsetY = 0);
        void draw(int x, int y, uint16_t num);

    private:
        uint16_t _maxTiles;
        int _tileSizeX, _tileSizeY;
        uint8_t* _tiles     = nullptr;

        VGA_esp32 &_vga; 
};