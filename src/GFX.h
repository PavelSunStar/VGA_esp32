#pragma once

#include "VGA_esp32.h"

class GFX{
    public:
        GFX(VGA_esp32& vga);
        ~GFX();

        inline uint16_t getColor(uint8_t r, uint8_t g, uint8_t b){
            if (_vga._scr.bpp == _16BIT) {
                return ((r >> 3) << 11) |
                    ((g >> 2) << 5 ) |
                    ( b >> 3);
            } else {
                return (r & 0xE0) | ((g >> 3) & 0x1C) | (b >> 6);
            }
        }

        // ---- 8 bit (RGB332) ----
        inline uint8_t RGB8(uint8_t r, uint8_t g, uint8_t b) { return ((r >> 5) << 5) | ((g >> 5) << 2) | (b >> 6); }
        inline uint8_t R8(uint8_t c) { return (c >> 5) & 0x07; }
        inline uint8_t G8(uint8_t c) { return (c >> 2) & 0x07; }
        inline uint8_t B8(uint8_t c) { return  c       & 0x03; }

        // ---- 16 bit (RGB565) ----
        inline uint16_t RGB16(uint8_t r, uint8_t g, uint8_t b) { return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3); }
        inline uint16_t R16(uint16_t c) { return (c >> 11) & 0x1F; }
        inline uint16_t G16(uint16_t c) { return (c >> 5)  & 0x3F; }
        inline uint16_t B16(uint16_t c) { return  c        & 0x1F; }

        void cls(uint16_t col = 0);
        void putPixel(int x, int y, uint16_t col);
        void putPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
        void putPixel(int x, int y, uint16_t col, Screen &scr);
        void putPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, Screen &scr);
        void hLine(int x1, int y, int x2, uint16_t col);
        void vLine(int x, int y1, int y2, uint16_t col);
        void testRGBPanel();

    private:
        Screen _img;

        VGA_esp32 &_vga; 
};