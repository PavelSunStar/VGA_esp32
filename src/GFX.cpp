#include "GFX.h"

GFX::GFX(VGA_esp32& vga) : _vga(vga){

}

GFX::~GFX(){

}

void GFX::cls(uint16_t col){
    if (!_vga._inited) return;

    if (_vga._scr.bpp == _16BIT){
        uint16_t* scr = (uint16_t*)(_vga._scr.buf + _vga._backBuf);

        if ((uint8_t)col == (uint8_t)(col >> 8)){
            memset(scr, (uint8_t)(col & 0xFF),_vga._scr.fullSize);
        } else {
            int size = 0;
            uint16_t* cpy = scr;
            while (size++ < _vga._scr.width) *scr++ = col;

            int dummy = 1; 
            int lines = _vga._scr.maxY;  
            int copyBytes = _vga._scr.lineSize;
            int offset = _vga._scr.width;
        
            while (lines > 0){ 
                if (lines >= dummy){
                    memcpy(scr, cpy, copyBytes);
                    lines -= dummy;
                    scr += offset;
                    copyBytes <<= 1;
                    offset <<= 1;
                    dummy <<= 1;
                } else {
                    copyBytes =_vga._scr.lineSize * lines;
                    memcpy(scr, cpy, copyBytes);
                    break;
                }
            }            
        }
    } else {
        memset(_vga._scr.buf + _vga._backBuf, (uint8_t)(col & 0xFF), _vga._scr.fullSize);
    }    
}

void GFX::putPixel(int x, int y, uint16_t col){
    if (!_vga._inited || x < _vga._scr.x1 || y < _vga._scr.y1 || x > _vga._scr.x2 ||  y > _vga._scr.y2) return;

    if (_vga._scr.bpp == _16BIT){
        _vga._scr.line16[_vga._backBufLine + y][x] = col;
    } else {
        _vga._scr.line8[_vga._backBufLine + y][x] = (uint8_t)(col & 0xFF);
    }
}

void GFX::putPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b){
    if (!_vga._inited || x < _vga._scr.x1 || y < _vga._scr.y1 || x > _vga._scr.x2 ||  y > _vga._scr.y2) return;

    if (_vga._scr.bpp == _16BIT){
        _vga._scr.line16[_vga._backBufLine + y][x] = RGB16(r, g, b);
    } else {
        _vga._scr.line8[_vga._backBufLine + y][x] = RGB8(r, g, b);
    }
}

void GFX::putPixel(int x, int y, uint16_t col, Screen &scr){
    if (!_vga._inited || x < scr.x1 || y < scr.y1 || x > scr.x2 || y > scr.y2) return;

    if (scr.bpp == _16BIT){
        scr.line16[y][x] = col;
    } else {
        scr.line8[y][x] = (uint8_t)(col & 0xFF);
    }
}

void GFX::putPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, Screen &scr){
    if (!scr.buf || x < scr.x1 || y < scr.y1 || x > scr.x2 || y > scr.y2) return;

    if (scr.bpp == _16BIT){
        scr.line16[y][x] = RGB16(r, g, b);
    } else {
        scr.line8[y][x] = RGB8(r, g, b);
    }
}

void GFX::hLine(int x1, int y, int x2, uint16_t col){
    if (!_vga._inited) return;
    
    auto& s = _vga._scr;
    if (y < s.y1 || y > s.y2) return;
    if (x1 > x2) std::swap(x1, x2);
    if (x1 > s.x2 || x2 < s.x1) return;

    x1 = std::max(s.x1, x1);
    x2 = std::min(s.x2, x2);

    if (s.bpp == _16BIT){
        uint16_t* scr = &s.line16[_vga._backBufLine + y][x1]; 

        if ((uint8_t)col == (uint8_t)(col >> 8)){
            memset(scr, (uint8_t)(col & 0xFF), (x2 - x1 + 1) << 1);
        } else {
            while (x1++ <= x2) *scr++ = col;
        }          
    } else {
        uint8_t* scr = &s.line8[_vga._backBufLine + y][x1];
        memset(scr, (uint8_t)(col & 0xFF), x2 - x1 + 1);        
    }
}

void GFX::vLine(int x, int y1, int y2, uint16_t col){
    if (!_vga._inited) return;
    
    auto& s = _vga._scr;
    if (x < s.x1 || x > s.x2) return;
    if (y1 > y2) std::swap(y1, y2);
    if (y1 > s.y2 || y2 < s.y1) return;

    y1 = std::max(s.y1, y1);
    y2 = std::min(s.y2, y2);

    int skip = s.width;
    if (s.bpp == _16BIT){ 
        uint16_t* scr = &s.line16[_vga._backBufLine + y1][x];

        while (y1++ <= y2){ 
            *scr = col;
            scr += skip;
        }
    } else {
        uint8_t* scr = &s.line8[_vga._backBufLine + y1][x];

        uint8_t color = (uint8_t)(col & 0xFF);
        while (y1++ <= y2){ 
            *scr = color;
            scr += skip;
        }       
    }      
}

/*
void GFX::fillRect(int x1, int y1, int x2, int y2, uint16_t col){
    if (!_vga._inited) return;

    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);

    if (x1 > _vga._scr.x2 || x2 < _vga._scr.x1 ||
        y1 > _vga._scr.y2 || y2 < _vga._scr.y1) return;

    x1 = std::max(x1, _vga._scr.x1);
    y1 = std::max(y1, _vga._scr.y1);
    x2 = std::min(x2, _vga._scr.x2);
    y2 = std::min(y2, _vga._scr.y2);

    int w = x2 - x1 + 1;
    int h = y2 - y1 + 1;

    if (_vga._scr.bpp == _16BIT){
        uint16_t* dst = &_vga._scr.line16[_vga._backBufLine + y1][x1];

        if ((uint8_t)col == (uint8_t)(col >> 8)){
            int bytes = w << 1;
            for (int y = 0; y < h; y++){
                memset(dst, (uint8_t)(col & 0xFF), bytes);
                dst += _vga._scr.width;
            }
        } else {
            uint16_t* first = dst;

            for (int i = 0; i < w; i++) first[i] = col;
            dst += _vga._scr.width;

            int done = 1;
            while (done < h){
                int copyLines = done;
                if (done + copyLines > h) copyLines = h - done;

                memcpy(dst, first, copyLines * (w << 1));
                dst += copyLines * _vga._scr.width;
                done += copyLines;
            }
        }
    } else {
        uint8_t c = (uint8_t)(col & 0xFF);
        uint8_t* dst = &_vga._scr.line8[_vga._backBufLine + y1][x1];

        for (int y = 0; y < h; y++){
            memset(dst, c, w);
            dst += _vga._scr.width;
        }
    }
}
*/

void GFX::testRGBPanel() {
    if (!_vga._inited) return;

    auto& s = _vga._scr;
    if (s.width <= 0 || s.height <= 0) return;

    const int w = s.width;
    const int h = s.height;

    // Центр экрана
    const int midX = w >> 1;
    const int midY = h >> 1;

    // Размер тестового RGB-блока относительно экрана
    // Можно менять коэффициенты как нравится
    int rectW = w / 2;      // 50% ширины экрана
    int rectH = h / 5;      // 20% высоты экрана

    // Ограничения, чтобы не был слишком маленький или слишком большой
    if (rectW < 96)  rectW = 96;
    if (rectH < 60)  rectH = 60;

    if (rectW > w) rectW = w;
    if (rectH > h) rectH = h;

    // Желательно, чтобы высота делилась на 3 полосы ровно
    rectH -= (rectH % 3);
    if (rectH < 3) rectH = 3;

    const int bandH = rectH / 3;

    // Координаты прямоугольника строго по центру
    const int cx = midX - (rectW >> 1);
    const int cy = midY - (rectH >> 1);

    const int maxX = (w > 1) ? (w - 1) : 1;
    const int maxY = (h > 1) ? (h - 1) : 1;

    if (s.bpp == 16) {
        // ---- фон (градиент на весь экран) ----
        for (int y = 0; y < h; y++) {
            uint8_t gy = (uint32_t)y * 255 / maxY;
            uint16_t gg = (uint16_t)(gy >> 2);   // 0..63

            uint16_t* row = s.line16[y];
            for (int x = 0; x < w; x++) {
                uint8_t rx = (uint32_t)x * 255 / maxX;
                uint16_t rr = (uint16_t)(rx >> 3);          // 0..31
                uint16_t bb = (uint16_t)((255 - rx) >> 3);  // 0..31
                row[x] = (rr << 11) | (gg << 5) | bb;
            }
        }

        hLine(0, 0, s.maxX, 65535);
        hLine(0, s.cy, s.maxX, 65535);
        hLine(0, s.maxY, s.maxX, 65535);

        vLine(0, 0, s.maxY, 65535);
        vLine(s.cx, 0, s.maxY, 65535);
        vLine(s.maxX, 0, s.maxY, 65535);  

        // ---- центральный RGB-блок ----
        for (int y = 0; y < bandH; y++) {
            uint16_t* rowR = s.line16[cy + y];
            uint16_t* rowG = s.line16[cy + y + bandH];
            uint16_t* rowB = s.line16[cy + y + bandH * 2];

            for (int x = 0; x < rectW; x++) {
                uint8_t v = (rectW > 1) ? ((uint32_t)x * 255 / (rectW - 1)) : 255;

                uint16_t r = (uint16_t)(v >> 3); // 5 бит
                uint16_t g = (uint16_t)(v >> 2); // 6 бит
                uint16_t b = (uint16_t)(v >> 3); // 5 бит

                rowR[cx + x] = (r << 11);  // Красный
                rowG[cx + x] = (g << 5);   // Зелёный
                rowB[cx + x] = b;          // Синий
            }
        }

        // ---- белая рамка вокруг блока ----
        for (int x = 0; x < rectW; x++) {
            s.line16[cy][cx + x]             = 0xFFFF;
            s.line16[cy + rectH - 1][cx + x] = 0xFFFF;
        }
        for (int y = 0; y < rectH; y++) {
            s.line16[cy + y][cx]             = 0xFFFF;
            s.line16[cy + y][cx + rectW - 1] = 0xFFFF;
        }

    } else {
        // ---- фон (градиент на весь экран) ----
        for (int y = 0; y < h; y++) {
            uint8_t gy = (uint32_t)y * 255 / maxY;
            uint8_t gg = gy >> 5;   // 0..7

            uint8_t* row = s.line8[y];
            for (int x = 0; x < w; x++) {
                uint8_t rx = (uint32_t)x * 255 / maxX;
                uint8_t rr = rx >> 5;          // 0..7
                uint8_t bb = (255 - rx) >> 6;  // 0..3
                row[x] = (uint8_t)((rr << 5) | (gg << 2) | bb);
            }
        }

        hLine(0, 0, s.maxX, 255);
        hLine(0, s.cy, s.maxX, 255);
        hLine(0, s.maxY, s.maxX, 255);

        vLine(0, 0, s.maxY, 255);
        vLine(s.cx, 0, s.maxY, 255);
        vLine(s.maxX, 0, s.maxY, 255);        

        // ---- центральный RGB-блок ----
        for (int y = 0; y < bandH; y++) {
            uint8_t* rowR = s.line8[cy + y];
            uint8_t* rowG = s.line8[cy + y + bandH];
            uint8_t* rowB = s.line8[cy + y + bandH * 2];

            for (int x = 0; x < rectW; x++) {
                uint8_t v = (rectW > 1) ? ((uint32_t)x * 255 / (rectW - 1)) : 255;

                uint8_t r = (v >> 5); // 3 бит
                uint8_t g = (v >> 5); // 3 бит
                uint8_t b = (v >> 6); // 2 бит

                rowR[cx + x] = (uint8_t)(r << 5); // RRR00000
                rowG[cx + x] = (uint8_t)(g << 2); // 000GGG00
                rowB[cx + x] = (uint8_t)(b);      // 000000BB
            }
        }

        // ---- белая рамка вокруг блока ----
        const uint8_t white = 0xFF;
        for (int x = 0; x < rectW; x++) {
            s.line8[cy][cx + x]             = white;
            s.line8[cy + rectH - 1][cx + x] = white;
        }
        for (int y = 0; y < rectH; y++) {
            s.line8[cy + y][cx]             = white;
            s.line8[cy + y][cx + rectW - 1] = white;
        }
    }
}