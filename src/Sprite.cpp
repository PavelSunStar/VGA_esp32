#include "Sprite.h"
#include "Matrix.h"

Sprite::Sprite(VGA_esp32 &vga) : _vga(vga){

}

Sprite::~Sprite(){
    
}

void Sprite::resetParam(uint8_t num){
    if (num == 0) return;

    _images = num;

    if (_img){
        delete[] _img;
        _img = nullptr;
    }

    _img = new Image[_images];
}

bool Sprite::setBufferAddr() {
    if (!_buf || !_img || _images == 0) {
        Serial.println("setBufferAddr: invalid sprite buffer");
        return false;
    }

    if (_line8) {
        delete[] _line8;
        _line8 = nullptr;
    }

    if (_line16) {
        delete[] _line16;
        _line16 = nullptr;
    }

    int totalLines = 0;
    for (int i = 0; i < _images; i++) totalLines += _img[i].height;

    int line = 0;
    if (_bpp == _16BIT){
        uint16_t* base = (uint16_t*)_buf;
        _line16 = new uint16_t*[totalLines];

        for (int i = 0; i < _images; i++){
            Image &im = _img[i];
            for (int y = 0; y < im.height; y++){
                _line16[line++] = base;
                base += im.width;
            }
        }
    } else {
        uint8_t* base = _buf;
        _line8 = new uint8_t*[totalLines];

        for (int i = 0; i < _images; i++){
            Image &im = _img[i];
            for (int y = 0; y < im.height; y++){
                _line8[line++] = base;
                base += im.width;
            }
        }
    }

    return true;
}

bool Sprite::loadImages(const uint8_t* data){
    if (!data) return false;

    uint8_t imgBPP = *data++;
    if (imgBPP != _8BIT && imgBPP != _16BIT) return false;
    resetParam(*data++);

    data += _images << 2;
    const uint8_t* ptr = data;    

    int fullSize = 0;
    uint32_t offsetLine = 0;
    _bpp = _vga._scr.bpp; 
    _shift = (_bpp == _16BIT ? 1 : 0);
    int imgShift = (imgBPP == 16 ? 1 : 0); 

    for (int i = 0; i < _images; i++){
        _img[i].width       = (*data++) | (*data++ << 8);
        _img[i].height      = (*data++) | (*data++ << 8);
        _img[i].maxX        = _img[i].width - 1;
        _img[i].maxY        = _img[i].height - 1;
        _img[i].cx          = _img[i].width >> 1;
        _img[i].cy          = _img[i].height >> 1;
        _img[i].lineSize    = _img[i].width << _shift;
        _img[i].size        = _img[i].width * _img[i].height;
        _img[i].fullSize    = _img[i].lineSize * _img[i].height;
        _img[i].offset      = fullSize;
        _img[i].offsetLine  = offsetLine;

        // viewport
        _img[i].x0 = 0; _img[i].x1 = _img[i].maxX; 
        _img[i].y0 = 0; _img[i].y1 = _img[i].maxY;

        offsetLine  += _img[i].height;
        fullSize    += _img[i].fullSize;
        data        += (_img[i].width << imgShift) * _img[i].height;
    }

    if (!_vga.allocateMemory(_buf, fullSize, false)) return false;  // DMA = false;
    if (!setBufferAddr()) return false;

    for (int i = 0; i < _images; i++){
        ptr += 4;
        int size = _img[i].size;

        if (imgBPP == _bpp){
            memcpy(_buf + _img[i].offset, ptr, _img[i].fullSize);   // Convert not needed
            ptr += _img[i].fullSize; 
        } else if (imgBPP == 8 && _bpp == 16){                      // Convert 8 -> 16
            uint16_t* dest = (uint16_t*)(_buf + _img[i].offset);

            while (size-- > 0){
                uint8_t col8 = *ptr++;

                uint8_t r3 = (col8 >> 5) & 0x07;
                uint8_t g3 = (col8 >> 2) & 0x07;
                uint8_t b2 =  col8       & 0x03;

                uint16_t r5 = (r3 << 2) | (r3 >> 1);
                uint16_t g6 = (g3 << 3) |  g3;
                uint16_t b5 = (b2 << 3) | (b2 << 1) | (b2 >> 1);

                *dest++ = (r5 << 11) | (g6 << 5) | b5;
            }
        } else if (imgBPP == 16 && _bpp == 8){
            uint16_t* sour = (uint16_t*)ptr;
            uint8_t* dest = _buf + _img[i].offset;

            while (size-- > 0){
                uint16_t col16 = *sour++;

                uint8_t r3 = (col16 >> 13) & 0x07;
                uint8_t g3 = (col16 >> 8)  & 0x07;
                uint8_t b2 = (col16 >> 3)  & 0x03;
                
                *dest++ = (r3 << 5) | (g3 << 2) | b2;
            }
            
            ptr += _img[i].fullSize;
        }
    }

    return (_created = true);
}

void Sprite::putImage(int x, int y, uint8_t num){
    if (!_created || num >= _images) return;
    
    auto& s = _vga._scr;
    Image &im = _img[num];
    if (x > s.x1 || y > s.y1) return;    
    int xx = x + im.maxX;
    int yy = y + im.maxY;
    if (xx < s.x0 || yy < s.y0) return;

    int imageLineSize   = im.lineSize; 
    int scrLineSize     = s.lineSize;

    if (x >= s.x0 && y >= s.y0 && xx <= s.x1 && yy <= s.y1){
        uint8_t* img = _buf + im.offset;
        uint8_t* scr = (_bpp == _16BIT) ? (uint8_t*)&s.line16[_vga._backBufLine + y][x] : (uint8_t*)&s.line8[_vga._backBufLine + y][x];     

        int lines = im.height;  
        while (lines-- > 0){
            memcpy(scr, img, imageLineSize);
            scr += scrLineSize;
            img += imageLineSize;
        }        
    } else {
        int sxl = (x  < s.x0 ? (s.x0 - x)  : 0);
        int sxr = (xx > s.x1 ? (xx - s.x1) : 0);
        int syu = (y  < s.y0 ? (s.y0 - y)  : 0);
        int syd = (yy > s.y1 ? (yy - s.y1) : 0);

        int copyX = (im.width  - sxl - sxr) << _shift;
        int copyY =  im.height - syu - syd; 
        if (copyX <= 0 || copyY <= 0) return;   
        
        uint8_t* img = (_bpp == _16BIT)
            ? (uint8_t*)&_line16[im.offsetLine + syu][sxl]
            : (uint8_t*)&_line8[im.offsetLine + syu][sxl];
        uint8_t* scr = (_bpp == _16BIT)
            ? (uint8_t*)&s.line16[_vga._backBufLine + y + syu][x + sxl]
            : (uint8_t*)&s.line8 [_vga._backBufLine + y + syu][x + sxl];  
            
        while (copyY-- > 0){
            memcpy(scr, img, copyX);
            scr += scrLineSize;
            img += imageLineSize;
        }            
    }
}    

void Sprite::putSprite(int x, int y, uint16_t maskColor, uint8_t num){
    if (!_created || num >= _images) return;
    
    auto& s = _vga._scr;
    Image &im = _img[num];
    if (x > s.x1 || y > s.y1) return;    
    int xx = x + im.maxX;
    int yy = y + im.maxY;
    if (xx < s.x0 || yy < s.y0) return;

    if (x >= s.x0 && y >= s.y0 && xx <= s.x1 && yy <= s.y1){
        int lines = im.height;  
        int skip = s.width - im.width;  

        if (_bpp == _16BIT){
            uint16_t* img = &_line16[im.offsetLine][0];
            uint16_t* scr = &s.line16[_vga._backBufLine + y][x];

            while (lines-- > 0){
                for (int xx = 0; xx < im.width; xx++){
                    if (*img != maskColor) *scr = *img;
                    img++;
                    scr++;
                }
                scr += skip;
            }            
        } else {
            uint8_t* img = &_line8[im.offsetLine][0];
            uint8_t* scr = &s.line8[_vga._backBufLine + y][x]; 

            while (lines-- > 0){
                for (int xx = 0; xx < im.width; xx++){
                    if (*img != maskColor) *scr = *img;
                    img++;
                    scr++;
                }
                scr += skip;
            }  
        }
    } else { 
        int sxl = (x  < s.x0 ? (s.x0 - x)  : 0);
        int sxr = (xx > s.x1 ? (xx - s.x1) : 0);
        int syu = (y  < s.y0 ? (s.y0 - y)  : 0);
        int syd = (yy > s.y1 ? (yy - s.y1) : 0);

        int copyX = im.width  - sxl - sxr;
        int copyY =  im.height - syu - syd; 
        if (copyX <= 0 || copyY <= 0) return;   
        int skipSour = im.width - copyX;
        int skipScr = s.width - copyX;

        if (_bpp == _16BIT){
            uint16_t* img = &_line16[im.offsetLine + syu][sxl];
            uint16_t* scr = &s.line16[_vga._backBufLine + y + syu][x + sxl];

            while (copyY-- > 0){
                for (int xx = 0; xx < copyX; xx++){
                    if (*img != maskColor) *scr = *img;
                    img++;
                    scr++;
                }

                scr += skipScr;
                img += skipSour;
            } 
        } else {
            uint8_t* img = &_line8[im.offsetLine + syu][sxl];
            uint8_t* scr = &s.line8[_vga._backBufLine + y + syu][x + sxl];

            while (copyY-- > 0){
                for (int xx = 0; xx < copyX; xx++){
                    if (*img != maskColor) *scr = *img;
                    img++;
                    scr++;
                }

                scr += skipScr;
                img += skipSour;
            }             
        }
    }
}

void Sprite::putAffineSprite(int dstX, int dstY, float ang, uint16_t zoomX, uint16_t zoomY, uint16_t maskColor, uint8_t num){
    if (!_created || num >= _images) return;

    auto& s = _vga._scr;    
    Image& im = _img[num];

    const int srcW = im.width;
    const int srcH = im.height;
    Affine2D m = Matrix::make(  (float)dstX, (float)dstY, 
                                (float)srcW * 0.5f, (float)srcH * 0.5f, 
                                angle(ang), 
                                percentTo(zoomX), percentTo(zoomY)
    );

    RectBounds rb = Matrix::bounds(m, (float)srcW, (float)srcH);
    int x0 = (int)floorf(rb.sx); // округляет число вниз до ближайшего целого
    int y0 = (int)floorf(rb.sy);
    int x1 = (int)ceilf(rb.ex);  // округляет число вверх до ближайшего целого.
    int y1 = (int)ceilf(rb.ey);

    x0 = std::max(x0, s.x0);
    y0 = std::max(y0, s.y0);
    x1 = std::min(x1, s.x1 + 1);
    y1 = std::min(y1, s.y1 + 1);
    if (x0 >= x1 || y0 >= y1) return;

    Affine2DInv inv;
    if (!Matrix::invert(inv, m)) return;

    // стартовые координаты для первой точки (x0, y0)
    int32_t row_u = inv.a * x0 + inv.b * y0 + inv.c + HALF;
    int32_t row_v = inv.d * x0 + inv.e * y0 + inv.f + HALF;
      
    uint8_t** srcLines = _line8 + im.offsetLine;
    uint8_t* dstBase   = &s.line8[_vga._backBufLine + y0][x0];
    const uint8_t mask = (uint8_t)(maskColor & 0xFF);
    //const int dstSkip  = s.width - x1 + x0;

    for (int y = y0; y < y1; y++){
        uint8_t* dst = dstBase;
        int32_t u = row_u;
        int32_t v = row_v;

        for (int x = x0; x < x1; x++){
            int sx = u >> FP_SHIFT;
            int sy = v >> FP_SHIFT;

            if ((unsigned)sx < (unsigned)srcW &&
                (unsigned)sy < (unsigned)srcH)
            {
                uint8_t* srcRow = srcLines[sy];
                uint8_t col = srcRow[sx];
                if (col != mask) *dst = col;
            }

            ++dst;
            u += inv.a;
            v += inv.d;
        }

        dstBase += s.width;
        row_u   += inv.b;
        row_v   += inv.e;
    }
}

/*
void Sprite::putMatImage(int32_t dstX, int32_t dstY, float ang, uint8_t num){
    if (!_created || num >= _images) return;

    auto& s = _vga._scr;    
    Image& im = _img[num];

    const int srcW = im.width;
    const int srcH = im.height;
    if (srcW <= 0 || srcH <= 0) return;

    Affine2D m = Matrix::make(
        (float)dstX,
        (float)dstY,
        (float)srcW * 0.5f,
        (float)srcH * 0.5f,
        ang,
        1.0f,
        1.0f
    );

    RectBounds rb = Matrix::bounds(m, (float)srcW, (float)srcH);

    int x0 = (int)floorf(rb.sx);
    int y0 = (int)floorf(rb.sy);
    int x1 = (int)ceilf(rb.ex);
    int y1 = (int)ceilf(rb.ey);

    x0 = std::max(x0, s.x0);
    y0 = std::max(y0, s.y0);
    x1 = std::min(x1, s.x1 + 1);
    y1 = std::min(y1, s.y1 + 1);
    if (x0 >= x1 || y0 >= y1) return;

    Affine2DInv inv;
    if (!Matrix::invert(inv, m)) return;

    const int32_t A = inv.a;
    const int32_t B = inv.b;
    const int32_t C = inv.c;
    const int32_t D = inv.d;
    const int32_t E = inv.e;
    const int32_t F = inv.f;

    const int32_t uu = A * x0 + C + HALF;
    const int32_t vv = D * x0 + F + HALF;

    for (int y = y0; y < y1; y++){
        uint8_t* dst = &s.line8[y + _vga._backBufLine][x0];
        int32_t u = uu + B * y;
        int32_t v = vv + E * y;

        for (int x = x0; x < x1; x++){
            const int sx = u >> FP_SHIFT;
            const int sy = v >> FP_SHIFT;

            if ((unsigned)sx < (unsigned)srcW && (unsigned)sy < (unsigned)srcH){
                const uint8_t col = _line8[sy + im.offsetLine][sx];
                if (col != 0b11100) *dst = col;
            }

            ++dst;
            u += A;
            v += D;
        }
    }
}

void Sprite::putAffineSprite(int32_t dstX, int32_t dstY, float ang, uint16_t maskColor, uint8_t num){
    if (!_created || num >= _images) return;

    auto& s  = _vga._scr;
    Image& im = _img[num];

    const int srcW = im.width;
    const int srcH = im.height;

    // ВАЖНО:
    // Matrix::make() уже сам переводит angle -> rad,
    // поэтому сюда передаём просто ang, без angle(ang).
    Affine2D m = Matrix::make(
        (float)dstX,
        (float)dstY,
        (float)srcW * 0.5f,
        (float)srcH * 0.5f,
        ang,
        2.0f,
        2.0f
    );

    RectBounds rb = Matrix::bounds(m, (float)srcW, (float)srcH);

    int x0 = (int)floorf(rb.sx);
    int y0 = (int)floorf(rb.sy);
    int x1 = (int)ceilf(rb.ex);
    int y1 = (int)ceilf(rb.ey);

    // clip to screen
    x0 = std::max(x0, s.x0);
    y0 = std::max(y0, s.y0);
    x1 = std::min(x1, s.x1 + 1);
    y1 = std::min(y1, s.y1 + 1);

    if (x0 >= x1 || y0 >= y1) return;

    Affine2DInv inv;
    if (!Matrix::invert(inv, m)) return;

    // стартовые координаты для первой точки (x0, y0)
    int32_t row_u = inv.a * x0 + inv.b * y0 + inv.c + HALF;
    int32_t row_v = inv.d * x0 + inv.e * y0 + inv.f + HALF;

    if (_bpp == _16BIT) {
        uint16_t** srcLines = _line16 + im.offsetLine;
        uint16_t* dstBase   = &s.line16[_vga._backBufLine + y0][x0];
        const uint16_t mask = maskColor;
        const int dstSkip   = s.width - (x1 - x0);

        for (int y = y0; y < y1; y++) {
            uint16_t* dst = dstBase;

            int32_t u = row_u;
            int32_t v = row_v;

            for (int x = x0; x < x1; x++) {
                int sx = u >> FP_SHIFT;
                int sy = v >> FP_SHIFT;

                if ((unsigned)sx < (unsigned)srcW &&
                    (unsigned)sy < (unsigned)srcH)
                {
                    uint16_t* srcRow = srcLines[sy];
                    uint16_t col = srcRow[sx];
                    if (col != mask) *dst = col;
                }

                ++dst;
                u += inv.a;
                v += inv.d;
            }

            dstBase += s.width;
            row_u   += inv.b;
            row_v   += inv.e;
        }
    } else {
        uint8_t** srcLines = _line8 + im.offsetLine;
        uint8_t* dstBase   = &s.line8[_vga._backBufLine + y0][x0];
        const uint8_t mask = (uint8_t)(maskColor & 0xFF);
        const int dstSkip  = s.width - (x1 - x0);

        for (int y = y0; y < y1; y++) {
            uint8_t* dst = dstBase;

            int32_t u = row_u;
            int32_t v = row_v;

            for (int x = x0; x < x1; x++) {
                int sx = u >> FP_SHIFT;
                int sy = v >> FP_SHIFT;

                if ((unsigned)sx < (unsigned)srcW &&
                    (unsigned)sy < (unsigned)srcH)
                {
                    uint8_t* srcRow = srcLines[sy];
                    uint8_t col = srcRow[sx];
                    if (col != mask) *dst = col;
                }

                ++dst;
                u += inv.a;
                v += inv.d;
            }

            dstBase += s.width;
            row_u   += inv.b;
            row_v   += inv.e;
        }
    }
}
*/

uint16_t Sprite::getPixel(int x, int y, uint8_t num){
    if (!_created || num >= _images) return 0; 
    
    Image& im = _img[num];
    if (x < im.x0 || y < im.y0 || x > im.x1 ||  y > im.y1) return 0;

    return ((_bpp == _16BIT) ? _line16[im.offsetLine + y][x] : (uint16_t)_line8[im.offsetLine + y][x]);
}

