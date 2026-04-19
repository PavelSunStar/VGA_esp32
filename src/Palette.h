#pragma once

#include "VGA_esp32.h"
#include "structure.h"

class Palette{
    public:
        Palette(VGA_esp32& vga);
        ~Palette();

        bool init(palette_struct &src, uint8_t size = 256);
        bool loadPal(const uint8_t* src, palette_struct &dest);
        void destroy(palette_struct &src);

    private:
        uint8_t _bpp = 0;

        bool allocateMemory(uint8_t size);

        VGA_esp32 &_vga;

    protected:
        void loadPalDef();    
};