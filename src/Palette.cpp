#include "Palette.h"

Palette::Palette(VGA_esp32& vga) : _vga(vga){

}

Palette::~Palette(){

}

bool Palette::init(palette_struct &src, uint8_t size){
    if (size < 2) return false;
    destroy(src);

    _bpp = _vga._scr.bpp;
    src.buf = (uint8_t*)malloc(size * sizeof(uint16_t));
    if (!src.buf) return false;
    src.buf16 = (_bpp == _16BIT ? (uint16_t*)src.buf : nullptr); 
    
    src.size = size;
    return true;
}

bool Palette::loadPal(const uint8_t* src, palette_struct &dest){
    if (!src) return false;
    destroy(dest);

    _bpp = _vga._scr.bpp;
    const uint8_t* buf = src;
    uint8_t srcFormat = *buf++;
    uint8_t size = *buf++;
    if (((srcFormat != _16BIT) && (srcFormat != _8BIT)) || (size < 2)) return false;

    if (_bpp == _16BIT){
        dest.buf = (uint8_t*)malloc(size * sizeof(uint16_t));
        if (!dest.buf) return false;
        dest.buf16 = (uint16_t*)dest.buf; 
    } else {
        dest.buf = (uint8_t*)malloc(size * sizeof(uint8_t));
        if (!dest.buf) return false;
        dest.buf16 = nullptr; 
    }

    dest.size = size;
    
    // -----------------------------
    // load / convert palette
    // -----------------------------
    if (_bpp == srcFormat){
        if (_bpp == _16BIT){
            // 16 -> 16
            uint16_t* buf16 = (uint16_t*)buf;
            for (int i = 0; i < dest.size; i++){
                dest.buf16[i] = *buf16++;
            }
        } else {
            // 8 -> 8
            for (int i = 0; i < dest.size; i++){
                dest.buf[i] = *buf++;
            }
        }
    } else if ((_bpp == _16BIT) && (srcFormat == _8BIT)){
        // 8 -> 16   RGB332 -> RGB565
        for (int i = 0; i < dest.size; i++){
            uint8_t c = *buf++;

            uint8_t r = (c >> 5) & 0x07;
            uint8_t g = (c >> 2) & 0x07;
            uint8_t b =  c       & 0x03;

            // expand to 8-bit
            r = (r << 5) | (r << 2) | (r >> 1);
            g = (g << 5) | (g << 2) | (g >> 1);
            b = (b << 6) | (b << 4) | (b << 2) | b;

            // RGB565
            dest.buf16[i] =
                ((r & 0xF8) << 8) |
                ((g & 0xFC) << 3) |
                ((b & 0xF8) >> 3);
        }
    } else if ((_bpp == _8BIT) && (srcFormat == _16BIT)){
        // 16 -> 8   RGB565 -> RGB332
        uint16_t* buf16 = (uint16_t*)buf;
        for (int i = 0; i < dest.size; i++){
            uint16_t c = *buf16++;

            uint8_t r = (c >> 11) & 0x1F;
            uint8_t g = (c >> 5)  & 0x3F;
            uint8_t b =  c        & 0x1F;

            dest.buf[i] =
                ((r >> 2) << 5) |
                ((g >> 3) << 2) |
                (b >> 3);
        }
    } else {
        Serial.println("Error: Unsupported palette conversion.");
        destroy(dest);
        return false;
    }

    return true;
}


/*
bool Palette::loadPal(uint8_t* pal){
    if (_palInited) destroy();

    _bpp = _vga._scr.bpp;
    if (_bpp != _16BIT && _bpp != _8BIT) return (_palInited = false);

    auto &p = _pal;
    p.format = _bpp;

    // -----------------------------
    // no external palette -> create default
    // -----------------------------
    if (pal == nullptr){
        if (palSize < 2) return (_palInited = false);

        p.size = palSize;

        if (_bpp == _16BIT){
            p.buf = (uint8_t*)malloc(p.size * sizeof(uint16_t));
            if (!p.buf) return (_palInited = false);
            p.buf16 = (uint16_t*)p.buf;
        } else {
            p.buf = (uint8_t*)malloc(p.size * sizeof(uint8_t));
            if (!p.buf) return (_palInited = false);
            p.buf16 = nullptr;
        }

        loadPalDef();
        return (_palInited = true);
    }

    // -----------------------------
    // external palette format:
    // [0] = src format (_8BIT / _16BIT)
    // [1] = palette size
    // [2...] = palette data
    // -----------------------------
    uint8_t* buf = pal;
    uint8_t srcFormat = *buf++;
    p.size = *buf++;

    if (p.size < 2){
        Serial.println("Error: Too small palette size.");
        return (_palInited = false);
    }

    if (srcFormat != _8BIT && srcFormat != _16BIT){
        Serial.println("Error: Unknown palette format.");
        return (_palInited = false);
    }

    // allocate memory in CURRENT VGA format
    if (_bpp == _16BIT){
        p.buf = (uint8_t*)malloc(p.size * sizeof(uint16_t));
        if (!p.buf) return (_palInited = false);
        p.buf16 = (uint16_t*)p.buf;
    } else {
        p.buf = (uint8_t*)malloc(p.size * sizeof(uint8_t));
        if (!p.buf) return (_palInited = false);
        p.buf16 = nullptr;
    }

    // -----------------------------
    // load / convert palette
    // -----------------------------
    if (_bpp == srcFormat){
        if (_bpp == _16BIT){
            // 16 -> 16
            uint16_t* buf16 = (uint16_t*)buf;
            for (int i = 0; i < p.size; i++){
                p.buf16[i] = *buf16++;
            }
        } else {
            // 8 -> 8
            for (int i = 0; i < p.size; i++){
                p.buf[i] = *buf++;
            }
        }
    } else if ((_bpp == _16BIT) && (srcFormat == _8BIT)){
        // 8 -> 16   RGB332 -> RGB565
        for (int i = 0; i < p.size; i++){
            uint8_t c = *buf++;

            uint8_t r = (c >> 5) & 0x07;
            uint8_t g = (c >> 2) & 0x07;
            uint8_t b =  c       & 0x03;

            // expand to 8-bit
            r = (r << 5) | (r << 2) | (r >> 1);
            g = (g << 5) | (g << 2) | (g >> 1);
            b = (b << 6) | (b << 4) | (b << 2) | b;

            // RGB565
            p.buf16[i] =
                ((r & 0xF8) << 8) |
                ((g & 0xFC) << 3) |
                ((b & 0xF8) >> 3);
        }
    } else if ((_bpp == _8BIT) && (srcFormat == _16BIT)){
        // 16 -> 8   RGB565 -> RGB332
        uint16_t* buf16 = (uint16_t*)buf;
        for (int i = 0; i < p.size; i++){
            uint16_t c = *buf16++;

            uint8_t r = (c >> 11) & 0x1F;
            uint8_t g = (c >> 5)  & 0x3F;
            uint8_t b =  c        & 0x1F;

            p.buf[i] =
                ((r >> 2) << 5) |
                ((g >> 3) << 2) |
                (b >> 3);
        }
    } else {
        Serial.println("Error: Unsupported palette conversion.");
        destroy();
        return false;
    }

    return (_palInited = true);
}

void Palette::loadPalDef(){
    if (_bpp == _16BIT){
        for (int i = 0; i < _pal.size; i++){
            // разбираем индекс как RGB332
            uint8_t r = (i >> 5) & 0x07; // 3 бита
            uint8_t g = (i >> 2) & 0x07; // 3 бита
            uint8_t b =  i       & 0x03; // 2 бита

            // расширяем до 8 бит
            r = (r << 5) | (r << 2) | (r >> 1); // 3 → 8
            g = (g << 5) | (g << 2) | (g >> 1); // 3 → 8
            b = (b << 6) | (b << 4) | (b << 2) | b; // 2 → 8

            // переводим в RGB565
            _pal.buf16[i] =
                ((r & 0xF8) << 8) |
                ((g & 0xFC) << 3) |
                ((b & 0xF8) >> 3);
        }
    } else {
        for (int i = 0; i < _pal.size; i++){
            _pal.buf[i] = i; // обычный RGB332
        }
    }
}

uint16_t Palette::getColor(uint8_t r, uint8_t g, uint8_t b){
    if (_bpp == _16BIT){
        // RGB565: R(5) G(6) B(5)
        return ((r & 0xF8) << 8) |   // 5 бит красного
               ((g & 0xFC) << 3) |   // 6 бит зелёного
               ((b & 0xF8) >> 3);    // 5 бит синего
    } else {
        // RGB332: R(3) G(3) B(2)
        return ((r & 0xE0)     ) |   // 3 бита
               ((g & 0xE0) >> 3) |   // 3 бита
               ((b & 0xC0) >> 6);    // 2 бита
    }
}

void Palette::set(uint8_t index, uint16_t col){
    if (index >= _pal.size || !_palInited) return;

    if (_bpp == _16BIT){
        _pal.buf16[index] = col;
    } else {
        _pal.buf[index] = (uint8_t) col;
    }
}

uint16_t Palette::get(uint8_t index){
    if (index >= _pal.size || !_palInited) return 0;

    return (_bpp == _16BIT ? _pal.buf16[index] : _pal.buf[index]);
}
*/
void Palette::destroy(palette_struct &src){
    if (src.buf){
        free(src.buf);
        src.buf = nullptr;
    }

    src.buf16 = nullptr;
    src.size = 0;
}