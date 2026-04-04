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

uint16_t GFX::getPixel(int x, int y){
    if (!_vga._inited) return 0;
    
    auto& s = _vga._scr;
    if (x < s.x0 || y < s.y0 || x > s.x1 ||  y > s.y1) return 0;

    if (s.bpp == _16BIT){
        return s.line16[y + _vga._backBufLine][x];
    } else {
        return s.line8[y + _vga._backBufLine][x];
    }

    return 0;    
}

void GFX::putPixel(int x, int y, uint16_t col){
    if (!_vga._inited) return;
    
    auto& s = _vga._scr;
    if (x < s.x0 || y < s.y0 || x > s.x1 ||  y > s.y1) return;

    if (s.bpp == _16BIT){
       s.line16[y + _vga._backBufLine][x] = col;
    } else {
        s.line8[y + _vga._backBufLine][x] = (uint8_t)(col & 0xFF);
    }
}

void GFX::putPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b){
    if (!_vga._inited) return;
    
    auto& s = _vga._scr;
    if (x < s.x0 || y < s.y0 || x > s.x1 ||  y > s.y1) return;

    if (s.bpp == _16BIT){
       s.line16[y + _vga._backBufLine][x] = RGB16(r, g, b);
    } else {
        s.line8[y + _vga._backBufLine][x] = RGB8(r, g, b);
    }
}

void GFX::hLine(int x0, int y, int x1, uint16_t col){
    if (!_vga._inited) return;
    
    auto& s = _vga._scr;
    if (y < s.y0 || y > s.y1) return;
    if (x0 > x1) std::swap(x0, x1);
    if (x0 > s.x1 || x1 < s.x0) return;

    x0 = std::max(s.x0, x0);
    x1 = std::min(s.x1, x1);
 
    if (s.bpp == _16BIT){
        if ((uint8_t)col == (uint8_t)(col >> 8)){
            memset(&s.line16[_vga._backBufLine + y][x0], (uint8_t)col, (x1 - x0 + 1) << 1);
        } else {
            uint16_t* scr = &s.line16[_vga._backBufLine + y][x0]; 
            while (x0++ <= x1) *scr++ = col;
        }          
    } else {
        memset(&s.line8[_vga._backBufLine + y][x0], (uint8_t)col, x1 - x0 + 1);        
    }
}

void GFX::hLineOr(int x0, int y, int x1, uint16_t col){
    if (!_vga._inited) return;
    
    auto& s = _vga._scr;
    if (y < s.y0 || y > s.y1) return;
    if (x0 > x1) std::swap(x0, x1);
    if (x0 > s.x1 || x1 < s.x0) return;

    x0 = std::max(s.x0, x0);
    x1 = std::min(s.x1, x1);
 
    if (s.bpp == _16BIT){
        uint16_t* scr = &s.line16[_vga._backBufLine + y][x0]; 
        while (x0++ <= x1) *scr++ |= col;
    } else {
        uint8_t* scr = &s.line8[_vga._backBufLine + y][x0];
        uint8_t c = (uint8_t)col;
        while (x0++ <= x1) *scr++ |= c;
    }
}

void GFX::hLineLength(int x, int y, int w, uint16_t col){
    if (!_vga._inited || w <= 0) return;

    auto& s = _vga._scr;
    int x1 = x + w - 1;
    if (x > s.x1 || (x1 < s.x0) || y < s.y0 || y > s.y1) return;

    x = std::max(s.x0, x);
    x1 = std::min(s.x1, x1);
    w = x1 - x + 1;

    if (s.bpp == _16BIT){
        if ((uint8_t)col == (uint8_t)(col >> 8)){
            memset(&s.line16[_vga._backBufLine + y][x], (uint8_t)col, w << 1);
        } else {
            uint16_t* scr = &s.line16[_vga._backBufLine + y][x];            
            while (w-- > 0) *scr++ = col;
        }          
    } else {
        memset(&s.line8[_vga._backBufLine + y][x], (uint8_t) col, w);        
    }    
}

void GFX::vLine(int x, int y0, int y1, uint16_t col){
    if (!_vga._inited) return;
    
    auto& s = _vga._scr;
    if (x < s.x0 || x > s.x1) return;
    if (y0 > y1) std::swap(y0, y1);
    if (y0 > s.y1 || y1 < s.y0) return;

    y0 = std::max(s.y0, y0);
    y1 = std::min(s.y1, y1);

    int skip = s.width;
    if (s.bpp == _16BIT){ 
        uint16_t* scr = &s.line16[_vga._backBufLine + y0][x];

        while (y0++ <= y1){ 
            *scr = col;
            scr += skip;
        }
    } else {
        uint8_t* scr = &s.line8[_vga._backBufLine + y0][x];

        uint8_t color = (uint8_t)(col & 0xFF);
        while (y0++ <= y1){ 
            *scr = color;
            scr += skip;
        }       
    }      
}

void GFX::vLineOr(int x, int y0, int y1, uint16_t col){
    if (!_vga._inited) return;
    
    auto& s = _vga._scr;
    if (x < s.x0 || x > s.x1) return;
    if (y0 > y1) std::swap(y0, y1);
    if (y0 > s.y1 || y1 < s.y0) return;

    y0 = std::max(s.y0, y0);
    y1 = std::min(s.y1, y1);

    int skip = s.width;
    if (s.bpp == _16BIT){ 
        uint16_t* scr = &s.line16[_vga._backBufLine + y0][x];

        while (y0++ <= y1){ 
            *scr |= col;
            scr += skip;
        }
    } else {
        uint8_t* scr = &s.line8[_vga._backBufLine + y0][x];

        uint8_t color = (uint8_t)(col & 0xFF);
        while (y0++ <= y1){ 
            *scr |= color;
            scr += skip;
        }       
    }  
}

void GFX::vLineLength(int x, int y, int h, uint16_t col){
    if (!_vga._inited || h <= 0) return;

    auto& s = _vga._scr;
    int y1 = y + h - 1;
    if (x > s.x1 || x < s.x0 || y > s.y1 || y1 < s.y0) return;

    y = std::max(s.y0, y);
    y1 = std::min(s.y1, y1);
    h = y1 - y + 1;    
    int skip = s.width;

    if (s.bpp == _16BIT){ 
        uint16_t* scr = &s.line16[_vga._backBufLine + y][x];

        while (h-- > 0){ 
            *scr = col;
            scr += skip;
        }
    } else {
        uint8_t* scr = &s.line8[_vga._backBufLine + y][x];
        uint8_t color = (uint8_t)col;

        while (h-- > 0){ 
            *scr = color;
            scr += skip;
        }       
    }     
}

void GFX::hLineFast(int x, int y, int len, uint16_t col){
    if (len <= 0) return;

    auto& s = _vga._scr;

    if (s.bpp == _16BIT){
        if ((uint8_t)col == (uint8_t)(col >> 8)){
            memset(&s.line16[_vga._backBufLine + y][x], (uint8_t)col, len << 1);
        } else {
            uint16_t* scr = &s.line16[_vga._backBufLine + y][x];            
            while (len-- > 0) *scr++ = col;
        }          
    } else {
        memset(&s.line8[_vga._backBufLine + y][x], (uint8_t) col, len);        
    } 
}

void GFX::vLineFast(int x, int y, int len, uint16_t col){
    if (len <= 0) return;

    auto& s = _vga._scr;
    int skip = s.width;

    if (s.bpp == _16BIT){ 
        uint16_t* scr = &s.line16[_vga._backBufLine + y][x];

        while (len-- > 0){ 
            *scr = col;
            scr += skip;
        }
    } else {
        uint8_t* scr = &s.line8[_vga._backBufLine + y][x];

        uint8_t color = (uint8_t)(col & 0xFF);
        while (len-- > 0){ 
            *scr = color;
            scr += skip;
        }       
    }  
}

void GFX::line(int x0, int y0, int x1, int y1, uint16_t col){
    if (!_vga._inited) return;

    auto& s = _vga._scr;
    bool    steep = abs(y1 - y0) > abs(x1 - x0); 
    int     xstart = s.x0;
    int     ystart = s.y0;
    int     xend   = s.x1;
    int     yend   = s.y1;

    if (steep){
        std::swap(xstart, ystart);
        std::swap(xend, yend);
        std::swap(x0, y0);
        std::swap(x1, y1);
    } if (x0 > x1){
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    if (x0 > xend || x1 < xstart) return;
    xend = std::min(x1, xend);
    
    int     dy = abs(y1 - y0);
    int     ystep = (y1 > y0) ? 1 : -1;
    int     dx = x1 - x0;
    int     err = dx >> 1;    

    while (x0 < xstart || y0 < ystart || y0 > yend){
        err -= dy;
        if (err < 0){
            err += dx;
            y0 += ystep;
        }
        
        if (++x0 > xend) return;
    } 
    int     xs = x0;
    int     dlen = 0;
    if (ystep < 0) std::swap(ystart, yend);
    yend += ystep;    
    
    if (steep){
        do{
            ++dlen;
            if ((err -= dy) < 0){
                vLineFast(y0, xs, dlen, col);
                err += dx;
                xs = x0 + 1; dlen = 0; y0 += ystep;
                if (y0 == yend) break;
            }
        } while (++x0 <= xend);

      if (dlen) vLineFast(y0, xs, dlen, col);
    } else {
        do{
            ++dlen;
            if ((err -= dy) < 0)
            {
            hLineFast(xs, y0, dlen, col);
            err += dx;
            xs = x0 + 1; dlen = 0; y0 += ystep;
            if (y0 == yend) break;
            }
      } while (++x0 <= xend);

      if (dlen) hLineFast(xs, y0, dlen, col);
    }    
}

void GFX::rect(int x0, int y0, int x1, int y1, uint16_t col){
    if (!_vga._inited) return;

    if (x0 == x1){
        vLine(x0, y0, y0, col);
        return;
    }

    if (y1 == y0){
        hLine(x0, y0, x1, col);
        return;
    }

    if (x0 > x1) std::swap(x0, x1);
    if (y0 > y1) std::swap(y0, y1);

    auto& s = _vga._scr;
    if (x0 > s.x1 || y0 > s.y1 || x1 < s.x0 ||  y1 < s.y0){
        return;
    } else if (x0 >= s.x0 && y0 >= s.y0 && x1 <= s.x1 && y1 <= s.y1){
        int sizeX = x1 - x0 + 1; 
        int sizeY = y1 - y0 - 1;
        int width = s.width;
        int skip1 = x1 - x0; 
        int skip2 = width - x1 + x0;

        if (s.bpp == _16BIT){
            uint16_t* scr = &s.line16[y0 + _vga._backBufLine][x0];
            uint16_t* cpy = scr;
            int lineSize = sizeX << 1;
            
            while (sizeX-- > 0) *scr++ = col;
            scr += skip2 - 1;

            while (sizeY-- > 0){
                *scr = col; scr += skip1;
                *scr = col; scr += skip2;
            }
            memcpy(scr, cpy, lineSize);
        } else {
            uint8_t* scr = &s.line8[y0 + _vga._backBufLine][x0];
            uint8_t color = (uint8_t)col;

            memset(scr, color, sizeX);
            scr += width;

            while (sizeY-- > 0){
                *scr = color; scr += skip1;
                *scr = color; scr += skip2;
            }
            memset(scr, color, sizeX);
        }
    } else {
        hLine(x0, y0, x1, col); 
        hLine(x0, y1, x1, col); 
        vLine(x0, y0, y1, col);  
        vLine(x1, y0, y1, col);  
    }
}

void GFX::fillRect(int x0, int y0, int x1, int y1, uint16_t col){
    if (!_vga._inited) return;

    if (x0 == x1){
        vLine(x0, y0, y1, col);
        return;
    }

    if (y1 == y0){
        hLine(x0, y0, x1, col);
        return;
    }

    if (x0 > x1) std::swap(x0, x1);
    if (y0 > y1) std::swap(y0, y1);

    auto& s = _vga._scr;
    if (x0 > s.x1 || y0 > s.y1 || x1 < s.x0 ||  y1 < s.y0) return; 
    
    x0 = std::max(s.x0, x0);
    x1 = std::min(s.x1, x1);
    y0 = std::max(s.y0, y0);
    y1 = std::min(s.y1, y1);  
    
    int sizeX = x1 - x0 + 1;
    int sizeY = y1 - y0 + 1;     
    int width = s.width;

    if (s.bpp == _16BIT){
        uint16_t* scr = &s.line16[y0 + _vga._backBufLine][x0];
        uint16_t* cpy = scr;

        int skip = width - x1 + x0 - 1;
        int copyBytes = sizeX << 1;
        
        while (sizeX-- > 0) *scr++ = col;
        scr += skip;
        sizeY--;

        while (sizeY-- > 0){
            memcpy(scr, cpy, copyBytes);
            scr += width;
        }        
    } else {
        uint8_t* scr = &s.line8[y0 + _vga._backBufLine][x0];
        uint8_t color = (uint8_t)col;

        while (sizeY-- > 0){
            memset(scr, color, sizeX);
            scr += width;
        }        
    }
}

void GFX::triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t col){
    line(x0, y0, x1, y1, col);
    line(x1, y1, x2, y2, col);
    line(x2, y2, x0, y0, col);
}

void GFX::circle(int xc, int yc, int r, uint16_t col){
    if (!_vga._inited || r < 0) return;

    if (r == 0){
        putPixel(xc, yc, col);
        return;
    }

    int32_t f = 1 - r;
    int32_t ddF_y = - (r << 1);
    int32_t ddF_x = 1;
    int32_t i = 0;
    int32_t j = -1;

    do {
        while (f < 0) {
            ++i;
            f += (ddF_x += 2);
        }
        f += (ddF_y += 2);

        hLineLength(xc - i    , yc + r, i - j, col);
        hLineLength(xc - i    , yc - r, i - j, col);
        hLineLength(xc + j + 1, yc - r, i - j, col);
        hLineLength(xc + j + 1, yc + r, i - j, col);

        vLineLength(xc + r, yc + j + 1, i - j, col);
        vLineLength(xc + r, yc - i    , i - j, col);
        vLineLength(xc - r, yc - i    , i - j, col);
        vLineLength(xc - r, yc + j + 1, i - j, col);
        
        j = i;
    } while (i < --r);    
}

void GFX::fillCircle(int xc, int yc, int r, uint16_t col){
    if (!_vga._inited || r < 0) return;

    if (r == 0){
        putPixel(xc, yc, col);
        return;
    }

    int x = 0;
    int y = r;
    int d = 3 - (r << 1);

    hLine(xc - r, yc, xc + r, col);

    while (y >= x){
        if (x > 0){
            hLine(xc - y, yc + x, xc + y, col);
            hLine(xc - y, yc - x, xc + y, col);
        }
        if (y > x){
            hLine(xc - x, yc + y, xc + x, col);
            hLine(xc - x, yc - y, xc + x, col);
        }

        x++;
        if (d > 0){
            y--;
            d = d + ((x - y) << 2) + 10;
        } else {
            d = d + (x << 2) + 6;
        }
    }
}

void GFX::testRGBPanel() {
    if (!_vga._inited) return;

    auto& s = _vga._scr;
    if (s.width <= 0 || s.height <= 0) return;

    const int w = s.width;
    const int h = s.height;

    const int midX = w >> 1;
    const int midY = h >> 1;

    int rectW = w / 2;
    int rectH = h / 5;

    if (rectW < 96) rectW = 96;
    if (rectH < 60) rectH = 60;

    if (rectW > w) rectW = w;
    if (rectH > h) rectH = h;

    rectH -= (rectH % 3);
    if (rectH < 3) rectH = 3;

    const int bandH = rectH / 3;

    const int cx = midX - (rectW >> 1);
    const int cy = midY - (rectH >> 1);

    const int maxX = (w > 1) ? (w - 1) : 1;
    const int maxY = (h > 1) ? (h - 1) : 1;

    // ---- фон (градиент на весь экран) ----
    for (int y = 0; y < h; y++) {
        uint8_t gy = (uint32_t)y * 255 / maxY;

        for (int x = 0; x < w; x++) {
            uint8_t rx = (uint32_t)x * 255 / maxX;

            if (s.bpp == _16BIT) {
                uint16_t rr = (uint16_t)(rx >> 3);
                uint16_t gg = (uint16_t)(gy >> 2);
                uint16_t bb = (uint16_t)((255 - rx) >> 3);

                putPixel(x, y, (rr << 11) | (gg << 5) | bb);
            } else {
                uint8_t rr = rx >> 5;
                uint8_t gg = gy >> 5;
                uint8_t bb = (255 - rx) >> 6;

                putPixel(x, y, (uint8_t)((rr << 5) | (gg << 2) | bb));
            }
        }
    }

    // ---- белые контрольные линии ----
    hLine(0, 0, s.maxX, 0xFFFF);
    hLine(0, s.cy, s.maxX, 0xFFFF);
    hLine(0, s.maxY, s.maxX, 0xFFFF);

    vLine(0, 0, s.maxY, 0xFFFF);
    vLine(s.cx, 0, s.maxY, 0xFFFF);
    vLine(s.maxX, 0, s.maxY, 0xFFFF);

    // ---- центральный RGB-блок ----
    for (int y = 0; y < bandH; y++) {
        for (int x = 0; x < rectW; x++) {
            uint8_t v = (rectW > 1) ? ((uint32_t)x * 255 / (rectW - 1)) : 255;

            if (s.bpp == _16BIT) {
                uint16_t r = (uint16_t)(v >> 3);
                uint16_t g = (uint16_t)(v >> 2);
                uint16_t b = (uint16_t)(v >> 3);

                putPixel(cx + x, cy + y,             (r << 11));
                putPixel(cx + x, cy + y + bandH,     (g << 5));
                putPixel(cx + x, cy + y + bandH * 2, b);
            } else {
                uint8_t r = (v >> 5);
                uint8_t g = (v >> 5);
                uint8_t b = (v >> 6);

                putPixel(cx + x, cy + y,             (uint8_t)(r << 5));
                putPixel(cx + x, cy + y + bandH,     (uint8_t)(g << 2));
                putPixel(cx + x, cy + y + bandH * 2, (uint8_t)b);
            }
        }
    }

    // ---- белая рамка вокруг блока ----
    if (s.bpp == _16BIT) {
        rect(cx, cy, cx + rectW - 1, cy + rectH - 1, 0xFFFF);
    } else {
        rect(cx, cy, cx + rectW - 1, cy + rectH - 1, 0xFF);
    }
}

void GFX::hBandOr(int y, int x0, int x1, uint16_t step){
    if (!_vga._inited) return;

    auto& s = _vga._scr;
    if (x0 > x1) std::swap(x0, x1);
    if (x0 > s.x1 || x1 < s.x0) return;

    x0 = std::max(s.x0, x0);
    x1 = std::min(s.x1, x1);

    if (s.bpp == _16BIT){
        for (int i = 0; i < 32; i++){
            int yy = y + i;
            if ((unsigned)yy <= (unsigned)s.maxY){
                uint16_t* scr = &s.line16[_vga._backBufLine + yy][x0];
                uint16_t col = i * step;
                int len = x1 - x0 + 1;
                while (len--) *scr++ |= col;
            }
        }

        for (int i = 0; i < 32; i++){
            int yy = y + 32 + i;
            if ((unsigned)yy <= (unsigned)s.maxY){
                uint16_t* scr = &s.line16[_vga._backBufLine + yy][x0];
                uint16_t col = (31 - i) * step;
                int len = x1 - x0 + 1;
                while (len--) *scr++ |= col;
            }
        }
    } else {
        uint8_t st = (uint8_t)step;

        for (int i = 0; i < 32; i++){
            int yy = y + i;
            if ((unsigned)yy <= (unsigned)s.maxY){
                uint8_t* scr = &s.line8[_vga._backBufLine + yy][x0];
                uint8_t col = (uint8_t)(i * st);
                int len = x1 - x0 + 1;
                while (len--) *scr++ |= col;
            }
        }

        for (int i = 0; i < 32; i++){
            int yy = y + 32 + i;
            if ((unsigned)yy <= (unsigned)s.maxY){
                uint8_t* scr = &s.line8[_vga._backBufLine + yy][x0];
                uint8_t col = (uint8_t)((31 - i) * st);
                int len = x1 - x0 + 1;
                while (len--) *scr++ |= col;
            }
        }
    }
}

void GFX::vBandOr(int x, int y0, int y1, uint16_t step){
    if (!_vga._inited) return;

    auto& s = _vga._scr;
    if (y0 > y1) std::swap(y0, y1);
    if (y0 > s.y1 || y1 < s.y0) return;

    y0 = std::max(s.y0, y0);
    y1 = std::min(s.y1, y1);

    if (s.bpp == _16BIT){
        for (int i = 0; i < 32; i++){
            int xx = x + i;
            if ((unsigned)xx <= (unsigned)s.maxX){
                uint16_t col = i * step;
                for (int y = y0; y <= y1; y++){
                    s.line16[_vga._backBufLine + y][xx] |= col;
                }
            }
        }

        for (int i = 0; i < 32; i++){
            int xx = x + 32 + i;
            if ((unsigned)xx <= (unsigned)s.maxX){
                uint16_t col = (31 - i) * step;
                for (int y = y0; y <= y1; y++){
                    s.line16[_vga._backBufLine + y][xx] |= col;
                }
            }
        }
    } else {
        uint8_t st = (uint8_t)step;

        for (int i = 0; i < 32; i++){
            int xx = x + i;
            if ((unsigned)xx <= (unsigned)s.maxX){
                uint8_t col = (uint8_t)(i * st);
                for (int y = y0; y <= y1; y++){
                    s.line8[_vga._backBufLine + y][xx] |= col;
                }
            }
        }

        for (int i = 0; i < 32; i++){
            int xx = x + 32 + i;
            if ((unsigned)xx <= (unsigned)s.maxX){
                uint8_t col = (uint8_t)((31 - i) * st);
                for (int y = y0; y <= y1; y++){
                    s.line8[_vga._backBufLine + y][xx] |= col;
                }
            }
        }
    }
}

void GFX::hBandFill(int y, int x0, int x1, uint16_t step){
    if (!_vga._inited) return;

    auto& s = _vga._scr;
    if (x0 > x1) std::swap(x0, x1);
    if (x0 > s.x1 || x1 < s.x0) return;

    x0 = std::max(s.x0, x0);
    x1 = std::min(s.x1, x1);

    if (s.bpp == _16BIT){
        for (int i = 0; i < 32; i++){
            int yy = y + i;
            if ((unsigned)yy <= (unsigned)s.maxY){
                uint16_t* scr = &s.line16[_vga._backBufLine + yy][x0];
                uint16_t col = i * step;
                int len = x1 - x0 + 1;
                while (len--) *scr++ = col;
            }
        }

        for (int i = 0; i < 32; i++){
            int yy = y + 32 + i;
            if ((unsigned)yy <= (unsigned)s.maxY){
                uint16_t* scr = &s.line16[_vga._backBufLine + yy][x0];
                uint16_t col = (31 - i) * step;
                int len = x1 - x0 + 1;
                while (len--) *scr++ = col;
            }
        }
    } else {
        uint8_t st = (uint8_t)step;

        for (int i = 0; i < 32; i++){
            int yy = y + i;
            if ((unsigned)yy <= (unsigned)s.maxY){
                uint8_t* scr = &s.line8[_vga._backBufLine + yy][x0];
                uint8_t col = (uint8_t)(i * st);
                int len = x1 - x0 + 1;
                while (len--) *scr++ = col;
            }
        }

        for (int i = 0; i < 32; i++){
            int yy = y + 32 + i;
            if ((unsigned)yy <= (unsigned)s.maxY){
                uint8_t* scr = &s.line8[_vga._backBufLine + yy][x0];
                uint8_t col = (uint8_t)((31 - i) * st);
                int len = x1 - x0 + 1;
                while (len--) *scr++ = col;
            }
        }
    }
}

void GFX::vBandFill(int x, int y0, int y1, uint16_t step){
    if (!_vga._inited) return;

    auto& s = _vga._scr;
    if (y0 > y1) std::swap(y0, y1);
    if (y0 > s.y1 || y1 < s.y0) return;

    y0 = std::max(s.y0, y0);
    y1 = std::min(s.y1, y1);

    if (s.bpp == _16BIT){
        for (int i = 0; i < 32; i++){
            int xx = x + i;
            if ((unsigned)xx <= (unsigned)s.maxX){
                uint16_t col = i * step;
                for (int y = y0; y <= y1; y++){
                    s.line16[_vga._backBufLine + y][xx] = col;
                }
            }
        }

        for (int i = 0; i < 32; i++){
            int xx = x + 32 + i;
            if ((unsigned)xx <= (unsigned)s.maxX){
                uint16_t col = (31 - i) * step;
                for (int y = y0; y <= y1; y++){
                    s.line16[_vga._backBufLine + y][xx] = col;
                }
            }
        }
    } else {
        uint8_t st = (uint8_t)step;

        for (int i = 0; i < 32; i++){
            int xx = x + i;
            if ((unsigned)xx <= (unsigned)s.maxX){
                uint8_t col = (uint8_t)(i * st);
                for (int y = y0; y <= y1; y++){
                    s.line8[_vga._backBufLine + y][xx] = col;
                }
            }
        }

        for (int i = 0; i < 32; i++){
            int xx = x + 32 + i;
            if ((unsigned)xx <= (unsigned)s.maxX){
                uint8_t col = (uint8_t)((31 - i) * st);
                for (int y = y0; y <= y1; y++){
                    s.line8[_vga._backBufLine + y][xx] = col;
                }
            }
        }
    }
}