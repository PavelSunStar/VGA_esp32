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

        //void cls(uint16_t col = 0);
        uint16_t getPixel(int x, int y);
        void putPixel(int x, int y, uint16_t col);
        void putPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
        void putPixel(int x, int y, uint8_t index, const palette_struct &pal);
        void hLine(int x0, int y, int x1, uint16_t col);
        void hLineOr(int x0, int y, int x1, uint16_t col);
        void hLineLength(int x, int y, int w, uint16_t col);
        void vLine(int x, int y0, int y1, uint16_t col);
        void vLineOr(int x, int y0, int y1, uint16_t col);
        void vLineLength(int x, int y, int h, uint16_t col);
        void line(int x0, int y0, int x1, int y1, uint16_t col);
        void rect(int x0, int y0, int x1, int y1, uint16_t col);
        void fillRect(int x0, int y0, int x1, int y1, uint16_t col);
        void triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t col);
        void circle(int xc, int yc, int r, uint16_t col);
        void fillCircle(int xc, int yc, int r, uint16_t col);

        void hBandOr(int y, int x0, int x1, uint16_t step);
        void vBandOr(int x, int y0, int y1, uint16_t step);     
        void hBandFill(int y, int x0, int x1, uint16_t step);
        void vBandFill(int x, int y0, int y1, uint16_t step);           
        void testRGBPanel();

    private:
        Screen _img;

        VGA_esp32 &_vga;
        
    protected:
        void hLineFast(int x, int y, int len, uint16_t col);
        void vLineFast(int x, int y, int len, uint16_t col);
};