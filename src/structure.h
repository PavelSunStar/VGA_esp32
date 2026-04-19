#pragma once;

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

struct Screen{
    // Buffer
    uint8_t*    buf = nullptr;
    uint8_t**   line8 = nullptr;
    uint16_t**  line16 = nullptr;
    uint8_t*    bg = nullptr;

    // Screen config
    uint8_t     bpp;
	int         width, height;
	int         maxX, maxY;
	int         cx, cy;
    int         lineSize;
    int         size, fullSize;
    
    // viewport
    int         x0, x1;
    int         y0, y1;
};

struct virtualImage{
    uint8_t*    buf;
    int         width, height;
    uint8_t     bpp;

    // viewport
    int         x0, x1;
    int         y0, y1;
};

struct Image{
    int width, height;
    int maxX, maxY;
    int cx, cy;    
    int lineSize, size;
    int fullSize;
    int offset, offsetLine;

    // viewport
    int x0, y0;
    int x1, y1;    
};

struct palette_struct{
    uint8_t size;
    uint8_t* buf;
    uint16_t* buf16;
};